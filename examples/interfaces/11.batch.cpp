/*
 *	PROGRAM:	Object oriented API samples.
 *	MODULE:		11.batch.cpp
 *	DESCRIPTION:	A trivial sample of using Batch interface.
 *
 *					Example for the following interfaces:
 *					IBatch - interface to work with FB pipes
 *
 *	c++ 11.batch.cpp -Wl,-rpath,../../gen/Debug/firebird/lib -L../../gen/Debug/firebird/lib -lfbclient -o batch
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
 *  Copyright (c) 2016 Alexander Peshkoff <peshkoff@mail.ru>
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

	try
	{
		// attach employee db
		att = prov->attachDatabase(&status, "employee", 0, NULL);
		tra = att->startTransaction(&status, 0, NULL);

		// cleanup
		att->execute(&status, tra, 0, "delete from country where currency='lim'", SAMPLES_DIALECT,
			NULL, NULL, NULL, NULL);

		// Message to store in a table
		FB_MESSAGE(Msg, ThrowStatusWrapper,
			(FB_VARCHAR(15), country)
			(FB_VARCHAR(10), currency)
		) msg(&status, master);
		msg.clear();

		// create batch
		const char* sqlStmt = "insert into country values(?, ?)";
		batch = att->createBatch(&status, tra, 0, sqlStmt, SAMPLES_DIALECT, msg.getMetadata(), 0, NULL);

		// fill batch with data
		msg->country.set("Lemonia");
		msg->currency.set("lim");
		batch->add(&status, 1, msg.getData());

		msg->country.set("Banania");
		msg->currency.set("lim");
		batch->add(&status, 1, msg.getData());

		// and execute it
		cs = batch->execute(&status, tra);
		printf("upcount=%d\n", cs->getSize(&status));

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

	status.dispose();
	prov->release();

	return rc;
}
