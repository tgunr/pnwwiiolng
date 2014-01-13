#pragma once
// Created: Mon Feb  2 18:38:30 CST 2009
//
// Prototypes for chatGroupPermissions.cpp
//
// Copyright (C) PlayNet, Inc 1999-2009 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatGroupsType.h"

struct ChatPermissions;
const ChatPermissions* chatGroupPermissionsForGroups(const CHAT_GROUPS& groups);

// Load/update the master table of chat group permissions
void chatReadGroupPermissions();

// House keeping wrapper for chatReadGroupPermissions
bool chatHousekeepGroupPermissions();

// Read a comma-separated list of groups into a CHAT_GROUPS structure
void chatReadGroups(const char* commandSeparatedList, CHAT_GROUPS& into);

extern const ChatPermissions nullChatPermissions;

