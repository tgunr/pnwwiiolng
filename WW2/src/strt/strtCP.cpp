#include <syscfg.h> // Path is project specific.
// This module handles all the interal functions necessary to run one
// CP.  This includes running the production and repair, handling damage
// events, and changes of ownership.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#define BOTH_WWII_SIDES_PRODUCE	FALSE

#include "chat/chatBroadcast.h"
#include "chat/chatName.h"
#include "chat/chatSupportRPCMessages.h"
#include "chat/chatTypes.h"
#include "cnvy/cnvyMain.h"
#include "include/arenaLockEnums.h"
#include "miss/missionDamageEvent.h"
#include "strt/strtCaptureAction.h"
#include "strt/strtConstants.h"
#include "strt/strtCP.h"
#include "strt/strtCPStruct.h"
#include "strt/strtData.h"
#include "strt/strtEnemyReport.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtFB.h"
#include "strt/strtHCUnits.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostLua.h"
#include "strt/strtHostMessages.h"
#include "strt/strtIO.h"
#include "strt/strtLinkStruct.h"
#include "strt/strtObjectStruct.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtProtos.h"
#include "strt/strtServerTypes.h"
#include "strt/strtSpawn.h"
#include "strt/strtSupply.h"
#include "strt/strtTypes.h"
#include "teul/teulTime.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"

#ifdef IS_STRAT_HOST
#include "strt/strtHostDB.h"
#endif

struct WWII_SIDE_ACC
{
	UINT32 RPsProduced, depotStorage, depotRPs, RPs;
	FLOAT32 factoryMultiplier;
	SINT32 RPsUsed;
};

#ifdef HEAVY_DEBUG
# define HEAVYLog(...) dbugLog(__VA_ARGS__)
#else
# define HEAVYLog(...)
#endif

//#ifdef BETA_RELEASE
//# define TABLE_REBUILD 30
//#else // ! BETA_RELEASE
//# define TABLE_REBUILD 60
//#endif // BETA_RELEASE

struct CP_ACC
{
	WWII_SIDE_ACC	axis;
	WWII_SIDE_ACC	allied;
};

//forward declaration
//	strat system fuctions
static void _distributeRPs(CPStruct& cp, SINT32 rpsProduced, WWII_SIDE side);
static void _repairCP(CPStruct& cp, CP_ACC& fa);
static SINT32 _runFacility(FacilityStruct& currentFacility, WWII_SIDE_ACC& sa);
static void _repairObject(UCHAR* lastPercentDamage, ObjectStruct& currentObject);
static void _repairBridge(ObjectStruct& bridgeObject);
static bool _objectReadyToRepair(ObjectStruct& object);

//	Control type functions
//This function is called whane one side takes control of a CP they previously
//	did not control.  This function handles all the bookkeeping, including the
//	opening/closing of relevant bases and firebases
//This function is called when a side and country that has ownership but not
//	control of a CP manages to reclaim control (ie, they were pushed back but
//	have rallied to reclaim their army bases).

//This function is called to remove a CP from contention.  It sets the
//	contention flag, opens all the bases and links
static bool _changeLinks(BRIDGE_ENTRY* linkList, bool closed);

using FACTORIES = std::vector<ObjectStruct*>;
static FACTORIES _factories;
FLOAT64 factoryDamageFactor[MAX_COUNTRIES];

using REPAIR_QUEUE = std::map<FacilityStruct*, UINT32>;
static REPAIR_QUEUE _repairQueue;

//////////////////////////////////////////////////////////////////////
// What vis level should an object have if we repair it right now?

void
ObjectStruct::repairVisLevel()
{
	//dbugLog(_FLN, "Performing repairVisLevel");
	CPStruct &cp = Facility().CP();

	setVisLevel(0);
	setSmokeLevel(0);

#ifdef IS_STRAT_HOST
# ifdef BRIDGES
	// If we just repaired a bridge, the ownership needs to change to reflect it
	if ( type == BUILDING_BRIDGE )
	{
		cp.country = (WWII_COUNTRY)BRIDGE_BUILT_COUNTRY;
		Facility().setCountry(cp.country);
		setCountry(Facility().country, Facility().side);
		cp.controllingCountry = cp.country;
		cp.setContention(false);

		strtQueueCPStatus(&cp);
	}
# endif

	strtQueueVisLevelUpdate(this);
#endif //IS_STRAT_HOST
}

//////////////////////////////////////////////////////////////////////
// Set a destroyed vis level for an object

void
ObjectStruct::damagedVisLevel(UINT32 damagedBy)
{
	//dbugLog(_FLN, "Performing damagedVisLevel");
	setVisLevel(VISLEVEL_DAMAGED);
	m_damagedBy = damagedBy;

#ifdef IS_STRAT_HOST
# ifdef BRIDGES
	// If we just repaired a bridge, the ownership needs to change to reflect it
	if ( type == BUILDING_BRIDGE )
	{
#ifdef BETA_RELEASE
		chatSendBroadcastAll(LSTR_BETA, LSTR_OBJECT_DESTROYED, facility->name);
#endif
		CPStruct& cp = *facility->cp;

		cp.country = (WWII_COUNTRY)BRIDGE_DESTROYED_COUNTRY;
		facility->setCountry(cp.country);
		setCountry(facility->country, facility->side);
		cp.controllingCountry = cp.country;
		cp.setContention(true);

		strtQueueCPStatus(&cp);
	}
# endif

	strtQueueVisLevelUpdate(this);
#endif //IS_STRAT_HOST
}

//////////////////////////////////////////////////////////////////////
// Recalculate the "capacity" value of a particular facility

void
FacilityStruct::recalculateCapacity()
{
	capacity = 0;
	for ( ObjectStruct* it = objectList; it != nullptr; it = it->next )
	{
		if ( it->m_maxDamageJoules > 0 )
			capacity += it->m_capacity;
	}
	if ( capacity < 0 )
		capacity = 0;
	else if ( numObjects > 1 )
		capacity = capacity / numObjects;

	if ( capacity > maxCapacity() )
		capacity = maxCapacity();
}

//////////////////////////////////////////////////////////////////////

bool
_isAutoRebuildObject(const ObjectStruct& object)
{
	switch ( object.type )
	{
		case BUILDING_AUTOGUNNER:
		case BUILDING_BRIDGE:
			return true;


		case BUILDING_RESEARCH:
			return (object.isProductionCenter());

		default:
			return false;
	}
}

//////////////////////////////////////////////////////////////////////
//

bool
strtInitBases(CPStruct* cpList)
{
	CPStruct* cp;
	FacilityStruct* facility;

	//dbugLog(_FLN, "Performing strtInitBases");

	if ( cpList == nullptr)
	{
		dbugLog(_FLA, "Trying to initialize bases on a NULL stratSytem");
		return false;
	}

	for ( cp = cpList; cp != nullptr; cp = cp->next )
	{
		if ( cp->isBridge() && cp->isContested() )
		{
			cp->setContention(false);
			strtQueueCPStatus(cp);
		}

		for ( facility = cp->facilityList; facility != nullptr; facility = facility->next )
		{
			switch( facility->type )
			{
				case FACILITY_DEPOT:
				case FACILITY_ARMYBASE:
				case FACILITY_AIRBASE:
				case FACILITY_NAVALBASE:
				case FACILITY_FIREBASE:
					strtSetupSpawn(facility);
					break;
				default:
					break;
			}

			switch(facility->type)
			{
				case FACILITY_DEPOT:
					strtOpenBase(*facility);
				break;

				case FACILITY_NAVALBASE:
				case FACILITY_AIRBASE:
				case FACILITY_ARMYBASE:
					if ( facility->side != cp->controllingSide )
						strtCloseBase(*facility);
					else
						strtOpenBase(*facility);
					break;
				default:
					break;
			}
		}
	}

	return true;
}


//////////////////////////////////////////////////////////////////////
//
// Calculate the repair cost of an object
static inline UINT32
_repairRPCost(const ObjectStruct& object)
{
	FLOAT64 required = (FLOAT64) object.totalRPs * (FLOAT64) object.m_damageJoules / (FLOAT64) object.m_maxDamageJoules;

	if ( required <= 0 )
		return 0;
	else return (UINT32) required;
}

//////////////////////////////////////////////////////////////////////
//
// This function actually runs the internal stuff on a CP
static SINT32
_runFacility(FacilityStruct& currentFacility, WWII_SIDE_ACC& sa)
{
	FLOAT32 capacity;

//	//dbugLog(_FLN, "Performing _runFacility");
 
	if (currentFacility.side == NEUTRAL)
		return 0;

	//Check to see if facility is under contention and check to
	//	make sure we are on the same side.
	if (!(currentFacility.CP().isContested()))
	{
		capacity = (FLOAT32)currentFacility.capacity / (FLOAT32)currentFacility.maxCapacity();
		switch (currentFacility.type)
		{
			//If its a City, do city stuff
			case FACILITY_CITY:
				//Add the RPs to the pool for this CP
				sa.RPsProduced += (UINT32)(currentFacility.rpsPerUnitTime * capacity);
				break;
			//If its a factory, do factory stuff
			case FACILITY_FACTORY:
				sa.factoryMultiplier += (UINT32)(currentFacility.multiplier * capacity);
				break;
			//If its a Depot, do depot stuff
			case FACILITY_OMT:
			case FACILITY_SHIPYARD:
			case FACILITY_DEPOT:
				sa.depotStorage += (UINT32)((currentFacility.requestThreshold - currentFacility.storedRPs) * capacity);
				sa.depotRPs += currentFacility.storedRPs;
				break;
			//case FACILITY_BASE: deprecated, no longer used
			case FACILITY_AIRBASE:
			case FACILITY_NAVALBASE:
			case FACILITY_ARMYBASE:
			case FACILITY_FIREBASE:
			default:
			break;
		}
	}

	if (currentFacility.CP().originalSide != currentFacility.side)
	{
		sa.RPsProduced /= 2;
		sa.factoryMultiplier /= 2;
	}

	//Multiply by factory output
	//sa->RPsProduced*=sa->factoryMultiplier;

	//Run the object damage stuff on all objects for this CP
	sa.RPs = sa.RPsProduced + sa.depotRPs;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Periodically update the health of a firebase

static bool
_repairFirebase(FacilityStruct* fb)
{
	//dbugLog(_FLN, "Performing _repairFirebase");
	if (host_config.fbRepairIntervalM == 0
		|| !fb->isOpen()
		|| fb->capacity == fb->maxCapacity()
		|| fb->m_lastDamaged > teulTime() - (host_config.fbRepairIntervalM*60)
		)
	{
		dbugLog(_FLD, "FB %s repaired, removing from queue", fb->name);
		_repairQueue.erase(fb);
		fb->m_onRepairQueue = false;
		return false;
	}

	ObjectStruct* obj;
	bool processed = false;
	for ( obj = fb->objectList; obj != nullptr; obj = obj->next )
	{
		if ( obj->m_damageJoules <= 0 )
			continue;
		SINT64 repairJoules = obj->m_maxDamageJoules / 100;
		if ( repairJoules > obj->m_damageJoules )
			repairJoules = obj->m_damageJoules;
		if ( repairJoules <= 0 )
			repairJoules = 1;

		dbugLog(_FLD, "Repairing %s for %" PR64BitT "j", obj->name, repairJoules);

		obj->applyDamage(-repairJoules, 0);

		processed = true;
	}

	fb->m_repairRate = (fb->m_repairRate * 80) / 100;
	if ( fb->m_repairRate < 15 )
		fb->m_repairRate = 15;

	// Bump the processing of this fb
	_repairQueue[fb] = teulTime() + fb->m_repairRate;

	return processed;
}

//////////////////////////////////////////////////////////////////////
// Process the firebase repair queue

void
strtProcessFBRepairQueue()
{
	//dbugLog(_FLN, "Performing strtProcessFBRepairQueue");
	REPAIR_QUEUE::iterator it = _repairQueue.begin();
	UINT32 affected = 0;
	while ( it != _repairQueue.end() )
	{
		FacilityStruct* fb = it->first;
		UINT32 processAt = it->second;
		++it;

		if ( processAt < teulTime() && _repairFirebase(fb) )
			++affected;
	}
	if ( affected > 0 )
	{
		dbugLog(_FLI, "repaired %u firebases", affected);
	}
}

//////////////////////////////////////////////////////////////////////
// Put a firebase on the repair queue

static void
_scheduleFirebaseRepairs(FacilityStruct* facility)
{
//	//dbugLog(_FLN, "Performing _scheduleFirebaseRepairs");
	if (host_config.fbRepairIntervalM == 0)
		return;
	if ( facility->type != FACILITY_FIREBASE )
		return;
	if ( facility->capacity >= facility->maxCapacity() || !facility->isOpen() )
		return;
	if ( facility->m_onRepairQueue )
		return;

	const UINT32 REBUILD_OLDER_THAN = teulTime() - (host_config.fbRepairIntervalM*60);
	if ( facility->m_lastDamaged > REBUILD_OLDER_THAN )
		return;

	dbugLog(_FLD, "Adding %s to repair queue", facility->name);

	_repairQueue[facility] = teulTime();
	facility->m_repairRate = host_config.fbRepairIntervalM * 60;
	facility->m_onRepairQueue = true;
}

static void
_repairAI(ObjectStruct& object)
{
	//dbugLog(_FLN, "Performing _repairAI");
	UCHAR lastPercentDamage = 10;
	object.m_damageJoules = 1;
	object.m_repairRPs = object.totalRPs;
	object.calcPercentDamage();
	_repairObject(&lastPercentDamage, object);
}

static void
_repairBridge(ObjectStruct& object)
{
	//dbugLog(_FLN, "Performing _repairBridge");
	UCHAR lastPercentDamage = object.percentDamage;

#ifdef DEBUG
	chatSendBroadcast(Chat::_EVERYONE, Instance::_CAMPAIGN, 0, LSTR_BETA, LSTR_AUTO_REPAIR, object.facility->name);
#endif

	dbugLog(_FLD, "Bridge %s being repaired, %d, %" PR64BitT "/%" PR64BitT "",
			object.facility->name,
			object.visLevel(),
			object.m_damageJoules,
			object.m_maxDamageJoules);

	_repairObject(&lastPercentDamage, object);

	dbugLog(_FLD, "Bridge %s repaired, %d, %" PR64BitT "/%" PR64BitT "",
			object.facility->name,
			object.visLevel(),
			object.m_damageJoules,
			object.m_maxDamageJoules);
}

//////////////////////////////////////////////////////////////////////
//
// This function actually runs the internal stuff on a CP
void
strtRunCP(CPStruct& cp)
{
	//dbugLog(_FLN, "Performing strtRunCP");
	if (!host_config.isArenaOpen())
		return;

	//Get the current time, which is meaningful for time delayed stuff
	//runtime=time(NULL);
	const UINT32 runTimeStamp = teulTime();

	//Fix the nextEvent if its 0
	if (cp.nextEvent == 0)
		cp.nextEvent = runTimeStamp;

	//Set the time for the next event.
	//cp.nextEvent+=cp.timeToProduce;
	cp.nextEvent = cp.nextEvent + (cp.timeToProduce * host_config.rpTime);

	CP_ACC ca;
	FacilityStruct* currentFacility = nullptr;

	if ( !cp.isBridge() )
	{
		if ( cp.side == NEUTRAL )
			return;

		// Update the EWS counters etc.
		cp.runCycleEvents(runTimeStamp);

		memset(&ca,0,sizeof(ca));

		ca.axis.factoryMultiplier = 1;
		ca.allied.factoryMultiplier = 1;

		//If the cp is NOT under contention OR both sides are allowed to produce on a CP
		//Run every Facility
		for ( currentFacility=cp.facilityList; currentFacility!=nullptr; currentFacility=currentFacility->next)
		{
			//We don't run firebases
			if (currentFacility->type != FACILITY_FIREBASE)
			{
				if ( currentFacility->side == AXIS)
					_runFacility(*currentFacility, ca.axis);
				else
					_runFacility(*currentFacility, ca.allied);
			}
			else
				_scheduleFirebaseRepairs(currentFacility);
		}

		ca.axis.RPsProduced = (UINT32)(ca.axis.RPsProduced * ca.axis.factoryMultiplier);
		ca.allied.RPsProduced = (UINT32)(ca.allied.RPsProduced * ca.allied.factoryMultiplier);

		_repairCP(cp, ca);

		//Distribute RPs to the Depots.
		if (BOTH_WWII_SIDES_PRODUCE)
		{
			_distributeRPs(cp,ca.allied.RPsProduced-ca.allied.RPsUsed,ALLIED);
			_distributeRPs(cp,ca.axis.RPsProduced-ca.axis.RPsUsed,AXIS);
		}
		else
		{
			if	(cp.side == ALLIED)
				_distributeRPs(cp,ca.allied.RPsProduced-ca.allied.RPsUsed,ALLIED);
			else
				_distributeRPs(cp,ca.axis.RPsProduced-ca.axis.RPsUsed,AXIS);
		}
	}
	else	// Bridge
	{
		// Drain the histories (to purge objectives, etc)
		cp.drainHistories();

		ObjectStruct& bridge = *cp.facilityList->objectList;
		if (_objectReadyToRepair(bridge))
		{
			dbugLog(_FLD, "signalling bridge for incremental repair .. %s", bridge.name);
			_repairBridge(bridge);
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
void
_requestRPs(FacilityStruct* currentFacility)
{
//	//dbugLog(_FLN, "Performing _requestRPs");
	if (currentFacility == nullptr)
		return;

	LinkStruct* currentLink = currentFacility->m_incomingLink;
	if ( currentLink == nullptr )
		return;

	// Check the RPs present and inbound to see if they are below the watermark (threshold)
	if ( currentFacility->incomingRPs + currentFacility->storedRPs >= currentFacility->requestThreshold )
		return;

	// Check conditions that close a link.
	if ( currentLink->playerValve || currentLink->isEnabled() == false || currentLink->m_convoy )
		return;

	const FacilityStruct* const remoteDepot = currentLink->startDepot;
	if ( remoteDepot == nullptr )
		return;

	// Depot ownership is breaking supply.
	if ( remoteDepot->side != currentFacility->side )
		return;

	// Doesn't have enough RPs stored to ship.
	if ( remoteDepot->storedRPs < remoteDepot->shippingThreshold )
		return;

	const SINT32 wantRPs = (SINT32)currentFacility->requestThreshold - (SINT32)currentFacility->storedRPs;
	if ( wantRPs < 0 )
		return;

	UINT32 requestRPs = std::min((UINT32)wantRPs, std::min((UINT32)currentLink->capacity, (UINT32)remoteDepot->storedRPs));
	if ( requestRPs > 0 )
	{
		currentLink->startDepot->storedRPs -= requestRPs;
		cnvyCreateConvoy(currentFacility->side, currentFacility->country, currentLink, requestRPs);
	}
}


//////////////////////////////////////////////////////////////////////
//
//This function takes an rpsProduced value and distributes it amongst
//	the different depot facilities of the CP.  If its a negative number,
//	it distributes the negative as well.
static void
_distributeRPs(CPStruct& cp, SINT32 rpsProduced, WWII_SIDE side)
{
	//dbugLog(_FLN, "Performing _distributeRPs");
	FacilityStruct *currentFacility;
	SINT32 availableStorage;	//The storage available for any one depot

	//Distribute leftover RPs to the Depot facilities
	for (	currentFacility=cp.facilityList
		;currentFacility!=nullptr
		;currentFacility=currentFacility->next)
	{
		if ( currentFacility->side != side )
			continue;
		if ( currentFacility->type != FACILITY_DEPOT
			&& currentFacility->type != FACILITY_SHIPYARD
			&& currentFacility->type != FACILITY_OMT )
			continue;

		UINT32 storedRPs = currentFacility->storedRPs;

		//If we have RPs to store
		if (rpsProduced > 0)
		{
			//Check to see if we can store any RPs
			availableStorage = currentFacility->requestThreshold - storedRPs;

#ifdef DEBUG_RPS
			dbugLog(_FLD, "We have %d available RPs at %s, and we need to store %d total"
						, availableStorage, currentFacility->name, rpsProduced
						);
#endif

			if ( availableStorage > 0 )
			{
				//Check to see if we can store all the RPs
				if ( availableStorage > rpsProduced )
				{
					storedRPs += rpsProduced;
					rpsProduced = 0;
				}
				else	//We can't store all the RPs
				{
					storedRPs += availableStorage;
					rpsProduced -= availableStorage;
				}
			}
#ifdef DEBUG_RPS
			dbugLog(_FLD,"Depot %s now has %d RPs", currentFacility->name, storedRPs);
#endif
			//Check for the error condition that may cause RP bloat
			if ( storedRPs > currentFacility->requestThreshold )
			{
				dbugLog(_FLW,"Depot %s contained %u RPs was above its limit of %d RPs."
						, currentFacility->name
						, storedRPs
						, currentFacility->requestThreshold);

				if ( currentFacility->requestThreshold > 0 )
					storedRPs = currentFacility->requestThreshold;
				else
					storedRPs = 0;

				if ( storedRPs != currentFacility->storedRPs )
				{
					currentFacility->storedRPs = storedRPs;
#ifdef IS_STRAT_HOST
					strtSaveFacility(currentFacility);
#endif
				}
			}
		}
		else if ( rpsProduced < 0  && currentFacility->storedRPs > 0 )
		{
			//We have RPs to take away
			UINT32 rpDemand = (UINT32)(abs(rpsProduced));		// rpsProduced is NEGATIVE.

			//Check to see if we can take all the RPs
			if ( currentFacility->storedRPs < rpDemand )
				rpDemand = currentFacility->storedRPs;

			currentFacility->storedRPs -= rpDemand;
			rpsProduced += rpDemand;

#ifdef IS_STRAT_HOST
			strtSaveFacility(currentFacility);
#endif
		}

		//This is where we do all the requests from the other depots
		//	we are connected to

		//fprintf(stderr,"Convoy? #%" PR64BitT " stored: %d request: %d\n\n",currentFacility->id
				//, currentFacility->incomingRPs + currentFacility->storedRPs
				//, currentFacility->requestThreshold);

		//If we don't have enough RPs coming in
		if ((currentFacility->incomingRPs + currentFacility->storedRPs)
			< (currentFacility->requestThreshold))
		{
			_requestRPs(currentFacility);
		}
	}
}

//////////////////////////////////////////////////////////////////////
// Check to see if the smoke time on a building has expired and if so decrement
// the smoke state that the building is currently in

static void
_expireSmoke(ObjectStruct& object)
{
	//dbugLog(_FLN, "Performing _expireSmoke");
	if (object.smokeLevel() <= 0)
		return;

	const UINT32 smokeTime = teulTime() - (object.rebuildPeriod * host_config.rpTime / 3);
	if ( object.m_lastSmokeTime < smokeTime )
	{
		// Decrease a smoke level
		object.setSmokeLevel(object.smokeLevel() - 1);
		object.m_lastSmokeTime = teulTime();
	}
}

//////////////////////////////////////////////////////////////////////
//
static void
_repairObject(UCHAR* lastPercentDamage, ObjectStruct& object)
{
	//dbugLog(_FLN, "Performing _repairObject");
	SINT64 repairValue = 0;

	//Rebuild one damage level's worth of RPs
	repairValue = (SINT64)
					((FLOAT64) object.m_maxDamageJoules
					/ (FLOAT64)object.totalRPs
					* (FLOAT64)object.rpsPerDamageLevel
					);

	//Make sure that we are doing some sort of repair, no matter how small it is
	if (repairValue == 0)
	{
#ifdef HEAVY_DEBUG
		dbugLog(_FLD, "_repairObject: 0 joules repair, forcing 1 joule");
		repairValue =1;
#endif // HEAVY_DEBUG
	}

	if ( repairValue >= object.m_damageJoules )
		object.setDamageJoules(0);
	else
		object.setDamageJoules(object.m_damageJoules - repairValue);

	//decrement necessary rps to repair
	if ( object.rpsPerDamageLevel >= object.m_repairRPs )
		object.setRepairRPs(0);
	else
		object.setRepairRPs(object.m_repairRPs - object.rpsPerDamageLevel);

	//Set time of next repair event
	object.addRepairTime(object.rebuildPeriod);

	//Calculate damage percentiles
	*lastPercentDamage = object.percentDamage;
	object.calcPercentDamage();

	dbugLog(_FLI,"%s:repairing:%s %d->%d%%"
					, object.facility->name
					, object.name
					, *lastPercentDamage
					, object.percentDamage
					);

	// bridges are all or nothing repair ..
	if (object.type == BUILDING_BRIDGE || object.type == BUILDING_AUTOGUNNER )
	{
		if (object.m_damageJoules == 0)
		{
			object.repairVisLevel();
			_changeLinks(object.linkList, false);
		}
	}
	else
	{
		if (*lastPercentDamage >= 5 && object.percentDamage < 5)
		{
			object.repairVisLevel();
			object.setSmokeLevel(0);
			_changeLinks(object.linkList, false);
		}
		else
		{
			// Check to see if we need to lower the smoke
			_expireSmoke(object);
		}
	}

	object.recalculateCapacity();
	object.facility->recalculateCapacity();
}

//////////////////////////////////////////////////////////////////////
//
static bool
_objectReadyToRepair(ObjectStruct& object)
{
	//dbugLog(_FLN, "Performing _objectReadyToRepair");
	UINT32 rpsNeededToRepair;

	// Basic conditions which say "no, its not ready to repair"
	if ( object.m_damageJoules == 0 || object.m_repairTime > teulTime() )
		return false;

	// Prevent rebuild of AI on Beta
	if ( object.type == BUILDING_AUTOGUNNER && host_config.arena.rebuildAI == false )
		return false;

	if ( object.m_side == NEUTRAL )
		return false;

	rpsNeededToRepair = _repairRPCost(object);

	// Auto-rebuild objects like AI don't use RPs to rebuild, they just rebuild
	// as soon as they have reached their timer.
	if ( _isAutoRebuildObject(object) )
	{
#ifdef DEBUG
		dbugLog(_FLD, "blessing %s with %d rps to repair %" PR64BitT "/%" PR64BitT "j dmg, totalRPs is %u"
					, object.name
					, rpsNeededToRepair
					, object.m_damageJoules
					, object.m_maxDamageJoules
					, object.totalRPs
					);
#endif
		object.m_repairRPs = rpsNeededToRepair;
		return true;
	}

	//If its damaged and we are ready to repair the object, then
	//	repair it
	if ( object.m_repairRPs >= object.rpsPerDamageLevel || object.m_repairRPs >= rpsNeededToRepair )
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////
//
static void
_repairFacility(FacilityStruct& facility, WWII_SIDE_ACC& sa, UCHAR* lastPercentDamage)
{
	UINT32 rpsNeededToRepair, rpsNeededFromDepot, canget = 0;
	SINT32* rpsUsed;
	ObjectStruct* currentObject;
	UINT32 rpsTransferred;
	SINT32 rpsAvailable, rpsTransAvailable;
	UINT32 const * RPs;
	SINT32	originalCapacity;
	bool needSave = false;

	// rpsTransferred tracks the transfer limit for the facility
	rpsTransferred=0;

	// Repairs drain our capacity; remember the original
	originalCapacity = facility.capacity;

	//Run through all the objects
	for (	currentObject=facility.objectList
		; currentObject!=nullptr
		; currentObject=currentObject->next)
	{
		// No damage, no repairs
		if ( currentObject->m_damageJoules == 0 )
			continue;

		//If its damaged and we are ready to repair the object, then
		//	repair it now already
		if ( _objectReadyToRepair(*currentObject) )
		{
 			_repairObject(lastPercentDamage,  *currentObject);

			needSave = true;

 			continue;
		}

		//Calculate the rps needed to repair the object
		rpsNeededToRepair = _repairRPCost(*currentObject);

		rpsUsed = &sa.RPsUsed;
		RPs = &sa.RPs;
		rpsAvailable = (*RPs) - (*rpsUsed);
		rpsTransAvailable = facility.rpTransferLimit - rpsTransferred;

		if ( rpsAvailable > 0 && rpsTransAvailable > 0 )
		{
			rpsNeededFromDepot = rpsNeededToRepair - currentObject->m_repairRPs;

			//If we need RPs and we aren't under contention, then
			//	get more RPs.  *contention does not effect rebuilding!*
			if ((rpsNeededFromDepot>0) && (!(facility.cp->isContested())))
			{
			//Get more RPs!

				canget = (rpsTransAvailable < rpsAvailable) ? rpsTransAvailable : rpsAvailable;
				canget = (canget > rpsNeededFromDepot) ? rpsNeededFromDepot : canget;

				if (canget > 0)
				{
					currentObject->setRepairRPs(currentObject->m_repairRPs + canget);
					rpsTransferred += canget;
					(*rpsUsed) += canget;
				}
			}
		}

		//If we have enough RPs to repair, then begin repairs
		if ( currentObject->m_repairTime == 0
			&& ( currentObject->m_repairRPs >= currentObject->rpsPerDamageLevel
				|| currentObject->m_repairRPs >= rpsNeededToRepair) )
		{
			currentObject->addRepairTime(currentObject->rebuildPeriod);
		}
	}
	//Back Up The Facility
#ifdef IS_STRAT_HOST
	if ( needSave || facility.capacity != originalCapacity )
		strtSaveFacility(&facility);
#endif
}

//////////////////////////////////////////////////////////////////////
//This function runs the damage repair on every object owned by one
//CP.  Returns the amount of RPs used.

static void
_repairCP(CPStruct& cp, CP_ACC& ca)
{
	//dbugLog(_FLN, "Performing _repairCP");
	UCHAR lastPercentDamage;
	FacilityStruct* currentFacility;

	//Run through every facility
	for ( currentFacility=cp.facilityList; (currentFacility!=nullptr); currentFacility=currentFacility->next)
	{
		// We don't run firebases
		if ( currentFacility->type == FACILITY_FIREBASE )
			continue;

		//Set the pointers to point the variables based on the side
		if (currentFacility->side == ALLIED)
			_repairFacility(*currentFacility, ca.allied, &lastPercentDamage);
		else
			_repairFacility(*currentFacility, ca.axis, &lastPercentDamage);
	}
}

//////////////////////////////////////////////////////////////////////
//This function allows the Strat System to accept a convoy into its
//  depot

void
strtAcceptConvoy(CONVOYSTRUCT& convoy)
{
	//dbugLog(_FLN, "Performing strtAcceptConvoy");
	FacilityStruct *depot = convoy.link->endDepot;
	if ( convoy.numRPs > 0 )
	{
		//Transfer the RPs to the desitination Depot
		UINT32 newRPs = depot->storedRPs + convoy.numRPs;
		if ( newRPs > depot->requestThreshold)
			newRPs = depot->requestThreshold;
		if ( newRPs != depot->storedRPs )
		{
#ifdef IS_STRAT_HOST
			depot->storedRPs = newRPs;
			//save Facility to DB
			strtSaveFacility(convoy.link->endDepot);
#endif
		}
	}

	convoy.link->m_convoy = false;
	convoy.link->startDepot->m_incomingLink->m_convoy = false;

	//Backup the current Facility
	//strtBackUpFacility(convoy->link->endDepot);

	//Free the convoy
	cnvyFreeConvoy(&convoy);
}

//////////////////////////////////////////////////////////////////////
//This function actually damages an object in the strat system

void
ObjectStruct::applyDamage(const SINT64& joules, UINT32 originatingSortieID)
{
	//dbugLog(_FLN, "Performing applyDamage");
	if (setDamageJoules(m_damageJoules + joules) == false)
	{
		dbugLog(_FLI, "%" PR64BitT " joules of damage* not applied* to %s (%s/" PRF_FULL_OBJ_ID ")"
					, joules, name, filename
					, PRA_FULL_OBJ_ID(fullObjID)
					);
		return;
	}

	dbugLog(_FLI,"applying %" PR64BitT " joules of damage to object %s(%s/%d:%d:%d) capacity %d (prev %d) of %d, facilityCapacity %d parent %s"
				, joules
				, name
				, filename
				, fullObjID.cell.ox
				, fullObjID.cell.oy
				, fullObjID.id
				, m_capacity
				, m_lastCapacity
				, m_maxCapacity
				, facility->capacity
				, facility->name
				);

	//Do stuff to calculate capacity here
	UINT32 lastPercentDamage = percentDamage;

	calcPercentDamage();

	//Check the 50% threshold
#ifdef DEBUG
	dbugLog(_FLD,"lastpercent: %d percentDamage: %d", lastPercentDamage, percentDamage);
#endif

	if ( joules < 0 )
	{
#ifdef BRIDGES
		if ( type == BUILDING_BRIDGE )
		{
			// if we're a bridge, and we're destroyed, and our new damage level is 0
			// then flag ourselves to be fixed
			if ( visLevel() != 0 && m_damageJoules <= 0)
			{
				dbugLog(_FLD, "signalling bridge for repair due to repair kit .. %s", name);
				_repairBridge(*this);
			}
		}
		else
#endif // BRIDGES
		if ( type == BUILDING_AUTOGUNNER )
		{
			if ( joules < 0 && m_damageJoules <= 0 )
			{
				dbugLog(_FLD, "%s ai repair complete", name);
				_repairAI(*this);
			}
		}
	}

	if (m_capacity > 0)
	{
		recalculateCapacity();
		facility->recalculateCapacity();

		// In the case where a firebase becomes destroyed
		if ( facility->type == FACILITY_FIREBASE && facility->capacity == 0 )
			strtProcessFirebase(Facility());

#ifdef IS_STRAT_HOST
		strtSaveFacility(facility);
#endif
	}else
	{
		//dbugLog(_FLN,"MEAT: Facility: %s %d:%d:%d, No Change!",object.facility->name,object.facility->fullObjID.cell.ox,object.facility->fullObjID.cell.oy,object.facility->fullObjID.id);
	}

	UINT32 percentToDestroy;
	if (type == BUILDING_BRIDGE)
		percentToDestroy = 100;
	else
		percentToDestroy = 80;

	if ( percentDamage >= percentToDestroy && visLevel() == 0 )
	{
		dbugLog(_FLI, "v:%u destroyed %s (" PRF_FULL_OBJ_ID ")"
						, originatingSortieID, name
						, PRA_FULL_OBJ_ID(fullObjID)
						);
		damagedVisLevel(originatingSortieID);
		setSmokeLevel(SMOKE_START_LEVEL);
	}

	if ( facility->type == FACILITY_FIREBASE
			&& percentDamage < lastPercentDamage
			&& percentDamage <= 75
			&& visLevel() != VISLEVEL_NORMAL )
	{
		repairVisLevel();
	}

#ifdef IS_STRAT_HOST
	if ( joules > SMOKE_JOULES )
	{
		// If they create enough joules to make smoke, bump the timer
		// and if there is no minor smoke, ensure it kicks in again
		// thus a tank shell to a building may re-trigger smoke but
		// it won't cause a raging fire from a "settled" state
		m_lastSmokeTime = teulTime();
		if ( smokeLevel() < SMOKE_LINGER_LEVEL )
			setSmokeLevel(SMOKE_LINGER_LEVEL);
	}
#endif
}

//////////////////////////////////////////////////////////////////////
// Old version: can't damage bridges
#ifndef BRIDGES
static bool
_canApplyJoulesToBridge(WEAPON_SERIAL& weapon, const ObjectStruct* currentObject, SINT64 damageJoules)
{
	return false;
}
#endif

//////////////////////////////////////////////////////////////////////
// New version: Can only damage bridges with an objective
#ifdef BRIDGES

// Helper to tell all crew the damage was refused
static void
_tellCrew(const POSTED_WEAPON* posting
			, LSTRING_ID prefix
			, LSTRING_ID message
			, const CPStruct* bridge
			, const SINT64& joules
			)
{
	char numJoules[32];

	//dbugLog(_FLN, "Performing _tellCrew");

	if (joules < 0)
		snprintf(numJoules, 31, "%" PR64BitT "", -joules);
	else
		snprintf(numJoules, 31, "%" PR64BitT "", joules);

	WeapJoinRequestList::const_iterator it;
	for ( it = posting->m_crewList.begin(); it != posting->m_crewList.end(); ++it )
	{
		const WeapJoinRequestNode* node = *it;
		chatSendLocalizedMessage(node->m_playerID, prefix, message, bridge->name, numJoules);
	}
}

//////////////////////////////////////////////////////////////////////
// Can we apply these joules of damage to an AI

static bool
_canApplyJoulesToAI(const WEAPON_SERIAL& weapon, const ObjectStruct* object, SINT64& joules, const WWII_SIDE weaponSide)
{
	//dbugLog(_FLN, "Performing _canApplyJoulesToAI");
	// No joules, no point.
	if ( joules == 0LL )
		return false;

	const WWII_SIDE facSide = object->facility->side;
	if ( !isValidSide(facSide) || !isValidSide(weaponSide) )
		return false;

	// Positive joules = damage, can only be done to an enemy
	if ( joules > 0LL )
		return (weaponSide != facSide);

	// This must be repair then...
	assert( joules < 0LL );

	// Is this functionality enabled?
	if ( !host_config.arena.manualAIRepair )
		return false;

	// Multiple kits to repair; round the number up so we don't underflow.
	joules = -( (SINT64)ceill((FLOAT64)object->m_maxDamageJoules / (FLOAT64)host_config.arena.manualAIRepair) );

	// In order to repair an AI, it must be friendly and destroyed.
	return ( weaponSide == facSide && object->visLevel() == VISLEVEL_DAMAGED );
}

// Test to determine if the damage can be applied
static bool
_canApplyJoulesToBridge(const WEAPON_SERIAL& weapon, const ObjectStruct* bridge, const SINT64& joules)
{
	//dbugLog(_FLN, "Performing _canApplyJoulesToBridge");
	POSTED_WEAPON *posting;
	WWII_SIDE side;
	CPStruct* cp = bridge->facility->cp;

	// Look up who is doing the damage
	posting = strtFindPostedWeapon(weapon.serial);
	if ( posting == nullptr )
	{
		dbugLog(_FLW, "Discarding damage packets from despawned vehicle: %d"
						, weapon.serial);
		return false;
	}

	// If the crewlist is empty, the vehicle is empty.
	if ( posting->m_crewList.empty() )
	{
		dbugLog(_FLW, "Discarding damage packets from ghost-crewed vehicle: %d"
						, weapon.serial);
		return false;
	}

	side = (WWII_SIDE)weapon.spec.side();

	// Repair attempt
	if ( joules < 0 )
	{
		if ( !cp->isDefendObjective(side) )
		{
			_tellCrew(posting, LSTR_CAUTION, LSTR_BRREP_NO_OBJECTIVE, bridge->facility->cp, joules);
			return false;
		}

#ifdef BETA_RELEASE
		_tellCrew(posting, LSTR_BETA, LSTR_BRREP_ALLOWING, bridge->facility->cp, joules);
#endif
		return true;
	}

	// Damage attempt
	if ( !cp->isAttackObjective(side) )
	{
		// A minimum number of joules required to prevent annoying spam of
		// damage messages (near misses, etc)
		// OGS: Original based on ~ 1/100th of a gp40
		static const SINT64 LOWER_DAMAGE_THRESHOLD = 100000;
		if ( joules > LOWER_DAMAGE_THRESHOLD )
			_tellCrew(posting, LSTR_CAUTION, LSTR_BRDMG_NO_OBJECTIVE, bridge->facility->cp, joules);
		return false;
	}

#ifdef BETA_RELEASE
	_tellCrew(posting, LSTR_BETA, LSTR_BRDMG_ALLOWING, bridge->facility->cp, joules);
#endif

	return true;
}

#endif

//////////////////////////////////////////////////////////////////////
// Determine what stratdb damage effect describes a particular
// joules application and [possible] vislevel change...

static STRAT_DAMAGE_EFFECT
_calcDamageEffect(SINT64 appliedJoules, UCHAR visLevel, UCHAR prevVisLevel)
{
	//dbugLog(_FLN, "Performing _calcDamageEffect");
	if (appliedJoules > 0)
	{
		// Damage
		if ( visLevel == prevVisLevel )
			return SDE_DAMAGED;
		if ( visLevel == VISLEVEL_HIDDEN )	// Completely gone
			return SDE_DEMOLISHED;
		return SDE_DESTROYED;				// Ruined
	}
	else
	{
		// Repair
		if ( visLevel != prevVisLevel && visLevel == VISLEVEL_NORMAL )
			return SDE_REBUILT;
		return SDE_REPAIRED;
	}
}

//////////////////////////////////////////////////////////////////////
// Calculate the STRAT_DAMAGE_TARGET of a target
static STRAT_DAMAGE_TARGET
_getDamageTargetType(const ObjectStruct& object, STRAT_DAMAGE_EFFECT effect)
{
	//dbugLog(_FLN, "Performing _getDamageTargetType");
	switch (object.type)
	{
	case BUILDING_AUTOGUNNER:
		if ( effect == SDE_DESTROYED || effect == SDE_DEMOLISHED )
			return SDT_AUTOGUNNER;
	break;
	case BUILDING_BRIDGE:
	return SDT_BRIDGE;

	case BUILDING_RESEARCH:
		if ( object.isProductionCenter() )
			return SDT_FACTORY;
	break;
	default:
		if ( object.facility->type == FACILITY_FIREBASE )
			return SDT_FIREBASE;
	}

	// Anything else is a strat object
	return SDT_STRAT_OBJECT;
}

//////////////////////////////////////////////////////////////////////
// Determine if a damage event is above the threshold for the object
// that the damage-type is being applied to

//////////////////////////////////////////////////////////////////////
// Record the application of damage against a building

static void
_recordJoulesApplied(const ObjectStruct& object
					, UCHAR prevVisLevel
					, const WEAPON_SERIAL& weapon
					, SINT64 joules
					, SINT64 appliedJoules
					, UINT32 playerID
					)
{
	//dbugLog(_FLN, "Performing _recordJoulesApplied");
	STRAT_DAMAGE_EFFECT effectLevel = _calcDamageEffect(appliedJoules, object.visLevel(), prevVisLevel);
	DBThread::Do(DBThread::Default
		, "INSERT INTO strat_damage"
				"(sortie_id, facility_oid"
				", object_ox, object_oy, object_id, object_type"
				", object_country, object_maxdamage"
				", damage_joules, applied_joules, effect_level"
				")"
			" VALUES"
				"(%u, %u"
				", %d, %d, %u, %u"
				", %u, %" PR64BitUT ""
				", %" PR64BitT ", %" PR64BitT ", %d"
				")"
			, weapon.serial
			, object.facility->uOID
			, object.fullObjID.cell.ox, object.fullObjID.cell.oy, object.fullObjID.id, object.type
			, object.m_country, object.m_maxDamageJoules
			, joules, appliedJoules, effectLevel
	);

	// Notify the mmhost
	ENDPOINT* mapEndpoint = serverPeerEndpointByType(MAP_HOST);
	if ( appliedJoules != 0 && mapEndpoint != nullptr )
	{
		Teul::Packet outgoing(Teul::PacketId(Strat::CallbackX, STRT_MM_DAMAGE_EVENT_Y));
		auto& mde = outgoing.construct<MissionDamageEvent>(playerID, weapon.serial, object.fullObjID.Key(), _getDamageTargetType(object, effectLevel), effectLevel, appliedJoules);

		dbugLog(_FLI, "mapt:%d sending p:%u v:%u tt:%d ef:%d j:%" PR64BitT " mission damage event"
						, mapEndpoint->teulIndex(), playerID, weapon.serial
						, mde.m_targetType
						, mde.m_effectType
						, mde.m_joules);

		const SINT32 sendError = outgoing.send(*mapEndpoint);
		if ( sendError < 0 )
			dbugLog(_FLW, "mt:%d mde send failed: %d", mapEndpoint->teulIndex(), sendError);
	}
}

//////////////////////////////////////////////////////////////////////
// Check the amount of damage someone is trying to do to a building
// and restrict it based on various rules

static SINT64
_checkDamage(WWII_SIDE vehSide, ObjectStruct* object, const WEAPON_SERIAL& weapon, const SINT64& damageJoules)
{
	//dbugLog(_FLN, "Performing _checkDamage");
	CPStruct *cp = object->facility->cp;
	if ( object->type == BUILDING_GENERIC && cp->isCity() && object->facility->side == vehSide )
	{
		if ( !cp->isEnemyLinked() && !cp->isObjective() )
			return 0LL;

		// But in the enemy's town you blast the crap outta things
		return damageJoules;
	}

	if ( vehSide == object->facility->side )
		return 0LL;

	if ( object->facility->type == FACILITY_FIREBASE && weapon.spec.wCategory != _INFANTRY )
	{
		dbugLog(_FLI, "ignoring %" PR64BitUT " non-infantry damage joules to half-damaged firebase", damageJoules);
		return 0LL;
	}

	return damageJoules;
}

//////////////////////////////////////////////////////////////////////
//
// Apply a given number of joules of damage to a building/strat object
// Primarily used as a receptor for damage packets forwarded from the
// stratUpdater

void
strtDamage(const FULL_OBJ_ID fullObjID, SINT64 damageJoules, const WEAPON_SERIAL& weapon, UINT32 playerID)
{
	ObjectStruct*			object = nullptr;
	UCHAR					prevVisLevel;
	SINT64					prevDamageJoules;
	SINT64					appliedJoules;

	//dbugLog(_FLN, "Performing strtDamage");
	// We should never receive zero joules
	if ( damageJoules == 0 )
	{
		dbugLog(_FLW, "received 0 joules of damage?");
		return;
	}

	//Find the object
	object = strtGetObject(fullObjID);
	if (object == nullptr)
	{
		dbugLog(_FLD, "couldn't find damage object %d:%d:%d",fullObjID.cell.ox, fullObjID.cell.oy,fullObjID.id);
		return;
	}
	if ( !object->facility )
	{
		dbugLog(_FLN, "received %" PR64BitT "j joules to non-linked building %d:%d:%u", damageJoules, fullObjID.cell.ox, fullObjID.cell.oy, fullObjID.id);
		return;
	}

	// Peculiar states that say "you can't hurt me"
	if ( object->m_country == WWII_CHINA )	// Russia is neutral-hostile
		return;

	WWII_SIDE weaponSide = (WWII_SIDE)weapon.spec.side();
	switch ( object->type )
	{
		case BUILDING_BRIDGE:
			if ( !_canApplyJoulesToBridge(weapon, object, damageJoules) )
				return;
		break;
		case BUILDING_AUTOGUNNER:
			if ( !_canApplyJoulesToAI(weapon, object, damageJoules, weaponSide) )
				return;
		break;
		default:
		break;
	}

	bool recordable = true;

	if ( object->type != BUILDING_BRIDGE && damageJoules > 0LL )
	{
		damageJoules = _checkDamage(weaponSide, object, weapon, damageJoules);
		if ( damageJoules == 0LL )
			return;

		if ( object->type == BUILDING_GENERIC && object->facility->side == weaponSide )
			recordable = false;
	}

	dbugLog(_FLI, "%u doing %" PR64BitT "j+%" PR64BitT "j/%" PR64BitT "j of damage to %s"
					, weapon.serial
					, object->m_damageJoules
					, damageJoules
					, object->m_maxDamageJoules
					, object->name
					);

	prevVisLevel = object->visLevel();
	prevDamageJoules = object->m_damageJoules;

	object->applyDamage(damageJoules, weapon.serial);

	appliedJoules = object->m_damageJoules - prevDamageJoules;
	// if we applied no damage, don't record this impact
	if (appliedJoules == 0)
		return;

	if ( appliedJoules > 0 )
		object->facility->m_lastDamaged = teulTime();

	if ( recordable )
		_recordJoulesApplied(*object, prevVisLevel, weapon, damageJoules, appliedJoules, playerID);
}

//////////////////////////////////////////////////////////////////////
//This function processes a firebase

bool
strtProcessFirebase(FacilityStruct& firebase)
{
	//dbugLog(_FLN, "Performing strtProcessFirebase");
	if (firebase.link == nullptr)
		return false;

	dbugLog(_FLN,"Firebase %s completely destroyed", firebase.name);

	//Call close firebase
	strtCloseFirebase(firebase);
	strtOpenFirebase(*(firebase.link->endDepot->m_outgoingLink->firebase));

	return true;
}

//////////////////////////////////////////////////////////////////////
// Restart the supplies of any StratHCUnits present

void
CPStruct::restartSupplies()
{
	//dbugLog(_FLN, "Performing restartSupplies");
	if (routDestination[country] == this || isSupplied() == false)
		return;

	HCUNIT_INDEX::iterator it;
	for ( it = m_units.begin(); it != m_units.end(); ++it )
	{
		if ( it->second->isDivision() == false && it->second->isBrigade() == false && it->second->isGarrison() == false )
			continue;
		StratHCUnit* unit = (StratHCUnit*)it->second;
			unit->restartSupply(host_config.supply.trickleStart_m * 60, host_config.supply.trickleTime_m * 60, false);
		}
	}

//////////////////////////////////////////////////////////////////////
//
// Capture a facility by reclaiming or taking it.

bool
strtCaptureFacility(FacilityStruct& facility, WWII_COUNTRY taker, WWII_COUNTRY holder, bool force, bool quietly)
{
	//dbugLog(_FLN, "Performing strtCaptureFacility");
	// Firebases are destroyed, not captured.
	if ( facility.type == FACILITY_FIREBASE )
	{
		dbugLog(_FLW, "attempting to change ownership of firebase via capture facility???");
		return false;
	}

	CPStruct& cp = *facility.cp;
	WWII_SIDE takerSide = wwiiSide(taker);

	// Avoid confusion by making RDP factories always take their original ownership
	if ( facility.isRDPFactory() && takerSide == facility.originalSide )
		taker = facility.originalCountry;

	dbugLog(_FLI, "%s(%s, %d, %d, %d, %d)", __FUNCTION__, facility.name, taker, holder, force, quietly);

	// Disallow capture without an objective, unless
	// the caller is really sure they this doing, and
	// sends us a "force == true"
	if ( !force && !(cp.isObjective(takerSide) || (taker == cp.country && cp.isContested())) )
	{
		return false;
	}

	// Execute the capture action
	CaptureAction capture(facility, taker, quietly);
	capture.Execute();

	// We can look at capture for the results
	dbugLog(_FLI, "capture(%s, %u, %u) : ec:%u lc:%u sc:%u so:%u rec:%u"
					, facility.name, taker, quietly
					, capture.m_enteredContention
					, capture.m_leftContention
					, capture.m_setControl
					, capture.m_setOwner
					, capture.m_reclamation
					);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Convert a facility only if it needs to be
// Note: Quietly is assumed 'true' when forcing a facility to change to its
// existing owner. This is to avoid confusing chat spam.

bool
strtConvertFacility(FacilityStruct& facility, WWII_COUNTRY toCountry, bool force, bool quietly)
{
	dbugLog(_FLI, "converting %s from %d->%d [%s][%s]"
					, facility.name
					, facility.country
					, toCountry
					, ((force) ? "force":"norm" )
					, ((quietly) ? "quiet":"norm" )
					);

	if ( facility.country == toCountry && force )
	{
		// Quietly force conversion
		dbugLog(_FLI, "conversion needs to be forced, so it will be quiet");
		strtCaptureFacility(facility, toCountry, facility.country, true, true);
		return true;
	}
	else if ( facility.country != toCountry )
	{
		return strtCaptureFacility(facility, toCountry, facility.country, force, quietly);
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
//

static inline void
_checkEnemyLinked(CPStruct& cp)
{
	//dbugLog(_FLN, "Performing _checkEnemyLinked");
	bool enemyLinked = false;

	if ( cp.country != WWII_CHINA )	// Russia is neutral-hostile
	{
		const FacilityStruct* facility;
		for ( facility = cp.facilityTypeIndex[FACILITY_DEPOT]; facility != nullptr; facility = facility->typeNext )
		{
			if ( facility->m_outgoingLink == nullptr )
				continue;

			const CPStruct* const endCP = facility->m_outgoingLink->endDepot->cp;
			if ( endCP->controllingSide == cp.controllingSide )
				continue;

			if ( endCP->country == WWII_CHINA )	// Russia is neutral-hostile
				continue;

			dbugLog(_FLI, "cp %s side %u country %u is linked to enemy cp %s side %u country %u"
					, cp.name, cp.controllingSide, cp.country
					, endCP->name,endCP->controllingSide, endCP->country);

			enemyLinked = true;
			break;
		}
	}

	if ( enemyLinked != cp.isEnemyLinked() )
	{
		if ( cp.isEnemyLinked() )
			dbugLog(_FLI,"cp %s was enemy Linked", cp.name);
		else
			dbugLog(_FLI,"cp %s was NOT enemy Linked", cp.name);

		strtEnemyLink(cp, enemyLinked);

		if ( cp.isEnemyLinked() )
			dbugLog(_FLI,"cp %s is enemy Linked", cp.name);
		else
			dbugLog(_FLI,"cp %s is NOT enemy Linked", cp.name);

#ifdef IS_STRAT_HOST
		cp.resetRouteing();

		// Tell everyone
		strtQueueCPStatus(&cp);

		//save changes to DB
		strtSaveCP(&cp);
#endif
	}
}


//////////////////////////////////////////////////////////////////////
//
static void
_updateSupplyAndLinkStatus(CPStruct& cp)
{
	//dbugLog(_FLN, "Performing _updateSupplyAndLinkStatus");
	cp.updateFriendlyLinkStatus();
	_checkEnemyLinked(cp);
	strtQueueCPStatus(&cp);
}

void
FacilityStruct::updateSupplyLinks()
{
	//dbugLog(_FLN, "Performing updateSupplyLinks");
	if (type != FACILITY_DEPOT || m_outgoingLink == nullptr || m_incomingLink == nullptr)
		return;
	assert( cp != nullptr );
	assert( m_outgoingLink->hasFirebase() == m_incomingLink->hasFirebase() );

	FacilityStruct& endDepot = *m_outgoingLink->endDepot;

	assert( m_outgoingLink == endDepot.m_incomingLink );
	assert( m_incomingLink == endDepot.m_outgoingLink );

	//Open links that belong to the side
	if ( endDepot.cp->controllingSide == cp->controllingSide )
	{
		if( subType != DEPOT_TRAIN )
		{
			dbugLog(_FLI, "CP++");

			// make sure the link to us is open and was closed (we dont increment for already open links)
			if ( m_incomingLink->isEnabled() == false )
			{
				dbugLog(_FLI, "c%d ", cp->numOpenLinks);
				cp->numOpenLinks++;
			}

			// make sure the link to them is open and was closed (we dont increment for already open links)
			if ( m_outgoingLink->isEnabled() == false )
			{
				dbugLog(_FLI, "e%d ", endDepot.cp->numOpenLinks);
				endDepot.cp->numOpenLinks++;

				_updateSupplyAndLinkStatus(endDepot.CP());
			}

			dbugLog(_FLI,"%s now has  %d open links and other link CP %s has %d open links state %i:%i"
				, cp->name, cp->numOpenLinks
				, endDepot.cp->name
				, endDepot.cp->numOpenLinks
				, m_incomingLink->isEnabled()
				, m_outgoingLink->isEnabled());
		}

//		endDepot.m_incomingLink->setEnabled(true);
//		endDepot.m_outgoingLink->setEnabled(true);

		m_outgoingLink->setEnabled(true);
		m_incomingLink->setEnabled(true);
	}
	else
	{
		if ( country != WWII_CHINA && subType != DEPOT_TRAIN )	// Russia is neutral-hostile
		{
			dbugLog(_FLI, "CP--");

			strtEnemyLink(*cp, true);
			// make sure the link to them was open  (we dont decrement for already closed links)
			if( m_incomingLink->isEnabled() )
			{
				dbugLog(_FLI, "c%d ", cp->numOpenLinks);
				cp->numOpenLinks--;
			}

			// make sure the link to them was open  (we dont decrement for already closed links)
			if( endDepot.m_incomingLink->isEnabled() )
			{
				dbugLog(_FLI, "e%d ", endDepot.cp->numOpenLinks);
				endDepot.cp->numOpenLinks--;

				_updateSupplyAndLinkStatus(endDepot.CP());
			}

			dbugLog(_FLI,"%s now has  %d open links and other link CP %s has %d open links state %i:%i"
				, cp->name, cp->numOpenLinks
				, endDepot.cp->name
				, endDepot.cp->numOpenLinks
				, m_incomingLink->isEnabled()
				, endDepot.m_incomingLink->isEnabled());
		}

		//Close all links so convoys can't travel them
//		endDepot.m_incomingLink->setEnabled(false);
//		endDepot.m_outgoingLink->setEnabled(false);

		m_outgoingLink->setEnabled(false);
		m_incomingLink->setEnabled(false);
	}

	if ( m_outgoingLink->isEnabled() != endDepot.m_incomingLink->isEnabled()
		|| m_incomingLink->isEnabled() != endDepot.m_outgoingLink->isEnabled() )
	{
		LogAbort("isEnabled()s don't match");
	}
}

//////////////////////////////////////////////////////////////////////
//
void
strtResetMonkeyHead(ObjectStruct& object, UINT32 visLevel)
{
	//dbugLog(_FLN, "Performing strtResetMonkeyHead");
	object.setVisLevel(visLevel);
	object.setDamageJoules(0);
	object.m_repairTime = 0;
	object.setCapacity(object.m_maxCapacity);
	object.percentDamage = 0;

	object.facility->recalculateCapacity();

#ifdef IS_STRAT_HOST
	strtQueueVisLevelUpdate(&object);
	strtQueueFacilityStatus(object.facility);
#endif
}

//////////////////////////////////////////////////////////////////////
//
void
strtCloseBase(FacilityStruct& facility)
{
	//dbugLog(_FLN, "Performing strtCloseBase");
	dbugLog(_FLD, "Closing facility %s " PRF_FULL_OBJ_ID ""
            , facility.name
            , PRA_FULL_OBJ_ID(facility.fullObjID)
            );

	//Set the flag on the facility indicating its closed
	facility.setOpen(false);

#ifdef IS_STRAT_HOST
	//save Facility to DB
	strtSaveFacility(&facility);
	strtQueueFacilityStatus(&facility);
#endif
}

//////////////////////////////////////////////////////////////////////
//

void
strtOpenBase(FacilityStruct& facility)
{
	//dbugLog(_FLN, "Performing strtOpenBase");
	dbugLog(_FLD, "Opening facility %s " PRF_FULL_OBJ_ID "", facility.name, PRA_FULL_OBJ_ID(facility.fullObjID));

	//Set the flag on the facility indicating its open
	facility.setOpen(true);

#ifdef IS_STRAT_HOST
	//save Facility to DB
	strtSaveFacility(&facility);
	strtQueueFacilityStatus(&facility);
#endif
}

//////////////////////////////////////////////////////////////////////
//
//Rip through a list of bridge entries and change all the links
static bool _changeLinks(BRIDGE_ENTRY* linkList, bool closed)
{
	//dbugLog(_FLN, "Performing _changeLinks");
	if (linkList == nullptr)
		return false;

	BRIDGE_ENTRY* current = linkList;
	while (current != nullptr)
	{
		linkList->link->setEnabled(!closed);
#ifdef DEBUG
		if (closed == TRUE)
		{
			dbugLog(_FLD,"Bridge out!  Closing link %s", linkList->link->name);
		}
		else
		{
			dbugLog(_FLD,"Bridge repaired!  Opening link %s", linkList->link->name);
		}
#endif
		linkList=linkList->next;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// Capture a facility and log the change and effect.

void
strtCaptureAndLog(FacilityStruct* facility, const ChatHandle handle, UINT32 customerID, UINT32 missionID, const HCUnit* unit, UINT32 seconds)
{
	//dbugLog(_FLN, "Performing strtCaptureAndLog");
	if (unit == nullptr)
	{
		dbugLog(_FLW, "f:%s:cap cannot capture facilities with a null unit", facility->name);
		return;
	}

	// Is this a save?
	if ( facility->side == unit->side() )
	{
		dbugLog(_FLD, "f:%s:cap saved by %s", facility->name, Chat::Name(handle).c_str());
		strtCaptureLog(facility, CAP_SAVED, facility->country, handle, customerID, missionID, unit, teulTime(), seconds, false, false, false);

		if ( facility->cp->isObjective() )
			facility->setCapturePending();
		else
			facility->setCaptureDisabled();

		return;
	}

	bool wasContested = facility->cp->isContested();
	WWII_SIDE oldOwner = facility->cp->side;
	WWII_SIDE oldControl = facility->cp->controllingSide;
	WWII_COUNTRY oldCountry = facility->country;

	// They appear to be good for capture
	strtCaptureFacility(*facility, unit->country(), oldCountry, true, false);

	// Determine if they changed things
	bool hasTakenControl = (oldControl != unit->side() && facility->cp->controllingSide == unit->side());
	bool hasTakenOwnership = (oldOwner != unit->side() && facility->cp->side == unit->side());

	strtCaptureLog(facility, CAP_CAPTURED, oldCountry, handle, customerID, missionID
					, unit, teulTime(), seconds, wasContested, hasTakenControl, hasTakenOwnership);
}

//////////////////////////////////////////////////////////////////////
// Check to see if the facility contains the specified building

/*
static bool
_fromInsideFacility(const FacilityStruct* facility, const FULL_OBJ_ID buildingID, UINT32 customerID, const ChatHandle handle)
{
	ObjectStruct* object = strtGetObject(buildingID);
	if ( object == NULL || object->facility != facility )
	{
		dbugLog(_FLA, "p:%u c:%s fac:%s attempt to capture from invalid building " PRF_FULL_OBJ_ID ""
						, customerID, Chat::Name(handle).c_str()
						, facility->name, PRA_FULL_OBJ_ID(buildingID));
		return false;
	}

	return true;
}
*/

void
strtForceRebuildCP(CPStruct& cp)
{
	//dbugLog(_FLN, "Performing strtForceRebuildCP");
	for (FacilityStruct *facil = cp.facilityList; facil != nullptr; facil = facil->next)
	{
		// Don't rebuild closed FBs
		if ( facil->type == FACILITY_FIREBASE && facil->isOpen() == false )
			continue;

		// Bump the capture progression state.
		facil->setCaptureEnabled();

		for ( ObjectStruct* obj = facil->objectList; obj != nullptr; obj = obj->next )
		{
			if ( obj->m_damageJoules <= 0 && obj->m_repairTime == 0 )
			{
				if ( obj->type == BUILDING_AUTOGUNNER )
				{
					dbugLog(_FLI, " NOT rebuilding %d,%d.%u %s"
									, obj->fullObjID.cell.ox, obj->fullObjID.cell.oy, obj->fullObjID.id, obj->name);
				}
				continue;
			}

			dbugLog(_FLI, "*AM* rebuilding %d,%d.%u %s"
							, obj->fullObjID.cell.ox, obj->fullObjID.cell.oy, obj->fullObjID.id, obj->name);

			obj->m_damageJoules = 1;
			obj->m_repairRPs = obj->totalRPs;
			UCHAR ldp = 100;

			_repairObject(&ldp, *obj);
		}
	}
}

void
strtDestroyObject(ObjectStruct& object)
{
	//dbugLog(_FLN, "Performing strtDestroyObject");
	object.applyDamage(object.m_maxDamageJoules, 0);
}

//////////////////////////////////////////////////////////////////////
// Register the existence of an rdp-producing factory;
//  - schedule it for periodic rdp output
//  - add it to the list of known factories for the owning country

void
strtAddFactory(ObjectStruct* object)
{
	//dbugLog(_FLN, "Performing strtAddFactory");
	dbugLog(_FLI, "Object %s(" PRF_FULL_OBJ_ID ") is production center, starting %ds cycle"
					, object->name
					, PRA_FULL_OBJ_ID(object->fullObjID)
					, host_config.factoryPeriod);

	// Schedule the first output event
	UINT32 firstEvent = host_config.factoryPeriod
					  + (host_config.factoryWarmupTimeM * 60);
	object->facility->cp->flags |= CP_STAT_MAKES_RDP;
	cnvyBeginFactoryProduction(object, firstEvent);

	// Add to the list of factories belonging to the original owner
	_factories.push_back(object);
}

//////////////////////////////////////////////////////////////////////
// Periodically work out how much output the factories are
// currently generating.

void
strtCalculateRDPOutput()
{
	//dbugLog(_FLN, "Performing strtCalculateRDPOutput");
	UINT32 factoryCount[MAX_COUNTRIES];
	memset(factoryCount, 0, sizeof(factoryCount));
	memset(factoryDamageFactor, 0, sizeof(factoryDamageFactor));

	for ( UINT32 i = 0; i < _factories.size(); ++i )
	{
		ObjectStruct* factory = _factories[i];
		factoryDamageFactor[factory->facility->originalCountry] += factory->factoryDamageFactor();
		++factoryCount[factory->facility->originalCountry];
	}

	for ( UINT32 i = 0; i < MAX_COUNTRIES; ++i )
	{
		if ( factoryCount[i] == 0 )
			factoryDamageFactor[i] = 1.01;
		else
			factoryDamageFactor[i] /= factoryCount[i];
	}
}

//////////////////////////////////////////////////////////////////////
// Check if there are any valid links for falling back

// If you want to allow fallback to cross links that we do not own
// the outgoing depot for the link
#define ALLOW_FALLBACK_FROM_CONTESTED_LINK

static UINT32
_validFallbackCandidates(const CPStruct* cp, WWII_SIDE forSide)
{
	CPStruct::LINKS::const_iterator it;
	UINT32 validCandidates = 0;
	//dbugLog(_FLN, "Performing _validFallbackCandidates");
	for (it = cp->m_links.begin(); it != cp->m_links.end(); ++it)
	{
		const LinkStruct& link = *(*it);
		// Things that block our egress
		if  (
#ifndef ALLOW_FALLBACK_FROM_CONTESTED_LINK
			link.startDepot->side == forSide
			&& link.startDepot->isOpen() == true
			&&
#endif
			   link.hasEnemyFirebase(forSide) == false
			&& link.endDepot->side == forSide
			&& link.endDepot->isOpen() == true
			&& link.endDepot->armyBase->side == forSide
			&& link.endDepot->armyBase->isOpen() == true
			)
			++validCandidates;
	}

	return validCandidates;
}

//////////////////////////////////////////////////////////////////////
// Check if there are any brigades that cannot fallback

static bool
_allBrigadesCanFallback(const CPStruct* cp, UINT32 playerID)
{
	bool allValid = true;
	HCUNIT_INDEX::const_iterator it;
	const StratHCUnit* unit;
	//dbugLog(_FLN, "Performing _allBrigadesCanFallback");
	for (it = cp->m_units.begin(); it != cp->m_units.end(); ++it)
	{
		unit = (StratHCUnit*)it->second;
		if ( unit->findFallback() == nullptr )
		{
			dbugLog(_FLI, "hcu:%u @ %s could not find fallback destination for fallback"
							, unit->unitID()
							, cp->name
							);
			chatSendLocalizedMessage(playerID, LSTR_CAUTION, LSTR_NO_FALLBACK_DEST, unit->name());
			allValid = false;
		}
	}

	return allValid;
}

//////////////////////////////////////////////////////////////////////
// Process a fallback query/executation order

#define FALLBACK_ASSERT(_result_, _x_) if ( (_x_) ) { dbugLog(_FLI, #_result_ ": " #_x_); return (_result_); }

UINT32
strtFallback(CPStruct* cp, WWII_SIDE forSide, FALLBACK_MODE mode, UINT32 playerID)
{
	//dbugLog(_FLN, "Performing strtFallback");
	dbugLog(_FLI, "p:%u %s %s fallback @ %s"
					, playerID, (mode == FALLBACK_TEST ? "Test" : "Execute"),  _sideNameAdj[forSide], cp->name);

	// You have to own to fallback.
	FALLBACK_ASSERT( SCMD_WRONG_SIDE, cp->side != forSide );

	// You can't fallback if a HAAC order is in place.
	FALLBACK_ASSERT( SCMD_OTHER_ORDERS, cp->isHolding() );

	// Check for redundant order (multiple people issuing the command).
	FALLBACK_ASSERT( SCMD_REPEAT,  mode == FALLBACK_TEST && cp->isFallingBack() );

	// Eliminate neutral sides from performing fallbacks
	FALLBACK_ASSERT( SCMD_NO_ABS, forSide == WWII_NO_SIDE || forSide == NEUTRAL );

	// Must be a defense objective for your side
	FALLBACK_ASSERT ( SCMD_NO_OBJECTIVE, cp->isDefendObjective(forSide) == false )

	// Players must have issued a /overrun report.
	FALLBACK_ASSERT( SCMD_NOT_OVERRUN, mode == FALLBACK_TEST && cp->isOverrun() == false );

	// Must have a link to a friendly town.
	FALLBACK_ASSERT( SCMD_NO_LINKS, cp->m_friendlyLinked == false );

	// The town /must/ be contested at the time the order is issued.
	FALLBACK_ASSERT( SCMD_NO_CONTENTION, mode != FALLBACK_EXECUTE && cp->isContested() == false );

	// There must be at least one valid fallback candidate.
	FALLBACK_ASSERT( SCMD_NO_LINKS, _validFallbackCandidates(cp, forSide) == 0 );

	// All present units /must/ be able to fallback to allow a manual fallback to be issued.
	FALLBACK_ASSERT( SCMD_STUCK_UNITS, _allBrigadesCanFallback(cp, playerID) == false );

	// Must be an attacking unit listed so we can decide which country to award ownership.
	///CAVEAT: This wouldn't be true if we required contention at execution.
	FALLBACK_ASSERT( SCMD_ERROR, cp->attackingUnit() == nullptr );

	// If we're only testing, then permission granted.
	FALLBACK_ASSERT( SCMD_ALLOWED, mode != FALLBACK_EXECUTE );

	FacilityStruct* facility;
	for ( facility = cp->facilityList; facility != nullptr; facility = facility->next )
	{
		if ( facility->side == forSide && facility->isMilitaryFacility() )
		{
			static const ChatHandle systemHandleID = chatGetHandle("SYSTEM");
			strtCaptureAndLog(facility, systemHandleID, 0, 0, cp->attackingUnit(), 0);
			facility->setCaptureEnabled();
		}
	}

	return SCMD_SUCCESS;
}

//////////////////////////////////////////////////////////////////////
// Return the number of units of a specific branch that are here

UINT32
CPStruct::numBrigades(BRANCH_ENUM forBranch) const
{
	//dbugLog(_FLN, "Performing numBrigades");
	if (m_units.empty())
		return 0;

	UINT32 units = 0;

	for ( HCUNIT_INDEX::const_iterator it = m_units.begin(); it != m_units.end(); ++it )
	{
		const HCUnit* unit = it->second;
		if ( (unit->isBrigade() == false && unit->isGarrison() == false) || unit->branch() != forBranch || unit->movementType() != forBranch )
			continue;
		++units;
	}

	return units;
}

//////////////////////////////////////////////////////////////////////
// Return the factories and their current status'

std::string strtGetFactoryList(WWII_COUNTRY forCountry)
{
	//dbugLog(_FLN, "Performing strtGetFactoryList");
	// Re-order the _factories vector so that the factories are put into the proper order when displayed
	std::sort(_factories.begin(), _factories.end(), [](const ObjectStruct *obj1, const ObjectStruct *obj2) {
		return strcmp(obj1->name, obj2->name) < 0;
	});
	
    std::string result = "";

    for (const ObjectStruct *obj : _factories) {
		// If the factory isn't part of the specified country, skip this loop
        if (obj->facility->originalCountry != forCountry) {
            continue;
        }

        char buffer[512];

        if (obj->m_country == forCountry) {
            strprintf(buffer, "%s: %u%% (%.3s) ", obj->name, obj->percentDamage, obj->facility->cp->name);
        } else {
			// If someone else besides the orginal owner owns this factory, make a note of it
            strprintf(buffer, "%s: %u%% (OWNER: %s) ", obj->name, obj->percentDamage, _countryCode[obj->m_country]);
        }

		// Clean up the UK/FR/DE_PF#_FACTORY names
		std::string tempString = buffer;
		tempString.replace(0, 5, "#");
		tempString.replace(2, 8, "");
	
        result += tempString;
    }

	if ( result.empty() ) {
		return "No factories";
	}

	return result;
}
