#pragma once
// Date: Thu Apr 12 19:30:24 2001
//
// PostedWeapon's are descriptors for vehicles that have
// been assigned a serial number and are either 'in play'
// or spawned in to the game world.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtJoinList.h"
#include "weap/weapSerial.h"
#include <list>

class WeaponEntry;
struct STRAT_UPDATE;
class ResupplyTicket;

//this is a reserved weapon.  An entry in this list is generated whenever
//	someone has a "weapons post".  Spawn requests are granted out of this
//	list
struct POSTED_WEAPON
{
	enum _STATUS
	{
		_FREE, _POSTED, _RESERVED, _SPAWNED, _CANCELLED
	};

	POSTED_WEAPON(const WEAPON_SERIAL& withWeaponSerial, SINT16 withPortal, UINT32 withMissionID
				, UINT32 withCommander, const ChatHandle withHandle, UINT32 withCrew
				, StratHCUnit* withBrigade, StratHCUnit* withSupplyingBrigade, FacilityStruct* withFacility
				, bool withMulticrew, bool withImmediateSpawn, bool withIsNonSupplied
				)
		: m_weaponSerial(withWeaponSerial)
		, m_portal(withPortal)
		, m_missionID(withMissionID)
		, m_commander(withCommander)
		, m_handle(withHandle)
		, m_crew(withCrew)
		, m_brigade(withBrigade)
		, m_supplyingBrigade(withSupplyingBrigade)
		, m_facility(withFacility)
		, m_expiryUpdate(nullptr)
		, m_crewList(JOIN_LIST::_POST_CREW, nullptr, this)
		, m_joinList(JOIN_LIST::_POST_JOIN_REQ, nullptr, this)
		, m_status(_FREE)
		, m_playerExitCode(0)
		, m_postingEntry(nullptr)
		, m_lastEWSFacility(nullptr)
		, m_multicrew(withMulticrew)
		, m_immediateSpawn(withImmediateSpawn)
		, m_isDead(false)
		, m_isNonSupplied(withIsNonSupplied)
		, m_resupplyTicket(nullptr)
		, m_posted(0)
		, m_SpawnedFromMobileSpawn(false)
	{
		_register();
	}

	~POSTED_WEAPON();

	bool multicrew() const { return m_multicrew; }
	bool immediateSpawn() const { return m_immediateSpawn; }
	bool isDead() const { return m_isDead; }
	bool isNonSupplied() const { return m_isNonSupplied; }
	UINT32 serial() const { return m_weaponSerial.serial; }
	WEAPON_SPEC spec() const { return m_weaponSerial.spec; }

	// Posted weapon list management
	void insertInto(POSTED_WEAPON*& list);
	void removeFrom(POSTED_WEAPON*& list);

	// Detach ourselves from the posted weapon lists and cancel
	// our reinforcement
	void cancelPost();

	// Tell the system not to reinforce us.
	void cancelReinforcement();

	// Delete our reinforcement entry so that we don't cancel it
	void detachReinforcement();

	// Detach all crew
	void detachCrewAndJoiners(bool includeCommander);

	// Detach anyone on the join list who isn't crew.
	void detachJoiners(bool announceFails);

	// When a vehicle is reserved but not yet spawned, it goes
	// onto a posted weapon list so that we don't have to pass
	// all the information to the host again.
	void post(WeaponEntry* atEntry, bool isReserve);

	// Attempt to re-reserve this vehicle
	bool reReserve();

	// Attempt to under a weapon reservation and return the
	// vehicle to its spawn lists. Returns true if the weapon
	// was delivered division -> brigade or to a different Garrison.
	bool returnWeapon(bool allowOverstock, bool hasBeenHit, FULL_OBJ_ID originFacilityID = nullObjID);

	// If we don't already have one, schedule a replacement for me
	void scheduleReinforcement();

	// Set the multicrew flag and notify the MM host
	void setMulticrew(bool isMulticrew);

	// Attach ourselves to the spawned weapon lists and schedule
	// our reinforcement
	bool spawn();

	WEAPON_SERIAL	m_weaponSerial;	//Marty's WEAPON_SERIAL,
									//	will expire after a certain time
	SINT16			m_portal;		//the portal from which it'll spring
	UINT32			m_missionID;	//The missionID of the commander

	UINT32			m_commander;	//The commander of this vehicle
	ChatHandle		m_handle;
	UINT32			m_crew;			//The crewmember of this vehicle

	// HQUnit that owns and HQUnit that supplies this weapon
	StratHCUnit*	m_brigade;		//Brigade it was reserved to
	StratHCUnit*	m_supplyingBrigade; // Brigade it was supplied FROM

	FacilityStruct* m_facility;		//The facility to which its attached

	STRAT_UPDATE*	m_expiryUpdate;	//The expiration update for this weapon
	JOIN_LIST		m_crewList;
	JOIN_LIST		m_joinList;
	_STATUS 		m_status;
	// PLAYER_EXIT_CODE when the vehicle is finished with
	UINT32			m_playerExitCode;

	WeaponEntry* 	m_postingEntry;			// Which weapon entry we're posted to spawn at
	FacilityStruct*	m_lastEWSFacility;

	bool			m_multicrew : 1;		// if TRUE we are multicrew waiting for a crew
	bool			m_immediateSpawn : 1;	// If true, the person is spawned immediately
	bool			m_isDead : 1;
	bool			m_isNonSupplied : 1;	// Indicates this vehicle was not drawn
											// from the vehicle supply system, e.g.
											// by a user with ALL_SPAWN, and should
											// not be returned to the vehicle supply
											// system.
	ResupplyTicket*	m_resupplyTicket;		// Pointer to our resupply ticket if we
											// are attached to one.
	UINT32 			m_posted;				//Timestamp at which this was posted.  Posts

	bool            m_SpawnedFromMobileSpawn;	// True if spawned at a mobile spawn.
	FacilityStruct* m_SpawnedFromFacility;		// The actual facility where a player spawns from

private:
	// Allocate me a serial number and post me to the postedWeaponList
	void _register();
	// Release my serial number
	void _unregister();
};

using POSTED_WEAPON_LIST = std::list<POSTED_WEAPON*>;

//Finds someone in the postedWeapon list
extern POSTED_WEAPON* strtFindPostedWeapon(UINT32 serialID);

