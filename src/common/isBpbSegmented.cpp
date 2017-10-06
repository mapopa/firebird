/*
 *	PROGRAM:		Firebird authentication
 *	MODULE:			isBpbSegmented.cpp
 *	DESCRIPTION:	Checks BPB for being segmented or stream
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2017 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"
#include "../common/classes/ClumpletReader.h"
#include "../common/StatusArg.h"

using namespace Firebird;

namespace fb_utils {

bool isBpbSegmented(unsigned parLength, const unsigned char* par)
{
	if (parLength && !par)
	{
		(Arg::Gds(isc_random) << "Malformed BPB").raise();
	}

	ClumpletReader bpb(ClumpletReader::Tagged, par, parLength);
	if (bpb.getBufferTag() != isc_bpb_version1)
	{
		(Arg::Gds(isc_random) << "Malformed BPB").raise();
	}

	if (!bpb.find(isc_bpb_type))
	{
		return true;
	}
	int type = bpb.getInt();
	return type & isc_bpb_type_stream ? false : true;
}

} // namespace fb_utils

