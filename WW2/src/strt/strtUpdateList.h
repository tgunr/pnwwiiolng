#pragma once
// Date: Fri Mar 23 16:14:09 2001
//
// List management routines for a strat push persona updates.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Creates an stratUpdate list of numItems size
extern struct STRAT_UPDATE* strtCreateStratUpdateList(UINT32 numItems);

//Removes the first item from an stratUpdatelist
extern STRAT_UPDATE* strtRemoveFirstStratUpdate(STRAT_UPDATE** stratUpdateList);

//Inserts a updateTime into the stratUpdatelist
extern LOGICAL strtInsertStratUpdate(STRAT_UPDATE** stratUpdateList, STRAT_UPDATE* stratUpdate);

//Deletes a updateTime from the stratUpdatelist
extern STRAT_UPDATE* strtDeleteStratUpdate(STRAT_UPDATE** stratUpdateList, STRAT_UPDATE* stratUpdate);

//Finds someone in the stratUpdate list
extern STRAT_UPDATE* strtFindStratUpdate(STRAT_UPDATE* stratUpdateList, UINT32 updateTime);

//Prints an stratUpdate list
extern void strtPrintStratUpdates(STRAT_UPDATE* stratUpdateList);

