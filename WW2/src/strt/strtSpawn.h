#pragma once
// Date: Tue Jan 23 10:27:02 2001
//
// All the spawn initialization and functions for use in the strat System
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

// Chat macros
#ifdef BETA_RELEASE
# define TELL_TESTER(id, ...)	\
	chatSendDirectMessage(id, LSTR_BETA, LSTR_GENERIC, __VA_ARGS__)
#else
# define TELL_TESTER(...)
#endif

#define MAX_VEHICLE_CLASS	5
#define DEFAULT_MAX_VEHICLE_RETURN_TIMEOUT 3 * 60 * 60

#define REINFORCEMENTS_START_AFTER		30 * 60
#define INITIAL_REINFORCEMENT_OFFSET	(DEFAULT_MAX_VEHICLE_RETURN_TIMEOUT - REINFORCEMENTS_START_AFTER)

// How fast we ask the client to refresh the weapons list
#define	DEFAULT_WEAPON_LIST_REFRESH_RATE	30

// How long between capture of a facility and delivery of first vehicle (seconds)
#define	DEFAULT_WEAPON_DELIVERY_DELAY		0			// 0 seconds
//#define DEFAULT_WEAPON_DELIVERY_DELAY		120			// 2 minutes

// The end of the weapon delivery window
#define DEFAULT_WEAPON_DELIVERY_TIME		(60 * 60)	// 60 minutes

class CPStruct;
class FacilityStruct;
struct MM_POST_REQUEST;
struct MM_POST_ACCEPT;
struct MM_POST_DENY;
struct MM_JOIN_REQUEST;
struct MM_JOIN_ACCEPT;
struct MM_JOIN_DENY;
struct POSTED_WEAPON;
struct MM_SPAWN_GRANT_REQUEST;
struct MM_SPAWN_GRANT_RESPONSE;
struct MM_POST_INFO_UPDATE;
struct MM_POST_INFO_REQUEST;
struct MM_POST_INFO_RESPONSE;
struct MM_SPAWN_CANCEL;

//Initializes the spawning system from filename
extern void strtInitSpawn();

//Processes a post list request.
//	WARNING - whoever uses this function needs to free postListResponse->postedWeapon
extern SINT32 strtProcessPostListRequest(SINT32 teulIndex, const struct MM_POST_LIST_REQUEST& postListRequest);

//Sets up spawning on a facility based on the spawn objects attached to it.
extern SINT32 strtSetupSpawn(FacilityStruct* facility);

//Processes a post Request and populates either the deny or the accept field
//	accordingly.  The return value tells you whether the request was good or
//	bad.
extern bool strtProcessPostRequest(SINT32 teulIndex, const MM_POST_REQUEST& postRequest, MM_POST_ACCEPT& postAccept, MM_POST_DENY& postDeny);

//Processes an mm join request
extern bool strtProcessJoinRequest(const MM_JOIN_REQUEST& joinRequest, MM_JOIN_ACCEPT& joinAccept, MM_JOIN_DENY& joinDeny);

//This function is called to expire a specific weapon
extern void strtExpireWeapon(POSTED_WEAPON& weapon);

//This function processes a grant request from the mmhost
extern bool strtProcessSpawnGrantRequest(const MM_SPAWN_GRANT_REQUEST& spawnGrantRequest, MM_SPAWN_GRANT_RESPONSE& spawnGrantResponse);

extern bool strtProcessJoinListRequest(SINT32 teulIndex, const MM_POST_INFO_REQUEST& postInfoRequest);

extern bool strtProcessPostInfoUpdate(MM_POST_INFO_UPDATE* infoUpdate);

//This function processes a spawn cancellation (post join spawn cancel)
extern bool strtProcessSpawnCancel(const MM_SPAWN_CANCEL& spawnCancel);

//This function expires weapons that have been spawned longer than X hours
extern void strtExpireSpawnedWeapons();

extern UINT32 strtFallback(CPStruct* cp, WWII_SIDE issuer, FALLBACK_MODE execute, UINT32 playerID);
