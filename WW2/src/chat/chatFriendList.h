#pragma once
// Created: Sat Feb  4 15:02:43 CST 2006
//
// List my current friend list
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

typedef std::vector<const char*> ARGV;

// Fetch the friends list for a player in the background
extern void chatFetchFriendsAndIgnoreList(class Player* player);

// List the friendslist of a player in the foreground
extern void chatFriendsList(class Player& player);

// Modify a player's social list; add or remove the named players
extern void chatModifySocialList(class Player& player, ARGV& args, bool ignore);

