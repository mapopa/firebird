/*
 *	PROGRAM:	Object oriented API samples.
 *	MODULE:		11.batch.cpp
 *	DESCRIPTION:	A trivial sample of using Batch interface.
 *
 *					Example for the following interfaces:
 *					IBatch - interface to work with FB pipes
 *
 *	c++ 11.batch.cpp -lfbclient
 *
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2017 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "ifaceExamples.h"
#include <firebird/Message.h>

static IMaster* master = fb_get_master_interface();

static void errPrint(IStatus* status)
{
	char buf[256];
	master->getUtilInterface()->formatStatus(buf, sizeof(buf), status);
	fprintf(stderr, "%s\n", buf);
}

template <typename T>
static inline T align(T target, uintptr_t alignment)
{
	return (T) ((((uintptr_t) target) + alignment - 1) & ~(alignment - 1));
}

static void putMsg(unsigned char*& ptr, const void* from, unsigned size, unsigned alignment)
{
	memcpy(ptr, from, size);
	ptr += align(size, alignment);
}

static void putBlob(unsigned char*& ptr, const void* from, unsigned size, unsigned alignment, ISC_QUAD* id)
{
	memcpy(ptr, id, sizeof(ISC_QUAD));
	ptr += sizeof(ISC_QUAD);
	memcpy(ptr, &size, sizeof(unsigned));
	ptr += sizeof(unsigned);
	memcpy(ptr, from, size);
	ptr += size;
	ptr = align(ptr, alignment);
}

static void print_cs(ThrowStatusWrapper& status, IBatchCompletionState* cs, IUtil* utl)
{
	unsigned p = 0;
	IStatus* s2 = NULL;

	unsigned upcount = cs->getSize(&status);
	unsigned unk = 0, succ = 0;
	for (p = 0; p < upcount; ++p)
	{
		int s = cs->getState(&status, p);
		switch (s)
		{
		case IBatchCompletionState::EXECUTE_FAILED:
			printf("Message %u - execute failed\n", p);
			break;

		case IBatchCompletionState::SUCCESS_NO_INFO:
			++unk;
			break;

		default:
			printf("Message %u - %d updated records\n", p, s);
			++succ;
			break;
		}
	}
	printf("total=%u success=%u success(but no update info)=%u\n", upcount, succ, unk);

	s2 = master->getStatus();
	for(p = 0; (p = cs->findError(&status, p)) != IBatchCompletionState::NO_MORE_ERRORS; ++p)
	{
		try
		{
			cs->getStatus(&status, s2, p);

			char text[1024];
			utl->formatStatus(text, sizeof(text) - 1, s2);
			text[sizeof(text) - 1] = 0;
			printf("Message %u: %s\n", p, text);
		}
		catch (const FbException& error)
		{
			// handle error
			fprintf(stderr, "Message %u: ", p);
			errPrint(error.getStatus());
		}
	}

	printf("\n");
	if (s2)
		s2->dispose();
}

int main()
{
	int rc = 0;

	// set default password if none specified in environment
	setenv("ISC_USER", "sysdba", 0);
	setenv("ISC_PASSWORD", "masterkey", 0);

	// With ThrowStatusWrapper passed as status interface FbException will be thrown on error
	ThrowStatusWrapper status(master->getStatus());

	// Declare pointers to required interfaces
	IProvider* prov = master->getDispatcher();
	IUtil* utl = master->getUtilInterface();
	IAttachment* att = NULL;
	ITransaction* tra = NULL;
	IBatch* batch = NULL;
	IBatchCompletionState* cs = NULL;
	IXpbBuilder* pb = NULL;

	unsigned char streamBuf[10240];		// big enough for demo
	unsigned char* stream = NULL;

	try
	{
		// attach employee db
		att = prov->attachDatabase(&status, "employee", 0, NULL);
		tra = att->startTransaction(&status, 0, NULL);

		// cleanup
		att->execute(&status, tra, 0, "delete from project where proj_id like 'BAT%'", SAMPLES_DIALECT,
			NULL, NULL, NULL, NULL);

		//
		// Part 1. Simple messages. Adding one by one or by groups of messages.
		//

		// Message to store in a table
		FB_MESSAGE(Msg1, ThrowStatusWrapper,
			(FB_VARCHAR(5), id)
			(FB_VARCHAR(10), name)
		) project1(&status, master);
		project1.clear();
		IMessageMetadata* meta = project1.getMetadata();

		// sizes & alignments
		unsigned mesAlign = meta->getAlignment(&status);
		unsigned mesLength = meta->getMessageLength(&status);
		unsigned char* streamStart = align(streamBuf, mesAlign);

		// set batch parameters
		pb = utl->getXpbBuilder(&status, IXpbBuilder::BATCH, NULL, 0);
		pb->insertInt(&status, IBatch::RECORD_COUNTS, 1);

		// create batch
		const char* sqlStmt1 = "insert into project(proj_id, proj_name) values(?, ?)";
		batch = att->createBatch(&status, tra, 0, sqlStmt1, SAMPLES_DIALECT, meta,
			pb->getBufferLength(&status), pb->getBuffer(&status));

		// fill batch with data record by record
		project1->id.set("BAT11");
		project1->name.set("SNGL_REC");
		batch->add(&status, 1, project1.getData());

		project1->id.set("BAT12");
		project1->name.set("SNGL_REC2");
		batch->add(&status, 1, project1.getData());

		// execute it
		cs = batch->execute(&status, tra);
		print_cs(status, cs, utl);

		// fill batch with data using many records at once
		stream = streamStart;

		project1->id.set("BAT13");
		project1->name.set("STRM_REC_A");
		putMsg(stream, project1.getData(), mesLength, mesAlign);

		project1->id.set("BAT14");
		project1->name.set("STRM_REC_B");
		putMsg(stream, project1.getData(), mesLength, mesAlign);

		project1->id.set("BAT15");
		project1->name.set("STRM_REC_C");
		putMsg(stream, project1.getData(), mesLength, mesAlign);

		batch->add(&status, 3, streamStart);

		stream = streamStart;

		project1->id.set("BAT15");		// PK violation
		project1->name.set("STRM_REC_D");
		putMsg(stream, project1.getData(), mesLength, mesAlign);

		project1->id.set("BAT16");
		project1->name.set("STRM_REC_E");
		putMsg(stream, project1.getData(), mesLength, mesAlign);

		batch->add(&status, 1, streamStart);

		// execute it
		cs = batch->execute(&status, tra);
		print_cs(status, cs, utl);

		// close batch
		batch->release();
		batch = NULL;

		//
		// Part 2. Simple BLOBs. Multiple errors return.
		//

		// Message to store in a table
		FB_MESSAGE(Msg2, ThrowStatusWrapper,
			(FB_VARCHAR(5), id)
			(FB_VARCHAR(10), name)
			(FB_BLOB, desc)
		) project2(&status, master);
		project2.clear();
		meta = project2.getMetadata();

		mesAlign = meta->getAlignment(&status);
		mesLength = meta->getMessageLength(&status);
		streamStart = align(streamBuf, mesAlign);

		pb->clear(&status);
		pb->insertInt(&status, IBatch::MULTIERROR, 1);
		pb->insertInt(&status, IBatch::BLOB_IDS, IBatch::BLOB_IDS_ENGINE);
		//pb->insertInt(&status, IBatch::BLOB_IDS, blobStr ? IBatch::BLOB_IDS_STREAM : userId ? IBatch::BLOB_IDS_USER : IBatch::BLOB_IDS_ENGINE);

		// create batch
		const char* sqlStmt2 = "insert into project(proj_id, proj_name, proj_desc) values(?, ?, ?)";
		batch = att->createBatch(&status, tra, 0, sqlStmt2, SAMPLES_DIALECT, meta,
			pb->getBufferLength(&status), pb->getBuffer(&status));

		// fill batch with data
		project2->id.set("BAT21");
		project2->name.set("SNGL_BLOB");
		batch->addBlob(&status, strlen(sqlStmt2), sqlStmt2, &project2->desc);
		batch->add(&status, 1, project2.getData());

		// execute it
		cs = batch->execute(&status, tra);
		print_cs(status, cs, utl);

		// fill batch with data
		project2->id.set("BAT22");
		project2->name.set("SNGL_REC1");
		batch->addBlob(&status, strlen(sqlStmt1), sqlStmt1, &project2->desc);
		batch->add(&status, 1, project2.getData());

		project2->id.set("BAT22");		// PK violation
		project2->name.set("SNGL_REC2");
		batch->addBlob(&status, 2, "r2", &project2->desc);
		batch->add(&status, 1, project2.getData());

		project2->id.set("BAT23");
		project2->name.set("SNGL_REC3");
		batch->addBlob(&status, 2, "r3", &project2->desc);
		batch->add(&status, 1, project2.getData());

		project2->id.set("BAT23");		// PK violation
		project2->name.set("SNGL_REC4");
		batch->addBlob(&status, 2, "r4", &project2->desc);
		batch->add(&status, 1, project2.getData());

		// execute it
		cs = batch->execute(&status, tra);
		print_cs(status, cs, utl);

		// cleanup
		batch->release();
		batch = NULL;
		tra->commit(&status);
		tra = NULL;
		att->detach(&status);
		att = NULL;
	}
	catch (const FbException& error)
	{
		// handle error
		rc = 1;
		errPrint(error.getStatus());
	}

	// release interfaces after error caught
	if (cs)
		cs->dispose();
	if (batch)
		batch->release();
	if (tra)
		tra->release();
	if (att)
		att->release();

	// cleanup
	if (pb)
		pb->dispose();
	status.dispose();
	prov->release();

	return rc;
}
