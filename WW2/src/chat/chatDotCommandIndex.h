#pragma once
// Created: Sun Apr  2 04:40:34 CDT 2006
//
// Index of dot commands
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <vector>
#include <map>
#include <string>

class DotCommand;

using DOT_INDEX = std::vector<DotCommand*>;
using DOT_ALIASES = std::map<const std::string, const DotCommand*>;

class DotCommandIndex : public DOT_INDEX
{
private:
	// List of commands that we know to be aliases for each other
	DOT_ALIASES m_aliases;

public:
	// Register a new dot command into the index
	void add(DotCommand* dotCommand);

	// Register a new alias ('as' is an alias for old).
	void addAlias(const char* old, const char* alias);

	// Find the help for a dot command in the context of a specific client
	// Process a dot command from a user and apply the appropriate rules and
	bool helpFor(Player& player, const char* command) const;

	// Look for a matching dot command; if cleanMatchOnly is true, only
	// exact matches or unambiguous matches are returned

	const DotCommand* lookup(const std::string& command, bool cleanMatchOnly=false) const;
	// call the action function
	bool process(Player& player, struct Chat::ClientMessage* message) const;

	// Flush and load permission sets
	void readPermissions();

protected:
	//! [Re]Build the Teul::Packet with the list of dot commands.
	void buildCommandListPacket();

	//! Send all players the current command list packet.
	void broadcastCommandListPacket();
};

