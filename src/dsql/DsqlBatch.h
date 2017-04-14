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

#ifndef DSQL_BATCH_H
#define DSQL_BATCH_H

#include "../jrd/TempSpace.h"
#include "../common/classes/alloc.h"

namespace Jrd {

class dsql_req;
class thread_db;
class JBatch;
class Attachment;

class DsqlBatch
{
public:
	DsqlBatch(dsql_req* req);
	~DsqlBatch();

	static const unsigned RAM_BATCH = 128 * 1024;

	static DsqlBatch* open(thread_db* tdbb, dsql_req* req, Firebird::IMessageMetadata* inMetadata, unsigned parLength, const UCHAR* par);

	Attachment* getAttachment() const;
	void setInterfacePtr(JBatch* interfacePtr) throw();

	void add(thread_db* tdbb, unsigned count, const void* inBuffer);
	void addBlob(thread_db* tdbb, unsigned length, const void* inBuffer, ISC_QUAD* blobId);
	void appendBlobData(thread_db* tdbb, unsigned length, const void* inBuffer);
	void addBlobStream(thread_db* tdbb, uint length, const Firebird::BlobStream* inBuffer);
	void registerBlob(thread_db* tdbb, const ISC_QUAD* existingBlob, ISC_QUAD* blobId);
	Firebird::IBatchCompletionState* execute(thread_db* tdbb);
	void cancel(thread_db* tdbb);

private:
	dsql_req* const m_request;
	JBatch* m_batch;

	class DataCache : public Firebird::PermanentStorage
	{
	public:
		DataCache(MemoryPool& p)
			: PermanentStorage(p), m_cache(p), m_used(0)
		{ }

		void put(const void* data, unsigned dataSize);
		unsigned get(UCHAR** buffer);

	private:
		Firebird::Array<UCHAR> m_cache;
		Firebird::AutoPtr<TempSpace> m_space;
		FB_UINT64 m_used, m_got;
	};

	DataCache m_messages, m_blobs;
	ULONG m_messageSize;
};

} // namespace

#endif // DSQL_BATCH_H
