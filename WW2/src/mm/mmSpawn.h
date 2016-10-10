#pragma once
// Created: Wed Apr 18 15:39:01 2001
//
// Spawn functions in the mmHost
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

class Player;

//Takes a post request, attaches the correct customerID to it, then
//	forwards it along to the strat system for processing
extern bool mmProcessFEPostRequest(Player& player, const struct FE_POST_REQUEST& postRequest);

//Handles the canceling of a post.
extern bool mmProcessFEPostCancel(Player& player, const struct FE_POST_CANCEL& cancelPost);

//Handles a postListRequest from the client
extern bool mmProcessFEPostListRequest(Player& player, const struct FE_POST_LIST_REQUEST& postListRequest);

//Handles someone making a join request.  This is more complicated, b/c
//	we have to ask the front end the player is wanting to join up with
//	whether its ok or not
extern bool mmProcessFEJoinRequest(Player& player, const struct FE_JOIN_REQUEST& joinRequest);

extern bool mmProcessFECrewRequest(Player& player, const struct FE_CREW_REQUEST& crewRequest);

//Handles a joinPermResponse.  Asks the strat server to see if its ok for the
//	person to join the vehicle
extern bool mmProcessFECrewAccept(Player& player, const struct FE_CREW_ACCEPT& joinPermResponse);

//Handles someone killing a join request on thieir posted vehicle
extern bool mmProcessFEJoinPermKill(Player& player, const struct FE_JOIN_PERM_KILL& joinPermKill);

//Handles a client spawn.  Asks the strat server to spawn the people
extern bool mmProcessFEClientSpawn(Player& player, const struct FE_CLIENT_SPAWN& clientSpawn);

extern void mmSpawnDeleteVehicle(Player& player);

//-> from strat

//Handles the post accept handed it by the strat server.  Looks up the
//	appropriate client and forwards the message to them
extern bool mmProcessPostAccept(class Player& mapPersona, const struct MM_POST_ACCEPT& postAccept, UINT32 acceptType);

//Handles the post accept handed it by the strat server.  Looks up the
//	appropriate client and forwards the message to them
extern bool mmProcessPostDeny(const struct MM_POST_DENY& postDeny);

//Handles a post Expired coming from the strat to the mmHost.  Forwards
//	it on to all who need to know about it
extern bool mmProcessPostExpired(const struct MM_POST_EXPIRED& postExpired);

//Handles the join Accept coming from the strat system.  Tell the accepted
//	person everything is good to go, then tells the other unlucky people that
//	they were not chosen to accompany the person on their trip into the magic
//	wwiionline kingdom
extern bool mmProcessJoinAccept(const struct MM_JOIN_ACCEPT& joinAccept);

//Handles a join deny.  Tells the person that tried to join that it was
//	denied.  Don't know that this should EVER happen, but just in case...
extern bool mmProcessJoinDeny(const struct MM_JOIN_DENY& joinDeny);

//handles a crew bailing message
extern bool mmProcessCrewBailed(const struct MM_CREW_BAILED& crewBailed);

extern bool mmProcessJoinListResponse(const struct MM_JOIN_LIST_RESPONSE& response);

extern bool mmProcessLeaveListResponse(const struct MM_POST_INFO_RESPONSE& postInfoResponse);

extern bool mmDirectSpawn(Player& player, const struct WEAPON_SPEC& weapon, bool setIdentity, bool useCustomLocation, const FLOAT64VECTOR& customLocation);

