#include <syscfg.h> // Path is project specific.
// Core implementation of dot commands on the strat host;
// this are the various hooks and stubs for supporting
// out of band data commands, primarily from the chat host,
// but potentialy from the other hosts.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfsone" Smith
// Supporting Programmer:  Marty "Vandal" Poulin

#include <sys/stat.h>

#include "chat/chatDefines.h"
#include "chat/chatStratSupportMessages.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatTimeInterval.h"
#include "chat/chatTypes.h"
#include "include/arenaLockEnums.h"
#include "include/luaGlue.h"
#include "include/wthrTime.h"
#include "strt/strtCPClass.h"
#include "strt/strtCP.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtDotCommands.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFB.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostLua.h"
#include "strt/strtHostMessages.h"
#include "strt/strtIO.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtMain.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupply.h"
#include "strt/strtTOE.h"
#include "strt/strtTypes.h"
#include "strt/strtWeaponEntry.h"
#include "strt/strtWeaponUpdate.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiConfigVars.h"
#include "wwii/wwiiIRC.h"

using namespace std;

#define	RESPOND(...)	chatSendDirectMessage(message.m_originatorID, LSTR_NOTE, LSTR_GENERIC, __VA_ARGS__)
#define	BETAMSG(...)	chatSendDirectMessage(message.m_originatorID, LSTR_BETA, LSTR_GENERIC, __VA_ARGS__)
#define RESPOND_LOC(pfx, ...) chatSendDirectMessage(message.m_originatorID, pfx, __VA_ARGS__)
#define LOCMESSAGE(pfx, ...) chatSendLocalizedMessage(message.m_originatorID, pfx, __VA_ARGS__)

#if defined(BETA_RELEASE) || defined(TEST_RELEASE) || defined(TRAINING_SERVER)
# define EXTRA_CONFIGURATIONS
#endif

struct RDPInfo
{
	// True if the last update > 1 hour old
	bool Expired() const { return m_lastUpdate < teulTime() - (60 * 60); }
	// True if we've received any data
	bool Populated() const { return m_lastUpdate > 0; }
	// Process an update
	void Update(WWII_COUNTRY country, UINT32 cycle, UINT32 percent);
	// Create/update the text if neccessary
	void Transcribe();

	UINT32	m_cycle[MAX_COUNTRIES];
	UINT32	m_percent[MAX_COUNTRIES];
	bool	m_loaded[MAX_COUNTRIES];
	UINT32	m_lastUpdate;
	std::string m_detail;
};
static RDPInfo rdpInfo;

static const char*
strtAbbrFacilityName(const FacilityStruct* facility, const CPStruct* cp)
{
	UINT32 uCPNameLen;

	if ( facility->cp != cp )
		return facility->name;

	uCPNameLen = strlen(cp->name);
	if ( strncmp(facility->name, facility->cp->name, uCPNameLen) == 0 )
	{
		if ( facility->name[uCPNameLen] == ' ' || facility->name[uCPNameLen] == '-' )
			return &facility->name[uCPNameLen + 1];
		return &facility->name[uCPNameLen];
	}

	return facility->name;
}

static void
_listSpawnables(const CPStruct* cp, OOBMessage& message)
{
	UINT32 i;
	FacilityStruct* fac = cp->facilityList;

	for ( i = 0; fac && i < cp->ownableFacilities + cp->numFirebases; i++, fac = fac->next )
	{
		BETAMSG("%4d:%s) %s [%s]"
					, fac->uOID
					, _countryCode[fac->country]
					, strtAbbrFacilityName(fac, cp)
					, (fac->isOpen()) ? "Open" : "closed"
				);
	}
}

static size_t
_vectorize(char* source, std::vector<const char*>& dest, int maxArguments=100, bool translate=false)
{
	int arguments = 0;

	// Remove any current entries
	dest.clear();
	if ( *source == 0 )
		return 0;

	while ( *source != 0 && arguments < maxArguments )
	{
		// Strip leading spaces from each argument
		while ( isspace(*source) )
			*(source++) = 0;

		if ( *source == 0 )
			break;

		char separator;
		if ( *source == '"' )
		{
			++source;
			separator = '"';
		}
		else
			separator = ' ';

		// Add this string
		dest.push_back(source);
		arguments++;

		while ( *source != 0 )
		{
			if ( *source == separator )
			{
				*source = ' ';
				break;
			}
			// Replace underscores with spaces after testing for space as break;
			if ( translate && *source == '_' )
				*source = ' ';
			++source;
		}
	}

	if ( isspace(*source) )
		*source = 0;

	return dest.size();
}

static CPStruct*
_getCPByArguments(char* arguments, OOBMessage& message, bool allowBridges)
{
	SINT32 cpOID;
	CPStruct* cp;
	if ( arguments == nullptr || *arguments == 0 )
	{
		if ( allowBridges )
			cp = strtGetCPorBridge(message.m_cpID);
		else
			cp = strtGetCP(message.m_cpID);

		if ( cp == nullptr )
		{
			// If we couldn't find a cp, maybe they have a facility selected
			FacilityStruct* facility;
			facility = strtGetFacility(message.m_cpID);
			if ( facility == nullptr || facility->cp == nullptr )
			{
				LOCMESSAGE(LSTR_CAUTION, LSTR_SPECIFY_TOWN);
				return nullptr;
			}
			cp = facility->cp;
		}
	}
	else
	{
		cpOID = atol(arguments);

		if ( cpOID > 0 )
			cp = strtGetCPByOID(cpOID);
		else
			cp = strtGetCPByName(arguments);

		if ( cp == nullptr )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_INVALID_CP_NAME, arguments);
			return nullptr;
		}

		if ( cp->isBridge() && !allowBridges )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_CANNOT_USE_BRIDGE);
			return nullptr;
		}
	}

	return cp;
}

UINT32 _arenaLockedAt = 0;
UINT32 _arenaLockLevel = 0;

static std::string
_lockDescription(UINT32 lock)
{
	bool plural = false;
	std::vector<const char*> locks;
	std::string text = "";

	if ( 0 == lock )
		return "<Unknown Lock> is";

	if ( lock & AL_ENTRY )
		locks.push_back("Persona Selection");
	if ( lock & AL_KILL )
		locks.push_back("Kills");
	if ( lock & AL_SPAWN )
		locks.push_back("Spawning");
	if ( lock & AL_CAPTURE )
		locks.push_back("Capture");

	if ( locks.size() > 1 )
	{
		plural = true;
		while ( locks.size() > 1 )
		{
			text += locks[0];
			locks.erase(locks.begin());
			if ( locks.size() > 1 )
				text += ", ";
		}
		text += " and ";
	}

	text += locks[0];
	if ( false == plural && 's' == *(text.rbegin()) )
		plural = true;

	text += ( plural ) ? " are" : " is";

	return text;
}

void
dotApplyConfig()
{
	strtBroadcastConfigUpdate();

	const bool needAnnouncement = (chatGetPopulation(WWII_NO_SIDE) > 0);
	const UINT32 curTime = teulTime();
	if ( host_config.arena.lock == 0 )
	{
		if ( needAnnouncement && _arenaLockedAt > 0 )
		{
			std::string message;
			if ( AL_KILL == _arenaLockLevel )
				message = "[+] CEASE FIRE LIFTED: Kills are now ENABLED [+]";
			else
			{
				message = "[+] SERVER UNLOCKED! ";
				message += _lockDescription(_arenaLockLevel);
				message += " now ENABLED [+]";
			}
			///LOCALIZE
			chatSendBroadcastAll(LSTR_SYSTEM, LSTR_GENERIC, message.c_str());
		}
		_arenaLockedAt = 0;
		_arenaLockLevel = host_config.arena.lock;
		return;
	}

	if ( _arenaLockLevel == host_config.arena.lock && _arenaLockedAt > (curTime - 120) )
		return;

	if ( needAnnouncement )
	{
		if ( host_config.arena.lock == AL_KILL )
		{
			chatSendBroadcastAll(LSTR_SYSTEM, LSTR_CEASEFIRE_ON);
		}
		else
		{
			std::string text = "[X] Server Locked: ";
			text += _lockDescription(host_config.arena.lock);
			text += " DISABLED [X]";
			chatSendBroadcastAll(LSTR_SYSTEM, LSTR_GENERIC, text.c_str());
		}
	}

	_arenaLockLevel = host_config.arena.lock;

	_arenaLockedAt = curTime;
}

// Display a list of commands
static
bool _dotHelp(char* arguments, OOBMessage& message)
{
	UINT32 i;
	char buf[2048];

	if ( arguments == nullptr || *arguments == 0 )
	{
		BETAMSG("';' commands available:");
		buf[0] = 0;
		for ( i = 0; dotCommands[i].command; i++ )
		{
			if ( buf[0] )
				strcat(buf, ", ");

			if ( buf[0] && i % 8 == 0 )
			{
				BETAMSG(buf);
				buf[0] = 0;
			}
			strcat(buf, dotCommands[i].command);
		}

		BETAMSG(buf);
		return true;
	}

	for ( i = 0; dotCommands[i].command; i++ )
	{
		if ( strcasecmp(arguments, dotCommands[i].command) == 0 )
		{
			BETAMSG("%s: %s", arguments, dotCommands[i].description);
			return true;
		}
	}

	BETAMSG("No such command - type;help for list of commands");
	return false;
}

template<typename Tp>
static bool
_setIntegerValue(const char* arguments, const OOBMessage& message
				, Tp& variable
				, const char* fieldName
				, const char* unit
				, int min
				, int max
				, const char* zeroMessage
				)
{
	Tp newVal;

	if ( arguments == nullptr || *arguments == 0 )
	{
		BETAMSG("Please specify %s in %s.", fieldName, unit);
		return false;
	}

	if ( *arguments == '?' )
	{
		if ( zeroMessage != nullptr && variable <= (Tp)min )
			BETAMSG(zeroMessage);
		else
			BETAMSG("Current %s: %d %s.", fieldName, variable, unit);
		return false;
	}

	if ( !isdigit(*arguments) )
	{
		BETAMSG("Woah there, captain keyboard, '%c' doesn't look like a number to me, and %s wants a number.", (*arguments), fieldName);
		return false;
	}
	if ( *arguments == '-' )
	{
		BETAMSG("%s doesn't go backwards, so please don't type '-' infront of what you are trying to set it to.", fieldName);
		return false;
	}

	newVal = static_cast<Tp>(atol(arguments));

	if ( newVal < (Tp)min )
	{
		BETAMSG("Invalid %s (%d < %d).", fieldName, newVal, min);
		return false;
	}

	if ( max > 0 && newVal > (Tp)max )
	{
		BETAMSG("Invalid %s (%d > %d).", fieldName, newVal, max);
		return false;
	}

	if ( zeroMessage && newVal == (Tp)min )
		BETAMSG(zeroMessage);
	else
		BETAMSG("Changing %s from %d to %d %s.", fieldName, variable, newVal, unit);

	variable = newVal;

	return true;
}

// Lock/unlock arena
static bool
_dotArenaLock(char* arguments, OOBMessage& message)
{
	if ( _setIntegerValue(arguments, message
				, host_config.arena.lock
				, "arena lock", "(0=off, 1=cap, 2=spawn, 4=kill, 8=entry)", 0, 15
				, nullptr
				) )
	{
		dotApplyConfig();
		return true;
	}

	return false;
}

static const char*
_describeEws(UINT32 cpFlags, UINT32 lightFlag, UINT32 heavyFlag)
{
	if ( (cpFlags & lightFlag) && (cpFlags & heavyFlag) )
		return "both";
	else if ( cpFlags & lightFlag )
		return "light";
	else if ( cpFlags & heavyFlag )
		return "!heavy!";
	else
		return "off";
}

static void
_printCPInfo(OOBMessage& message, CPStruct* const cp)
{
	if ( cp->fullObjID.objType == CP_BRIDGE )
	{
		BETAMSG("%s(%d%s%s%s%s%s) %s"
				, cp->name
				, cp->uOID
				, (cp->modified) ? "/DB":""
				, (cp->flags & CP_STAT_ALLIED_ATTACK_OBJ) ? "/AlAtt":""
				, (cp->flags & CP_STAT_ALLIED_DEFEND_OBJ) ? "/AlDef":""
				, (cp->flags & CP_STAT_AXIS_ATTACK_OBJ) ? "/AxAtt":""
				, (cp->flags & CP_STAT_AXIS_DEFEND_OBJ) ? "/AxDef":""
				, (cp->isContested()) ? "DESTROYED" : "standing"
				);
		return;
	}
	BETAMSG(
		"%s(%d%s%s%s%s%s%s%s%s): "
			"%s owns, "
			"%s controls, "
			"%s orig. "
			"Facs: %u (%u). "
			"FBs: %d. "
			"ABs: %d (Ax:%d/Al: %d). "
			"Links: %d/%d. "
			"Supplied: %s."
				,	cp->name
				,	cp->uOID
				,	(cp->isContested()) ? "/Con" : ""
				,	(cp->isEnemyLinked()) ? "/Front" : ""
				,	(cp->isUnderAttack) ? "/EWS" : ""
				,	(cp->modified) ? "/DB":""
				,	(cp->flags & CP_STAT_ALLIED_ATTACK_OBJ) ? "/AlAtt":""
				,	(cp->flags & CP_STAT_ALLIED_DEFEND_OBJ) ? "/AlDef":""
				,	(cp->flags & CP_STAT_AXIS_ATTACK_OBJ) ? "/AxAtt":""
				,	(cp->flags & CP_STAT_AXIS_DEFEND_OBJ) ? "/AxDef":""
				,	_countryCode[cp->country]
				,	_countryCode[cp->controllingCountry]
				,	_countryCode[cp->originalCountry]
				,	cp->ownableFacilities,	cp->facilitiesOwned[cp->side]
				,	cp->numFirebases
				,	cp->numArmyBases
				,	cp->armyBasesOwned[AXIS],	cp->armyBasesOwned[ALLIED]
				,	cp->numOpenLinks
				,	cp->numLinks
				,	((cp->isSupplied()) ? "Yes" : "No")
	);
	if ( cp->isUnderAttack )
	{
		BETAMSG("EWS: Inf %s, Armor %s, Air %s, Naval %s"
				, _describeEws(cp->flags, CP_STAT_ENEMY_INFANTRY, CP_STAT_HEAVY_ENEMY_INF)
				, _describeEws(cp->flags, CP_STAT_ENEMY_ARMOR, CP_STAT_HEAVY_ENEMY_ARMOR)
				, _describeEws(cp->flags, CP_STAT_ENEMY_AIRCRAFT, CP_STAT_HEAVY_ENEMY_AIR)
				, _describeEws(cp->flags, CP_STAT_ENEMY_SEACRAFT, CP_STAT_HEAVY_ENEMY_SEA));
	}
}

static void
_makeList(CPStruct* cp, const OOBMessage& message)
{
	const FacilityStruct* fac;
	const char* joiner = nullptr;

	std::string& list = cp->ownMessage[message.m_side];

	if ( list.size() == 0 )
	{
		try
		{
			list = ( message.m_side == AXIS ) ? "Allied: " : "Axis: ";

			for ( fac = cp->facilityList; fac != nullptr; fac = fac->next )
			{
				if ( fac->side == message.m_side || fac->type == FACILITY_FIREBASE )
					continue;

				if ( joiner == nullptr )
					joiner = ", ";
				else
					list += joiner;

				list += strtAbbrFacilityName(fac, cp);
			}
		}
		catch (std::exception& e)
		{
			dbugLog(_FLW, "Failed building ownership list: %s", e.what());
			list = "<error>";
		}
	}

	LOCMESSAGE(LSTR_NOTE, LSTR_GENERIC, list.c_str());
}

//////////////////////////////////////////////////////////////////////
// The ".own" of firebases

static bool
_bridgeOwnership(const std::string& title, CPStruct* cp, ObjectStruct* bridge, OOBMessage& message)
{
	const char* state = (cp->isContested()) ? "DESTROYED":"standing";
	RESPOND("%s: %s. %u%% damage", title.c_str(), state, (UINT32)bridge->percentDamage);
	return true;
}

static bool
_factoryOwnership(FacilityStruct* fac, OOBMessage& message)
{
	if ( fac->side != fac->originalSide )
	{
		RESPOND("%s is owned by %s - no RDP contribution.", fac->name, _countryCode[fac->country]);
		return true;
	}

	UINT32 damage = 0;
	UINT32 factories = 0;
	for ( ObjectStruct* object = fac->objectList; object != nullptr; object = object->next )
	{
		if ( !object->isProductionCenter() )
			continue;
		++factories;
		damage += object->percentDamage;
	}

	if ( factories == 0 )
		return RESPOND("%s has no factories. If you believe this is an error, please /report this for the developers' attention.", fac->name);

	damage /= factories;
	if ( damage >= 100 )
		return RESPOND("%s is DESTROYED - no RDP contribution", fac->name);

	RESPOND("%s: %u%% damage", fac->name, damage);

	return true;
}

static bool
_fbOwnership(FacilityStruct* fb, OOBMessage& message)
{
	if ( fb->isOpen() == false )
	{
		RESPOND("%s is closed", fb->name);
		return true;
	}

	std::string result = "";
	UINT64 otherMax = 0LL;
	UINT64 otherDmg = 0LL;
	char pctg[6];
	ObjectStruct* obj;
	for ( obj = fb->objectList; obj != nullptr; obj = obj->next )
	{
		if ( obj->m_maxDamageJoules == 0 )
			continue;
		if ( strncmp(obj->filename, "inf", 3) == 0 )
			result += "Inf: ";
		else if ( strncmp(obj->filename, "arm", 3) == 0 )
			result += "Veh: ";
		else
		{
			otherMax += obj->m_maxDamageJoules;
			otherDmg += obj->m_damageJoules;
			continue;
		}
		if ( obj->m_damageJoules >= obj->m_maxDamageJoules )
		{
			result += "DOWN. ";
			continue;
		}

		snprintf(pctg, 5, "%u%%", (UINT32)obj->percentDamage);
		result += pctg;
		result += ". ";
	}

	if ( otherMax > 0 )
	{
		result += "Other: ";
		if ( otherDmg >= otherMax )
			result += "DOWN. ";
		else
		{
			snprintf(pctg, 5, "%u%%", (UINT32)(otherDmg * 100 / otherMax));
			result += pctg;
		}
	}

	RESPOND("%s: %s", fb->name, ((result.length() > 0) ? result.c_str() : "No information available"));

	return true;
}

//////////////////////////////////////////////////////////////////////

static bool
_dotOwn(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	if ( message.m_side == WWII_NO_SIDE )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_SIDE);
		return false;
	}

	if ( arguments != nullptr && *arguments != 0 && !isalpha(*arguments) )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SPECIFY_TOWN);
		return false;
	}

	FacilityStruct* fb = strtGetFacility(message.m_cpID);
	if ( fb != nullptr )
	{
		if ( fb->type == FACILITY_FIREBASE )
			return _fbOwnership(fb, message);
		if ( fb->type == FACILITY_FACTORY && fb->cp && fb->cp->m_isSupplyCenter )
			return _factoryOwnership(fb, message);
	}

	cp = _getCPByArguments(arguments, message, true);
	if ( cp == nullptr )
		return false;

	// Describe objectives
	const char* obj = "";
	if ( cp->isAttackObjective(message.m_side) )
		obj = "ATTACK";
	else if ( cp->isDefendObjective(message.m_side) )
		obj = "DEFENSE";

	std::string title = cp->name;
	if ( *obj )
	{
		title += "(";
		title += obj;
		title += ")";
	}

	if ( cp->isBridge() )
		return _bridgeOwnership(title, cp, cp->facilityList->objectList, message);

	const char* front = (cp->isEnemyLinked()) ? "FRONT LINE. " : "";
	const char* supply = (cp->isSupplied()) ? "" : "NO FACTORY LINK. ";

	const char* con = (cp->isContested()) ? "CONTESTED" : "Uncontested";
	const char* hold = (message.m_side == cp->side && cp->isHolding()) ? " HOLD" : "";
	const char* orun = (message.m_side == cp->side && cp->isOverrun()) ? " OVERRUN" : "";

	RESPOND("%s: " 					// title
				"%s owns, "			// country
				"%s controls. "		// controllingCountry
				"%s"				// Front line
				"%s"				// supply
				"%s"				// contention
				"%s"				// hold
				"%s"				// overrun
			, title.c_str()
			, _countryCode[cp->country]
			, _countryCode[cp->controllingCountry]
			, front
			, supply
			, con
			, hold
			, orun
			);

	if ( cp->isContested() )
		_makeList(cp, message);

	return true;
}

static void
_makeFBList(CPStruct* cp, const OOBMessage& message)
{
	const FacilityStruct* fac;
	const FacilityStruct* fb;
	static std::string lists[MAX_SIDES];

	lists[ALLIED] = "";
	lists[AXIS] = "";

	// Iterate across all the depots and check to see if we have an FB on it
	for ( fac = cp->facilityTypeIndex[FACILITY_DEPOT]; fac != nullptr; fac = fac->typeNext )
	{
		if ( fac->m_outgoingLink == nullptr
			|| fac->m_incomingLink == nullptr
			|| fac->m_outgoingLink->endDepot == nullptr )
			continue;

		fb = fac->m_outgoingLink->firebase;
		if ( fb == nullptr || fb->isOpen() == false )
		{
			fb = fac->m_incomingLink->firebase;
			if ( fb == nullptr || fb->isOpen() == false )
				continue;
		}

		if ( !isValidSide(fb->side) )
			continue;

		if ( lists[fb->side].size() > 0 )
			lists[fb->side] += ", ";

		lists[fb->side] += fb->name;
	}

	if ( lists[ALLIED].size() == 0 && lists[AXIS].size() == 0 )
		LOCMESSAGE(LSTR_NOTE, LSTR_NO_ACTIVE_FIREBASES);
	else
	{
		if ( lists[ALLIED].size() > 0 )
			LOCMESSAGE(LSTR_NOTE, LSTR_ALLIED_FBS, lists[ALLIED].c_str());
		if ( lists[AXIS].size() > 0 )
			LOCMESSAGE(LSTR_NOTE, LSTR_AXIS_FBS, lists[AXIS].c_str());
	}
}

static bool
_dotFBs(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	if ( message.m_side == WWII_NO_SIDE )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_SIDE);
		return false;
	}

	if ( arguments != nullptr && *arguments != 0 && !isalpha(*arguments) )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SPECIFY_TOWN);
		return false;
	}

	cp = _getCPByArguments(arguments, message, false);
	if ( cp == nullptr )
		return false;

	const char* hold = (message.m_side == cp->side && cp->isHolding()) ? " HOLD" : "";
	const char* orun = (message.m_side == cp->side && cp->isOverrun()) ? " OVERRUN" : "";

	RESPOND("%s: %s owns %s controls. %s%s%s"
			, cp->name
			, _countryCode[cp->country]
			, _countryCode[cp->controllingCountry]
			, (cp->isContested()) ? "CONTESTED" : "Uncontested"
			, hold
			, orun
			);

	_makeFBList(cp, message);

	return true;
}

// Send CP info
static bool
_dotCPInfo(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	cp = _getCPByArguments(arguments, message, true);
	if ( cp == nullptr )
		return false;

	_printCPInfo(message, cp);

	return true;
}

static const char*
_captureDescription(const FacilityStruct* fac)
{
	static char description[64];
	switch ( fac->m_captureStage )
	{
	case FacilityStruct::_CAP_DISABLED:
		return "disabled";
	break;
	case FacilityStruct::_CAP_PENDING:
		snprintf(description, 63, "in %us", fac->getCaptureRemaining());
		return description;
	break;
	case FacilityStruct::_CAP_ENABLED:
		snprintf(description, 63, "at %3.2f%%", fac->m_captureProgress);
		return description;
	break;
	default:
		return "!UNKNOWN CAPSTAGE!";
	}
}

// Send CP/Facility info
static bool
_dotCPDump(char* arguments, OOBMessage& message)
{
	CPStruct* cp;
	FacilityStruct* fac;
	bool fullInfo = false;
	UINT32 i;

	if ( arguments && strncmp(arguments, "detail", 6) == 0 )
	{
		fullInfo = true;

		// Skip the string
		arguments = strchr(arguments, ' ');
		if ( arguments )
			arguments++;
    }

	cp = _getCPByArguments(arguments, message, true);
	if ( cp == nullptr )
		return false;

	_printCPInfo(message, cp);

	fac = cp->facilityList;
	for ( i = 0; fac && i < cp->ownableFacilities + cp->numFirebases; i++, fac = fac->next )
	{
		BETAMSG("%4u %-32s %s %s %s cap:%s"
				, fac->uOID
				, strtAbbrFacilityName(fac, cp)
				, _countryCode[fac->country]
				, (fac->isOpen() ? "Open" : "Closed")
				, (fac->m_spawnable ? "SPAWN" : "non-spawn")
				, _captureDescription(fac)
				);
		if ( fullInfo )
		{
			const FacilityStruct* supplier = fac->armyBase;
			if ( fac->CP().numUnits() > 0 )
				supplier = fac->supplyingFacility((StratHCUnit&)*fac->CP().m_units.begin()->second, false);

			const char* supplyName;
			if ( supplier != nullptr )
				supplyName = strtAbbrFacilityName(supplier, cp);
			else
				supplyName = "self";

			BETAMSG("+-- AB:%s, Supply: %s"
					, strtAbbrFacilityName(fac->armyBase, cp)
					, supplyName
					);
		}
	}

	return true;
}

static bool
_dotPwnFacQuery(OOBMessage& message, CPStruct* cp)
{
	if ( cp == nullptr )
	{
		BETAMSG("You must select a CP at the map first");
		return false;
	}
	BETAMSG("%s [own:%s, con:%s, ori:%s]"
			, cp->name
			, _countryCode[cp->country]
			, _countryCode[cp->controllingCountry]
			, _countryCode[cp->originalCountry]
			);

	SINT32 i;
	FacilityStruct* facility;

	for ( i = 0, facility = cp->facilityList; facility != nullptr; facility = facility->next )
	{
		if ( facility->type == FACILITY_FIREBASE )
			continue;
		i++;
		BETAMSG("%d) %s [own:%s,ori:%s]"
				, facility->uOID
				, facility->name
				, _countryCode[facility->country]
				, _countryCode[facility->originalCountry]
				);
	}
	if ( i == 0 )
	{
		BETAMSG("There are no facilities at this CP.");
	}
	return true;
}

static bool
_dotPwnFac(char* arguments, OOBMessage& message)
{
	CPStruct* cp;
	FacilityStruct* facility;
	UINT32 uFacNo;
	char* facilityStr;
	WWII_COUNTRY newCountry;
	bool quietly = false;

	cp = strtGetCP(message.m_cpID);

	if ( message.m_text[0] == 'c' )
		quietly = true;

	if ( arguments != nullptr && *arguments != 0 )
	{
		if ( *arguments == '?' )
			return _dotPwnFacQuery(message, cp);

		facilityStr = strchr(arguments, ' ');
		if ( facilityStr != nullptr )
		{
			*(facilityStr++) = 0;

			newCountry = isCountryCode(arguments);
			if ( newCountry == 0 )
			{
				BETAMSG("Unrecognized country code, '%s'", arguments);
				return false;
			}

			uFacNo = atoi(facilityStr);
			if ( uFacNo < 1 || (facility = strtGetFacilityByOID(uFacNo)) == nullptr )
			{
				BETAMSG("Invalid facility number, '%s', use '%s ?' for cp-facility list"
						, facilityStr
						, message.m_text
						);
				return false;
			}

			BETAMSG("Changing %s [%d] from %s to %s"
					, facility->name
					, facility->uOID
					, _countryCode[facility->country]
					, _countryCode[newCountry]
					);

			return strtConvertFacility(*facility, newCountry, true, quietly);
		}
	}

	BETAMSG("%s <country> <facilityID> OR %s ? for cp-facility list"
			, message.m_text
			, message.m_text
			);

	return false;
}

// Change the ownership of a CP
static bool
_dotPwnCP(char* arguments, OOBMessage& message)
{
	char* cpName = nullptr;
	WWII_COUNTRY newOwner;
	CPStruct* cp;
	bool quietly = false;
	bool force = true;

	// Find the second argument
	if ( arguments == nullptr || *arguments == 0 )
	{
		BETAMSG("Specify Country Code and (optionally) CP name");
		return false;
	}

	cpName = strchr(arguments, ' ');
	if ( cpName != nullptr )
		*(cpName++) = 0;

	if ( *arguments == '~' )
	{
		force = false;
		arguments++;
	}

	newOwner = isCountryCode(arguments);
	if ( newOwner == WWII_NO_COUNTRY )
	{
		BETAMSG("Unrecognized country code, '%s'", arguments);
		return false;
	}

	cp = _getCPByArguments(cpName, message, false);
	if ( cp == nullptr )
		return false;

	if ( strncmp(message.m_text, "ch", 2) == 0 )
		quietly = true;

	if ( cp->changeOwnership(newOwner, force, quietly) )
	{
		if ( quietly )
			LOCMESSAGE(LSTR_NOTE, (force ? LSTR_FORCED_OWNERSHIP : LSTR_CHANGED_OWNERSHIP), cp->name, _countryCode[newOwner]);
		else
			chatSendBroadcastAll(LSTR_SYSTEM, LSTR_GM_CHANGED_OWNERSHIP, cp->name, _sideName[wwiiSide(newOwner)]);
	}
	else
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_NO_FACILIITIES_CHANGED, cp->name);
	}

	return true;
}

static bool
_dotFacilities(char* arguments, OOBMessage& message)
{
	UINT32 uCount;
	UINT16 uNextOID;
	FacilityStruct* cur;

	if ( arguments == nullptr || *arguments == '?' || *arguments == 0 )
	{
		BETAMSG("*** Use with diligence - this operation is expensive");
		BETAMSG("Usage: %s <beginning of facility name>, e,g. %s Antw"
				, message.m_text
				, message.m_text
				);
		return false;
	}

	uNextOID = 0;
	uCount = 0;
	while ( (cur = strtNextMatchingFacility(arguments, uNextOID)) != nullptr )
	{
		if ( uCount == 15 )
		{
			BETAMSG("<List truncated>");
			break;
		}

		BETAMSG("%u) %s [%s, %s]"
				, cur->uOID
				, cur->name
				, (cur->isOpen()) ? "Open" : "Closed"
				, _countryCode[cur->country]
				);

		uCount++;
		uNextOID = cur->uOID + 1;
	}

	return true;
}

static bool
_dotRebuildCP(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	cp = _getCPByArguments(arguments, message, true);
	if ( cp == nullptr )
		return false;

	strtForceRebuildCP(*cp);
	BETAMSG("Rebuild at %s completed", cp->name);

	return true;
}

// Open or close a facility
static bool
_dotOpenCloseFac(char* arguments, OOBMessage& message)
{
	bool opening = true;				// Open is the default
	FacilityStruct* fac = nullptr;
	UINT32 facID;

	if ( arguments == nullptr || *arguments == 0 )
	{
		CPStruct* cp;
		cp = strtGetCP(message.m_cpID);
		if ( cp == nullptr )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_FACILITY);
			return false;
		}

		_listSpawnables(cp, message);
		return false;
	}

	if ( tolower(message.m_text[0]) == 'c' )
		opening = false;

	facID = atoi(arguments);
	if ( facID > 0 )
		fac = strtGetFacilityByOID(facID);

	if ( fac == nullptr )
	{
		BETAMSG("Invalid facility number, '%s', use '%s' for list of facilities"
						, arguments
						, message.m_text
						);
		return false;
	}

	if ( opening )
	{
		if ( fac->isOpen() )
			BETAMSG("'%s' (%d) is already open", fac->name, fac->uOID);
		else
		{
			strtOpenBase(*fac);
			BETAMSG("'%s' (%d) opened", fac->name, fac->uOID);
		}
	}
	else
	{
		if ( fac->isOpen() == false )
			BETAMSG("'%s' (%d) is already closed", fac->name, fac->uOID);
		else
		{
			strtCloseBase(*fac);
			BETAMSG("'%s' (%d) closed", fac->name, fac->uOID);
		}
	}

	return false;
}

static bool
_dotForceFacilitys(char* arguments, OOBMessage& message)
{
	bool doAll = false;					// Whether or not to do all firebases
	bool opening = true;					// Open is the default
	UINT32 affected = 0;					// How many FBs we affected
	CPStruct* cp;
	FacilityStruct* fac;

	if ( arguments != nullptr && strcasecmp(arguments, "all") == 0 )
	{
		cp = strtGetCPList();
		doAll = true;
	}
	else
		cp = _getCPByArguments(arguments, message, false);

	if ( cp == nullptr )
		return false;


	if ( tolower(message.m_text[0]) == 'c' )
		opening = false;

	for (; cp != nullptr; cp = cp->next )
	{
		// Find all matching CPs (or 'all' cps)
		if ( arguments != nullptr && doAll == false && strcasecmp(arguments, cp->name) != 0 )
			continue;

		for ( fac = cp->facilityList; fac != nullptr; fac = fac->next )
		{
			if ( fac->type == FACILITY_FIREBASE )
				continue;

			if ( fac->isOpen() != opening )
			{
				fac->setOpen(opening);
				affected++;
			}
		}

		// If we aren't processing all FBs, we were given a name,
		// we found it, we can stop.
		if ( !doAll )
			break;
	}

	if ( affected == 0 )
	{
		if ( doAll )
		{
			BETAMSG("*CAUTION* No facilities indexed in world???");
			return false;
		}
		else if ( arguments != nullptr && *arguments != 0 )
		{
			BETAMSG("No CP matching '%s' found", arguments);
			return false;
		}
	}
	else
	{
		if ( opening )
			BETAMSG("Opened %d facilities", affected);
		else
			BETAMSG("Closed %d facilities", affected);
	}

	return true;
}

#ifdef EXTRA_CONFIGURATIONS
static bool
_dotDebugSupply(char* arguments, OOBMessage& message)
{
	if ( arguments && strcmp(arguments, "on") == 0 )
	{
		strtEnableDebugSupply();
		BETAMSG("Supply debugging enabled");
	}
	else if ( arguments && strcmp(arguments, "off") == 0 )
	{
		strtDisableDebugSupply();
		BETAMSG("Supply debugging disabled");
	}
	else
	{
		BETAMSG("Specify 'on' or 'off'");
	}

	return true;
}

static bool
_dotForceFBs(char* arguments, OOBMessage& message)
{
	bool doAll = false;					// Whether or not to do all firebases
	bool opening = true;				// Open is the default
	UINT32 affected = 0;				// How many FBs we affected
	CPStruct* cp;
	FacilityStruct* fb;

	if ( arguments != nullptr && strcasecmp(arguments, "all") == 0 )
	{
		cp = strtGetCPList();
		doAll = true;
	}
	else
	{
		cp = _getCPByArguments(arguments, message, false);
		if ( cp == nullptr )
			return false;
	}

	if ( tolower(message.m_text[0]) == 'c' )
		opening = false;

	if ( arguments && strcmp(arguments, "all") == 0 )
	{
		doAll = true;
	}

	for (; cp != nullptr; cp = cp->next )
	{
		// Find all matching CPs (or 'all' cps)
		if ( doAll == false && arguments && strcasecmp(arguments, cp->name) != 0 )
			continue;

		for ( fb = cp->facilityTypeIndex[FACILITY_FIREBASE]; fb != nullptr; fb = fb->typeNext )
		{
			strtInitFirebase(*fb, opening);
			affected++;
		}

		// If we aren't processing all FBs, we were given a name,
		// we found it, we can stop.
		if ( !doAll )
			break;
	}

	if ( affected == 0 )
	{
		if ( doAll )
		{
			BETAMSG("*CAUTION* No FBS indexed in world???");
			return false;
		}
		else
		{
			BETAMSG("No CP matching '%s' found", arguments);
			return false;
		}
	}
	else
	{
		if ( opening )
			BETAMSG("Opened %d firebases", affected);
		else
			BETAMSG("Closed %d firebases", affected);
	}

	return true;
}
#endif

static bool
_dotLua(char* arguments, OOBMessage& message)
{
	std::string object((const char*)arguments);
	Chat::Name sender(message.m_handleID);
	bool success = Lua::RunObject(sender.c_str(), object);
	if ( !success )
	{
		dbugLog(_FLW, "FAIL: '%s' failed for %s", arguments, sender.c_str());
	}

	Lua::SCRIPT_RESULTS& results = Lua::Results();

	if ( results.empty() )
	{
		if ( success )
			RESPOND("OK.");
		else
			LOCMESSAGE(LSTR_ERROR, LSTR_INTERNAL_ERROR, "");
	}
	else
	{
		for ( size_t i = 0, size = results.size(); i < size; ++i )
		{
			RESPOND("%s", results[i].c_str());
		}
	}

	results.clear();

	return true;
}

#ifndef PUBLIC_RELEASE
static bool
_dotSetTime(char* arguments, OOBMessage& message)
{
	bool haveTime = false;
	bool freezeTime = false;

	if ( *arguments == '@' )
	{
		freezeTime = true;
		++arguments;
	}

	UINT32 hour = 0, minutes = 0;
	if ( sscanf(arguments,"%2u:%2u", &hour, &minutes) == 2 )
		haveTime = true;
	else if ( sscanf(arguments, "%2u%2u", &hour, &minutes) > 0 )
		haveTime = true;

	if ( !haveTime || hour > 23 || minutes > 59 )
	{
		BETAMSG("Invalid time value: %s. Accepted formats HH:MM, HHMM and HH. Prefix time with '@' to lock that particular time.", arguments);
		return false;
	}

	if ( freezeTime )
		host_config.arena.freezeClock = hour * 100 + minutes;
	else
		host_config.arena.freezeClock = 0;

	wthrSetGameTime((hour * 3600) + (minutes * 60));

	extern void cnvyUpdateTime();
	cnvyUpdateTime();

	char hourBuf[4];
	char minutesBuf[4];
	sprintf(hourBuf, "%02u", hour);
	sprintf(minutesBuf, "%02u", minutes);
	chatSendBroadcastAll(LSTR_SYSTEM, LSTR_GM_SET_GAMETIME, hourBuf, minutesBuf);

	return true;
}
#endif

SINT32 _lastCPBroadcast;
static bool
_dotBroadcastCP(char* arguments, OOBMessage& message)
{
	SINT32 currentTime = teulTime();

	if ( arguments != nullptr && arguments[0] == '?' )
	{
		LOCMESSAGE(LSTR_USAGE, LSTR_BROADCAST_USAGE, message.m_text);
		LOCMESSAGE(LSTR_DETAIL, LSTR_BROADCAST_DETAIL);
		return true;
	}

	if ( arguments != nullptr && strcasecmp(arguments, "all") == 0 )
	{
		if ( currentTime > _lastCPBroadcast + 60 )
		{
			strtBroadcastCPs();
			LOCMESSAGE(LSTR_NOTE, LSTR_CPS_REBROADCAST);
			_lastCPBroadcast = currentTime;
		}
		else
			LOCMESSAGE(LSTR_CAUTION, LSTR_CMD_ONCE_PER_MINUTE, message.m_text);
	}
	else
	{
		CPStruct* cp;

		cp = _getCPByArguments(arguments, message, true);
		if ( cp == nullptr )
			return false;

		strtQueueCPStatus(cp);
		LOCMESSAGE(LSTR_NOTE, LSTR_CP_REBROADCAST, cp->name);
	}

	return true;
}

static bool
_dotBroadcastFacs(char* arguments, OOBMessage& message)
{
	SINT32 i = 0;
	CPStruct* cp;
	FacilityStruct* facility;

	if ( arguments != nullptr && arguments[0] == '?' )
	{
		BETAMSG("%s [cp name]: Retransmit status of facilities in specified or current CP", message.m_text);
		return true;
	}

	cp = _getCPByArguments(arguments, message, true);
	if ( cp == nullptr )
		return false;

	for ( facility = cp->facilityList; facility != nullptr; facility = facility->next )
	{
		strtQueueFacilityStatus(facility);
		i++;
	}
	strtQueueCPStatus(cp);

	BETAMSG("%d facilities retransmitted", i);

	return true;
}

static StratHCUnit*
_getHCUnit(const char* arguments, const OOBMessage& message)
{
	StratHCUnit* unit = nullptr;

	if ( arguments != nullptr && arguments[0] != 0 )
	{
		unit = StratHCUnit::Lookup(arguments);
		if ( unit == nullptr )
			RESPOND("%s requires you be in or specify a brigade", message.m_text);
	}
	else
	{
		unit = StratHCUnit::Lookup(message.m_brigadeID);
		if ( unit == nullptr )
			RESPOND("Unrecognized brigadeID, sorry");
	}

	return unit;
}

static bool
_dotResupplyUnit(char* arguments, OOBMessage& message)
{
	StratHCUnit* unit = _getHCUnit(arguments, message);
	if ( unit == nullptr )
		return false;

	BETAMSG("Resetting full supply of %s (%s)", unit->name(), unit->callsign());

	unit->resetSupply();

	return true;
}

static bool
_dotRestartSupply(char* arguments, OOBMessage& message)
{
	StratHCUnit* unit = _getHCUnit(arguments, message);
	if ( unit == nullptr )
		return false;

	RESPOND("Restart resupply queue for %s (%s) [no effect if unit is out of supply]", unit->name(), unit->callsign());

	unit->purgeResupply(false);
	unit->restartSupply(60, 900, false);
	return true;
}

static bool
_dotDumpResupply(char* arguments, OOBMessage& message)
{
	bool listAll = false;
	if ( strncmp(arguments, "all", 3) == 0 )
	{
		listAll = true;
		arguments += 3;
		while ( *arguments && isspace(*arguments) )
			++arguments;
	}

	StratHCUnit* unit = _getHCUnit(arguments, message);
	if ( unit == nullptr )
		return false;

#ifdef PUBLIC_RELEASE
	if ( unit->side() != message.m_side )
	{
		BETAMSG("You can't view enemy supply queues.");
		return false;
	}
#endif

	std::vector<std::string> results;
	if ( !unit->fetchResupplyQueue(results, 36, listAll) || results.empty() )
	{
		BETAMSG("%s resupply queue is empty", unit->callsign());
		return true;
	}

	BETAMSG("%s resupply queue (%s):", unit->callsign(), results[0].c_str());
	std::string output = "";
	for ( UINT32 i = 1; i < results.size(); ++i )
	{
		if ( output.length() + results[i].length() >= MAX_MESSAGE_LENGTH - 4 )
		{
			BETAMSG(output.c_str());
			output = "";
		}
		output += results[i];
		output += " ";
	}
	if ( output.empty() == false )
		BETAMSG(output.c_str());

	return true;
}

static bool
_dotBlowFB(char* arguments, OOBMessage& message)
{
	FacilityStruct* fb = nullptr;
	UINT32 fbOID = 0;

	if ( arguments != nullptr && *arguments != 0 )
	{
		fbOID = atol(arguments);
		if ( fbOID > 0 )
			fb = strtGetFacilityByOID(fbOID);
	}
	else
		fb = strtGetFacility(message.m_cpID);

	if ( fb == nullptr || fb->type != FACILITY_FIREBASE )
	{
		BETAMSG("Specify firebase to destroy");
		return false;
	}

	if ( fb->isOpen() == false )
	{
		BETAMSG("%s [%d] is already closed", fb->name, fbOID);
		return false;
	}

	if ( !strtProcessFirebase(*fb) )
	{
		BETAMSG("%s [%d] failed to blow", fb->name, fbOID);
		return true;
	}

	BETAMSG("%s [%d] blown", fb->name, fbOID);

	return true;
}

//////////////////////////////////////////////////////////////////////

static bool
_dotStatus(char* arguments, OOBMessage& message)
{
	RESPOND("NOTE: Please use /own (or /ow or .ow) command instead.");

	return _dotOwn(arguments, message);
}

//////////////////////////////////////////////////////////////////////

#ifdef BETA_RELEASE
static bool
_dotRestart(char* arguments, OOBMessage& message)
{
	dbugLog(_FLA, "restarting the strat host");
	if ( system("(sleep 1; " PN_PLAYNET_PATH "/bin/stophost; sleep 1; " PN_PLAYNET_PATH "/bin/starthost) &") < 0 )
	{
	    dbugLog(_FLA, "dotRetsart system call failed");
	    RESPOND("System call failed");
	}

	return true;
}
#endif // BETA_RELEASE

static bool
_dotShutdownStrat(char* arguments, OOBMessage& message)
{
	if ( strcmp(arguments, "stratSystem") != 0 )
		return false;

	char command[64] = "2";
	_dotArenaLock(command, message);

	strtProcessDBQueue();
	strtSaveTOESupplies();
	std::this_thread::sleep_for(std::chrono::seconds(1));

    if ( system(PN_PLAYNET_PATH "/bin/stophost &") < 0 )
    {
        RESPOND("System call failed");
        LogAbort("dotShutdownStrat system call failed");
    }

	return true;
}

//////////////////////////////////////////////////////////////////////

static bool
_dotFallback(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	cp = strtGetCP(message.m_cpID);
	if ( cp == nullptr )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_FACILITY);
		return false;
	}

	// Whether we are in "try" or "do" mode depends on the supply of a '!'
	bool testOnly = ( arguments == nullptr || *arguments != '!' );
	SCMD_RESULT result = cp->issueFallback(message.m_side, testOnly, message.m_originatorID);

	chatSendSCMDResult(message.m_originatorID, SC_FALLBACK, cp->fullObjID, message.m_side, result);

	return true;
}

static void
_nextMove(const OOBMessage& message, const StratHCUnit* unit)
{
	std::string text = "";
	UINT32 nextMoveIn = unit->nextMoveIn();
	if ( nextMoveIn == 0 )
		text = "ready";
#if defined(PUBLIC_RELEASE) || defined(RELEASE_CANDIDATE)
	else if ( message.m_side != unit->side() )
	{
		static const UINT32 _ACCURACY_S = 5 * 50;
		nextMoveIn = nextMoveIn + _ACCURACY_S - (nextMoveIn % _ACCURACY_S);
		text = "~";
		text += chatTimeInterval(nextMoveIn);
	}
#endif
	else
	{
		text = chatTimeInterval(nextMoveIn);
	}

	text += ".";

	if ( unit->offMap() == false )
	{
		text += " Fallback: ";

		const CPStruct* fallback = unit->findFallback();
		if ( nullptr != fallback )
			text += fallback->name;
		else
			text += "None";
		text += ".";
	}

	LOCMESSAGE(LSTR_NOTE, LSTR_LABELLED_GENERIC, unit->callsign(), text.c_str());
}

static bool
_dotNextMove(char* arguments, OOBMessage& message)
{
	std::vector<const char*> args;
	_vectorize(arguments, args);

	StratHCUnit* unit;

	if ( args.empty() )
	{
		unit = _getHCUnit(arguments, message);
		if ( unit == nullptr )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_NO_BRIGADE);
			return true;
		}

		_nextMove(message, unit);

		return true;
	}

	std::set<StratHCUnit*> visited;

	if ( args.size() > 6 )
		args.resize(6);

	for ( UINT32 i = 0; i < args.size() && visited.size() < 6; ++i )
	{
		unit = StratHCUnit::Lookup(args[i]);
		if ( unit == nullptr )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_UNKNOWN_UNIT, args[i]);
			return true;
		}

		if ( visited.find(unit) != visited.end() )
			continue;

		_nextMove(message, unit);

		visited.insert(unit);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////

static bool
_dotHoldAtAllCosts(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	cp = strtGetCP(message.m_cpID);
	if ( cp == nullptr )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_FACILITY);
		return false;
	}

	// Pass a trailing '*' to indicate 'do it now'
	bool execute = ( arguments != nullptr && *arguments == '!' );
	SCMD_RESULT result = cp->holdAtAllCosts(message.m_side, execute);

	chatSendSCMDResult(message.m_originatorID, SC_HOLD, cp->fullObjID, message.m_side, result);

	return true;
}

//////////////////////////////////////////////////////////////////////

static bool
_dotOverrun(char* arguments, OOBMessage& message)
{
	CPStruct* cp;

	cp = strtGetCP(message.m_cpID);
	if ( cp == nullptr )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_FACILITY);
		return false;
	}

	SCMD_RESULT result = cp->overrun(message.m_side);

	chatSendSCMDResult(message.m_originatorID, SC_OVERRUN, cp->fullObjID, message.m_side, result);

	return true;
}

//////////////////////////////////////////////////////////////////////

static void
_showObjectiveInfo(OOBMessage& message)
{
	// Have they targetted something
	CPStruct* cp = _getCPByArguments(nullptr, message, true);
	if ( cp == nullptr )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SEL_TARGET);
		return;
	}

	if ( !cp->isObjective(message.m_side) )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_NO_OBJECTIVE, cp->name);
		return;
	}

	// Advice when objective is being withdrawn
	if ( cp->m_clearing[message.m_side] )
		LOCMESSAGE(LSTR_CAUTION, LSTR_OBJECTIVE_WITHDRAWING, cp->name);

	if ( cp->isBridge() )
	{
		if ( cp->isAttackObjective(message.m_side) )
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_BRIDGE_ATTACK, cp->name);
		else
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_BRIDGE_DEFEND, cp->name);
		if ( cp->isContested() )
			LOCMESSAGE(LSTR_NONE, LSTR_INFO_BRIDGE_OUT);
	}
	else
	{
		if ( cp->isAttackObjective(message.m_side) )
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_CITY_ATTACK, cp->name);
		else if ( cp->isHolding(message.m_side) )
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_CITY_DEFEND_HAAC, cp->name);
		else if ( cp->isFallingBack() )
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_CITY_DEFEND_FALL, cp->name);
		else
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_CITY_DEFEND, cp->name);
		if ( cp->isContested() )
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_CITY_CONTESTED, cp->name);

		int lived = teulTime() - cp->m_attackStarted;

		// Turn it into minutes, with a minimum of "0"
		lived = ( lived < 0 ) ? 0 : lived / 60;

		if ( lived <= 1 )
		{
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_NEW_OBJECTIVE);
		}
		else
		{
			char num1[11], num2[11];
			snprintf(num1, 10, "%d", (int)(lived / 60));
			snprintf(num2, 10, "%d", (int)(lived % 60));
			LOCMESSAGE(LSTR_NOTE, LSTR_INFO_OBJECTIVE_PLACED, num1, num2);
		}
	}
}

//////////////////////////////////////////////////////////////////////

static void
_listObjectiveNumbers(OOBMessage& message)
{
	static ObjectiveStats& stats = g_objectiveStats;
	WWII_SIDE forSide = message.m_side;
	stats.update(forSide);

	///LOCALIZE or transfer to client.
	RESPOND_LOC(LSTR_NOTE, LSTR_GENERIC
					, "Capture Objectives: %u limit, %u placed (%u engagements), %u spare."
						, stats.m_cityLimit
						, stats.m_cityAttacks[forSide]
						, stats.m_battles[forSide]
						, stats.m_cityAvailable[forSide]
						);

	RESPOND_LOC(LSTR_NONE, LSTR_GENERIC, "Defense Objectives: %d.", stats.m_cityDefends[forSide]);

	RESPOND_LOC(LSTR_NONE, LSTR_GENERIC
					, "Bridge Objectives: %d limit, %d attack, %d defense, %d spare."
						, stats.m_bridgeLimit
						, stats.m_bridgeAttacks[forSide]
						, stats.m_bridgeDefends[forSide]
						, stats.m_bridgeAvailable[forSide]
						);
}

//////////////////////////////////////////////////////////////////////

static bool
_dotObjectives(char* arguments, OOBMessage& message)
{
	if ( message.m_side == WWII_NO_SIDE )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_SELECT_SIDE);
		return false;
	}

	switch ( tolower(arguments[0]) )
	{
	case 'c':
		{
			const std::string& text = strtListObjectives(message.m_side, true);
			LOCMESSAGE(LSTR_NOTE, LSTR_GENERIC, text.c_str());
		}
	break;

	case 'l':
		{
			const std::string& text = strtListObjectives(message.m_side, false);
			LOCMESSAGE(LSTR_NOTE, LSTR_GENERIC, text.c_str());
		}
	break;

	case 'n':
		_listObjectiveNumbers(message);
	break;

	case 'i':
		_showObjectiveInfo(message);
	break;

	default:
		///LOCALIZE: URGLE.
		LOCMESSAGE(LSTR_CAUTION, LSTR_GENERIC, "Please specify: (c)itys, (l)ist, (n)umbers or (i)nfo (e.g. \".obj c\")");
	break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Set the target fields

static void
_setTarget(const FULL_OBJ_ID id)
{
	targeted_cp = nullptr;
	targeted_fac = nullptr;

	if ( id.id )
	{
		targeted_cp = strtGetCP(id);
		if ( targeted_cp == nullptr )
		{
			targeted_fac = strtGetFacility(id);
			if ( targeted_fac != nullptr )
				targeted_cp = targeted_fac->cp;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Redeploy a brigade:
//  -xfer tells it to "transfer" (trickle) the spawn lists instead of
//       keeping them intact
//  -move tells it to apply movement delays like a normal move

static bool
_dotRedeploy(char* arguments, OOBMessage& message)
{
	if ( arguments[0] == '?' || strcmp(arguments, "help") == 0 )
	{
		LOCMESSAGE(LSTR_USAGE, LSTR_REDEPLOY_USAGE);
		LOCMESSAGE(LSTR_DETAIL, LSTR_REDEPLOY_DETAIL);
		return true;
	}

	bool applyDelay = false;
	bool transferSupply = false;
	bool illegal = false;
	StratHCUnit* unit = nullptr;
	CPStruct* destination = nullptr;

	std::vector<const char*> args;
	_vectorize(arguments, args);

	for ( UINT32 i = 0; i < args.size(); ++i )
	{
		if ( strcasecmp(args[i], "-xfer") == 0 )
			transferSupply = true;
		else if ( strcasecmp(args[i], "-move") == 0 )
			applyDelay = true;
		else if ( args[i][0] == '-' )
		{
			LOCMESSAGE(LSTR_USAGE, LSTR_REDEPLOY_USAGE);
			LOCMESSAGE(LSTR_DETAIL, LSTR_REDEPLOY_DETAIL);
			return true;
		}
		else if ( unit == nullptr )
		{
			unit = StratHCUnit::Lookup(args[i]);
			if ( unit == nullptr )
			{
				LOCMESSAGE(LSTR_CAUTION, LSTR_UNKNOWN_UNIT, args[i]);
				return false;
			}
		}
		else if ( destination == nullptr )
		{
			UINT32 id = atol(args[i]);
			if ( id == 0 )
				destination = strtGetCPByName(args[i]);
			else
				destination = strtGetCPByOID(id);

			if ( destination == nullptr )
			{
				LOCMESSAGE(LSTR_CAUTION, LSTR_INVALID_CP_NAME, args[i]);
				return false;
			}
			if ( destination->isCity() == false )
			{
				LOCMESSAGE(LSTR_CAUTION, LSTR_SPECIFY_TOWN);
				return false;
			}
		}
	}

	if ( destination == nullptr )
	{
		destination = strtGetCP(message.m_cpID);
		if ( destination == nullptr )
		{
			FacilityStruct* fac = strtGetFacility(message.m_cpID);
			if ( fac != nullptr )
				destination = fac->cp;
		}

		if ( destination == nullptr )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_SPECIFY_TOWN);
			return false;
		}
	}

	if ( unit == nullptr )
	{
		unit = StratHCUnit::Lookup(message.m_brigadeID);
		if ( unit == nullptr )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_JOIN_OR_SPECIFY_BDE, message.m_text);
			return false;
		}
	}

	if ( unit->isDivision() == false && unit->isBrigade() == false )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_BDE_AND_DIV_ONLY);
		return false;
	}

	BRIGADE_MOVE_RESULT bmr = unit->deployPermission(destination, false);
	switch ( bmr )
	{
	case BMR_FAILED:
	case BMR_FAILED_OWNER:
	case BMR_FAILED_CONTROL:
		LOCMESSAGE(LSTR_CAUTION, LSTR_ENEMY_OWNS, destination->name);
		return false;
	break;

	case BMR_APPROVED:
		if ( unit->homeCP() == destination && !(transferSupply || applyDelay) )
		{
			LOCMESSAGE(LSTR_CAUTION, LSTR_UNIT_ALREADY_AT, unit->callsign(), destination->name);
			return false;
		}

	break;

	default:
		illegal = true;
	break;
	}

	///LOCALIZE
	RESPOND_LOC(LSTR_BETA, LSTR_GENERIC, "%s %s -> %s. Supply: %s, Move Delay: %s"
											, (illegal ? "**FORCING**" : "Moving")
											, unit->callsign(), destination->name
											, (transferSupply ? "trickle" : "instant")
											, (applyDelay ? "Yes" : "No")
											);

	unit->deploy(destination, (applyDelay) ? brigadeMoveTime_s : 0, Chat::Name(message.m_originatorID).c_str(), transferSupply, applyDelay);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Execute a lua script

std::vector<const char*> script_args;
OOBMessage* script_env;
CPStruct* targeted_cp;
FacilityStruct* targeted_fac;

static bool
_dotRunLua(char* arguments, OOBMessage& message)
{
	// Single argument
	if ( _vectorize(arguments, script_args) == 0 )
	{
		BETAMSG("No command specified");
		return false;
	}

	const char* script = script_args[0];

	dbugLog(_FLI, "Player %s running unit '%s'"
					, Chat::Name(message.m_originatorID).c_str()
					, script
					);

	// Sanitize the filename
	const char* p = script;
	while ( *p != 0 )
	{
		if ( isupper(*p) )
		{
			BETAMSG("Illegal command name '%s' (uppercase not allowed)");
			return false;
		}
		if ( !(isalnum(*p) || *p == '-' || (p > script && *p == '/')) )
		{
			BETAMSG("Illegal script name '%s'", script);
			return false;
		}
		p++;
	}

	if ( (int)(p - script) > 32 )
	{
		BETAMSG("Invalid command name (too long)");
		return false;
	}

	// Back up host_config so we can tell if the client modifies it
	WWII_CONFIG* old = new WWII_CONFIG;
	memcpy(old, &host_config, sizeof(WWII_CONFIG));

	bool publicMode = (message.m_text[0] == 'c');
	bool success = false;

	script_env = &message;

	std::string obj = "";

	_setTarget(message.m_cpID);

	if ( strtdbFetchLuaScript(script, obj) )
	{
		success = Lua::RunObject(script, obj);
	}
	else
	{
		static const UINT32 FILENAME_SIZE = 160;
		char filename[FILENAME_SIZE+1];
		if ( publicMode ) strprintf(filename, "%s" "/public/%s.lua", cv_luaPath.GetString(), script);
		else strprintf(filename, "%s" "/%s.lua", cv_luaPath.GetString(), script);
		success = Lua::RunScript(filename);
	}

	_setTarget(nullObjID);

	if ( !success ) dbugLog(_FLW, "FAIL: %s failed for %s", script, Chat::Name(message.m_originatorID).c_str());
	else if ( !publicMode ) dbugLog(_FLI, "OK: %s ran for %s", script, Chat::Name(message.m_originatorID).c_str());

	Lua::SCRIPT_RESULTS& results = Lua::Results();

	if ( publicMode && results.size() == 0 )
	{
		if ( success )
			BETAMSG("OK.");
		else
			BETAMSG("Command failed: Internal error.");
	}

	for ( size_t i = 0; i < results.size(); ++i )
		RESPOND("%s", results[i].c_str());
	results.clear();

	if ( !publicMode )
	{
		if ( !success )
			BETAMSG("Failed:%s", script);
		else
			BETAMSG("Ran:%s", script);
	}

	if ( memcmp(old, &host_config, sizeof(WWII_CONFIG)) != 0 )
		dotApplyConfig();

	// Garbage collection
	delete old;

	return true;
}

//////////////////////////////////////////////////////////////////////
// Reload the vehicle access rules

static bool
_dotUpdateVehicles(char* arguments, OOBMessage& message)
{
	strtdbLoadTOETemplates();
	strtdbReadVehicleAccess();
	BETAMSG("Updated vehicles (ranks and toe template lists)");

	return true;
}

//////////////////////////////////////////////////////////////////////
// Describe the current RDP state

static bool
_dotRDP(char* arguments, OOBMessage& message)
{
#ifdef BETA_RELEASE
	rdpInfo.Update(WWII_ENGLAND, 1, 0);
	rdpInfo.Update(WWII_FRANCE, 1, 0);
	rdpInfo.Update(WWII_GERMANY, 1, 0);
#endif

	if ( rdpInfo.Populated() == false )
	{
		RESPOND("No RDP Intel received yet - try again in a few minutes");
		return true;
	}

	// Skip it if the update is 60+ minutes old
	if ( rdpInfo.Expired() == true )
	{
		RESPOND("No RDP Intel received in the last hour - try again in a few minutes");
		return true;
	}

	rdpInfo.Transcribe();

	RESPOND("%s.", rdpInfo.m_detail.c_str());

	return true;
}

void
RDPInfo::Transcribe()
{
	if ( m_detail.empty() == false )
		return;

	// When is the update dated
	char buf[256];
	time_t now = teulTime();
	struct tm tm;
	(void)tm;
	strftime(buf, 255, "%T %Z", localtime_r(&now, &tm));

	m_detail = "As of ";
	m_detail += buf;
	m_detail += ":\n";

	strtCalculateRDPOutput();

	bool firstCountry = true;

	for ( UINT32 i = 1; i < MAX_COUNTRIES; ++i )
	{
		if ( m_loaded[i] == false )
			continue;

		if ( firstCountry == false )
			m_detail += ";  ";

		strprintf(buf, "%s Tier %u at %u%%", _countryNameAdj[i], m_cycle[i], m_percent[i]);
		m_detail += buf;
		if ( factoryDamageFactor[i] == 0.0 )
			m_detail += ", factories at full";
		else if ( factoryDamageFactor[i] >= 1.0 )
			m_detail += ", factories disabled";
		else
		{
			strprintf(buf, ", factories at %u%%", 100 - (UINT32)(factoryDamageFactor[i] * 100));
			m_detail += buf;
		}
		double penalty = stratRDPSupplyPenalty((WWII_COUNTRY)i) - 1.0;
		if ( penalty != 0 )
		{
			double percent = fabs(penalty * 100.0);
			if ( percent < 0.01 )
				percent = 0.01;
			strprintf(buf, ", resupply %0.2g%% %s", percent, (penalty > 0 ? "slower" : "faster"));
			m_detail += buf;
		}

		firstCountry = false;
	}
}

//////////////////////////////////////////////////////////////////////
// Set current RDP info for a country

static bool
_dotSetRDP(char* arguments, OOBMessage& message)
{
	std::vector<const char*> args;
	if ( _vectorize(arguments, args) != 3 )
	{
		BETAMSG("Incorrect usage - begone");
		return false;
	}

	WWII_COUNTRY country = static_cast<WWII_COUNTRY>(strtoul(args[0], nullptr, 10));
	unsigned int cycle = static_cast<unsigned int>(strtoul(args[1], nullptr, 10));
	unsigned int percent = static_cast<unsigned int>(strtoul(args[2], nullptr, 10));

	if ( WWII_NO_COUNTRY == country || country >= MAX_COUNTRIES ) {
		BETAMSG("Invalid data");
		return false;
	}

	if ( percent > 100 )
		percent = 100;

	rdpInfo.Update(country, cycle, percent);

	strtdbLoadTOETemplates();
	strtdbReadVehicleAccess();

	return true;
}

void
RDPInfo::Update(WWII_COUNTRY country, UINT32 cycle, UINT32 percent)
{
	if ( Expired() )
	{
		for ( UINT32 i = 1; i < MAX_COUNTRIES; ++i )
			m_loaded[i] = false;
	}

	m_cycle[country] = cycle;
	m_percent[country] = percent;
	m_loaded[country] = true;
	m_lastUpdate = teulTime();
	m_detail.clear();
}

//////////////////////////////////////////////////////////////////////

static const char*
_describeSHCU(StratHCUnit* unit)
{
	static std::string text;
	text = (unit->isBrigade()) ? "Brig" : "Div";
	text += "#";
	char buf[16];
	snprintf(buf, 15, "%u", unit->unitID());
	buf[15] = 0;
	text += buf;
	text += ":";
	text += unit->callsign();
	text += "@";
	text += (unit->offMap()) ? "OffMap" : unit->homeCP()->name;
	if ( unit->offMap() == false && unit->fallbackCP() )
	{
		text += "[";
		text += unit->fallbackCP()->name;
		text += "]";
	}

	return text.c_str();
}

static bool
_dotBrigade(char* arguments, OOBMessage& message)
{
	if ( arguments == nullptr || *arguments == 0 )
	{
		BETAMSG("Specificy brigade/division short name");
		return true;
	}

	StratHCUnit* unit = (StratHCUnit*)HCUnit::Lookup(arguments);
	if ( unit == nullptr || (!unit->isBrigade() && !unit->isDivision()) )
	{
		LOCMESSAGE(LSTR_CAUTION, LSTR_UNKNOWN_UNIT, arguments);
		return true;
	}

	BETAMSG("%s %s", _describeSHCU(unit), unit->describe());

	if ( unit->isDivision() && unit->next() )
	{
		std::string response = "";
		StratHCUnit* child;

		for ( child = unit->next(); child != nullptr; child = child->next() )
		{
			if ( !response.empty() ) response +=", ";
			response += "+";
			response += _describeSHCU(child);
		}

		BETAMSG(response.c_str());
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Report on the current factories and their statuses

extern std::string strtGetFactoryList(WWII_COUNTRY forCountry);

static bool
_dotFactories(char* arguments, OOBMessage& message)
{
	std::string result = "UK: " + strtGetFactoryList(WWII_ENGLAND);
	RESPOND("%s", result.c_str());
	result = "FR: " + strtGetFactoryList(WWII_FRANCE);
	RESPOND("%s", result.c_str());
	result = "DE: " + strtGetFactoryList(WWII_GERMANY);
	RESPOND("%s", result.c_str());
	return true;
}

//////////////////////////////////////////////////////////////////////

DOT_COMMANDS dotCommands[] =
{
	{	"help",			_dotHelp,			"Show this information" }
,	{	"arenalock",	_dotArenaLock,		"Lock/Unlock arena: 0=Unlocked, 1=Locked" }
,	{	"brigade",		_dotBrigade,		"Inspect a brigade/division" }
,	{	"broadcastcp",	_dotBroadcastCP,	"Re-broadcast CP status" }
,	{	"broadcastfacs", _dotBroadcastFacs,	"Re-broadcast Facility statuses" }
,	{	"blowfb",		_dotBlowFB,			"Destroys an FB as though blown" }
,	{	"call",			_dotRunLua,			"Call a lua script in a public sandbox" }
,	{	"chowncp",		_dotPwnCP,			"Change ownership of a CP" }
,	{	"chownfac",		_dotPwnFac,			"Change ownership of a Facility" }
,	{	"close",		_dotOpenCloseFac,	"Close a facility forcibly" }
,	{	"closefacs",	_dotForceFacilitys,	"Force facilitys closed" }
,	{	"cpdump",		_dotCPDump,			"Display detailed info on a CP and it's facilities" }
,	{	"cpinfo",		_dotCPInfo,			"Display detailed info on a CP" }
,	{	"dumpresupply",	_dotDumpResupply,	"Dump resupply info at a facility" }
,	{	"facilities",	_dotFacilities,		"Search facilities by name [expensive]" }
,	{	"factories",	_dotFactories,		"List all factories and their current state" }
,	{	"fallback",		_dotFallback,		"Fallback test command" }
,	{	"fbs",			_dotFBs,			"Display FBs for specified town" }
,	{	"hold",			_dotHoldAtAllCosts,	"Hold At All Costs test command" }
,	{	"lua",			_dotLua,			"Run a piece of lua code" }
,	{	"nextmove",		_dotNextMove,		"Report the next move time of a brigade" }
,	{	"objectives",	_dotObjectives,		"List current objectives" }
,	{	"open",			_dotOpenCloseFac,	"Open a facility forcibly" }
,	{	"openfacs",		_dotForceFacilitys,	"Force facilitys open" }
,	{	"overrun",		_dotOverrun,		"Mark a facility as overrun" }
,	{	"own",			_dotOwn,			"Describe ownership state of a town" }
,	{	"pwncp",		_dotPwnCP,			"Change ownership of a CP" }
,	{	"pwnfac",		_dotPwnFac,			"Change ownership of a Facility" }
,	{	"rdp",			_dotRDP,			"Prints current RDP info" }
,	{	"rebuildcp",	_dotRebuildCP,		"Force rebuild of all objects in a CP" }
,	{	"redeploy",		_dotRedeploy,		"Redeploy a brigade: redeploy [xfer] [move] [<brigade> [<destination>]]" }
,	{	"restartsupply",_dotRestartSupply,	"Purge and restart supply queue for brigade/division" }
,	{	"resupplyunit",	_dotResupplyUnit,	"Force reset of supply of a brigade/division" }
,	{	"run", 			_dotRunLua,			"Run Lua script" }
,	{	"setrdp",		_dotSetRDP,			"Set RDP cycle data for;rdp" }
#ifndef PUBLIC_RELEASE
,	{	"settime",			_dotSetTime,	"Change game time (hours only)" }
#endif
,	{	"shutdown",		_dotShutdownStrat,	"Shutdown the strat host" }
,	{	"status",		_dotStatus,			"Display status of an FB" }
,	{	"updatevehicles", _dotUpdateVehicles,	"Update vehicle access levels" }
#ifdef EXTRA_CONFIGURATIONS
,	{	"closefbs",		_dotForceFBs,		"Force FBs closed" }
,	{	"debugsupply",	_dotDebugSupply,	"Toggle supply debugging on/off" }
,	{	"openfbs",		_dotForceFBs,		"Force FBs open" }
#endif // EXTRA_CONFIGURATIONS
#ifdef BETA_RELEASE
,	{	"restart",		_dotRestart,		"Force strat-host restart" }
#endif // DEV_BUILD
,	{	nullptr,			nullptr,				nullptr }
};

// Process an out-of-band message such as a config change command, etc.
void strtProcessOOBMessage(OOBMessage& message)
{
	char* args;
	int i;

	dbugLog(_FLI, "p:%u c:%s oob message: [%s]"
					, message.m_originatorID
					, Chat::Name(message.m_originatorID).c_str()
					, message.m_text
					);

	// empty string
	if ( message.m_text[0] == 0 )
		return;

	// Locate the arguments or end of string
	args = strchr(message.m_text, ' ');
	if ( args != nullptr )
	{
		// We've found a space, nuke it and find the arguments
		*args++ = 0;

		while ( *args && isspace(*args) )
			args++;

		// Finding the EOS means there was only trailing whitespace
		if ( *args == 0 )
			args = nullptr;
	}

	if ( args == nullptr )
		args = const_cast<char*>("");

	for ( i = 0; dotCommands[i].command; i++ )
	{
		if ( strcmp(message.m_text, dotCommands[i].command) == 0 )
		{
			dotCommands[i].stub(args, message);
			return;
		}
	}
}

void OOBMessage::send() {}
