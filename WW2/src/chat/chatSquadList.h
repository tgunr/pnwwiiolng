#pragma once
// Created: Tue Sep 13 14:42:08 CDT 2005
//
// Functions/definitions for working with a squadlist
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatName.h"

class SquadList;
class Player;

SquadList* chatFindSquadList(UINT32 squadID);
SquadList* chatFindSquadListByName(const char*);
void chatRequestSquad(UINT32 squadID, Player* player);
ChatHandle chatGetSquadHandle(UINT32 squadID);
const char* chatGetSquadCallsign(UINT32 squadID);
const char* chatGetSquadRankName(RANK_NO squadRank);
bool chatCheckSquadLists();
const char* chatFindSquadMotd(UINT32 squadID);

// Remove a squad from the in-memory squad list tables
// Note: doesn't remove the squad from the database
void chatRemoveSquadList(UINT32 squadID);

void chatSquadMakeCO(Player* player, SquadList* list);

void chatFetchSquadNames();
void chatAddSquadName(const char* squadName);
bool chatCheckForSquadName(const char* squadName);
void chatRemoveSquadName(const char* squadName);
void chatAddSquadName(const ChatHandle handle);
bool chatCheckForSquadName(const ChatHandle handle);
void chatRemoveSquadName(const ChatHandle handle);

void chatRenameSquad(const Chat::Name& fromName, const Chat::Name& toName);

// Add/remove a player from the recruiter list
void chatUpdateRecruiterList(const Player* player, bool isRecruiting, class SquadList* squadList /* or NULL */);

// Send me a list of recruiters, please
void chatRequestRecruiter(Player* player, UCHAR language);

//! Load a squad member list from the database.
void chatRequestSquadMemberList(UINT32 squadID);
void chatRequestSquadMemberListImmediate(UINT32 squadID);

// For use by chatSquadControls
#include "wwii/wwiiSquads.h"
#include "teul/teulNetworkObject.h"

class DBSquad : public Squad
{
public:
    DBSquad() : Squad() {}
    DBSquad(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }
    ~DBSquad() {}
};

using CHATDB_SQUAD_XCHG = teul_network_object<DBSquad>;

