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

#include "dbug/dbug.h"
#include <assert.h>
#include <vector>
#include <string>

#include <stdlib.h>
#include <time.h>

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
#include "strt/strtUpdateList.h"
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

// ------------------------------ Prototypes

static LOGICAL _printConvoyList(CONVOYSTRUCT* convoy);
static LOGICAL _freeManifest(MANIFESTSTRUCT* manifest);

// Number of strat updates to creates
#define STRAT_UPDATES	4096

// Macro for telling an update that it is to be returned to the free pool
#define RETURN_UPDATE	update->updateTime = 0

// Seconds to use if none defined
#define	FACTORY_PERIOD_FALLBACK	300

// Minimum interval at which we will save a factory
#define FACTORY_PERIOD_MINIMUM	5

static UINT32 freeConvoys=MAXCONVOYS, numConvoys=0;
static SINT32 convoyID=0, payloadID=0;
static CONVOYSTRUCT* freeConvoyList;
static CONVOYSTRUCT* dispatchedConvoyList;
static STRAT_UPDATE* freeStratUpdates;
static STRAT_UPDATE* stratUpdateList=NULL;
static STRAT_UPDATE* _convoyUpdate=NULL;

static SINT32 _mapConnection=0;

//These are global to the Convoy System so they can be acessed by createConvoy
//	and such fun things
static LOGICAL _interrupted = FALSE;

static STRAT_UPDATE*
_getFreeStratUpdate()
{
	STRAT_UPDATE* stratUpdate = strtRemoveFirstStratUpdate(&freeStratUpdates);
	if (stratUpdate == NULL)
	{
		stratUpdate = strtCreateStratUpdateList(STRAT_UPDATES);
		if ( stratUpdate == NULL )
		{
			dbugLog(_FLA, "!!! strtCreateStratUpdateList FAILED !!!");
			return NULL;
		}
		freeStratUpdates = stratUpdate;

		stratUpdate = strtRemoveFirstStratUpdate(&freeStratUpdates);
		if ( stratUpdate == NULL )
		{
			dbugLog(_FLA, "unable to retrieve first free update after create");
			return NULL;
		}
	}

	return stratUpdate;
}

static void
_releaseStratUpdate(STRAT_UPDATE* update)
{
	//Remove the update from the update list
	strtDeleteStratUpdate(&stratUpdateList, update);

	//Zero it out and put it on the free update list
	update->updateTime = 0;
	update->data = NULL;

	strtInsertStratUpdate(&freeStratUpdates, update);
}

//////////////////////////////////////////////////////////////////////
//Interrupts the sleeping during convoy processing

extern LOGICAL
cnvyInterrupt(void)
{
	return _interrupted = TRUE;
}

//////////////////////////////////////////////////////////////////////
//Checks the value of the interrupt flag

extern LOGICAL
cnvyInterrupted(void)
{
	return _interrupted;
}

//////////////////////////////////////////////////////////////////////
//Clears the interrupt flag

extern LOGICAL
cnvyClearInterrupt(void)
{
	_interrupted = FALSE;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//Returns the next payloadid

SINT32
getNextPayloadID()
{
	if (++payloadID < 0)
		payloadID=1;

	return payloadID;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
cnvyGetMapConnection(void)
{
	return _mapConnection;
}

//////////////////////////////////////////////////////////////////////
//

SINT32
cnvySetMapConnection(SINT32 connection)
{
#ifdef DEBUG
	dbugLog(_FLD,"setting map connection %d", connection);
#endif
	_mapConnection = connection;
	return 0;
}

//////////////////////////////////////////////////////////////////////
//

PAYLOADSTRUCT*
cnvyMakeRPPayload(UINT32 RPs)
{
	PAYLOADSTRUCT* tmpPayload;

	//Get memory for our payload
	tmpPayload=(PAYLOADSTRUCT*) memAlloc(sizeof(PAYLOADSTRUCT));

	tmpPayload->next = NULL;
	tmpPayload->destination = NULL;
	tmpPayload->manifest = NULL;
	tmpPayload->spawnpoint = NULL;
	tmpPayload->contents = PAYLOADSTRUCT::_RPS;
	tmpPayload->startRPs = tmpPayload->RPs = RPs;

	return tmpPayload;
}

//////////////////////////////////////////////////////////////////////
//Takes a Convoy off the freeConvoylist, inits it with the information its
//	given, then puts it on the dispatched convoy list

LOGICAL
cnvyCreateConvoy(WWII_SIDE side, WWII_COUNTRY country, LinkStruct* link, PAYLOADSTRUCT* payload)
{
	CONVOYSTRUCT* tmpConvoy=NULL;
	STRAT_UPDATE* update;

	//If the freeConvoyList is empty, then get some more memory for it.
	if (freeConvoyList==NULL)
	{
		//We are out of convoys
		freeConvoyList = cnvyCreateConvoyList(MAXCONVOYS);
		freeConvoys = MAXCONVOYS;
	}

	tmpConvoy=cnvyRemoveFirstConvoy(&freeConvoyList);

	//Decrement freeConvoy count
	freeConvoys--;
	numConvoys++;

	//Now we have tmpConvoy pointing to our Convoy to populate
	if (++convoyID < 0)
		convoyID=0;
	tmpConvoy->id = convoyID;
	tmpConvoy->side = side;
	tmpConvoy->country = country;
	tmpConvoy->link = link;
	tmpConvoy->payload = payload;
	tmpConvoy->damageJoules = 0;
	tmpConvoy->maxDamageJoules = 10;

	//Point the payload back at the convoy
	payload->convoy=tmpConvoy;
	payload->cp=NULL;

	//Time calculations.  To be replaced later with an actual calculation
	//	based on the vehicle type, etc.
	tmpConvoy->nextEvent = tmpConvoy->arrivalTime = teulTime() + link->latency;

	cnvyInsertConvoy(&dispatchedConvoyList,tmpConvoy);

	if (_convoyUpdate != NULL)
	{
		//Make sure we sleep for the least amount of seconds necessary
		if (_convoyUpdate->updateTime > tmpConvoy->nextEvent)
		{
			//Pull it out of the update list
			strtDeleteStratUpdate(&stratUpdateList, _convoyUpdate);

			//Fix the time
			_convoyUpdate->updateTime = tmpConvoy->nextEvent;

			//Put it back in the list
			strtInsertStratUpdate(&stratUpdateList, _convoyUpdate);
		}
	}
	else
	{
		//We must create a convoy update for this convoy
		update = _getFreeStratUpdate();
		update->type = STRAT_UPDATE_CONVOY_SYSTEM;
		update->updateTime = tmpConvoy->nextEvent;
		strtInsertStratUpdate(&stratUpdateList, update);
		//Save the pointer to it, so we can find it quickly
		_convoyUpdate = update;
	}

	//dbugLog(_FLN,"Convoy %d sent from CP %s to CP %s", tmpConvoy->id, link->startDepot->cp->name, link->endDepot->cp->name);

	//Set flag on link
	link->convoy=TRUE;
	link->startDepot->m_incomingLink->convoy=TRUE;

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//Frees the convoy up and puts it on the freeConvoyList

LOGICAL
cnvyFreeConvoy(CONVOYSTRUCT* convoy)
{
	if (convoy==NULL)
		return FALSE;

	//Get rid of all the associated memory, manifests and such
	if (convoy->payload!=NULL)
	{
		//We have to free the payload
		//First, free the manifest list
		_freeManifest(convoy->payload->manifest);
		memFree(convoy->payload);
		convoy->payload=NULL;
	}

	//If we are freeing the head of the dispatchedConvoy List (the most
	//	common scenario)...
	if (dispatchedConvoyList==convoy)
	{
		cnvyRemoveFirstConvoy(&dispatchedConvoyList);
	}
	else
	{
		cnvyDeleteConvoy(&dispatchedConvoyList, convoy);
	}

	//Put freed convoy at the head of the list
	convoy->nextEvent = convoy->arrivalTime = 0;
	cnvyInsertConvoy(&freeConvoyList,convoy);

	freeConvoys++;
	numConvoys--;

	//strtDeleteConvoy(convoy->id);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
//

bool
cnvyAddWeaponToExpireList(POSTED_WEAPON& weapon, UINT32 expireSeconds)
{
	STRAT_UPDATE* stratUpdate = _getFreeStratUpdate();
	if ( stratUpdate == NULL )
		return false;

	//Set the pertinant strat update information
	stratUpdate->updateTime = teulTime() + expireSeconds;
	stratUpdate->type = STRAT_UPDATE_POST_EXPIRED;
	stratUpdate->data = &weapon;

	//Get a pointer back to this update so we can remove it from the
	//	expire list whenever it is spawned
	weapon.m_expiryUpdate = stratUpdate;

	//Put it on the update list
	strtInsertStratUpdate(&stratUpdateList, stratUpdate);

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
		return NULL;
	}

	if ( delay <= 5 )
	{
		dbugLog(_FLN, "hcu:%u brigade needs routing immediately", brigadeID);
		strtReformBrigade(brigadeID, NULL);
		return NULL;
	}

	STRAT_UPDATE* stratUpdate = _getFreeStratUpdate();
	if ( stratUpdate == NULL )
		return NULL;

	stratUpdate->type = STRAT_REFORM_BRIGADE;
	stratUpdate->updateTime = teulTime() + delay;
	stratUpdate->data = NULL;
	stratUpdate->value = brigadeID;

	strtInsertStratUpdate(&stratUpdateList, stratUpdate);

	return stratUpdate;
}

//////////////////////////////////////////////////////////////////////
// Cancel a brigade reformation event

void
cnvyCancelBrigadeEvent(STRAT_UPDATE* update)
{
	if ( update == NULL )
		return;

	_releaseStratUpdate(update);
}

//////////////////////////////////////////////////////////////////////
// Schedule an objective for cancellation

bool
cnvyScheduleObjectiveExpiry(CPStruct* cp, WWII_SIDE side, UINT32 expirySeconds)
{
	STRAT_UPDATE* stratUpdate;

	if ( cp == NULL )
	{
		dbugLog(_FLA, "%s called with NULL cp", __FUNCTION__);
		return false;
	}

	stratUpdate = _getFreeStratUpdate();
	if ( stratUpdate == NULL )
		return false;

	stratUpdate->type = STRAT_OBJECTIVE_EXPIRY;
	stratUpdate->updateTime = teulTime() + expirySeconds;
	stratUpdate->data = cp;
	stratUpdate->value = side;

	strtInsertStratUpdate(&stratUpdateList, stratUpdate);

	return true;
}

//////////////////////////////////////////////////////////////////////
//

void
cnvyRemoveWeaponFromExpireList(POSTED_WEAPON& weapon)
{
	//Get the update, and NULL it off the posted weapon
	STRAT_UPDATE* stratUpdate = weapon.m_expiryUpdate;
	if ( stratUpdate == NULL )
		return;

	weapon.m_expiryUpdate = NULL;

	//Remove the update from the update list
	strtDeleteStratUpdate(&stratUpdateList, stratUpdate);

	//Zero it out and put it on the free update list
	stratUpdate->updateTime = 0;
	strtInsertStratUpdate(&freeStratUpdates, stratUpdate);
}

//////////////////////////////////////////////////////////////////////
// Place a factory production update in the queue

LOGICAL
cnvyBeginFactoryProduction(ObjectStruct* object, UINT32 seconds)
{
	STRAT_UPDATE* stratUpdate = NULL;

	if ( object == NULL || host_config.factoryPeriod == 0 )
		return FALSE;

	if ( seconds == 0 )
		seconds = host_config.factoryPeriod;

	if ( seconds < FACTORY_PERIOD_MINIMUM )
		seconds = FACTORY_PERIOD_MINIMUM;

	//Set the pertinant strat update information
	stratUpdate = _getFreeStratUpdate();
	stratUpdate->updateTime = teulTime() + seconds;
	stratUpdate->type = STRAT_PROCESS_FACTORY;
	stratUpdate->data = object;
	stratUpdate->value = seconds;
	strtInsertStratUpdate(&stratUpdateList, stratUpdate);

	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Called when an objective expiry event occurs.

bool
_expireObjective(CPStruct* cp, WWII_SIDE forSide)
{
	// Sanity check
	if ( cp == NULL )
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

static const char* const CONFIG_UPDATE_QUERY = "SELECT variableName, value, UNIX_TIMESTAMP(lastModified) FROM `game`.`host_config` WHERE UNIX_TIMESTAMP(lastModified) > %u";

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
	virtual bool Process(const size_t numRows)
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
	virtual void Success()
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

LOGICAL
cnvyProcessStratUpdates(UINT32 currentTime)
{
	UINT32 nextHourEvent;
	UINT32 nextPolledEvent;
	static UINT32 lastStratUpdate = 0;

	if ( lastStratUpdate == 0)
		lastStratUpdate = teulTime();

	//If we have updates
	while ((stratUpdateList !=NULL) && (currentTime >= stratUpdateList->updateTime))
	{
		STRAT_UPDATE* update;

		update = strtRemoveFirstStratUpdate(&stratUpdateList);
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
				dbugLog(_FLD,"new lastStratUpdate and updateTime = %u",lastStratUpdate);
			}
			strtInsertStratUpdate(&stratUpdateList,update);
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
				strtInsertStratUpdate(&stratUpdateList, update);

				break;
			}

			cnvyUpdateTime();

			nextHourEvent = wthrRealSecondsToHour();
			nextPolledEvent = STRAT_TIME_UPDATE_FREQUENCY;

			if ( nextHourEvent > 0 && nextHourEvent < nextPolledEvent )
				nextPolledEvent = nextHourEvent;

			update->updateTime = currentTime + nextPolledEvent;

			strtInsertStratUpdate(&stratUpdateList, update);
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
			strtInsertStratUpdate(&stratUpdateList, update);
		break;

		case STRAT_UPDATE_CONVOY_SYSTEM:
			cnvyProcessConvoys();

			//put its next update on the list
			if (dispatchedConvoyList != NULL)
			{
				//If there are convoys, then we need to reinsert this
				//	update with an updated time
				update->updateTime = dispatchedConvoyList->nextEvent;
				strtInsertStratUpdate(&stratUpdateList,update);

				//Save the current convoyUpdate pointer
				_convoyUpdate = update;
			}
			else //We have no more convoys, so remove the update for it
			{
				//Let the convoy system know there aren't any updates
				_convoyUpdate = NULL;
				update->updateTime = 0;
			}

		break;

		case STRAT_UPDATE_POST_EXPIRED:

			if ( update->data == NULL )
				LogAbort("SUPPP has NULL data");
			else
			{
				POSTED_WEAPON& weapon = *(POSTED_WEAPON*)update->data;
				update->data = NULL;

				weapon.m_expiryUpdate = NULL;
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

				strtInsertStratUpdate(&stratUpdateList, update);
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
			update->data = NULL;
			strtInsertStratUpdate(&freeStratUpdates, update);
		}
	}

	if ( _interrupted )
		cnvyClearInterrupt();

	return TRUE;
}

//This function processes any waiting convoys
void
cnvyProcessConvoys()
{
	//Check status of any convoys that need it
	while ( dispatchedConvoyList !=NULL && dispatchedConvoyList->nextEvent <= teulTime() )
	{
		strtAcceptConvoy(*dispatchedConvoyList);
	}
}

//Initializes all the cnvy System stuff
LOGICAL
cnvyInit(void)
{
	STRAT_UPDATE* update;
	UINT32 currentTime;

	//Initialization stuff
	freeConvoyList = cnvyCreateConvoyList(MAXCONVOYS);

	stratUpdateList = NULL;
	freeStratUpdates = strtCreateStratUpdateList(STRAT_UPDATES * 10);
	_convoyUpdate = NULL;

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
	update = strtRemoveFirstStratUpdate(&freeStratUpdates);
	update->updateTime = currentTime;
	update->type = STRAT_UPDATE_STRAT_SYSTEM;
	strtInsertStratUpdate(&stratUpdateList, update);

	//Setup the first updateTime
	//Once setup, this update always exists
	update = strtRemoveFirstStratUpdate(&freeStratUpdates);

	//Make sure this time update takes place* After* the first strat run
	//Used to have to do this b/c strat controlled the time. Now I'm not
	//	so sure why, but don't change it if it works!
	update->updateTime = currentTime + 1;
	update->type = STRAT_UPDATE_TIME;
	strtInsertStratUpdate(&stratUpdateList, update);

	// Set up the 1-minute cycle
	update = strtRemoveFirstStratUpdate(&freeStratUpdates);
	update->updateTime = currentTime + 60;
	update->type = STRAT_MINUTE_CYCLE;
	update->data = NULL;
	strtInsertStratUpdate(&stratUpdateList, update);

	dispatchedConvoyList=NULL;

	return TRUE;
}

static LOGICAL _printConvoyList(CONVOYSTRUCT* convoy)
{
	if (convoy!=NULL)
	{
		fprintf(stderr,"%p:Convoy: %d\n",convoy,convoy->id);
		fprintf(stderr,"\tfrom: %d:%d:%d\n", convoy->link->startDepot->fullObjID.cell.ox, convoy->link->startDepot->fullObjID.cell.oy, convoy->link->startDepot->fullObjID.id);
		fprintf(stderr,"\tto: %d:%d:%d\n", convoy->link->endDepot->fullObjID.cell.ox, convoy->link->endDepot->fullObjID.cell.oy, convoy->link->endDepot->fullObjID.id);
		//fprintf(stderr,"\tRPs: %d\n",convoy->RPs);
		fprintf(stderr,"\tside: %d\n",convoy->side);
		fprintf(stderr,"\tcountry: %d\n",convoy->country);
		fprintf(stderr,"\tnextEvent: %d\n",convoy->nextEvent);
		fprintf(stderr,"\tarrivalTime: %d\n",convoy->arrivalTime);
		fprintf(stderr,"\tprev: %p\n",convoy->prev);
		fprintf(stderr,"\tnext: %p\n\n",convoy->next);
		_printConvoyList(convoy->next);
	}
	return TRUE;
}


static LOGICAL
_freeManifest(MANIFESTSTRUCT* manifest)
{
	if (manifest==NULL)
		return FALSE;

	_freeManifest(manifest->next);

	memFree(manifest);

	return TRUE;
}

