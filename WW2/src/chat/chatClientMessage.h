#pragma once
// Created: Sun Jan 13 15:44:03 CST 2008
//
// Chat::ClientMessage structure
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatDefines.h"
#include "wwii/wwiiFullObjID.h"

namespace Chat
{
	// Message sent from client to host
	struct ClientMessage
	{
		UINT32		m_id;		// Messages are assigned IDs by the host
								// to prevent duplicates
		UINT32		m_length;	// The length of the message in bytes
		UINT16		m_radios;	// Bitmap of radios the message was
								// transmitted on. Set to 0 for a system
								// message (?)
		UCHAR		m_country;
		ChatHandle	m_handle;	// Packed callsign of sender
		char		m_message[MAX_MESSAGE_LENGTH+1];
		FULL_OBJ_ID	m_uiTarget;	// CP or Facility targetted in UI

		// Packer
		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a);
	};
}

