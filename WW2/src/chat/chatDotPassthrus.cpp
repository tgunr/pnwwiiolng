#include <syscfg.h> // Path is project specific.
// Date: Fri Sep 24 2004
//
// Chat dot commands that pass on thru to the strat host
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdexcept>

#include "chat/chatPlayer.h"
#include "chat/chatDotCommand.h"
#include "chat/chatHostMessages.h"
#include "chat/chatHost.h"

//////////////////////////////////////////////////////////////////////
// Private variables

//////////////////////////////////////////////////////////////////////
//// Private helper functions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//// Dot Command Definitions
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Pass a command thru to strat
//////////////////////////////////////////////////////////////////////

class DotPassthru : public DotCommand
{
public:
	DotPassthru(const std::string& newCommand, const std::string& cmdDescription
				, PermissionMask mask, LSTRING_ID usage, LSTRING_ID detail)
		: DotCommand(newCommand, cmdDescription, mask, usage, detail)
		{ }

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		player.sendStratCommand(command(), arguments, true);
	}
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		player.sendStratCommand(command(), "", true);
	}
};

//////////////////////////////////////////////////////////////////////
// Pass-thru commands to strat

// Arena lock controls

class DotArenaLock : public DotPassthru
{
public:
	DotArenaLock() : DotPassthru("arenalock", "Query or change the arena lock status"
								, GAME_MANAGER, LSTR_ARENALOCK_USAGE, LSTR_ARENALOCK_DETAIL
								) { }
} dotArenaLock;

// Detailed dump of CP/facility states
class DotCPDump : public DotPassthru
{
public:
	DotCPDump() : DotPassthru("cpdump", "Dumps CP and facility details for a town"
								, NO_MASK, LSTR_CPDUMP_USAGE, LSTR_CPDUMP_DETAIL
								) { }
} dotCPDump;

// Summary dump of CP state
class DotCPInfo : public DotPassthru
{
public:
	DotCPInfo() : DotPassthru("cpinfo", "Dumps CP details for a town"
								, NO_MASK, LSTR_CPINFO_USAGE, LSTR_CPINFO_DETAIL
								) { }
} dotCPInfo;

class DotDumpResupply : public DotPassthru
{
public:
	DotDumpResupply() : DotPassthru("dumpresupply", "Dumps information on resupply of a brigade"
									, NO_MASK, LSTR_DUMPRESUPLY_USAGE, LSTR_DUMPRESUPLY_DETAIL
									) {}
} dotDumpResupply;

// Brief summary of CP state
class DotFBs : public DotPassthru
{
public:
	DotFBs() : DotPassthru("fbs"
							, "Shows FB information for closest or named town"
							, NO_MASK, LSTR_FBS_USAGE, LSTR_FBS_DETAIL
							) { }
} dotFBs;

// Brief summary of CP state
class DotObjectives : public DotPassthru
{
public:
	DotObjectives() : DotPassthru("objectives"
							, "List current objectives for your side"
							, NO_MASK, LSTR_OBJECTIVES_USAGE, LSTR_OBJECTIVES_DETAIL
							) { }
} dotObjectives;

// Brief summary of CP state
class DotOwn : public DotPassthru
{
public:
	DotOwn() : DotPassthru("own"
							, "Shows ownership/status information for towns, bridges and fbs"
							, NO_MASK, LSTR_OWN_USAGE, LSTR_OWN_DETAIL
							) { }
} dotOwn;

class DotPwnCP : public DotPassthru
{
public:
	DotPwnCP() : DotPassthru("pwncp", "Change ownership of a CP (town)", NO_MASK, LSTR_PWNCP_USAGE, LSTR_PWNCP_DETAIL) { }
} dotPwnCP;

class DotRDP : public DotCommand
{
public:
	DotRDP() : DotCommand("rdp", "Displays current RDP intel", NO_MASK, LSTR_RDP_USAGE, LSTR_RDP_DETAIL) {}
	virtual void action(Player& player, Chat::ClientMessage* /*message*/) const override
	{
		if ( !player.isPlaynet() && _featureLockedM(player, 1) )
			return;
		player.sendStratCommand(command(), "", false);
	}
} dotRDP;

class DotRedeploy : public DotPassthru
{
public:
	DotRedeploy() : DotPassthru("redeploy", "Forcibly redeploy a brigade without checking movement rules or affecting resupply", NO_MASK, LSTR_REDEPLOY_USAGE, LSTR_REDEPLOY_DETAIL) {}
} dotRedeploy;

#if defined(BETA_RELEASE) || defined(TRAINING)
class DotSetTime : public DotCommand
{
public:
	DotSetTime() : DotCommand("settime", "Set the game time", NO_MASK, LSTR_SETTIME_USAGE, LSTR_SETTIME_DETAIL, false) {}

	virtual void actionWith(Player& player, Chat::ClientMessage* /*message*/, char* arguments) const override
	{
		if ( !player.isPlaynet() && _featureLockedM(player, 5) )
			return;

		player.sendStratCommand(command(), arguments);
	}
} dotSetTime;
#endif



//////////////////////////////////////////////////////////////////////
// Display status of a targetted firebase

class DotStatus : public DotPassthru
{
public:
	DotStatus() : DotPassthru("status", "Display damage status of targetted FB", NO_MASK, LSTR_STATUS_USAGE, LSTR_STATUS_DETAIL) {}
	virtual void actionWith(Player& player, Chat::ClientMessage* message, char* arguments) const override;
} dotStatus;

void
DotStatus::actionWith(Player& player, Chat::ClientMessage* /*message*/, char* /*arguments*/) const
{
	help(player);
	return;
}

//////////////////////////////////////////////////////////////////////
// Report the next move time of a brigade.

class DotNextMove : public DotPassthru
{
public:
	DotNextMove() : DotPassthru("nextmove", "Reports on the next movement time for a brigade", NO_MASK, LSTR_NEXTMOVE_USAGE, LSTR_NEXTMOVE_DETAIL) {}
} dotNextMove;

//////////////////////////////////////////////////////////////////////


