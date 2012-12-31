#pragma once
// Created: Tue Jan 30 12:13:33 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Brian "Stick" Duelm

extern struct FULL_OBJ_ID* populateFullObjID(struct FULL_OBJ_ID* foid, SINT32 sx, SINT32 sy, struct REL_OBJ_ID roid) ;

extern void printMissionSummary(struct MISSION_SUMMARY* summary);

extern void printFULL_OBJ_ID(struct FULL_OBJ_ID* fullObjID);
