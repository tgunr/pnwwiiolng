#pragma once
// Created: Wed Dec  6 16:02:04 CST 2006
//
// Functions for manipulating (controlling) brigades
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

// Check to see if any of the current OIC elections have cycles
extern bool chatPatrolBrigadeElections();

// Place a movement proposal for a brigade, information HC officers and giving them
// opportunity to block the movement.
extern void chatBrigadeMoveProposal(UINT32 brigadeID, const struct FULL_OBJ_ID& destination, ChatHandle byHandle);

// Take a player out of the OIC position of the specified brigade
extern void chatRemoveOICStatus(UINT32 brigadeID, const ChatHandle& handle);

// Let people know that a unit has been moved
class HCUnit;
extern void chatAnnounceBrigadeMove(const HCUnit& unit, const FULL_OBJ_ID& origin, const FULL_OBJ_ID& destination, ChatHandle byHandle);

// Request an attack/clear vote
extern void chatBrigadeObjectiveProposal(class Player* player, FULL_OBJ_ID destination, bool attack);

// Announce placement of an AO
extern void chatBrigadeObjectivePlaced(class Player* player, const FULL_OBJ_ID& target, WWII_SIDE side);

// Announce removal of an AO
extern void chatBrigadeObjectiveRemoved(class Player* player, const FULL_OBJ_ID& target, WWII_SIDE side);

// Handle the arrival of an Attack Order result
extern void chatBrigadeAttackOrderResponse(struct AttackOrder& incoming);

