#include <syscfg.h> // Path is project specific.
// Created: Tue Oct 21 12:47:30 CDT 2008
//
// Exchange tutorial progress stuff between client/host
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "tut/tutGroupProgress.h"
#include "teul/teulPackers.h"

//	------------------------------	Declarations

namespace Tutorial
{

//////////////////////////////////////////////////////////////////////
// Network ctor

GroupProgress::GroupProgress(ACTION_OBJECT* const a)
	: m_groupHandle(0LL), m_flags(0L)
{
	this->packer(TEUL_UNPACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Pack the individual Group Progress objects

void
GroupProgress::packer(LOGICAL mode, ACTION_OBJECT* a)
{
	a->packChatHandle(mode, &m_groupHandle) ;
	a->packer(mode, &m_flags) ;
}

}

