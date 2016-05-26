#include <syscfg.h> // Path is project specific.
// Created: Wed Mar 29 12:05:57 2000
//
// This module is what actually controls the convoys and calls the strat
// System.  Right now its going to become a delay loop, then later it'll
// actually move trucks/ships/barges/mules/etc.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "teul/teulKit.h"
#include "strt/strtConstants.h"
#include "strt/strtEventQueue.h"
#include "strt/strtTypes.h"
#include "strt/strtServerTypes.h"
#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtLinkStruct.h"
#include "cnvy/cnvyMain.h"
#include "strt/strtMain.h"
#include "strt/strtCP.h"
#include "cnvy/cnvyConvoyList.h"
#include "include/wthrTime.h"
#include "strt/strtHostMessages.h"
#include "strt/strtSupply.h"
#include "strt/strtSpawn.h"
#include "chat/chatStratSupportMessages.h"
#include "dbse/dbseWrapper.h"
#include "include/mydbQuery.h"
#include "strt/strtDB.h"
#include "strt/strtHostDB.h"
#include "strt/strtHostLua.h"
#include "strt/strtSpawn.h"
#include "strt/strtDotCommands.h"
#include "strt/strtPostedWeaponList.h"
#include "strt/strtObjectiveStats.h"
#include "strt/strtHCUnits.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiDBThreads.h"
#include "wwii/wwiiDBConfig.h"

#include "mem/memPool.h"
#include "include/intrusiveList.h"

// ------------------------------ Prototypes

static Mem::Pool<STRAT_UPDATE, 4096> s_stratUpdatePool;

// Macro for telling an update that it is to be returned to the free pool
#define RETURN_UPDATE	update->updateTime = 0

// Seconds to use if none defined
#define	FACTORY_PERIOD_FALLBACK	300

// Minimum interval at which we will save a factory
#define FACTORY_PERIOD_MINIMUM	5

static UINT32 freeConvoys=MAXCONVOYS, numConvoys=0;
static CONVOYSTRUCT* freeConvoyList;
static CONVOYSTRUCT* dispatchedConvoyList;
typedef IntrusiveList<STRAT_UPDATE> StratUpdateList;
static StratUpdateList s_stratUpdateList;
static STRAT_UPDATE* _convoyUpdate;

//////////////////////////////////////////////////////////////////////
//Inserts a updateTime into the stratUpdatelist

static void _insertStratUpdate(STRAT_UPDATE* stratUpdate) {
	ABORT_IF ( stratUpdate == nullptr );

	for ( auto it = s_stratUpdateList.rbegin() ; it != s_stratUpdateList.rend() ; ++it ) {
		if ( it->updateTime > stratUpdate->updateTime )
			continue;

		if ( it->m_next == stratUpdate )
			dbugLog(_FLM, "Attempted to re-add strat update [%d : %d] @ %d",
			        stratUpdate->type, stratUpdate->value, stratUpdate->updateTime);
		else
			stratUpdate->link(it->m_next);

		return;
	}

	s_stratUpdateList.push_front(stratUpdate);
}

//////////////////////////////////////////////////////////////////////
// Take an update out of the update list and release it to the pool.

static void _releaseStratUpdate(STRAT_UPDATE* update)
{
	//Remove the update from the update list
	update->unlink();

	//Zero it out and put it on the free update list
	update->updateTime = 0;
	update->data = nullptr;

	s_stratUpdatePool.Free(update);
}

//////////////////////////////////////////////////////////////////////
//Takes a Convoy off the freeConvoylist, inits it with the information its
//	given, then puts it on the dispatched convoy list

void cnvyCreateConvoy(WWII_SIDE side, WWII_COUNTRY country, LinkStruct* link, UINT32 rps)
{
	CONVOYSTRUCT* newConvoy = nullptr;

	//If the freeConvoyList is empty, then get some more memory for it.
	if (freeConvoyList == nullptr)
	{
		//We are out of convoys
		freeConvoyList = cnvyCreateConvoyList(MAXCONVOYS);
		freeConvoys = MAXCONVOYS;
	}

	newConvoy = cnvyRemoveFirstConvoy(&freeConvoyList);

	//Decrement freeConvoy count
	freeConvoys--;
	numConvoys++;

	newConvoy->side = side;
	newConvoy->link = link;
	newConvoy->numRPs = rps;

	//Time calculations.  To be replaced later with an actual calculation
	//	based on the vehicle type, etc.
	newConvoy->arrivalTime = teulTime() + link->latency;

	cnvyInsertConvoy(&dispatchedConvoyList,newConvoy);

	if (_convoyUpdate != nullptr)
	{
		//Make sure we sleep for the least amount of seconds necessary
		if (_convoyUpdate->updateTime > newConvoy->arrivalTime)
		{
			// Pull the node out, adjust the time and reposition it.
			StratUpdateList::iterator it(_convoyUpdate->m_next), end(s_stratUpdateList.end());
			_convoyUpdate->unlink();
			_convoyUpdate->updateTime = newConvoy->arrivalTime;
			while (it != end && it->updateTime < _convoyUpdate->updateTime)
				++it;
			_convoyUpdate->link(it);
		}
	}
	else
	{
		//We must create a convoy update for this convoy
		_convoyUpdate = s_stratUpdatePool.New(newConvoy->arrivalTime, STRAT_UPDATE_CONVOY_SYSTEM);
		_insertStratUpdate(_convoyUpdate);
	}

	//dbugLog(_FLN,"Convoy %d sent from CP %s to CP %s", newConvoy->id, link->startDepot->cp->name, link->endDepot->cp->name);

	//Set flag on link
	link->m_convoy = true;
	link->startDepot->m_incomingLink->m_convoy = true;
}

//////////////////////////////////////////////////////////////////////
//Frees the convoy up and puts it on the freeConvoyList

void cnvyFreeConvoy(CONVOYSTRUCT* convoy)
{
	if (convoy == nullptr)
		return;

	cnvyDeleteConvoy(&dispatchedConvoyList, convoy);

	//Put freed convoy at the head of the list
	convoy->arrivalTime = 0;
	cnvyInsertConvoy(&freeConvoyList,convoy);

	freeConvoys++;
	numConvoys--;

	//strtDeleteConvoy(convoy->id);
}

//////////////////////////////////////////////////////////////////////
//

bool
cnvyAddWeaponToExpireList(POSTED_WEAPON& weapon, UINT32 expireSeconds)
{
	STRAT_UPDATE* stratUpdate = s_stratUpdatePool.New(teulTime() + expireSeconds, STRAT_UPDATE_POST_EXPIRED, 0, &weapon);

	//Get a pointer back to this update so we can remove it from the
	//	expire list whenever it is spawned
	weapon.m_expiryUpdate = stratUpdate;

	//Put it on the update list
	_insertStratUpdate(stratUpdate);

	return true;
}

//////////////////////////////////////////////////////////////////////
// Schedule a brigade reformation

STRAT_UPDATE*
cnvyScheduleBrigadeReformation(UINT32 brigadeID, UINT32 delay)
{
	if ( brigadeID == 0 )
	{
		dbugLog(_FLA, "Requested brigade reformation for id 0 - invalid");
		return nullptr;
	}

	if ( delay <= 5 )
	{
		dbugLog(_FLN, "hcu:%u brigade needs routing immediately", brigadeID);
		strtReformBrigade(brigadeID, nullptr);
		return nullptr;
	}

	STRAT_UPDATE* stratUpdate = s_stratUpdatePool.New(teulTime() + delay, STRAT_REFORM_BRIGADE, brigadeID);
	_insertStratUpdate(stratUpdate);

	return stratUpdate;
}

//////////////////////////////////////////////////////////////////////
// Cancel a brigade reformation event

void
cnvyCancelBrigadeEvent(STRAT_UPDATE* update)
{
	if ( update == nullptr )
		return;

	_releaseStratUpdate(update);
}

//////////////////////////////////////////////////////////////////////
// Schedule an objective for cancellation

bool
cnvyScheduleObjectiveExpiry(CPStruct* cp, WWII_SIDE side, UINT32 expirySeconds)
{
	if ( cp == nullptr )
	{
		dbugLog(_FLA, "%s called with NULL cp", __FUNCTION__);
		return false;
	}

	STRAT_UPDATE* stratUpdate = s_stratUpdatePool.New(teulTime() + expirySeconds, STRAT_OBJECTIVE_EXPIRY, side, cp);
	_insertStratUpdate(stratUpdate);

	return true;
}

//////////////////////////////////////////////////////////////////////
//

void
cnvyRemoveWeaponFromExpireList(POSTED_WEAPON& weapon)
{
	//Get the update, and NULL it off the posted weapon
	STRAT_UPDATE* stratUpdate = weapon.m_expiryUpdate;
	if ( stratUpdate != nullptr )
	{
		weapon.m_expiryUpdate = nullptr;
		_releaseStratUpdate(stratUpdate);
	}
}

//////////////////////////////////////////////////////////////////////
// Place a factory production update in the queue

void cnvyBeginFactoryProduction(ObjectStruct* object, UINT32 seconds)
{
	if ( object == nullptr || host_config.factoryPeriod == 0 )
		return;

	if ( seconds == 0 )
		seconds = host_config.factoryPeriod;

	if ( seconds < FACTORY_PERIOD_MINIMUM )
		seconds = FACTORY_PERIOD_MINIMUM;

	//Set the pertinent strat update information
	STRAT_UPDATE* stratUpdate = s_stratUpdatePool.New(teulTime() + seconds, STRAT_PROCESS_FACTORY, seconds, object);
	_insertStratUpdate(stratUpdate);
}

//////////////////////////////////////////////////////////////////////
// Called when an objective expiry event occurs.

bool
_expireObjective(CPStruct* cp, WWII_SIDE forSide)
{
	// Sanity check
	if ( cp == nullptr )
	{
		dbugLog(_FLA, "objective expiry with NULL cp");
		return false;
	}

	// More sanity
	if ( forSide == WWII_NO_SIDE || forSide >= MAX_SIDES )
	{
		dbugLog(_FLA, "objective expiry for %s with invalid side (%d)"
						, cp->name
						, forSide
						);
		return false;
	}

	// Make sure they didn't complete the objective
	if ( !cp->isObjective(forSide) )
	{
		dbugLog(_FLI, "%d objective for %s already withdrawn elsehow"
						, forSide
						, cp->name
						);
		return false;
	}

	// If this is an attacker pulling the plug, due notice
	if ( cp->isCity() && forSide == cp->attacker() && forSide != cp->side )
		cp->announceObjectiveWithdrawl();

	// Tell the CP to cancel this objective
	cp->clearObjective(forSide);

	return true;
}

void
cnvyUpdateTime()
{
	//Set the time of the next time update
	// Check to see if there is an hour event within 30 seconds of this event
	const UINT32 gameTime = wthrGetGameTime();
	const UINT32 gameTimeFactor = wthrGetTimeFactor();
	UINT32 gameHour = (gameTime / 3600) % 24;
	static UINT32 prevGameHour = 0;

	if ( gameHour < prevGameHour )
	{
		// Recalculate the weather pattern when shifting out of night
		strtStartNewDay();
	}
	prevGameHour = gameHour;

	const UINT32 weatherPattern = wthrGetWeatherPattern();
	const UINT32 fogPattern = wthrGetFogPattern();

	dbugLog(_FLI,"Broadcasting time update %ux%u weatherPattern %u fog %u"
				, gameTime, gameTimeFactor, weatherPattern, fogPattern);

	//Send out the time update
	strtBroadcastTimeUpdate(gameTime, gameTimeFactor, weatherPattern, fogPattern);
}

//////////////////////////////////////////////////////////////////////
// Pull an update of the config in the background

static const char* const CONFIG_UPDATE_QUERY = "SELECT variableName, value, UNIX_TIMESTAMP(lastModified) FROM `wwiionline`.`wwii_config` WHERE UNIX_TIMESTAMP(lastModified) > %u";

class ConfigUpdateQuery : public DBThreadQuery
{
public:
	ConfigUpdateQuery()
		: DBThreadQuery(DBThread::Default, "ConfigUpdateQuery", DBThreadQuery::_CALLBACK, CONFIG_UPDATE_QUERY, host_config.lastModified)
		, m_results()
		{}

private:
	struct Result
	{
		Result(const char* const varName_, const char* const value_, const char* const modStamp_)
			: m_varName(varName_), m_value(value_), m_modStamp(modStamp_)
			{}

		std::string m_varName;
		std::string m_value;
		std::string m_modStamp;
	};
	std::vector<Result> m_results;

public:
	virtual bool Process(const size_t numRows) override
	{
		if ( numRows > 0 )
		{
			while ( Query()->FetchRow() )
			{
				const DBRow& row = Query()->Row();
				m_results.push_back(Result(row[0], row[1], row[2]));
			}
		}
		return true;
	}

public:
	virtual void Success() override
	{
		extern bool dbUpdateWWIIConfigValue(const char* varName, const char* value, const char* modstamp);

		for ( size_t i = 0; i < m_results.size(); ++i )
		{
			dbUpdateWWIIConfigValue(m_results[i].m_varName.c_str(), m_results[i].m_value.c_str(), m_results[i].m_modStamp.c_str());
		}

		dotApplyConfig();
	}
};

//////////////////////////////////////////////////////////////////////
//This function processes any waiting strat updates

void cnvyProcessStratUpdates(UINT32 currentTime)
{
	UINT32 nextHourEvent;
	UINT32 nextPolledEvent;
	static UINT32 lastStratUpdate = 0;

	if ( lastStratUpdate == 0)
		lastStratUpdate = teulTime();

	// Check if the update list is in order.
	for (auto it = s_stratUpdateList.begin(), end = --s_stratUpdateList.end(); it != end; ++it)
	{
		if (it->updateTime > it->m_next->updateTime)
			LogAbort("stratUpdateList is messed up");
	}

	//If we have updates
	while (!s_stratUpdateList.empty())
	{
		auto update = s_stratUpdateList.front();
		if (update->updateTime > currentTime)
			break;
		update->unlink();
		switch (update->type)
		{
		case STRAT_UPDATE_STRAT_SYSTEM:
			//Run the strat system
#ifdef DEBUG_HEAVY
			dbugLog(_FLD,"Running strat system, lastStratUpdate = %d",lastStratUpdate);
#endif
			if ( strtMain(currentTime) )
			{
				//put its next update on the list
				update->updateTime = currentTime + STRATSLEEP;
			}
			else
			{
				lastStratUpdate = update->updateTime = currentTime + (STRAT_INTERVAL - (currentTime - lastStratUpdate));
				dbugLog(_FLD, "new lastStratUpdate and updateTime = %u", lastStratUpdate);
			}
			_insertStratUpdate(update);
		break;

		case STRAT_UPDATE_TIME:
			if ( host_config.arena.freezeClock )
			{
#ifndef DISABLE_WEATHER
				if ( host_config.weather.lockToPattern )
					wthrSetWeatherPattern(host_config.weather.lockToPattern);
				if ( host_config.weather.lockToFog )
					wthrSetFogPattern(host_config.weather.lockToFog);
#endif

				strtBroadcastTimeUpdate(wthrGetGameTime(), 1, wthrGetWeatherPattern(), wthrGetFogPattern());

				update->updateTime = currentTime + 120;
				_insertStratUpdate(update);

				break;
			}

			cnvyUpdateTime();

			nextHourEvent = wthrRealSecondsToHour();
			nextPolledEvent = STRAT_TIME_UPDATE_FREQUENCY;

			if ( nextHourEvent > 0 && nextHourEvent < nextPolledEvent )
				nextPolledEvent = nextHourEvent;

			update->updateTime = currentTime + nextPolledEvent;

			_insertStratUpdate(update);
		break;

		// Events that happen on a once a minute period
		case STRAT_MINUTE_CYCLE:
			// Request a check of the host_config
			(new ConfigUpdateQuery())->Queue();

			// Expire spawned weapons
			strtExpireSpawnedWeapons();

			g_objectiveStats.checkLimits(currentTime);

			strtCalculateRDPOutput();

			strtdbLoadTOETemplates();

			// Put me back on the queue
			update->updateTime = currentTime + 60;
			_insertStratUpdate(update);
		break;

		case STRAT_UPDATE_CONVOY_SYSTEM:
			cnvyProcessConvoys();

			//put its next update on the list
			if (dispatchedConvoyList != nullptr)
			{
				//If there are convoys, then we need to reinsert this
				//	update with an updated time
				update->updateTime = dispatchedConvoyList->arrivalTime;
				_insertStratUpdate(update);

				//Save the current convoyUpdate pointer
				_convoyUpdate = update;
			}
			else //We have no more convoys, so remove the update for it
			{
				//Let the convoy system know there aren't any updates
				_convoyUpdate = nullptr;
				update->updateTime = 0;
			}

		break;

		case STRAT_UPDATE_POST_EXPIRED:

			if ( update->data == nullptr )
				LogAbort("SUPPP has NULL data");
			else
			{
				POSTED_WEAPON& weapon = *(POSTED_WEAPON*)update->data;
				update->data = nullptr;

				weapon.m_expiryUpdate = nullptr;
				strtExpireWeapon(weapon);

				RETURN_UPDATE;
			}
		break;

		case STRAT_PROCESS_FACTORY:
			strtSaveFactoryOutput((ObjectStruct*)update->data);

			// Place the event back into the queue if factory output is
			// still enabled
			if ( host_config.factoryPeriod > 0 )
			{
				// Use the current factory period
				update->value = host_config.factoryPeriod;
				if ( update->value < FACTORY_PERIOD_MINIMUM )
					update->value = FACTORY_PERIOD_MINIMUM;
				update->updateTime = currentTime + update->value;

				_insertStratUpdate(update);
			}
			else
			{
				RETURN_UPDATE;
			}
		break;

		case STRAT_OBJECTIVE_EXPIRY:
			// Do all the business
			_expireObjective((CPStruct*)update->data, (WWII_SIDE)update->value);

			// Drop the update back onto the stack
			RETURN_UPDATE;
		break;

		case STRAT_REFORM_BRIGADE:
			// Redeploy a brigade to its division
			dbugLog(_FLI, "hcu:%u reforming brigade", (UINT32)update->value);
			strtReformBrigade((UINT32)update->value, update);

			RETURN_UPDATE;
		break;

		default:
			RETURN_UPDATE;
		break;
		}
		if ( update->updateTime == 0 )
		{
			//Put the update back on the free update list
			update->data = nullptr;
			s_stratUpdatePool.Free(update);
		}
	}
}

//This function processes any waiting convoys
void cnvyProcessConvoys()
{
	//Check status of any convoys that need it
	while ( dispatchedConvoyList != nullptr && dispatchedConvoyList->arrivalTime <= teulTime() )
	{
		strtAcceptConvoy(*dispatchedConvoyList);
	}
}

//Initializes all the cnvy System stuff
void cnvyInit()
{
	STRAT_UPDATE* update;
	UINT32 currentTime;

	//Initialization stuff
	freeConvoyList = cnvyCreateConvoyList(MAXCONVOYS);

	s_stratUpdatePool.Reserve(40960);
	_convoyUpdate = nullptr;

	dbugLog(_FLN,"Initializing Time System");
	wthrInitTime();

	//For starting at midnight
	//wthrSetTimeFactor(_timeFactor[STRAT_NIGHT]);
	//wthrSetGameTime(0);  //Midnight for testing

	//Set the time factor, then let the updater know what time it is
//	wthrSetTimeFactor(_timeFactor[STRAT_DAWN]);
	// Game starts at 7.30 am
	wthrSetGameTime(7 * 3600 + 1800);
	strtStartNewDay();

	currentTime = teulTime();

	//Setup the first strat run
	//Once setup, this update always exists
	update = s_stratUpdatePool.New(currentTime, STRAT_UPDATE_STRAT_SYSTEM);
	_insertStratUpdate(update);

	//Setup the first updateTime
	//Once setup, this update always exists
	//Make sure this time update takes place* After* the first strat run
	//Used to have to do this b/c strat controlled the time. Now I'm not
	//	so sure why, but don't change it if it works!
	update = s_stratUpdatePool.New(currentTime + 1, STRAT_UPDATE_TIME);
	_insertStratUpdate(update);

	// Set up the 1-minute cycle
	update = s_stratUpdatePool.New(currentTime + 60, STRAT_MINUTE_CYCLE);
	_insertStratUpdate(update);

	dispatchedConvoyList = nullptr;
}
