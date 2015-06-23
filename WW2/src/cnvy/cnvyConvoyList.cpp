#include <syscfg.h> // Path is project specific.
// Created: Thu Feb  8 12:05:58 2001
//
// Convoy list management routines.  Convoy lists need to be sorted by
// nextEvent, in that the soonest arriving convoy is on the top of
// the list.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson

#include "teul/teulCLList.h"
#include "cnvy/cnvyMain.h"

//Creates an spawn vehicle list of numItems size
CONVOYSTRUCT* cnvyCreateConvoyList(UINT32 numItems)
{
	ABORT_IF(numItems == 0);

	return (CONVOYSTRUCT*)listInitCList(numItems, sizeof(CONVOYSTRUCT));
}

//Removes the first item from an convoyList
CONVOYSTRUCT* cnvyRemoveFirstConvoy(CONVOYSTRUCT** convoyList)
{
	return (CONVOYSTRUCT*) listPopItem((LISTHEADER **) convoyList);
}

//Inserts a nextEvent into the convoyList
LOGICAL cnvyInsertConvoy(CONVOYSTRUCT** convoyList, CONVOYSTRUCT* convoy)
{
	if ((convoyList == nullptr) || (convoy == nullptr))
		return FALSE;

	if ((*convoyList) == nullptr)
	{
		//Its the first convoy
		*convoyList = convoy;
	}
	else if ((*convoyList)->arrivalTime >= convoy->arrivalTime)
	{
		//Belongs at the head of the list
		listPushItem((LISTHEADER **) convoyList, (LISTHEADER*) convoy);
	}
	else if ((*convoyList)->prev->arrivalTime < convoy->arrivalTime)
	{
		//Belongs at the end of the list
		listAddItemAfter((LISTHEADER **) convoyList, (LISTHEADER*) (*convoyList)->prev, (LISTHEADER*) convoy);
	}
	else
	{
		CONVOYSTRUCT* current = *convoyList;

		//Find the spot where it belongs on the list
		while ((current != nullptr) && (current->arrivalTime < convoy->arrivalTime))
			current = current->next;
		if (current != nullptr)
			listAddItemBefore((LISTHEADER**) convoyList, (LISTHEADER*) current, (LISTHEADER*) convoy);
	}

	return TRUE;
}

//Deletes a nextEvent from the convoyList
CONVOYSTRUCT* cnvyDeleteConvoy(CONVOYSTRUCT** convoyList, CONVOYSTRUCT* convoy)
{
	if ((convoyList == nullptr) || (*convoyList == nullptr) || (convoy == nullptr))
		return nullptr;

	listExtractItem((LISTHEADER **) convoyList, (LISTHEADER*) convoy);

	return convoy;
}
