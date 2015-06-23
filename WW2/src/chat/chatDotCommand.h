#pragma once
// Created: Thu Mar 04 12:00:00 2004
//
// Class and variable definitions for DotCommand & Index classes
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatClientMessage.h"
#include "chat/chatGroupsType.h"
#include "chat/chatLStringIDs.h"

const PermissionMask NO_MASK = 0;
const PermissionMask ALL_MASK = ~0ULL;

const PermissionMask _ADMINS = (IS_PLAYNET|GAME_MANAGER);

class Player;
class DotCommandIndex;

//////////////////////////////////////////////////////////////////////
// DotUsage maps a permission mask to a <usage/description> string pair
// this allows us to have multiple contextual descriptions of each
// dot command. Each DotCommand entry has a vector of rules

class DotUsage
{
public:
	PermissionMask	mask;
	// Set usage to LSTR_CAUTION if you want to hide the command from .help
	const LSTRING_ID usage;
	const LSTRING_ID detail;

	DotUsage(const ChatHandle& newMask, LSTRING_ID usageID, LSTRING_ID detailID) :
		mask(newMask),
		usage(usageID),
		detail(detailID)
	{ }

private:
	DotUsage& operator = (const DotUsage& rhs);
	DotUsage(const DotUsage& rhs);

public:
	bool isHidden() const;
};

typedef std::vector<const char*> ARGV;	//!< Vector of arguments.

//////////////////////////////////////////////////////////////////////
//! DotCommand class. Encapsulates a constructor which defines the
//! "rules" (permission masks) for access to the command, and the "action"
//! function which actually performs the commmand.

class DotCommand
{
public:
	const std::string m_command;		//!< The command itself.
	const std::string m_description;	//!< Internal description.
	bool m_public : 1;					//!< True if permission not required.
	bool m_playnetOnly : 1;			//!< Requires playnet priv to access.
	DotCommand* m_next;

public:
	typedef std::vector<const DotUsage*> Usages;
protected:
	Usages m_usages;
	CHAT_GROUPS m_groups;

private:
	DotCommand(const DotCommand& rhs);
	DotCommand& operator = (const DotCommand& rhs);

public:
	//! Constructor.
	DotCommand(const std::string& newCommand, const std::string& description
				, const ChatHandle& mask, LSTRING_ID usage, LSTRING_ID detail, bool playnetOnly=false);
	virtual ~DotCommand() { };

	//! Retrieves the master index of all dot commands.
	static DotCommandIndex* index();

	//! Transforms an incoming argument string into an ARGV (vector of arguments).
	static int vectorize(char* source, ARGV& dest, int maxArguments, bool translateSpaces=false);

	// Accessors
	const char* command() const { return m_command.c_str(); }
	const char* description() const { return m_description.c_str(); }
	void setAvailability(char level);
	void clearGroups() { m_groups.clear(); }
	void addGroup(UINT32 id) { m_groups.insert(id); }

	//! Add a new usage to this dot Command.
	void addUsage(const ChatHandle& mask, LSTRING_ID usage, LSTRING_ID detail)
	{
		m_usages.push_back(new DotUsage(mask, usage, detail));
	}

	//! Determine if a client has access to a dotCommand.
	const DotUsage* allow(Player& player) const;

	//! Display the contextual help for this client.
	//! @param[in]	player		Player to test.
	//! @return false if no permission.
	//! @note client is not const because we may send data to him.
	virtual bool help(Player& player) const;

	//! Virtual function that implements the actual command for "/command <something...>".
	//! Default implementation displays help so that if you do not
	//! overload, the player will get help when using the wrong
	//! command usage.
	//! @param[in]	player		Player who entered the command.
	//! @param[in]	message		Description of the incoming packet.
	//! @param[in]	arguments	Additional text following /command.
	virtual void actionWith(Player& player, struct Chat::ClientMessage* /*message*/, char* /*arguments*/) const
	{
		help(player);
	}

	//! Virtual function that implements the actual command for just "/command".
	//! Default implementation displays help so that if you do not
	//! overload, the player will get help when using the wrong
	//! command usage.
	//! @param[in]	player		Player who entered the command.
	//! @param[in]	message		Description of the incoming packet.
	virtual void action(Player& player, struct Chat::ClientMessage* /*message*/) const
	{
		help(player);
	}

	// Assistant function for checking if a command has been locked for < M minutes
	bool _featureLockedM(Player& player, UINT32 duration_m) const;

	//! Test if a player can use a chat feature yet.
	//! @param[in]	player		Player to be tested.
	//!	@param[in]	activity	Text label for the feature.
	//! @param[in]	interval_s	Seconds between accesses.
	//! @param[in]	now			Current time.
	//! @return true if the feature is locked, otherwise false.
	static bool lockedFeature(Player& player, const char* activity, UINT32 interval_s, UINT32 now);

	//! Send generic help text with a USAGE: and DETAIL: line to a player.
	//! @param[in]	player		Player to send to.
	//! @param[in]	usage		Summary of how to use the command.
	//! @param[in]	detail		Description of what the command does.
	//! @return Always returns true.
	bool genericHelp(Player& player, const char* usage, const char* detail) const;

	//! Test for public visibility of a command.
	bool isHidden() const;

	const Usages& usages() const { return m_usages; }
};

//! Convert a time-interval string into a number of seconds.
bool chatGetTimeInterval(class Player& player, const char* timestring, UINT32& interval, const char defaultUnit='m');

