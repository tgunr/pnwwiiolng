#pragma once
// Created: Thu Mar 05 15:50:16 2001
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

#include "parse/parseIO.h"
#include "strt/strtWOR.h"

// load module arg spec of parameter checking
#define	LOAD_MODULE_SPEC {T_STRING,PN_MAX_PATH},{T_FLOAT,4},{T_INT,4}

// maximum number of load module args
#define MAX_LM_ARG	3

// parse the wor level file with (world) callbacks
bool parseWor(const char* levelfile, PARSE_CALLBACK* world);

// clean up the memory
void parseWorKill();

// find the cp value for objName
CP_PARAMS* parseGetCPValues(const std::string& objName);

// find the facility value for objName
FACILITY_PARAMS* parseGetFacilityValues(const std::string& objName);

// find the building value for objName
BUILDING_PARAMS* parseGetBuildingValues(const std::string& objName);

// find the depot link value for objName
DEPOT_LINK_PARAMS* parseGetDepotLinkValues(const std::string& objName);
