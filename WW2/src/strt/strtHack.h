#pragma once
// Date: Thu May 17 14:04:34 2001
//
// Necessary code additions required to get bridges working in the strat
// system.  However, due to thier hackish nature, we don't want other things
// (namely the mmHost) to use it without extreme caution
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Takes the cp list and runs through it all, turning each standalone
//	bridge CP into one with a faked facility and object to take damage
extern LOGICAL strtFakeBridges(CPStruct* cpList);
