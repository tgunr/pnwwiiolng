#include <syscfg.h> // Path is project specific.
// Date: Thu Mar 04 12:00:00 2004
//
// Provides the DotCommand and DotCommandIndex classes
//
// Copyright (c) 2004 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <stdexcept>

#include "versionNo.h"

#include "chat/chatDB.h"
#include "chat/chatDotCommand.h"
#include "chat/chatDotCommandIndex.h"
#include "chat/chatCommandWithArgs.h"
#include "chat/chatConstants.h"
#include "chat/chatHostMessages.h"
#include "chat/chatPlayer.h"
#include "chat/chatTimeInterval.h"
#include "include/mydbQuery.h"
#include "teul/teulTime.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiThreadText.h"
#include "teul/teulPacket.h"	// For the all-commands list.
#include "chat/chatDotCommandList.h"
#include "chat/chatHostPlayerIndex.h"
#include "chat/chatLStringErr.h"

using Chat::ARGV;

static Teul::Packet* g_commandListPacket = nullptr;

DotCommand* g_commands;

//////////////////////////////////////////////////////////////////////
// Add all the queued up commands to the index.

void
chatRegisterDotCommands()
{
	for ( auto cmd = g_commands; cmd != nullptr; cmd = cmd->m_next )
	{
		DotCommand::index()->add(cmd);
	}
}

//////////////////////////////////////////////////////////////////////
// Test if dot command usage is hidden.

bool
DotUsage::isHidden() const
{
	return usage == LSTR_NONE;
}

//////////////////////////////////////////////////////////////////////
// LStringErr constructors.

Chat::LStringErr::LStringErr(const char* err)
	: m_lstring(LSTR_GENERIC), m_arg1(err), m_arg2("")
{
}

Chat::LStringErr::LStringErr(const char* err, const char* arg)
	: m_lstring(LSTR_LABELLED_GENERIC), m_arg1(err), m_arg2(arg)
{
}

//////////////////////////////////////////////////////////////////////
// DotCommand class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Constructor

DotCommand::DotCommand(const std::string& newCommand, const std::string& dotDescription
						, const PermissionMask mask, LSTRING_ID usage, LSTRING_ID detail
						, bool playnetOnly)
	: m_command(newCommand)
	, m_description(dotDescription)
	, m_public(false)				// Never assume its public
	, m_playnetOnly(playnetOnly)
	, m_next(nullptr)
	, m_usages()
{
	addUsage(mask, usage, detail);
	m_next = g_commands;
	g_commands = this;
}

//////////////////////////////////////////////////////////////////////
// Helper function for isolating unwated characters

static inline LOGICAL
_isMessageNoise(SCHAR c)
{
	return ( c != 0 && (UCHAR)c <= (UCHAR)' ' );
}

//////////////////////////////////////////////////////////////////////
// Transform the incoming argument string into a vector

int
DotCommand::vectorize(char* source, ARGV& dest, int maxArguments, bool translate)
{
	int arguments = 0;

	// Remove any current entries
	dest.clear();
	if ( *source == 0 )
		return 0;

	while ( *source != 0 && arguments < maxArguments )
	{
		// Strip leading spaces from each argument
		while ( _isMessageNoise(*source) )
			*(source++) = 0;

		if ( *source == 0 )
			break;

		if ( *source == '"' || *source == '\'' )
		{
			char terminator = *(source++);
			dest.push_back((const char*)source);
			arguments++;

			while ( *source != 0 && *source != terminator )
			{
				source += WWII::BytesFromCharacter[(unsigned char)*source];
			}

			// If we found the end of the string, break it off.
			if ( *source == terminator )
				*(source++) = 0;

			continue;
		}

		// Add this string
		dest.push_back((const char*)source);
		arguments++;

		while ( *source != 0 && *source != ' ' )
		{
			// Replace underscores with spaces after testing for space as break;
			if ( translate && *source == '_' )
				*source = ' ';
			source += WWII::BytesFromCharacter[(unsigned char)*source];
		}
	}

	return (int)dest.size();
}

#if 0
//////////////////////////////////////////////////////////////////////
// Append an integer passed in to the string destination provided
void DotCommand::catIntToText(const UINT32 value, char *dest) const
{
   char istring[MAX_INT_REP+1];

   // skip to the null character
   while (*dest++ != 0);

   // append the integer 
   sprintf(istring, "%u", value);

   dbugLog(_FLD, "Appending value %u to istring: %s", value, istring);

   istring[MAX_INT_REP] = 0;  // be safe before cat
   strcat(dest, istring);

   dbugLog(_FLD, "Just appended %s to location %p and result is %s", istring, dest, dest);
}
#endif

//////////////////////////////////////////////////////////////////////
// Set the current availability mode
// Availabilities are:
//	E	Everyone
//	P	Anyone with permission
//	C	CRS only
// 'C' is the default and used for an unrecognized level.

void
DotCommand::setAvailability(char level)
{
	switch (level)
	{
	case 'E':
		// Everyone has access
		dbugLog(_FLD, "%s available to everyone" , command());
		m_public = true;
		m_playnetOnly = false;
	break;

	case 'P':
		// By Permission only
		dbugLog(_FLD, "%s available by permission" , command());
		m_public = false;
		m_playnetOnly = false;
	break;

	case 'C':
		// CRS only
		dbugLog(_FLD, "%s restricted to CRS by permission" , command());
		m_public = false;
		m_playnetOnly = true;
		break;
	default:
#ifdef DEBUG
		LogAbort("'%s' command has unrecognized availability level %c", command(), level);
#else
		dbugLog(_FLA, "'%s' command has unrecognized availability level %c", command(), level);
#endif
		// If we somehow deployed with an invalid command, at least mark it as Playnet-only
		// to limit its potential use as much as possible.
		m_public = false;
		m_playnetOnly = true;
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// TEst for public visibility of a command.

bool
DotCommand::isHidden() const
{
	// Unless public, it needs to belong to a group.
	if ( m_public == false && m_groups.empty() )
		return true;

	for ( auto it = m_usages.begin(), end = m_usages.end(); it != end; ++it )
	{
		if ( (*it)->isHidden() == false )
			return false;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////
// Determine whether this dot command is allowed for a given mask,
// and return its help

const DotUsage*
DotCommand::allow(Player& player) const
{
	// Some dot commands* require* you to be playnet
	if ( m_playnetOnly && !player.isPlaynet() )
	{
		dbugLog(_FLD, "Command requires playnet status");
		return nullptr;
	}

	// Are group permissions required?
	if ( !player.isManager() && !m_public )
	{
		// Command is not in any groups, ergo not available
		if ( m_groups.empty() )
		{
			dbugLog(_FLD, "Non-public command cannot be run by non-manager player");
			return nullptr;
		}

		// Request the most recent group set; groups will load
		// in the player's permissions if they're not already loaded.
		const CHAT_GROUPS& clGroups = player.groups();

		// Client is not in any groups, don't bother checking
		if ( clGroups.empty() )
		{
			dbugLog(_FLD, "Command not available, player not in any groups");
			return nullptr;
		}

		// See if the player is in at least one of the required groups
		CHAT_GROUPS::iterator clgIt, clgEnd = clGroups.end();
		bool isIn = false;
		for (auto cmdIt = m_groups.begin(), end = m_groups.end(); isIn == false && cmdIt != end; ++cmdIt)
		{
			clgIt = clGroups.find(*cmdIt);
			if ( clgIt != clgEnd )
				isIn = true;
		}

		if ( !isIn )
		{
			dbugLog(_FLD, "Player is not in any of the required groups for Command:");
			for (auto cmdIt = m_groups.begin(), end = m_groups.end(); isIn == false && cmdIt != end; ++cmdIt)
			{
				dbugLog(_FLD, "   %d", *cmdIt);
			}
			return nullptr;
		}
	}

	for ( UINT32 i = 0; i < m_usages.size(); i++ )
	{
		if ( m_usages[i]->mask == NO_MASK || player.permissions(m_usages[i]->mask) )
			return m_usages[i].get();
	}

	dbugLog(_FLD, "Player with permissions %llx does not have permission to access command", player.m_permissionMask);

	return nullptr;
}


//////////////////////////////////////////////////////////////////////
// Send a user the relevant help for their version of this command

bool
DotCommand::help(Player& player) const
{
	const DotUsage* commandHelp = allow(player);

	// Does this player have permission?
	if ( commandHelp == nullptr || commandHelp->isHidden() )
		return false;

	player.message(LSTR_USAGE, commandHelp->usage, nullptr, nullptr);
	player.message(LSTR_DETAIL, commandHelp->detail, nullptr, nullptr);

	return true;
}

// Test if the current 'command' has been locked within duration_m minutes
bool
DotCommand::_featureLockedM(Player& player, UINT32 duration_m) const
{
	if ( !player.featureLock(m_command, teulTime(), duration_m * 60) )
		return false;

	char mins[12+1];
	strprintf(mins, "%u", duration_m);
	player.message(LSTR_CAUTION, LSTR_FEATURE_LOCKED, command(), mins);
	return true;
}

//////////////////////////////////////////////////////////////////////
// Get the DotCommand Index
//////////////////////////////////////////////////////////////////////

DotCommandIndex*
DotCommand::index()
{
	static DotCommandIndex _index;
	return &_index;
}

//////////////////////////////////////////////////////////////////////
// DotCommandIndex class
//////////////////////////////////////////////////////////////////////

void
DotCommandIndex::addAlias(const char* old, const char* alias)
{
	const DotCommand* command = lookup(old, true);

	if ( command == nullptr )
	{
		dbugLog(_FLW, "Unable to find command '%s' for alias '%s'", old, alias);
		return;
	}

	if ( lookup(alias, true) != nullptr )
	{
		dbugLog(_FLW, "Can't add alias '%s' for '%s', command already exists", alias, old);
		return;
	}

	dbugLog(_FLI, "Added %s as alias for %s", alias, old);

	m_aliases[alias] = command;
}

//////////////////////////////////////////////////////////////////////
// Find a registered dot command

const DotCommand*
DotCommandIndex::lookup(const std::string& target, bool cleanMatchOnly) const
{
	// Firstly, check aliases
	DOT_ALIASES::const_iterator alias = m_aliases.find(target);
	if ( alias != m_aliases.end() )
		return (alias->second);

	DotCommand* last = nullptr;
	int matches = 0;

	for ( auto it = begin(), endIt = end(); it != endIt; ++it )
	{
		DotCommand* cur = *it;

		// If what we typed is longer than the current command, it can't match
		if ( target.length() > cur->m_command.length() )
			continue;

		// Exact match? Return
		if ( cur->m_command == target )
			return cur;

		// If we're not looking exclusively for clean matches...
		if ( cleanMatchOnly || target.length() == cur->m_command.length() )
			continue;

		// ...and it results in a partial match
		if ( target.compare(cur->m_command.substr(0, target.length())) == 0 )
		{
			// Then remember it, but try to determine if its unique
			last = cur;
			matches++;
		}
	}

	// They're looking for good matches, but not exact matches;
	// if we only found one match, then return the single match.
	if ( !cleanMatchOnly && matches == 1 )
		return last;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// add a new dot command to the index

void
DotCommandIndex::add(DotCommand* dotCommand)
{
	dbugLog(_FLD, "Registering dotCommand: %s", dotCommand->command());
	betaLog(_FLI, "Registering dotCommand: %s", dotCommand->command());

	if ( lookup(dotCommand->m_command, TRUE) != nullptr )
		throw std::domain_error("Duplicate dot command");

	// Insert ourselves alphabetically
	iterator it = begin();
	while ( it != end() && dotCommand->m_command > (*it)->m_command )
		++it;
	insert(it, dotCommand);

	// Now add to the database incase it's not already there
	std::string desc = dotCommand->description();
        DBThread::EscapeString(desc);

	DBThread::Do(DBThread::Default
				, "INSERT IGNORE INTO chat_commands"
					" (command, description, availability)"
					" VALUES ('%s', '%s', '%c')"
					, dotCommand->command()
					, desc.c_str()
					, (dotCommand->m_playnetOnly) ? 'C' : 'P'
				);
}

//////////////////////////////////////////////////////////////////////
// Return a pointer to the help strings for a particular dot command
// in the context of a particular player

bool
DotCommandIndex::helpFor(Player& player, const char* command) const
{
	const DotCommand* dotcmd = lookup(command);

	if ( !dotcmd )
		return false;

	return dotcmd->help(player);
}

//////////////////////////////////////////////////////////////////////
// Our function for actually invoking dot commands

bool
DotCommandIndex::process(Player& player, Chat::ClientMessage* message) const
{
	if ( message == nullptr )
	{
		dbugLog(_FLW, "Tried to process NULL playerMessage");
		return false;
	}

	// Record activity for posterity
	dbugLog(_FLI, "t:%d p:%u c:%s %s"
					, player.teulIndex(), player.playerID(), player.callsign()
					, message->m_message
					);

	// Take a pointer to the first character after "."
	Chat::CommandWithArgs input(message->m_message + 1);
	if ( *input.Command() == 0 )
		return false;

	dbugLog(_FLD,"%s sending dotCommand %s %s", player.callsign(), input.Command(), input.Args());

	const DotCommand* dotCommand = lookup(input.Command());
	if ( !dotCommand )
	{
		player.log(_FLI, "couldn't find /command %s, ignoring", input.Command());
		return false;
	}

	if ( !dotCommand->allow(player) )
	{
		player.log(_FLI, "no permissions to use /command %s, ignoring", input.Command());
		return false;
	}

	try
	{
		if ( *input.Args() != 0 )
			dotCommand->actionWith(player, message, input.Args());
		else
			dotCommand->action(player, message);
	}
	catch (LSTRING_ID id)
	{
		player.message(LSTR_CAUTION, id, "<undef>", "<undef>");
	}
	catch (Chat::LStringErr& error)
	{
		player.message(LSTR_CAUTION, error.m_lstring, error.m_arg1.c_str(), error.m_arg2.c_str());
	}
	catch (std::exception& e)
	{
		LogAbort("fatal error: %s", e.what());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Flush any existing permission sets for commands and load in new
// data from the database

void
DotCommandIndex::readPermissions()
{
	// Reset all commands to Playnet availability and no groups
	// since deletions from the permissions data are not explicit
	dbugLog(_FLI, "Flushing old permissions [setting all chat commands to CRS only]");
	for ( auto it = begin(), endIt = end(); it != endIt; ++it )
	{
		DotCommand* dc = *it;
		dc->setAvailability('C');
		dc->clearGroups();
	}

	// Set the availabilities according to the database
	dbugLog(_FLI, "Reading new permissions");
	try
	{
		DBQuery dbq(dbh, "SELECT cc.command, cc.availability FROM chat_commands cc");
		while ( dbq.FetchRow() )
		{
			DBRow& row = dbq.Row();

			// Blank command or "." command
			if ( row[0] == nullptr || row[0][0] == '.' )
				continue;

			DotCommand* dc = const_cast<DotCommand*> (lookup(row[0], true));
			if ( dc == nullptr )
			{
				betaLog(_FLW, "chat_command %s not recognized", row[0]);
				continue;
			}
			dc->setAvailability(row[1][0]);
		}
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Unable to set availabilities: %s", e.what());
	}

	// Now set the groups according to the database
	try
	{
		UINT32 lastCmd = 0;
		DotCommand* dc = nullptr;

		DBQuery dbq(dbh,
					"SELECT cc.cmd_oid, cc.command, cgc.group_oid"
						" FROM	chat_commands cc, chat_group_cmds cgc"
						" WHERE	cc.cmd_oid = cgc.cmd_oid AND cc.availability != 'E'"
						" ORDER BY cc.cmd_oid");
		while ( dbq.FetchRow() )
		{
			DBRow& row = dbq.Row();
			UINT32 cmdNo = (UINT32)atol(row[0]);
			const char* cmd = row[1];

			// Find the command
			if ( lastCmd != cmdNo )
			{
				lastCmd = cmdNo;
				dc = const_cast<DotCommand*> (lookup(cmd, true));
				if ( dc == nullptr )
				{
					dbugLog(_FLD, "Ignoring unknown command '%s'", cmd);
					continue;
				}
			}

			// If we didn't know the command last time, ignore it again
			if ( dc == nullptr )
				continue;

			dc->addGroup((UINT32)atol(row[2]));
		}
	}
	catch (std::exception& e)
	{
		dbugLog(_FLA, "Unable to set chat permissions: %s", e.what());
	}

	buildCommandListPacket();
	broadcastCommandListPacket();
}

//////////////////////////////////////////////////////////////////////
// Build the command list packet.

void
DotCommandIndex::buildCommandListPacket()
{
	delete g_commandListPacket;

	g_commandListPacket = new Teul::Packet(Teul::PacketId(CHAT_X, Chat::CMD_LIST_Y));
	g_commandListPacket->setUnencrypted();

	g_commandListPacket->append((Chat::cmdcount_t)0);

	// Rebuild the command packet.
	for ( auto it = begin(), endIt = end(); it != endIt; ++it )
	{
		const DotCommand* const dc = (*it);
		// Skip commands that are intended to be hidden.
		if ( dc->isHidden() || dc->usages().empty() )
			continue;

		const UCHAR playnetOnly = dc->m_playnetOnly ? TRUE : FALSE;
		const SINT16 usage = dc->usages().front()->usage;
		const SINT16 detail = dc->usages().front()->detail;

		g_commandListPacket->append(playnetOnly);
		g_commandListPacket->append(dc->m_command.c_str(), dc->m_command.length() + 1);
		g_commandListPacket->append(usage);
		g_commandListPacket->append(detail);

		++g_commandListPacket->payloadRef<Chat::cmdcount_t>(0);
	}

	dbugLog(_FLD, "indexed %u visible chat commands", g_commandListPacket->payloadRef<Chat::cmdcount_t>(0));
}

//////////////////////////////////////////////////////////////////////
// Send the current command list packet to everyone.

void
DotCommandIndex::broadcastCommandListPacket()
{
	if ( g_commandListPacket == nullptr )
		return;

	// Send to all players.
	std::vector<ENDPOINT*> endpoints;
	for ( auto it = playerIndex.begin(), endIt = playerIndex.end(); it != endIt; ++it )
	{
		const Player* const player = it->second;
		if ( player->teulIndex() >= 0 )
		{
			ENDPOINT* const endpoint = teulGetEndpoint(player->teulIndex());
			if ( endpoint != nullptr )
				endpoints.push_back(endpoint);
		}
	}

	const size_t numEndpoints = endpoints.size();
	if ( numEndpoints <= 0 )
	{
		dbugLog(_FLD, "%" PRSizeT " command permissions loaded, no players to transmit to.", size());
		return;
	}

	const Chat::cmdcount_t& count = g_commandListPacket->payloadRef<Chat::cmdcount_t>(0);
	dbugLog(_FLD, "transmitting %u /commands in %u byte packets to %" PRSizeT " players.", count, g_commandListPacket->packetSizeBytes(), numEndpoints);
	for ( size_t i = 0; i < numEndpoints; ++i )
	{
		g_commandListPacket->send(*endpoints[i]);
	}
}

//////////////////////////////////////////////////////////////////////
// Test for/report on a locked feature

bool
DotCommand::lockedFeature(Player& player, const char* activity, UINT32 interval_s, UINT32 now)
{
	if ( !player.featureLock(activity, now, interval_s) )
		return false;

	player.message(LSTR_CAUTION, LSTR_ACTION_LIMITED, activity, chatTimeInterval(interval_s));
	return true;
}

//////////////////////////////////////////////////////////////////////
// Generic help response

bool
DotCommand::genericHelp(Player& player, const char* usage, const char* detail) const
{
	if ( !allow(player) )
		return false;

	if ( !usage && !detail )
	{
		player.message(LSTR_CAUTION, LSTR_NO_HELP);
		return true;
	}

	if ( usage )
		player.message(LSTR_USAGE, LSTR_GENERIC, usage);
	if ( detail )
		player.message(LSTR_DETAIL, LSTR_GENERIC, detail);

	return true;
}

//////////////////////////////////////////////////////////////////////

bool
chatGetTimeInterval(Player& player, const char* timeString, UINT32& interval, const char defaultUnit)
{
	char type = defaultUnit;
	(void)sscanf(timeString, "%u%c", &interval, &type);

	switch ( tolower(type) )
	{
		case 's': /* do nothing */; break;
		case 'm': interval *= 60; break;
		case 'h': interval *= 60 * 60; break;
		case 'd': interval *= 24 * 60 * 60; break;
		default:
			interval = 0;
			player.message(LSTR_CAUTION, LSTR_INVALID_TIME, timeString);
			return false;
		break;
	};

	return true;
}

//////////////////////////////////////////////////////////////////////
// Send a player the full list of chat commands.

void
chatSendCommandListPacket(const class Player* const player)
{
	if ( player->teulIndex() < 0 || g_commandListPacket == nullptr )
		return;

	ENDPOINT* const endpoint = teulGetEndpoint(player->teulIndex());
	if ( endpoint )
		g_commandListPacket->send(*endpoint);
}

