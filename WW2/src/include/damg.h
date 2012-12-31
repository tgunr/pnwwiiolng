#pragma once
// Created: Thu Mar 15 13:32:26 2001
//
// This module conatains damage handling routines for the host.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "updt/updtVehicleHit.h"

struct VEH_COMP_HIT
{
	UINT64		damage;
	UINT8		compID;
	UINT8		priority;
	UINT8		numHits;
} ;

struct VEH_HIT
{
	SINT32	timeStamp;		//timestamp
	UINT32	shooter;
	UINT32	target;
	UINT32	numComponents;
	VEH_COMP_HIT	compHit[VEH_MAX_OBJ_HIT + 1];
} ;

struct DAMAGE_NODE
{
	DAMAGE_NODE* prev;
	DAMAGE_NODE* next;

	Vehicle::Hit hit ;
} ;

