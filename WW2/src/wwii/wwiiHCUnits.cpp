#include <syscfg.h> // Path is project specific.
// Created: Fri Mar 18 13:22:17 CST 2005
//
// Base-class information about the various high command units
//
// Copyright (C) PlayNet, Inc 2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiHCUnits.h"
#include "include/mydbQuery.h"
#include "chat/chatHandle.h"
#include "chat/chatName.h"
#include "dbse/dbseHandle.h"

#if defined(IS_MAP_HOST)
# include "map/mapBrigades.h"
#elif defined(IS_STRAT_HOST)
# include "strt/strtHCUnits.h"
#endif

// Load the list of brigades for all active countries
static const char QRY_SELECT_HCUNITS[] =
	"SELECT shu.unit_id"
		", shu.parent_id"
		", shu.country_id"
		", shu.branch_id"
		", shu.title"
		", shh.host_struct_id"
		", shs.rank"
		", home.cp_ox"
		", home.cp_oy"
		", home.cp_id"
		", IF(owner.isplaynet,UPPER(owner.callsign),owner.callsign)"
		", ownerp.rankid"
		", LOWER(shu.name)"
		", shu.start_home_cp_oid"
		", UNIX_TIMESTAMP(shu.next_move)"
		", shu.nick"
		", shu.toe_template"
		", shu.movement_type"
		", shu.fallback_cp_oid"
		", shu.last_moved"
		", shu.last_moved_by"
		", toe.unit_class"
	" FROM (strat_hc_units shu"
		" LEFT JOIN strat_hc_structure shs ON (shu.structure_id = shs.structure_id)"
			" LEFT JOIN strat_hc_hqs shh ON (shs.structure_id = shh.structure_id)"
				" LEFT JOIN strat_cp home ON (home.cp_oid = shu.home_cp_oid)"
					" LEFT JOIN toe.headers toe ON (shu.toe_template = toe.id))"
		" LEFT JOIN wwii_player owner ON (shu.owner_id = owner.playerid)"
			" LEFT JOIN wwii_persona ownerp ON (owner.playerid = ownerp.playerid and shu.country_id = ownerp.countryid and shu.branch_id = ownerp.branchid)"
	" ORDER BY shs.rank asc, title asc"
;

//////////////////////////////////////////////////////////////////////
// Load in the entire hierachy of HC Units in depth-descending order
// so that we can build a complete tree in a single pass.

bool
wwiiReadHCUnits()
{
	try
	{
		DBQuery q(dbh, QRY_SELECT_HCUNITS);
		if ( q.Rows() == 0 )
			throw std::domain_error("No HC units matching query");

		UINT32 unitID = 0, parentID = 0;
		HCUNIT_TYPE typeID;
		WWII_COUNTRY countryID;
		BRANCH_ENUM branchID;
		BRANCH_ENUM movementType;
		const char* name;
		UINT8 rank;
		FULL_OBJ_ID homeCP = nullObjID;
		ChatHandle owner;
		RANK_NO ownerRank;
		ChatHandle handle;
		UINT32 startCpOID, fallbackCpOID;
		UINT32 nextMove;
		const char* nick;
		const char* toeTemplate;
		UINT32 lastMoved;
		ChatHandle lastMovedBy;
		UCHAR unitClass;

		while (q.FetchRow())
		{
			DBRow& row = q.Row();

			unitID = row.NextUInt();
			parentID = row.NextUInt();
			countryID = (WWII_COUNTRY)row.NextUInt();
			branchID = (BRANCH_ENUM)row.NextUInt();
			name = row.NextColumn();
			typeID = (HCUNIT_TYPE)row.NextUInt();
			rank = (UINT8)row.NextUInt();
			homeCP.cell.ox = (SINT16)row.NextSInt();
			homeCP.cell.oy = (SINT16)row.NextSInt();
			homeCP.id = (UINT16)row.NextUInt();
			owner = chatGetHandle(row.NextColumn());
			ownerRank = (UCHAR)row.NextUInt();
			handle = chatGetLowercaseHandle(row.NextColumn());
			startCpOID = row.NextUInt();
			nextMove = row.NextUInt();
			nick = row.NextColumn();
			toeTemplate = row.NextColumn();
			movementType = (BRANCH_ENUM)row.NextUInt();
			fallbackCpOID = row.NextUInt();
			lastMoved = row.NextUInt();
			lastMovedBy = chatGetHandle(row.NextColumn());
			unitClass = (UCHAR)row.NextUInt();

			if ( nick == nullptr )
			{
				dbugLog(_FLD, "hcu:%u(%s) unit has no nickname", unitID, name);
				nick = name;
			}

			if ( unitID == 0 ) throw std::domain_error("HCUnit list contains unit with NULL or 0 unitID");
			if ( countryID == 0 ) throw std::domain_error("HCUnit list contains unit with NULL or 0 countryID");
			if ( branchID == 0 ) throw std::domain_error("HCUnit list contains unit with NULL or 0 branchID");
			if ( movementType >= MAX_BRANCHES ) throw std::domain_error("HCUnit list contains unit with illegal movementType");
			if ( movementType == 0 )
				movementType = branchID;
			if ( name == nullptr ) throw std::domain_error("HCUnit list contains unit with NULL or empty name");

			if ( typeID > MAX_HCUNIT_TYPES )
				typeID = HCU_GENERIC;

			//// Objects are all self-registering, so we call new and leave
			//// them alone, this is not a memory leak.
			HCUnit* newUnit;

			if ( typeID != HCU_BRIGADE && typeID != HCU_DIVISION )
			{
				homeCP = nullObjID;
				startCpOID = 0;
			}

			switch ( typeID )
			{
				case HCU_BRIGADE:
#if defined(IS_MAP_HOST)
				// On the map host we have more detailed types for some sub-types
					newUnit = (HCUnit*)(new Brigade(unitID, parentID, countryID, branchID, movementType, unitClass, handle, name, nick, rank, homeCP, owner, ownerRank));
				break;
#elif defined(IS_STRAT_HOST)
				// Fallthru, treat brigade & division the same
#else
				// Fallthru
#endif
				case HCU_DIVISION:
#ifdef IS_STRAT_HOST
					newUnit = (HCUnit*)(new StratHCUnit(unitID, typeID, parentID, countryID, branchID, movementType, unitClass, handle, name, nick, rank, homeCP, owner, ownerRank, startCpOID, fallbackCpOID, lastMoved, lastMovedBy, nextMove, toeTemplate));
				break;
#else
				// Fallthru
#endif

				case HCU_GENERIC:
				default:
					newUnit = new HCUnit(unitID, typeID, parentID, countryID, branchID, movementType, unitClass, handle, name, nick, rank, homeCP, owner, ownerRank);
				break;
			}

			newUnit->addToRegistry();

			Chat::Name lastMovedByName(lastMovedBy);
			dbugLog(_FLD, "hcu:%u start:%u fall:%u nm:%u temp:%s lmb:%s lm:%u %p +ALLOC"
					, unitID, startCpOID, fallbackCpOID, nextMove, toeTemplate, lastMovedByName.c_str(), lastMoved
					, newUnit);
		}

		dbugLog(_FLI, "read %u hcunits", static_cast<UINT32>(hcUnits.size()));

		return true;
	}
	catch (std::exception& e)
	{
		LogAbort("%s: %s", __FUNCTION__, e.what());
	}
}

