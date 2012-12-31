#pragma once
// Created: Thu Feb  1 16:09:11 2001
//
// Description goes here
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

#include "miss/missionTypes.h"

class Mission ;
class Player ;
class MissionPacket ;
class FacilityStruct ;

extern void missInit();
extern Mission* mapFindMission(UINT32 id) ;

extern void checkForTimedOutMissions();

extern SINT32 missionSendMissionPostReply(struct MISSION_POST_REQ& request, Player* player, SINT32 connection);

//was a mission Successful?
extern LOGICAL missionSuccessful(const MissionPacket* mp);

//calculate their resulting score
extern void missionScoreMissionPacket(Player* player, struct MM_CAPACITY_RESPONSE* response);

//send a capacity request for the persona with the info of the mission they're
//on, taking into acct if it's an attack or defend mission when determing
//which fullobjid to pass to strtSendMMCapacityRequest()
extern SINT32 missionSendMMCapacityRequest(Player* player, CAPACITY_REQUEST_STATE tag);

extern LOGICAL missionSetPersonaAllegiance(Player* player, FacilityStruct* facility);

extern void missionUpdateMissionTaker(Player* player, RMLIST_ACTION action);

extern void missionRevokeMissionsFrom(const FacilityStruct& facility) ;
extern void missionFirebaseDestroyed(const FacilityStruct& facility) ;

extern SINT32 missionCancel(UINT32 missionID) ;

extern void missionCheckOrigins(class CPStruct* cp) ;

extern Mission* missionGetInstanceMission(instanceid_t instanceID, WWII_COUNTRY forCountry, BRANCH_ENUM forBranch) ;

extern void missDeleteInstanceMissions(instanceid_t instanceID) ;

extern void missAnnounceMissionsToChat() ;

extern void missCancelAllMobileSpawns() ;

