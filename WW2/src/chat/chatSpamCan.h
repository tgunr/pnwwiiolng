#pragma once
// Created: Thu Oct  6 16:03:24 CDT 2005
//
// Spam can variables and types
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

using SPAM_CAN = std::vector<std::string>;

extern SPAM_CAN theSpamCan;
extern UINT32 spamLoops;

extern TeulTimeMS nextSpamTime;
extern UINT32 nextSpamLine;
extern UINT32 spamInterval;

static const UINT32 LONG_SPAM_INTERVAL_MS   =	10 * 1000;	// 10 seconds long haul
static const UINT32 SHORT_SPAM_INTERVAL_MS	=	 	 4000;	// 3 seconds
