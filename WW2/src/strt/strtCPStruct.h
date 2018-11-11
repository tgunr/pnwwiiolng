#pragma once
// Date: Wed Dec  7 21:00:54 CST 2005
//
// Defines the CPStruct class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include <set>
#include "strt/strtProtos.h"
#include "wwii/wwiiHCUnits.h"
#include "strt/strtDTL.h"
#include "weap/weapDefs.h"
#include "wwii/wwiiHCUnitsIndex.h"
#include "strt/strtCPFlags.h"
#include "inst/instDescriptor.h"

class EWSReport;
struct WEAPON_SERIAL;

// Unit describing a player's request: <PlayerID,TimeStamp>
using PLAYER_REQUESTS = std::pair<UINT32,UINT32>;
// This would be a queue if they weren't bugged under rh6
using OVERRUNS = std::vector<UINT32>;
using EWSREPORTS = std::map<UINT32,EWSReport*>;
using HISTORY = std::vector<PLAYER_REQUESTS>;
using ACTIVE_BRIGADES = std::map<Brigade*,UINT32>;

//tolua_begin

// External data structures
extern const UINT32 attackFlags[MAX_SIDES];
extern const UINT32 defendFlags[MAX_SIDES];
extern const UINT32 objectiveFlags[MAX_SIDES];

class FacilityStruct;

struct CSAFE_CPStruct
{
	CSAFE_CPStruct()
	{
		memset(this, 0, sizeof(CSAFE_CPStruct));
	}
	~CSAFE_CPStruct()
	{
	}

//FULL_OBJ_ID must be the first field in the structure.
//DO NOT MOVE!!!!!!!!!! -Meat
	FULL_OBJ_ID 		fullObjID;
	bool 				modified;
	CPStruct*			prev;
	CPStruct*			next;

	SINT8				subType;		//Not used for now, but maybe later...
	char				name[MAX_STRT_NAME_LENGTH];
	const char*			filename;
//tolua_end
	const std::string*	m_filename;
//tolua_begin
	WWII_SIDE			side;			//the side that owns the CP
	WWII_COUNTRY		country;		//the country that owns the CP
	WWII_SIDE			originalSide;
	WWII_COUNTRY		originalCountry;
	WWII_SIDE			controllingSide;
	WWII_COUNTRY		controllingCountry;

	UINT32				nextEvent;		//Time at which the next RP generation is due to
										//	take place
	UINT32				numFirebases;	//total # of firebases
	UINT32				facilitiesOwned[MAX_SIDES];	// Facilities owned by each side
	UINT32				ownableFacilities;	// total-facilities - numFirebases
	UINT32				armyBasesOwned[MAX_SIDES];	//how many army bases the owner country controls
	UINT32				numArmyBases;	//total # of army bases the CP has
	UINT32				timeToProduce;	//Time interval between CP runs
	FacilityStruct*		facilityList; 	//the list of facilities in the CP
	FacilityStruct*		facilityTypeIndex[MAX_FACILITY_TYPES];	// Facilities by type
	//following 2 just used for mission system
	UINT32				numOpenFriendlyFBs;
	UINT32				numOpenEnemyFBs;

	//Map stuff
	UINT16				numObjects;

	SINT32				numOpenLinks;	//Number of open Links
	SINT32				numLinks;
	UINT32				flags;

	Supercell*			supercell;

	UINT16				uOID;			// Unique OID

	bool				m_needsSync;

	OVERRUNS*			m_overruns;		// Overrun reports
	bool				m_clearing[MAX_SIDES];

	// Counts of people tripping EWS; WWII_NO_SIDE used for
	// totals, [SIDE][0] for per-side totals
//tolua_end
	UINT16				uNumRareSpawns[MAX_SIDES][MAX_WEAP_CATEGORY];
	UINT16				m_players[MAX_SIDES][MAX_WEAP_CATEGORY];
//tolua_begin

	bool 				m_contention;
	bool 				m_enemyLinked;
	bool 				m_frontlineLinked;
	bool				isUnderAttack;
	bool				m_friendlyLinked;
	bool				m_isSupplyCenter;
	bool				m_isRouteingConnected;

	SINT32				m_posX, m_posY;

//tolua_end
	class Mission*		m_missionsFrom;
	class Mission*		m_missionsTo;
	UINT64				m_lastUpdateNo;
	TeulTimeMS			m_routeingRun;
//tolua_begin
};

enum TOWN_STATUS
{
	INACTIVE = 0,
	RESERVE,
	BACK_LINE,
	FRONT_LINE,
	// Add new statuses here
	MAX_TOWN_STATUS
};

class CPStruct : public CSAFE_CPStruct
{
public:
//tolua_end
	CPStruct()
		: CSAFE_CPStruct()
		, m_links()
		, ews()
		, m_fallback(WWII_NO_SIDE)
		, m_fallbackTime(0)
		, m_attacker(WWII_NO_SIDE)
		, m_attackStarted(0)
		, m_attackingUnit(nullptr)
		, m_units()
		, m_value(0)
		, m_brigades()
		, m_townstatus(INACTIVE)
	{}

	~CPStruct()
	{
	}

//tolua_begin
	enum { StratClass = CLASS_CP };

	WWII_SIDE		ControllingSide() const { return controllingSide; }
	WWII_COUNTRY	ControllingCountry() const { return controllingCountry; }

	// Accessors
	UINT16 getPlayers(WWII_SIDE forSide=WWII_NO_SIDE, WEAPON_CATEGORY forWeap=_NO_CATEGORY) const
											{ return m_players[forSide][forWeap]; }

	CPStruct* clearAlerts()				{ _changeFlags(flags&~_alertFlags()); return this;}
	CPStruct* clearFlag(UINT32 flag) { _changeFlags(flags&~flag); return this; }

	CPStruct* clearFallback()				{ m_fallback = WWII_NO_SIDE; m_fallbackTime = 0; return clearFlag(CP_STAT_FALLBACK); }
	CPStruct* clearHolding()				{ return clearFlag(CP_STAT_OWN_HAAC); }
	CPStruct* clearObjective(WWII_SIDE forSide);
	CPStruct* clearOverrun()				{ return clearFlag(CP_STAT_OVERRUN); }
	CPStruct* clearOverrunHistory()			{ delete m_overruns; m_overruns = nullptr; return this; }

	CPStruct* endContention();

	bool isAttackObjective(WWII_SIDE forSide) const { return ( (flags & attackFlags[forSide]) != 0 ); }
	bool isBridge() const { return (fullObjID.objType == CP_BRIDGE); }
	bool isCaptureObjective(WWII_SIDE forSide) const { return ( isCity() && isAttackObjective(forSide) ); }
	bool isCity() const { return (fullObjID.objType != CP_BRIDGE); }
	bool isDefendObjective(WWII_SIDE forSide) const { return (flags & defendFlags[forSide]) != 0; }
	bool isFallingBack()					const { return (flags & CP_STAT_FALLBACK) != 0; }
	bool isFallingBack(WWII_SIDE forSide)	const { return ( isFallingBack() && m_fallback == forSide ); }
	bool isHolding()						const { return ( flags & CP_STAT_OWN_HAAC ) != 0; }
	bool isHolding(WWII_SIDE thisSide) 		const { return ( thisSide == side && isHolding() ); }
	bool isObjective()						const { return (flags & (objectiveFlags[WWII_NO_SIDE])) != 0; }
	bool isObjective(WWII_SIDE forSide) const { return (flags & (objectiveFlags[forSide])) != 0; }
	bool isOverrun()						const { return ( flags & CP_STAT_OVERRUN ) != 0; }
	bool isOverrun(WWII_SIDE thisSide)		const { return ( thisSide == side && isOverrun() ); }
	bool hasFactories()						const { return ( flags & CP_STAT_MAKES_RDP ) != 0; }

	CPStruct* setAlert(UINT32 flag) { _changeFlags((flags&~_alertFlags())|flag); return this; }
	CPStruct* setAttackObjective(bool modifyObjectiveList=false, HCUnit* unit=nullptr);
	CPStruct* setDefendObjective(WWII_SIDE forSide, bool modifyObjectiveList=false);
	CPStruct* setFlag(UINT32 flag)	{ _changeFlags(flags|flag); return this; }
	CPStruct* checkForUnderAttack();

	bool isSurrendering() const { return ( flags & CP_STAT_SURRENDER_CANDIDATE ) != 0; }
	CPStruct* setSurrender(bool state) { if ( state ) _changeFlags(flags|CP_STAT_SURRENDER_CANDIDATE); else  _changeFlags(flags&(~CP_STAT_SURRENDER_CANDIDATE)); return this; }

	//// Strat Command based functions

	void drainHistories();
	SCMD_RESULT executeFallback();
	SCMD_RESULT holdAtAllCosts(WWII_SIDE issuer, bool execute);
	SCMD_RESULT	issueFallback(WWII_SIDE issuer, bool testOnly, UINT32 playerID);
	void markOverrunReported();			// Add an overrun event to the history
	SCMD_RESULT overrun(WWII_SIDE issuer);
	SCMD_RESULT requestAttack(bool execute, UINT32 brigadeID=0);
	SCMD_RESULT requestClear(WWII_SIDE forSide, UINT32& data, bool execute);
	SCMD_RESULT requestDefend(WWII_SIDE forSide, bool execute);

	bool placeAttackObjective(const StratHCUnit*);

	//// General functions

	// Add a HCUnit entry
	void addUnit(HCUnit&);

	// Announce the withdrawl of an objective
	void announceObjectiveWithdrawl() const;

	// Tell me who is attacking this town
	WWII_SIDE attacker() const { return m_attacker; }
	HCUnit* attackingUnit() const { return m_attackingUnit; }

	bool changeOwnershipNoCapture(WWII_COUNTRY toCountry, bool force, bool quietly);
	bool changeOwnership(WWII_COUNTRY toCountry, bool force, bool quietly);

	void checkTownStatus(CPStruct *, bool, int);
	
	void checkHCUnits();

	bool enemyReport(const WEAPON_SERIAL& weapon, FacilityStruct* facility);
	void evaluateEWS(bool annotate=false);
	void expireEnemyReports(UINT32 expiry);

//tolua_end
	CP_STATUS_FLAGS flagsFor(WWII_SIDE forSide, const Instance::Descriptor& instance) const;
//tolua_begin

	bool hasABs(WWII_SIDE forSide) const { return armyBasesOwned[forSide] > 0; }
	bool hasContactWithDivision(const StratHCUnit* unit) const;
	bool hasFacilities(UINT32 type, WWII_SIDE forSide) const;
	bool hasHCUnit(UINT32 brigadeID) const;

	bool hasArmorEWS() const { return (flags & (CP_STAT_ENEMY_ARMOR|CP_STAT_HEAVY_ENEMY_ARMOR)) != 0; }
	bool hasInfantryEWS() const { return (flags & (CP_STAT_ENEMY_INFANTRY|CP_STAT_HEAVY_ENEMY_INF)) != 0; }
	bool hasHeavyArmorEWS() const { return (flags & CP_STAT_HEAVY_ENEMY_ARMOR) != 0; }
	bool hasHeavyInfantryEWS() const { return (flags & CP_STAT_HEAVY_ENEMY_INF) != 0; }

	bool isSupplied() const { return m_isRouteingConnected == true || numLinks == 0; }
	bool isSupplyCenter() const { return side == originalSide && m_isSupplyCenter; }
	bool isRouteingHop(WWII_SIDE forSide) const
	{
		return ( ((side == forSide) & (controllingSide == forSide)) | (country == WWII_CHINA) );
	}

	bool isContested() const { return m_contention; }
	bool isEnemyLinked() const { return m_enemyLinked; }
	bool isFrontlineLinked() const { return m_frontlineLinked; }
	bool isFriendlyLinked() const { return m_friendlyLinked; }

    bool islinkedToAnAO();

	LinkStruct* link(UINT32 i) const;
	LinkStruct* linkTo(const FULL_OBJ_ID rhs) const;
	LinkStruct* linkTo(const CPStruct* rhs) const;

	UINT32 minimumObjectiveLifetime(UINT32 currentTime) const;

	UINT32 numBrigades(BRANCH_ENUM forBranch) const;
	UINT32 numUnits() const { return (m_units.empty()) ? 0 : (UINT32)m_units.size(); }

	void processFirebases(bool incoming);

//	UINT32 propagateSupplyLink(WWII_SIDE forSide);

	void removeUnit(const HCUnit&);

	void resetRouteing();

	// Iterate through all of the HCs in this town and restart their
	// resupply queues
	void restartSupplies();

	CPStruct* runCycleEvents(UINT32 runtime);

	void setContention(bool state);

	void setSide(WWII_SIDE);

	void setSupplyCenter();

	void setRouteing(bool isConnected);

	// Purges the supply and unsubscribes the Toe Template header of all Garrisons in this cp.
	void removeAllGarrisonsSupply();

	// Changes the country and the toe Template header for all Garrisons in this cp.
	void changeAllGarrisonCountries(WWII_COUNTRY countryID, bool manualChange);

	void disableAllGarrisons();
	void enableAllGarrisons();

	void setTableStates(bool isNewObjective = false);

	// Is a particular side allowed to spawn here?
	bool spawningEnabledFor(WWII_SIDE forSide) const { return forSide == side; }

	void startObjectiveExpiry(WWII_SIDE forSide, UINT32 currentTime);

	// Check and update the friendly link status
	void updateFriendlyLinkStatus();

//tolua_end
	using LINKS = std::set<LinkStruct*>;

public:
	LINKS				m_links;

//tolua_begin
	// Cached responses for .own messages for various sides;
	std::string 		ownMessage[MAX_SIDES];

	EWSREPORTS			ews;

	WWII_SIDE			m_fallback;			// Side issuing fallback
	UINT32				m_fallbackTime;		// When to execute fallback
	UINT32				m_fallbackBy;		// PlayerID of fallback issuer

	WWII_SIDE			m_attacker;			// Which side is attacking if any
	UINT32				m_attackStarted;	// When the last attack began
	HCUnit*				m_attackingUnit;	// Which brigade is attacking us

	HCUNIT_INDEX		m_units;

	// How much our current brigade occupants make us worth
	UINT32				m_value;

	TOWN_STATUS				m_townstatus;

	//tolua_end
	ACTIVE_BRIGADES		m_brigades;

private:
	//// Helper functions

	// Change the flags and, if this amounts to a change, queue for
	// network propagation of the change
	CP_STATUS_FLAGS _alertFlags() const { return (CP_STATUS_FLAGS)(CP_STAT_OWN_HAAC|CP_STAT_OVERRUN|CP_STAT_FALLBACK); }
	void _changeFlags(UINT32 newFlags);
	SCMD_RESULT _fallback(WWII_SIDE forSide, FALLBACK_MODE mode, UINT32 playerID);

	// Internal part of clearObjective
	void _removeObjective(WWII_SIDE forSide);

public:
//tolua_begin
	HCUIDX_ITERATOR	beginUnit() { return m_units.begin(); }
	HCUIDX_ITERATOR endUnit() { return m_units.end(); }
};
//tolua_end
