#pragma once
// Created: Sat Feb  4 15:02:43 CST 2006
//
// List my current friend list
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

using ARGV = std::vector<const char*>;

// Fetch the friends list for a player in the background
void chatFetchFriendsAndIgnoreList(class Player* player);

// List the friendslist of a player in the foreground
void chatFriendsList(class Player& player);

// Modify a player's social list; add or remove the named players
void chatModifySocialList(class Player& player, ARGV& args, bool ignore);

