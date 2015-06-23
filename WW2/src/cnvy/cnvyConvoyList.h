#pragma once
// Created: Thu Feb  8 12:06:01 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

//Creates an spawn vehicle list of numItems size
struct CONVOYSTRUCT* cnvyCreateConvoyList(UINT32 numItems);

//Removes the first item from an convoyList
CONVOYSTRUCT* cnvyRemoveFirstConvoy(CONVOYSTRUCT** convoyList);

//Inserts a arrivalTime into the convoyList
LOGICAL cnvyInsertConvoy(CONVOYSTRUCT** convoyList, CONVOYSTRUCT* convoy);

//Deletes a arrivalTime from the convoyList
CONVOYSTRUCT* cnvyDeleteConvoy(CONVOYSTRUCT** convoyList, CONVOYSTRUCT* convoy);
