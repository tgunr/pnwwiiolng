#pragma once
// Created: Sat Mar 25 20:37:05 CST 2006
//
// Host functions for manipulating server tracked objects
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

extern void chatReceiveSTO(UINT16 type, ACTION_OBJECT* a) ;
extern bool chatAddServerTrackedObject(class ServerTrackedObject* sto) ;
extern void chatRemoveServerTrackedObject(UINT32 stoUID) ;
extern bool chatPatrolSTOExpirations() ;

extern void chatLoadPPOXML() ;
