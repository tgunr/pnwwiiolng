#pragma once
// Created: Wed Dec  7 21:20:09 CST 2005
//
// Defines the facility struct and CountryWeaponList
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtProtos.h"
#include "mm/mmEnums.h"
#include "mem/mem.h"
#include <map>
#include "weap/weapSpec.h"
#include "teul/teulTime.h"
#include "strt/strtDTL.h"							// For MAX_STRT_NAME_LENGTH

#define MAX_FAC_LINKS 8

class WeaponEntry ;
struct WEAPON ;
struct JOIN_LIST ;
namespace Strat { class CaptureContribution ; }

//tolua_begin

struct CSAFE_FacilityStruct
{
//tolua_end
	CSAFE_FacilityStruct() { memset(this, 0, sizeof(CSAFE_FacilityStruct)) ; }
//tolua_begin

//FULL_OBJ_ID must be the first field in the structure.
//DO NOT MOVE!!!!!!!!!! -Meat
	FULL_OBJ_ID 	fullObjID;
	bool			modified;
	FacilityStruct* next;			//The next Facility in the CP
	CPStruct* 		cp;				//The CP to which the
									//	facility belogs
	SINT8 			subType;
	FULL_OBJ_ID		cpID;
	char			name[MAX_STRT_NAME_LENGTH];
	const char*		filename ;
	WWII_SIDE		side;			//the side that owns the facility
	WWII_COUNTRY	country;		//the country that owns the facility
	WWII_SIDE		originalSide;
	WWII_COUNTRY	originalCountry;
	FACILITY_TYPE	type;
	UINT16          uOID ;			// Unique OID

	ObjectStruct*	objectList;
	ObjectStruct*	m_captureBuilding ;

	UINT32 numObjects;				//The number of objects the facility has
	SINT32 capacity;				//The percent functionality of the facility
	UINT32 rpTransferLimit;			//Max RPs that can be pulled from the Depot
									//	per unit time.
	UINT32 objectRPTotal;	//The number of RPs required to rebuild the facility
							//in its entirety.  This is used by the mission
							//system to guage the relative value of a facility

	//Stuff for cities
	UINT32 rpsPerUnitTime;

	//Stuff for factories
	UINT32 multiplier;

	//Stuff for depots
	UINT32 storedRPs;
	UINT32 shippingThreshold; //Threshold below which a depot won't ship out
	UINT32 requestThreshold; //Threshold below which a depot will request RPs
	UINT32 incomingRPs;		//The number of RPs in transit to the Depot

	LinkStruct* m_outgoingLink ;
	LinkStruct* m_incomingLink ;

	//Shared between shipyards and OMT Depots
	UINT32 threshold;		//The threshold to build to or ship

	//For firebases
	FULL_OBJ_ID linkID;
	LinkStruct* link;	//The link to which we are attached
	FacilityStruct* mate ;		// reciprocal FB or depot.

	JOIN_LIST*	m_joinList;
	CP_STATUS_FLAGS		flags;
	FacilityStruct* typeNext ;	// Next facility of this type
	FacilityStruct* armyBase ;	// The AB to which I belong
	Supercell* supercell ;
	SINT32	m_posX ;			// Absolute X and Y co-ords in meters
	SINT32	m_posY ;

	//Stuff for bases
	bool	open ;			//Whether its open or not.  Applies to firebases
								//	as well
	bool	m_needsSync ;	// Linked list of facilities that need to be broadcast
	bool	m_spawnsInf ;	// Has infantry spawn
	bool 	m_spawnsVehicles ;	// Has vehicles
	bool	m_onRepairQueue ;
	bool	m_spawnable ;
	bool	m_building ;	//Flag for whether its in Depot or its special
								//	mode
	// Number of people at this facility by side; WWII_NO_SIDE used for total
	UINT32	m_players[MAX_SIDES] ;

	UINT32	m_lastDamaged ;
	UINT32	m_repairRate ;

	// What stage of the capture state system the facility is in.
	enum CaptureStage { _CAP_DISABLED, _CAP_PENDING, _CAP_ENABLED } ;
	CaptureStage m_captureStage ;

	// captureCounter value depends on captureStage:
	//  _CAP_DISABLED = nothing.
	//  _CAP_PENDING = epoch time (seconds) when it becomes capturable
	//  _CAP_ENABLED = capture score.
	UINT16	m_captureCounter ;
	FLOAT32	m_captureProgress ;
	FLOAT32	m_captureRate ;

	// Timestamp of last capture state change
	UINT32 m_captureStateChanged ;
} ;

class FacilityStruct : public CSAFE_FacilityStruct
{
public:
	FacilityStruct() : CSAFE_FacilityStruct(), m_weaponEntries() {}
	~FacilityStruct() ;

	typedef std::map<WEAPON_SPEC, WeaponEntry*> WEAPON_ENTRIES ;

public:
	void addWeaponSpawner(const WEAPON& weapon, const ObjectStruct& object) ;

	// Determine what branch I belong to
	BRANCH_ENUM branch() const
	{
		switch (type)
		{
			case FACILITY_AIRBASE: return BRANCH_AIR ;
			case FACILITY_NAVALBASE: return BRANCH_SEA ;
			default: return BRANCH_LAND ;
		}
	}

	bool canFollowSupplyLink() const ;

	const CPStruct& CP() const { return *cp ; }
	CPStruct& CP() { return *cp ; }

	const LinkStruct& Link() const { return *link ; }
	LinkStruct& Link() { return *link ; }

	bool canABSupply(WWII_SIDE forSide) const ;

	// Maximum capacity a facility can have
	SINT32 maxCapacity() const { return 1000 ; }

	bool isOpen() const { return open ; }

	bool isRDPFactory() const ;

	bool isMilitaryFacility() const
		{ return ( type == FACILITY_ARMYBASE || type == FACILITY_NAVALBASE || type == FACILITY_AIRBASE ) ; }

	bool isSpawnSource() const
	{
		return (isMilitaryFacility()) ;
	}

	WeaponEntry* weaponEntry(const WEAPON_SPEC& weapon) const ;

	// Track supply to see which facility is anchoring us to our TOE
	// Depots can specify 'ignoreDepot' if they want to skip ownership and open
	// tests on the first step of the search
	FacilityStruct* supplyingFacility(const HCUnit& forUnit, bool ignoreDepot, bool followLinks=true) const ;

	FacilityStruct* findLocalAB(WWII_SIDE favorSide) const ;
	UINT32 getSupplyingUnit(const HCUnit* brigade, const CPStruct* homeCP) const ;

	void recalculateCapacity() ;

	// Change the country and save if neccessary
	void setCountry(WWII_COUNTRY country) ;

	// Set the open state
	void setOpen(bool isOpen) ;

	// Update all supply links of this facility
	void updateSupplyLinks() ;

	const WEAPON_ENTRIES& weaponEntries() const { return m_weaponEntries ; }

	// Capture Functions
	void setCaptureDisabled() ;
	void setCapturePending() ;
	void setCaptureEnabled() ;
	void setCaptureProgress(FLOAT32 newValue, const Strat::CaptureContribution& claimaint) ;

	// Given CORRECT conditions, can this facility be captured?
	bool canBeCaptured() const { return NULL != m_captureBuilding ; }
	// Given CURRENT conditions, can this facility be captured?
	bool isCapturable() const ;
	bool isCaptureEnabled() const { return m_captureStage >= _CAP_ENABLED ; }
	bool isCapturePending() const { return m_captureStage == _CAP_PENDING ; }

	UINT16 getCaptureRemaining() const ;
	FLOAT32 getCaptureProgress() const { return m_captureProgress ; }

//tolua_end

private:
	WEAPON_ENTRIES	m_weaponEntries ;
	// Lowest level handling of capture stage updates.
#ifdef STRAT_HOST
	void _setCaptureState(CaptureStage toStage, UINT16 toCounter) ;
#else
	void _setCaptureState(CaptureStage toStage, UINT16 toCounter) { m_captureStage = toStage ; m_captureCounter = toCounter ; m_captureStateChanged = teulTime() ; }
#endif
//tolua_begin
} ;

//tolua_end
