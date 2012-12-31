#include <syscfg.h> // Path is project specific.
// Date: Thu Jul  5 12:04:31 2001
//
// Pack functions for the chat management tool.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "chat/chatManagementTypes.h"

using namespace Chat ;

//////////////////////////////////////////////////////////////////////
// Pack the version 2 management structure

SINT32
ManagementLogin2::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->packer(mode, &m_version) ;
	a->packer(mode, &m_wantHandle) ;
	return a->offset ;
}
