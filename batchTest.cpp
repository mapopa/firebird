/*
 *	export p=~/firebird/batch/gen/Debug/firebird; c++ -I$p/include batchTest.cpp -Wl,-rpath,$p/lib -L$p/lib -lfbclient -o batch
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ibase.h>
#include <firebird/Interface.h>
#include <firebird/Message.h>

#define SAMPLES_DIALECT SQL_DIALECT_V6


using namespace Firebird;

static IMaster* master = fb_get_master_interface();

static void errPrint(IStatus* status)
{
	char buf[256];
	master->getUtilInterface()->formatStatus(buf, sizeof(buf), status);
	fprintf(stderr, "%s\n", buf);
}

static int usage()
{
	printf("Usage: batchTest [-m(ulti error)] [-r(ecord counts)] [-b(lobs test)] [-u(ser blob IDs)] [-s(tream of messages/blobs)] [-n(etwork)] countries loops batches\n");
	return 1;
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

int main(int ac, char** av)
{
	int me = 0, rec = 0, bt = 0, net = 0;
	ISC_UINT64 userId = 0;
	unsigned char* blobStr = NULL;
	unsigned char* messageStr = NULL;

	while (ac > 4)
	{
		if (av[1][0] == '-')
		{
			switch (av[1][1])
			{
			case 'm':
				me = 1;
				break;
			case 'r':
				rec = 1;
				break;
			case 'b':
				bt = 1;
				break;
			case 'n':
				net = 1;
				break;
			case 'u':
				userId = 1;
				break;
			case 's':
				blobStr = new unsigned char[1024 * 1024];
				messageStr = new unsigned char[1024 * 1024];
				userId = 1;			// required for stream mode
				break;
			default:
				return usage();
			}
			av++;
			ac--;
		}
		else
			break;
	}

	if (ac != 4)
		return usage();
	const int countries = atoi(av[1]);
	const int loops = atoi(av[2]);
	const int batches = atoi(av[3]);

	int rc = 0;

	// set default password if none specified in environment
	setenv("ISC_USER", "sysdba", 0);
	setenv("ISC_PASSWORD", "masterkey", 0);

	// With ThrowStatusWrapper passed as status interface FbException will be thrown on error
	ThrowStatusWrapper status(master->getStatus());
	IStatus* s2 = master->getStatus();

	// Declare pointers to required interfaces
	IProvider* prov = master->getDispatcher();
	IUtil* utl = master->getUtilInterface();
	IAttachment* att = NULL;
	ITransaction* tra = NULL;
	IBatch* batch = NULL;
	IBatchCompletionState* cs = NULL;
	IXpbBuilder* pb = NULL;

	try
	{
		// attach employee db
		char db[100];
		if (net) strcpy(db, "localhost:");
		else db[0] = 0;
		strcat(db, "batchFull.fdb");
		att = prov->attachDatabase(&status, db, 0, NULL);
		tra = att->startTransaction(&status, 0, NULL);

		// cleanup
		att->execute(&status, tra, 0, "delete from country where currency='lim'", SAMPLES_DIALECT,
			NULL, NULL, NULL, NULL);

		pb = utl->getXpbBuilder(&status, IXpbBuilder::BATCH, NULL, 0);
		pb->insertInt(&status, IBatch::MULTIERROR, me);
		pb->insertInt(&status, IBatch::RECORD_COUNTS, rec);
		pb->insertInt(&status, IBatch::BLOB_IDS, blobStr ? IBatch::BLOB_IDS_STREAM : userId ? IBatch::BLOB_IDS_USER : IBatch::BLOB_IDS_ENGINE);

		// Message to store in a table
		FB_MESSAGE(Msg, ThrowStatusWrapper,
			(FB_VARCHAR(15), country)
			(FB_VARCHAR(10), currency)
			(FB_BLOB, b1)
			(FB_BLOB, b2)
		) msg(&status, master);
		msg.clear();

		int startCountry = 0;
		for (int b = 0; b < batches; ++b, startCountry += countries)
		{
			// create batch
			const char* sqlStmt = "insert into country values(?, ?, ?, ?)";
			IMessageMetadata* meta = msg.getMetadata();
			batch = att->createBatch(&status, tra, 0, sqlStmt, SAMPLES_DIALECT, meta,
				pb->getBufferLength(&status), pb->getBuffer(&status));

			unsigned mesAlign = meta->getAlignment(&status);
			unsigned mesLength = meta->getMessageLength(&status);
			unsigned blobAlign = batch->getBlobAlignment(&status);

			unsigned char* sblob = align(blobStr, blobAlign);
			unsigned char* smessage = align(messageStr, mesAlign);

			printf("fill batch with data\n");

			// fill batch with data
			for (int l = 0; l < loops; ++l)
			{
				for (int c = 0; c < countries; ++c)
				{
					char country[32];
					sprintf(country, "Country-%d", startCountry + c);
					msg->country.set(country);
					msg->currency.set("lim");

					if (bt)
					{
						const char* b2Text = "b2Text";
						if (c == 1)
						{
							msg->b1Null = 0;
							ISC_QUAD realId;
							IBlob* blob = att->createBlob(&status, tra, &realId, 0, NULL);
							const char* text = "Blob created using traditional API";
							blob->putSegment(&status, strlen(text), text);
							blob->close(&status);

							if (userId)
							{
								memcpy(&msg->b1, &userId, 8);
								++userId;
							}
							batch->registerBlob(&status, &realId, &msg->b1);
						}
						else if ((c + l) % 4 == 0)
						{
							msg->b1Null = -1;
							b2Text = "batch = att->createBatch(&status, tra, 0, sqlStmt, SAMPLES_DIALECT, msg.getMetadata(), pb->getBufferLength(&status), pb->getBuffer(&status));";
						}
						else
						{
							msg->b1Null = 0;
							const char* b1Text = "b1Text";
							if (userId)
							{
								memcpy(&msg->b1, &userId, 8);
								++userId;
							}
							if (blobStr)
								putBlob(sblob, b1Text, strlen(b1Text), blobAlign, &msg->b1);
							else
								batch->addBlob(&status, strlen(b1Text), b1Text, &msg->b1);
						}

						msg->b2Null = 0;
						if (userId)
						{
							memcpy(&msg->b2, &userId, 8);
							++userId;
						}
						if (blobStr)
							putBlob(sblob, b2Text, strlen(b2Text), blobAlign, &msg->b2);
						else
							batch->addBlob(&status, strlen(b2Text), b2Text, &msg->b2);
					}
					else
					{
						msg->b1Null = -1;
						msg->b2Null = -1;
					}

					if (messageStr)
						putMsg(smessage, msg.getData(), mesLength, mesAlign);
					else
						batch->add(&status, 1, msg.getData());
				}
			}

			// sent streams to batch if needed
			if (messageStr)
			{
				batch->addBlobStream(&status, sblob - blobStr, blobStr);
				unsigned cnt = (smessage - messageStr) / align(mesLength, mesAlign);
				batch->add(&status, cnt, messageStr);
			}

			printf("execute batch\n");

			// and execute it
			cs = batch->execute(&status, tra);
			unsigned upcount = cs->getSize(&status);
			unsigned unk = 0, succ = 0;
			for (unsigned p = 0; p < upcount; ++p)
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
			for(unsigned p = 0; (p = cs->findError(&status, p)) != IBatchCompletionState::NO_MORE_ERRORS; ++p)
			{
				cs->getStatus(&status, s2, p);
				char text[1024];
				utl->formatStatus(text, sizeof(text) - 1, s2);
				text[sizeof(text) - 1] = 0;
				printf("Message %u: %s\n", p, text);
			}

			// cleanup
			batch->release();
			batch = NULL;
		}

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
	if (pb)
		pb->dispose();
	if (batch)
		batch->release();
	if (tra)
		tra->release();
	if (att)
		att->release();

	status.dispose();
	s2->dispose();
	prov->release();

	return rc;
}
