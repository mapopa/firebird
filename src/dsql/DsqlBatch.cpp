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
#include "../dsql/dsql.h"

using namespace Firebird;
using namespace Jrd;

DsqlBatch::DsqlBatch(dsql_req* req)
	: m_request(req), m_batch(NULL),
	  m_messages(req->getPool()),
	  m_blobs(req->getPool()),
	  m_messageSize(req->getStatement()->getSendMsg()->msg_length)
{ }


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
	return NULL;
}

