#include <syscfg.h> // Path is project specific.
// Created: Mon Apr 25 15:42:25 CDT 2011
//
// Copyright (c) 2011 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
//
// Routines for handling server transfer objects.

#include "dbug/dbug.h"
#include "server/serverPersonaMirror.h"
#include "server/serverPersonaMirrorReply.h"

Server::Transfer::xferid_t Server::Transfer::g_xferID = 0 ;

//////////////////////////////////////////////////////////////////////
// Transfer (base class) doesn't allow onReceive events that
// aren't overloaded, but we can't just make it abstract at this
// point in time because of PersonaXfer :/

SINT32
Server::Transfer::onReceive(SINT32 teulIndex)
{
	LogAbort("t:%d raised Transfer::onReceive in an object that doesn't anticpate receiving an onReceive.", teulIndex) ;
}


SINT32
Server::PersonaMirror::onComplete(SINT32 /*resultCode*/) const
{
	LogAbort("xfer:%llu unhandled PersonaMirror::onComplete", xferID()) ;
}
