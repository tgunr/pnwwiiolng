#pragma once
// Date: Fri Dec  2 15:34:06 CST 2005
//
// Strat Extension of the HCUnit
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiHCUnits.h"
#include "chat/chatBrigadeMoveResults.h"
#include "strt/strtTOETypes.h"
#include <list>

class CPStruct;
struct STRAT_UPDATE;
struct WEAPON_SPEC;
class FacilityStruct;
class LinkStruct;
struct POSTED_WEAPON;
class StratHCUnitImpl;
class ResupplyTicket;

typedef std::list<POSTED_WEAPON*> POSTED_WEAPON_LIST;

//  ----------------------------------------  Variables
//tolua_begin

extern UINT32 brigadeMoveTime_s;
extern UINT32 brigadeFallbackTime_s;
extern UINT32 divisionMoveTime_s;
extern UINT32 divisionFallbackTime_s;
extern UINT32 unitReformTime_s;
extern FLOAT32 pocketMovementDelayFactor;
extern UINT32 pocketTimeToSlowMoves_s;

extern CPStruct* routDestination[MAX_COUNTRIES];

enum
{
	WS_RESERVED						// Weapon reserved
,	WS_ARENA_LOCKED					// Cannot reserve weapons while arena is locked
,	WS_WRONG_SIDE					// Weapon is wrong side for brigade
,	WS_WRONG_CTRY					// Weapon is wrong country for brigade
,	WS_NOT_LISTED					// Brigade does not have this weapon in its TOE
,	WS_NOT_AT_FACILITY				// Facility does not support this type of weapon
,	WS_NO_PERMISSION				// Insufficient rank or privileges
,	WS_OUT_OF_SUPPLY				// No units of this weapon available
};

class StratHCUnit : public HCUnit
{
public:
	StratHCUnit(UINT32 unitID, HCUNIT_TYPE type, UINT32 parentID
				, WWII_COUNTRY countryID, BRANCH_ENUM branchID, BRANCH_ENUM movementType, UCHAR unitClass
				, const ChatHandle handle, const char* name, const char* nick, UINT8 rank
				, const FULL_OBJ_ID homeCP, const ChatHandle owner, RANK_NO ownerRank, UINT32 startCPOID, UINT32 fallbackCpOID
				, UINT32 lastMove, const ChatHandle lastMovedBy, UINT32 nextMove
				, const char* toeTemplate);
	void initialize();
	virtual ~StratHCUnit();

public:
	// Broadcast our current location to other servers.
	void announceHome() const;

	// Check if we can deploy to a specific CP (not a manual move)
	bool canDeployTo(const CPStruct* cp) const { return deployPermission(cp, false) == BMR_APPROVED; }

	// Reset the unit to its original position and spawn lists.
	void reset();

	// Write this stuff to the database
	void save() const;

	void deploy(CPStruct* cp, UINT32 redeployDelay, const char* callsign, bool transferSupply, bool forceDelay=false);
	BRIGADE_MOVE_RESULT deployPermission(const CPStruct* cp, bool manualMove) const;
	const StratHCUnit* division() const { return (isBrigade() && parent() ? parent() : this); }
	StratHCUnit* division() { return (isBrigade() ? parent() : this); }
	bool fallback();
	const CPStruct* findFallback() const;
	UINT32 getSecondsToNextSupply() const;
	bool hasLinkTo(const CPStruct* cp, LinkStruct*& link) const;
	bool hasLinkTo(const StratHCUnit* unit) const;
	UINT32 lastTimeInSupply() const;
	bool isSupplyInTransit() const;
	BRIGADE_MOVE_RESULT move(const char* moveBy, CPStruct* toCP);
	StratHCUnit* parent() const { return (type() == HCU_BRIGADE) ? (StratHCUnit*)m_parent : nullptr; }
//tolua_end
	POSTED_WEAPON_LIST& postedWeaponList() { return m_postedWeaponList; }
	const POSTED_WEAPON_LIST& postedWeaponList() const { return m_postedWeaponList; }
//tolua_begin
	bool rout();
	UINT32 unitsOnMap() const;

	//// TOE Functions

	// Push all pending transfers through
	void deliverTransfers();

	// Deliver a number of vehicles into this TOE
	// Pass replaceTicket if you are planning to re-extract the vehicle on success
	bool depositWeapons(const WEAPON_SPEC spec, UINT16 numUnits, bool allowOverstock, ResupplyTicket* replaceTicket=nullptr);

	const char* describe() const;

	// Enable scripts/tests to force the next supply unit to be consumed
	// Returns false when no supply remains
	bool forciblyProcessSupply();

	// Expensive: Calculate what percentage of capacity a given category is at
	UINT32 getSupplyPctg(WEAPON_CATEGORY) const;

	// Specify a level for a particular vehicle
	bool initializeWeapon(const WEAPON_SPEC spec, UINT16 numUnits);

	bool isSpawnable() const { return ( m_toeTemplateHeader != nullptr && m_toe.empty() == false ); }

	// Delay the scheduling of the next resupply queue by this long
	void pauseResupplyQueue(UINT32 seconds);

	// Process the resupply queue
	void processResupply();

	// Purge the resupply list. If resetSupply is true, also resets all present values to 0
	void purgeResupply(bool resetSupply);

	// Set all capacities back to normal
	void resetSupply();

	// Generate resupply tickets for all absent vehicles
	// If "isTransfer" is true, all vehicles with a capacity will be set
	// to zero and scheduled to forcibly resupply (but not overstock),
	// as when a brigade moves.
	void restartSupply(UINT32 delay, UINT32 window, bool isTransfer);

	// Override the default fallback for this brigade.
	bool setFallbackCP(CPStruct* cp);

	// Withdraw a number of vehicles from this TOE
	bool withdrawWeapons(const WEAPON_SPEC spec, UINT16 numUnits);

//tolua_end
private:
	void setHomeCP(CPStruct* toCP, UINT32 redeployDelay=0, bool forceDelay=false);
//tolua_begin

public:
	const CPStruct* homeCP() const { return m_homeCP; }
	CPStruct* homeCP() { return m_homeCP; }
	const CPStruct* fallbackCP() const { return m_fallbackCP; }
	CPStruct* fallbackCP() { return m_fallbackCP; }
	UINT32 lastMoved() const { return m_lastMoved; }
	ChatHandle lastMovedBy() const { return m_lastMovedBy; }
	const CPStruct* startCP() const { return m_startCP; }
	CPStruct* startCP() { return m_startCP; }
	const StratHCUnit* next() const { return m_next; }
	StratHCUnit* next() { return m_next; }
	STRAT_UPDATE* nextEvent() { return m_nextEvent; }
	UINT32 nextMove() const { return m_nextMove; }
	UINT32 nextMoveIn() const;
	bool offMap() const { return ( !homeCP() || homeCP() == routDestination[country()] ); }
	const TOE_::TemplateHeader* toeTemplateHeader() const { return m_toeTemplateHeader; }
	const TOE_::WeaponInstance* toeWeaponInstance(const WEAPON_SPEC weapon) const;
	TOE_::WeaponInstance* toeWeaponInstance(const WEAPON_SPEC weapon);
	const TOE& toe() const { return m_toe; }

	void checkThrottleWindows(const FacilityStruct* facility) const;
	UINT32 checkThrottleWindow(const FacilityStruct* facility, const WEAPON_SPEC spec) const;
	void incrementThrottle(const POSTED_WEAPON* postedWeapon);
	void decrementThrottle(const POSTED_WEAPON* postedWeapon);

public:
	static StratHCUnit* Lookup(UINT32 unitID);
	static StratHCUnit* Lookup(const char* name);
//tolua_end

	void addTOEInstance(const TOE_::WeaponTemplate& weaponTemplate);

	void CheckResupplyStatus(bool fullCheck);

	bool fetchResupplyQueue(std::vector<std::string>& result, UINT32 limit, bool listAll);

	// Create and queue a resupply ticket for me; never overstocks.
	// Unless isTransfer is true, ticket will not be completed if the unit is
	// in a non-supplied CP when it comes up for processing.
	ResupplyTicket* createResupplyTicket(TOE_::WeaponInstance& entry, bool isTransfer, UINT32 inSeconds, UINT16 units, POSTED_WEAPON* source=nullptr);
	// Cancel a resupply ticket for me
	void cancelResupplyTicket(ResupplyTicket* ticket);

private:
	CPStruct*				m_homeCP;
	CPStruct*				m_startCP;
	CPStruct*				m_fallbackCP;
	UINT32					m_lastMoved;
	UINT32					m_nextMove;
	StratHCUnit*			m_next;
	ChatHandle				m_lastMovedBy;
	UINT32					m_facilityType;
	STRAT_UPDATE*			m_nextEvent;
	POSTED_WEAPON_LIST		m_postedWeaponList;

	// Header of the TOE Template we use (or NULL)
	TOE_::TemplateHeader* 	m_toeTemplateHeader;

	// The actual equipment we have
	TOE						m_toe;

	// Private functions/data we don't need to share
	StratHCUnitImpl&		m_impl;
//tolua_begin
};

extern bool strtDeployDivision(HCUnit* division, CPStruct* cp, bool transferSupply);

extern void strtInitializeHCUnits();

/// @return If calling this function caused the brigade to deploy.
extern bool strtReformBrigade(UINT32 brigadeID, STRAT_UPDATE* fromEvent=nullptr);

//tolua_end
