#pragma once
// Date: Tue Apr  4 23:05:50 2000
//
// These are all the input/output functions of the Strat System.  This
// includes but is not limited to all import and all printing functions.
//
// This may be a development only collection of tools.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "strt/strtProtos.h"
#include "strt/strtDTL.h"

// -------------------------------------- Non-Exposed Functions

class CPStruct* strtImportStratData();

// -------------------------------------- Exposed Functions
//tolua_begin

//This event structure allows us to simulate events

//This function encapsulates a big case statement that returns the
//string that correstponds to the country owner referenced by the
//COUNTRY parameter
extern const char* strtPrintCountry(WWII_COUNTRY country);

//This function prints out the enumerated Facilty Type that's passed in
extern const char* strtPrintFacilityType(FACILITY_TYPE facilityType);

//create an array (using malloc) whose elements are the lines of file pointed to by stream
void strtReleaseSupercells();

//tolua_end

