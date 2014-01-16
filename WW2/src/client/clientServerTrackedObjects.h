#pragma once
// Created: Fri Mar 24 16:27:26 CST 2006
//
// Client-side handling of STOs
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

bool clientReceiveServerTrackedObject(UINT16 type, ACTION_OBJECT* a);
void clientSendPendingSTOJoules();
