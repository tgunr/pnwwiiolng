#pragma once
// Created: Sun Jan 13 19:56:51 CST 2008
//
// Structure for declaring: User No-longer Available
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Chat
{
	//Let's someone know that someone is NLA
	struct UserNLA
	{
		UCHAR		radioIndex;
		ChatHandle	handle;

		// Member functions
		SINT32 packer(PackMode mode, ACTION_OBJECT* a);
	};
}
