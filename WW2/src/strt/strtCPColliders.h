#pragma once
// Date: Thu Mar  1 17:01:41 CST 2007
//
// Functions for building and managing the CP Collision tables
// and for detecting if a point is "within" a particular CP.
//
// Copyright (C) PlayNet, Inc 1999-2007 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

// Build the collider set - run once
extern void strtBuildCPColliders() ;

// Given specific meter coordinates, find the closest CP
class CPStruct ;
extern CPStruct* strtFindClosestCP(SINT32 x, SINT32 y) ;

