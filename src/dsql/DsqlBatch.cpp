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
#include "../jrd/exe_proto.h"
#include "../dsql/dsql.h"
#include "../dsql/errd_proto.h"
#include "../common/classes/ClumpletReader.h"

using namespace Firebird;
using namespace Jrd;

DsqlBatch::DsqlBatch(dsql_req* req, const dsql_msg* /*message*/, ClumpletReader& pb)
	: m_request(req), m_batch(NULL),
	  m_messages(req->getPool()),
	  m_blobs(req->getPool()),
	  m_messageSize(0),
	  m_flags(0),
	  m_bufferSize(10 * 1024 * 1024)
{
	m_messageSize = req->getStatement()->getSendMsg()->msg_length;

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

		case IBatch::BUFFER_BYTES_SIZE:
			m_bufferSize = pb.getBigInt();
			break;
		}
	}

	// todo - process message to find blobs in it

	m_messages.setBuf(m_bufferSize);
	// if (hasBlobs)
	//	m_blobs.setBuf(m_bufferSize);
}


DsqlBatch::~DsqlBatch()
{
	if (m_batch)
		m_batch->resetHandle();
}
/*
jrd_tra* DsqlBatch::getTransaction() const
{
	return m_request->req_transaction;
}
*/
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

	DsqlBatch* b = FB_NEW_POOL(req->getPool()) DsqlBatch(req, message, pb);
	req->req_batch = b;
	return b;
}

void DsqlBatch::add(thread_db* tdbb, unsigned count, const void* inBuffer)
{
	m_messages.put(inBuffer, count * m_messageSize);
}

void DsqlBatch::addBlob(thread_db* tdbb, unsigned length, const void* inBuffer, ISC_QUAD* blobId)
{
	Arg::Gds(isc_wish_list).raise();
}

void DsqlBatch::appendBlobData(thread_db* tdbb, unsigned length, const void* inBuffer)
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

	// todo - insert blobs here

	// execute request
	m_request->req_transaction = transaction;		// not sure really needed here...
	fb_assert(m_request->req_request);
	EXE_unwind(tdbb, m_request->req_request);
	EXE_start(tdbb, m_request->req_request, transaction);

	const dsql_msg* message = m_request->getStatement()->getSendMsg();
	fb_assert(message->msg_length == m_messageSize);
	unsigned remains;
	UCHAR* data;
	while ((remains = m_messages.get(&data)) > 0)
	{
		while (remains >= m_messageSize)
		{
			// todo - translate blob IDs here

			EXE_send(tdbb, m_request->req_request, message->msg_number, m_messageSize, data);

			data += m_messageSize;
			remains -= m_messageSize;
		}
		m_messages.remained(remains);
	}

	return NULL;
}

void DsqlBatch::cancel(thread_db* tdbb)
{
	m_messages.clear();
	m_blobs.clear();
}

void DsqlBatch::DataCache::setBuf(FB_UINT64 size)
{
	m_limit = size;
}

void DsqlBatch::DataCache::put(const void* data, unsigned dataSize)
{
	if (m_used + dataSize + m_cache.getCount() > m_limit)
		ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
			  Arg::Gds(isc_random) << "Internal buffer overflow - batch too big");

	m_cache.append(reinterpret_cast<const UCHAR*>(data), dataSize);
}

unsigned DsqlBatch::DataCache::get(UCHAR** buffer)
{
	*buffer = m_cache.begin();
	return m_cache.getCount();
}

void DsqlBatch::DataCache::remained(unsigned size)
{
	ERRD_post(Arg::Gds(isc_sqlerr) << Arg::Num(-104) <<
		  Arg::Gds(isc_random) << "Internal error: useless data remained in batch buffer");
}

void DsqlBatch::DataCache::clear()
{
	m_cache.clear();
}
