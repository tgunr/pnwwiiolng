#pragma once
// Date: Fri Aug 17 15:53:49 2001
//
// Handlers and stuff for mission related strat issues
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Processes the mmHosts request for the "health" of a facility
struct MM_CAPACITY_REQUEST ;
struct MM_CAPACITY_RESPONSE ;
struct FULL_OBJ_ID ;

extern SINT32 strtProcessCapacityRequest
	( const MM_CAPACITY_REQUEST& capacityRequest
	, MM_CAPACITY_RESPONSE& capacityResponse
	, FULL_OBJ_ID destFacility
	, bool vehicleDelivered
	);

