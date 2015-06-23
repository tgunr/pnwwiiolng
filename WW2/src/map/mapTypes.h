#pragma once
// Created: Thu Mar 30 15:50:16 2000
//
// These are all the data types that wll be utilized by the Strat System.
// All structures and enumerated types should go here, as well as any defines
// that impact the entire system.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"

struct CP_INFO
{
	FULL_OBJ_ID		cp;
	WWII_COUNTRY	owner;
	UINT32			cpFlags;
};

