#pragma once
// Date: Thu Oct  6 20:02:22 CDT 2005
//
// Save World function
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

class CPStruct ;
class DBQuery ;

// Pass the base of the CP list to this function and it will save the world to the database quickly
extern void strtdbSaveWorld(CPStruct* cp, DBQuery& query) ;
