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
#include "../common/classes/RefCounted.h"
#include "../common/classes/vector.h"
#include "../common/classes/GenericMap.h"

#define DEB_BATCH(x)


namespace Firebird {

class ClumpletReader;

}


namespace Jrd {

class dsql_req;
class dsql_msg;
class thread_db;
class JBatch;
class Attachment;

class DsqlBatch
{
public:
	DsqlBatch(dsql_req* req, const dsql_msg* message, Firebird::IMessageMetadata* inMetadata,
		Firebird::ClumpletReader& pb);
	~DsqlBatch();

#ifdef DEV_BUILD
	static const ULONG RAM_BATCH = 256;
	static const ULONG BUFFER_LIMIT = 2 * 1024;
	static const ULONG DETAILED_LIMIT = 4;
#else // DEV_BUILD
	static const ULONG RAM_BATCH = 128 * 1024;
	static const ULONG BUFFER_LIMIT = 10 * 1024 * 1024;
	static const ULONG DETAILED_LIMIT = 64;
#endif // DEV_BUILD
	static const ULONG SIZEOF_BLOB_HEAD = sizeof(ISC_QUAD) + sizeof(ULONG);
	static const unsigned BLOB_STREAM_ALIGN = 4;

	static DsqlBatch* open(thread_db* tdbb, dsql_req* req, Firebird::IMessageMetadata* inMetadata,
		unsigned parLength, const UCHAR* par);

	Attachment* getAttachment() const;
	void setInterfacePtr(JBatch* interfacePtr) throw();

	void add(thread_db* tdbb, ULONG count, const void* inBuffer);
	void addBlob(thread_db* tdbb, ULONG length, const void* inBuffer, ISC_QUAD* blobId);
	void appendBlobData(thread_db* tdbb, ULONG length, const void* inBuffer);
	void addBlobStream(thread_db* tdbb, uint length, const void* inBuffer);
	void registerBlob(thread_db* tdbb, const ISC_QUAD* existingBlob, ISC_QUAD* blobId);
	Firebird::IBatchCompletionState* execute(thread_db* tdbb);
	Firebird::IMessageMetadata* getMetadata(thread_db* tdbb);
	void cancel(thread_db* tdbb);

private:
	void genBlobId(ISC_QUAD* blobId);
	void blobPrepare();
	void blobCheckMode(bool stream, const char* fname);
	void blobCheckMeta();
	void registerBlob(const ISC_QUAD* engineBlob, const ISC_QUAD* batchBlob);

	dsql_req* const m_request;
	JBatch* m_batch;
	Firebird::RefPtr<Firebird::IMessageMetadata> m_meta;

	class DataCache : public Firebird::PermanentStorage
	{
	public:
		DataCache(MemoryPool& p)
			: PermanentStorage(p),
			  m_used(0), m_got(0), m_limit(0), m_shift(0)
		{ }

		void setBuf(ULONG size);

		void put(const void* data, ULONG dataSize);
		void put3(const void* data, ULONG dataSize, ULONG offset);
		void align(ULONG alignment);
		bool done();
		ULONG get(UCHAR** buffer);
		void remained(ULONG size, ULONG alignment = 0);
		ULONG getSize() const;
		void clear();

	private:
		typedef Firebird::Vector<UCHAR, DsqlBatch::RAM_BATCH, SINT64> Cache;
		Firebird::AutoPtr<Cache> m_cache;
		Firebird::AutoPtr<TempSpace> m_space;
		ULONG m_used, m_got, m_limit, m_shift;
	};

	struct BlobMeta
	{
		unsigned nullOffset, offset;
	};

	class QuadComparator
	{
	public:
	    static bool greaterThan(const ISC_QUAD& i1, const ISC_QUAD& i2)
    	{
        	return memcmp(&i1, &i2, sizeof(ISC_QUAD)) > 0;
	    }
	};

	DataCache m_messages, m_blobs;
	Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<ISC_QUAD, ISC_QUAD> >, QuadComparator> m_blobMap;
	Firebird::HalfStaticArray<BlobMeta, 4> m_blobMeta;
	ISC_QUAD m_genId;
	ULONG m_messageSize, m_alignedMessage, m_alignment, m_flags, m_detailed, m_bufferSize, m_lastBlob;
	bool m_setBlobSize;
	UCHAR m_blobPolicy;
};

} // namespace

#endif // DSQL_BATCH_H
