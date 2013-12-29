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

extern SquadList* chatFindSquadList(UINT32 squadID);
extern SquadList* chatFindSquadListByName(const char*);
extern void chatRequestSquad(UINT32 squadID, Player* player);
extern ChatHandle chatGetSquadHandle(UINT32 squadID);
extern const char* chatGetSquadCallsign(UINT32 squadID);
extern const char* chatGetSquadRankName(RANK_NO squadRank);
extern bool chatCheckSquadLists();
extern const char* chatFindSquadMotd(UINT32 squadID);

// Remove a squad from the in-memory squad list tables
// Note: doesn't remove the squad from the database
extern void chatRemoveSquadList(UINT32 squadID);

extern void chatSquadMakeCO(Player* player, SquadList* list);

extern void chatFetchSquadNames();
extern void chatAddSquadName(const char* squadName);
extern bool chatCheckForSquadName(const char* squadName);
extern void chatRemoveSquadName(const char* squadName);
extern void chatAddSquadName(const ChatHandle& handle);
extern bool chatCheckForSquadName(const ChatHandle& handle);
extern void chatRemoveSquadName(const ChatHandle& handle);

extern void chatRenameSquad(const Chat::Name& fromName, const Chat::Name& toName);

// Add/remove a player from the recruiter list
extern void chatUpdateRecruiterList(const Player* player, bool isRecruiting, class SquadList* squadList /* or NULL */);

// Send me a list of recruiters, please
extern void chatRequestRecruiter(Player* player, UCHAR language);

//! Load a squad member list from the database.
extern void chatRequestSquadMemberList(const UINT32 squadID);
