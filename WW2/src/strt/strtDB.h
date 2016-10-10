#pragma once
// Date: Thu May 11 14:50:26 2000
//
// This is the header file outlining the Strat System's connection to the
// database.  The Strat System needs to be able to back up its current
// state of the game world, and it needs to be able to rebuild part or all
// of its in game data from the database.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Tools for opening and closing the DB connection
extern bool strtInitDB();
extern void strtCloseDB();

extern void strtdbInitSupercells();

extern void strtLoadTOEsAndHCUnits();
