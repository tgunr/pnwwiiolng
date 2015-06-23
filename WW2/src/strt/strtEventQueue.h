#pragma once
// Date: Sat Apr 23 17:14:37 CDT 2005
//
// Classes and etc for the event queue
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "include/intrusiveList.h"

enum STRAT_UPDATE_TYPE
{
	STRAT_UPDATE_OBJECTS		//Incremental strat, used by strat Updater
								//	process
,	STRAT_UPDATE_TIME			//Update the time
,	STRAT_UPDATE_STRAT_SYSTEM	//Run the strat system
,	STRAT_UPDATE_CONVOY_SYSTEM 	//Run the convoy system
,	STRAT_UPDATE_POST_EXPIRED
,	STRAT_UPDATE_TIMEFACTOR		//Update the time factor
,	STRAT_SYNC_EVENTS			//Used only by the strat updater process!!!
,	STRAT_DELIVER_TIMED_WEAPON	// Add a non-oversupplying weapon
,	STRAT_MINUTE_CYCLE			// Events occurring on a one minute cycle
,	STRAT_PROCESS_FACTORY
,	STRAT_OBJECTIVE_EXPIRY
,	STRAT_REFORM_BRIGADE
,	MAX_STRAT_UPDATE_TYPE
};

//This is the item that is on the list that
struct STRAT_UPDATE;
struct STRAT_UPDATE : public IntrusiveList<STRAT_UPDATE>::Node
{
	STRAT_UPDATE() : updateTime(0), type(MAX_STRAT_UPDATE_TYPE), value(0), data(nullptr)
	{}
	explicit STRAT_UPDATE(UINT32 updateTime_, STRAT_UPDATE_TYPE type_, UINT32 value_=0, void* data_=nullptr)
		: Node()
		, updateTime(updateTime_)
		, type(type_)
		, value(value_)
		, data(data_)
	{}

	UINT32				updateTime;		// The time the update is going to go out
										//	in real world time
	STRAT_UPDATE_TYPE	type;			// type of update
	UINT32				value;			// Any value we need to pass
	void* 				data;			// In case of a weapon expiring (or other
										//	such events), it points to the
										//	appropriate data
};
