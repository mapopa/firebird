/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2017 Alexander Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ________________________________
 */

#include "firebird.h"

#include "../dsql/DsqlBatch.h"

#include "../jrd/EngineInterface.h"
#include "../jrd/jrd.h"
#include "../jrd/status.h"
#include "../jrd/exe_proto.h"
#include "../dsql/dsql.h"
#include "../dsql/errd_proto.h"
#include "../common/classes/ClumpletReader.h"
#include "../common/classes/auto.h"
#include "../common/classes/fb_string.h"
#include "../common/utils_proto.h"

using namespace Firebird;
using namespace Jrd;

namespace {

	const char* TEMP_NAME = "fb_batch";

	class BatchCompletionState FB_FINAL :
    	public DisposeIface<Firebird::IBatchCompletionStateImpl<BatchCompletionState, CheckStatusWrapper> >
	{
	public:
		BatchCompletionState(bool storeCounts, ULONG lim)
			: rare(getPool()),
			  reccount(0u),
			  detailedLimit(lim)
		{
			if (storeCounts)
				array = FB_NEW_POOL(getPool()) DenseArray(getPool());
		}

		void regError(thread_db* tdbb, IStatus* errStatus)
		{
			IStatus* newVector = nullptr;
			if (rare.getCount() < detailedLimit)
			{
				newVector = errStatus->clone();
				JRD_transliterate(tdbb, newVector);
			}
			rare.add(StatusPair(reccount, newVector));

			regUpdate(tdbb, IBatchCompletionState::EXECUTE_FAILED);
		}

		void regUpdate(thread_db*, SLONG count)
		{
			if (array)
				array->push(count);

			++reccount;
		}

		// IBatchCompletionState implementation
		unsigned getSize(CheckStatusWrapper*)
		{
			return reccount;
		}

		int getState(CheckStatusWrapper* status, unsigned pos)
		{
			try
			{
				if (pos >= reccount)
					(Arg::Gds(isc_random) << "Position is out of range").raise();
				if (array)
					return (*array)[pos];

				ULONG index = find(pos);
				return (index >= rare.getCount() || rare[index].first != pos) ?
					SUCCESS_NO_INFO : EXECUTE_FAILED;
			}
			catch (const Exception& ex)
			{
				ex.stuffException(status);
			}
			return 0;
		}

		unsigned findError(CheckStatusWrapper* status, unsigned pos)
		{
			try
			{
				if (pos >= reccount)
					(Arg::Gds(isc_random) << "Position is out of range").raise();

				ULONG index = find(pos);
				if (index < rare.getCount())
					return rare[index].first;
			}
			catch (const Exception& ex)
			{
				ex.stuffException(status);
			}
			return NO_MORE_ERRORS;
		}

		FB_BOOLEAN getStatus(CheckStatusWrapper* status, unsigned pos)
		{
			try
			{
				if (pos >= reccount)
					(Arg::Gds(isc_random) << "Position is out of range").raise();

				ULONG index = find(pos);
				if (index < rare.getCount() && rare[index].first == pos)
				{
					if (rare[index].second)
						fb_utils::copyStatus(status, rare[index].second);
					(Arg::Gds(isc_random) << "Detailed error info is missing in batch").raise();
				}

				return true;
			}
			catch (const Exception& ex)
			{
				ex.stuffException(status);
			}
			return false;
		}

		void dispose()
		{
			delete this;
		}

		~BatchCompletionState()
		{
			for (unsigned i = 0; i < rare.getCount() && rare[i].second; ++i)
				rare[i].second->dispose();
		}

	private:
		typedef Pair<NonPooled<ULONG, IStatus*> > StatusPair;
		typedef Array<Pair<NonPooled<ULONG, IStatus*> > > RarefiedArray;
		RarefiedArray rare;
		typedef Array<SLONG> DenseArray;
		AutoPtr<DenseArray> array;
		ULONG reccount, detailedLimit;

		ULONG find(ULONG recno) const
		{
			ULONG high = rare.getCount(), low = 0;
			while (high > low)
			{
				ULONG med = (high + low) / 2;
				if (recno > rare[med].first)
					low = med + 1;
				else
					high = med;
			}

			return low;
		}
	};
}

DsqlBatch::DsqlBatch(dsql_req* req, const dsql_msg* /*message*/, IMessageMetadata* inMeta, ClumpletReader& pb)
	: m_request(req),
	  m_batch(NULL),
	  m_meta(inMeta),
	  m_messages(req->getPool()),
	  m_blobs(req->getPool()),
	  m_messageSize(0),
	  m_flags(0),
	  m_detailed(DETAILED_LIMIT),
	  m_bufferSize(BUFFER_LIMIT),
	  m_hasBlob(false)
{
	FbLocalStatus st;
	m_messageSize = inMeta->getMessageLength(&st);
	check(&st);

	if (m_messageSize > RAM_BATCH)		// hops - message does not fit in our buffer
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				  Arg::Gds(isc_random) << "Message too long");
	}

	for (pb.rewind(); !pb.isEof(); pb.moveNext())
	{
		UCHAR t = pb.getClumpTag();
		switch(t)
		{
		case IBatch::MULTIERROR:
		case IBatch::RECORD_COUNTS:
		case IBatch::USER_BLOB_IDS:
			if (pb.getBoolean())
				m_flags |= (1 << t);
			else
				m_flags &= ~(1 << t);
			break;

		case IBatch::DETAILED_ERRORS:
			m_detailed = pb.getInt();
			if (m_detailed > DETAILED_LIMIT * 4)
				m_detailed = DETAILED_LIMIT * 4;
			break;

		case IBatch::BUFFER_BYTES_SIZE:
			m_bufferSize = pb.getInt();
			if (m_bufferSize > BUFFER_LIMIT * 4)
				m_bufferSize = BUFFER_LIMIT * 4;
			break;
		}
	}

	// todo - process message to find blobs in it

	m_messages.setBuf(m_bufferSize);
	if (m_hasBlob)
		m_blobs.setBuf(m_bufferSize);
}


DsqlBatch::~DsqlBatch()
{
	if (m_batch)
		m_batch->resetHandle();
	if (m_request)
		m_request->req_batch = NULL;
}

Attachment* DsqlBatch::getAttachment() const
{
	return m_request->req_dbb->dbb_attachment;
}

void DsqlBatch::setInterfacePtr(JBatch* interfacePtr) throw()
{
	fb_assert(!m_batch);
	m_batch = interfacePtr;
}

DsqlBatch* DsqlBatch::open(thread_db* tdbb, dsql_req* req, IMessageMetadata* inMetadata,
	unsigned parLength, const UCHAR* par)
{
	SET_TDBB(tdbb);
	Jrd::ContextPoolHolder context(tdbb, &req->getPool());

	// Validate cursor or batch being not already open

	if (req->req_cursor)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_dsql_cursor_open_err));
	}

	if (req->req_batch)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				  Arg::Gds(isc_random) << "Request has active batch");
	}

	// Sanity checks before creating batch

	if (!req->req_request)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-504) <<
				  Arg::Gds(isc_unprepared_stmt));
	}

	const DsqlCompiledStatement* statement = req->getStatement();

	if (statement->getFlags() & DsqlCompiledStatement::FLAG_ORPHAN)
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
		          Arg::Gds(isc_bad_req_handle));
	}

	switch(statement->getType())
	{
		case DsqlCompiledStatement::TYPE_INSERT:
		case DsqlCompiledStatement::TYPE_DELETE:
		case DsqlCompiledStatement::TYPE_UPDATE:
		case DsqlCompiledStatement::TYPE_EXEC_PROCEDURE:
		case DsqlCompiledStatement::TYPE_EXEC_BLOCK:
			break;

		default:
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
					  Arg::Gds(isc_random) << "Invalid type of statement used in batch");
	}

	const dsql_msg* message = statement->getSendMsg();
	if (! (inMetadata && message && req->parseMetadata(inMetadata, message->msg_parameters)))
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-901) <<
				  Arg::Gds(isc_random) << "Statement used in batch must have parameters");
	}

	// Open reader for parameters block

	ClumpletReader pb(ClumpletReader::WideTagged, par, parLength);
	if (pb.getBufferLength() && (pb.getBufferTag() != IBatch::VERSION1))
		ERRD_post(Arg::Gds(isc_random) << "Invalid tag in parameters block");

	// Create batch

	DsqlBatch* b = FB_NEW_POOL(req->getPool()) DsqlBatch(req, message, inMetadata, pb);
	req->req_batch = b;
	return b;
}

void DsqlBatch::add(thread_db* tdbb, ULONG count, const void* inBuffer)
{
	m_messages.put(inBuffer, count * m_messageSize);
}

void DsqlBatch::addBlob(thread_db* tdbb, ULONG length, const void* inBuffer, ISC_QUAD* blobId)
{
	Arg::Gds(isc_wish_list).raise();
}

void DsqlBatch::appendBlobData(thread_db* tdbb, ULONG length, const void* inBuffer)
{
	Arg::Gds(isc_wish_list).raise();
}

void DsqlBatch::addBlobStream(thread_db* tdbb, uint length, const Firebird::BlobStream* inBuffer)
{
	Arg::Gds(isc_wish_list).raise();
}

void DsqlBatch::registerBlob(thread_db* tdbb, const ISC_QUAD* existingBlob, ISC_QUAD* blobId)
{
	Arg::Gds(isc_wish_list).raise();
}

Firebird::IBatchCompletionState* DsqlBatch::execute(thread_db* tdbb)
{
	// todo - add new trace event here
	// TraceDSQLExecute trace(req_dbb->dbb_attachment, this);

	jrd_tra* transaction = tdbb->getTransaction();

	// execution timer
	thread_db::TimerGuard timerGuard(tdbb, m_request->setupTimer(tdbb), true);

	// sync internal buffers
	if (!m_messages.done())
	{
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
			  Arg::Gds(isc_random) << "Empty Internal buffer overflow - batch too big");
	}

	// todo - insert blobs here

	// execute request
	m_request->req_transaction = transaction;		// not sure really needed here...
	jrd_req* req = m_request->req_request;
	fb_assert(req);

	extern bool treePrt;
	treePrt = true;

	fprintf(stderr, "\n\n+++ Unwind\n\n");
	EXE_unwind(tdbb, req);
	fprintf(stderr, "\n\n+++ Start\n\n");
	EXE_start(tdbb, req, transaction);

	// prepare completion interface
	AutoPtr<BatchCompletionState, SimpleDispose<BatchCompletionState> > completionState
		(FB_NEW BatchCompletionState(m_flags & (1 << IBatch::RECORD_COUNTS), m_detailed));

	AutoSetRestore<bool> batchFlag(&req->req_batch, true);
	const dsql_msg* message = m_request->getStatement()->getSendMsg();
	ULONG remains;
	const UCHAR* data;
	while ((remains = m_messages.get(&data)) > 0)
	{
		if (remains < m_messageSize)
		{
			ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
				Arg::Gds(isc_random) << "Internal error: useless data remained in batch buffer");
		}

		while (remains >= m_messageSize)
		{
			// todo - translate blob IDs here

			m_request->mapInOut(tdbb, false, message, m_meta, NULL, data);
			data += m_messageSize;
			remains -= m_messageSize;

//			if (m_messages.left(remains) < m_messageSize)
//				req->req_batch = false;

			UCHAR* msgBuffer = m_request->req_msg_buffers[message->msg_buffer_number];
			fprintf(stderr, "\n\n+++ Send\n\n");
			try
			{
				ULONG before = req->req_records_inserted + req->req_records_updated +
					req->req_records_deleted;
				EXE_send(tdbb, req, message->msg_number, message->msg_length, msgBuffer);
				ULONG after = req->req_records_inserted + req->req_records_updated +
					req->req_records_deleted;
				completionState->regUpdate(tdbb, after - before);
			}
			catch (const Exception& ex)
			{
				FbLocalStatus status;
				ex.stuffException(&status);
				completionState->regError(tdbb, &status);
				if (!(m_flags & (1 << IBatch::MULTIERROR)))
					break;
			}
		}
		m_messages.remained(remains);
	}

	// reset to initial state
	cancel(tdbb);

	return completionState.release();
}

void DsqlBatch::cancel(thread_db* tdbb)
{
	m_messages.clear();
	if (m_hasBlob)
		m_blobs.clear();
}

void DsqlBatch::DataCache::setBuf(ULONG size)
{
	m_limit = size;

	// create ram cache
	fb_assert(!m_cache);
	m_cache = FB_NEW_POOL(getPool()) Cache;
}

void DsqlBatch::DataCache::put(const void* d, ULONG dataSize)
{
	if (m_used + (m_cache ? m_cache->getCount() : 0) + dataSize > m_limit)
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
			  Arg::Gds(isc_random) << "Internal buffer overflow - batch too big");

	const UCHAR* data = reinterpret_cast<const UCHAR*>(d);

	// Coefficient affecting direct data write to tempspace
	const ULONG K = 4;

	// ensure ram cache presence
	fb_assert(m_cache);

	// swap to secondary cache if needed
	if (m_cache->getCount() + dataSize > m_cache->getCapacity())
	{
		// store data in the end of ram cache if needed
		// avoid copy in case of huge buffer passed
		ULONG delta = m_cache->getCapacity() - m_cache->getCount();
		if (dataSize - delta < m_cache->getCapacity() / K)
		{
			m_cache->append(data, delta);
			data += delta;
			dataSize -= delta;
		}

		// swap ram cache to tempspace
		if (!m_space)
			m_space = FB_NEW_POOL(getPool()) TempSpace(getPool(), TEMP_NAME);
		const FB_UINT64 writtenBytes = m_space->write(m_used, m_cache->begin(), m_cache->getCount());
		fb_assert(writtenBytes == m_cache->getCount());
		m_used += m_cache->getCount();
		m_cache->clear();

		// in a case of huge buffer write directly to tempspace
		if (dataSize > m_cache->getCapacity() / K)
		{
			const FB_UINT64 writtenBytes = m_space->write(m_used, data, dataSize);
			fb_assert(writtenBytes == dataSize);
			m_used += dataSize;
			return;
		}
	}

	m_cache->append(data, dataSize);
}

bool DsqlBatch::DataCache::done()
{
	fb_assert(m_cache);

	if (m_cache->getCount() == 0 && m_used == 0)
		return false;

	if (m_cache->getCount() && m_used)
	{
		fb_assert(m_space);

		const FB_UINT64 writtenBytes = m_space->write(m_used, m_cache->begin(), m_cache->getCount());
		fb_assert(writtenBytes == m_cache->getCount());
		m_used += m_cache->getCount();
		m_cache->clear();
	}
	return true;
}

ULONG DsqlBatch::DataCache::get(const UCHAR** buffer)
{
	if (m_used > m_got)
	{
		// get data from tempspace
		ULONG delta = m_cache->getCapacity() - m_cache->getCount();
		if (delta > m_used - m_got)
			delta = m_used - m_got;
		UCHAR* buf = m_cache->getBuffer(m_cache->getCount() + delta);
		buf += m_cache->getCount();
		const FB_UINT64 readBytes = m_space->read(m_got, buf, delta);
		fb_assert(readBytes == delta);
		m_got += delta;
	}

	if (m_cache->getCount())
	{
		// return buffer full of data
		*buffer = m_cache->begin();
		return m_cache->getCount();
	}

	// no more data
	*buffer = nullptr;
	return 0;
}

void DsqlBatch::DataCache::remained(ULONG size)
{
	if (!size)
		m_cache->clear();
	else
		m_cache->removeCount(0, m_cache->getCount() - size);
}

ULONG DsqlBatch::DataCache::left(ULONG size)
{
	fb_assert(false); // todo	(or remove)
	FB_UINT64 total = FB_UINT64(m_used) + size;
	if (total > MAX_ULONG)
		return MAX_ULONG;
	return total;
}

void DsqlBatch::DataCache::clear()
{
	m_cache->clear();
	if (m_space)
		m_space->releaseSpace(0, m_used);
	m_used = m_got = 0;
}
