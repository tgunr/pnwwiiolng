#pragma once
// Created: Sun Jan 13 20:19:43 CST 2008
//
// Chat::UpdateChannel structure
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Chat
{
	//Turns off or on the receiveStatus of a radioIndex
	struct UpdateChannel
	{
		UpdateChannel(UCHAR index, bool status) : radioIndex(index), receiveStatus(status) {}
		UpdateChannel() : radioIndex(0), receiveStatus(false) {}

		UCHAR	radioIndex;
		bool	receiveStatus;

		// Member functions
		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;
	} ;
}
