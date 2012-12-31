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

#include <stdlib.h>

#include "teul/teulCLList.h"
#include "strt/strtServerTypes.h"
#include "cnvy/cnvyConvoyList.h"

//Creates an spawn vehicle list of numItems size
CONVOYSTRUCT* cnvyCreateConvoyList(UINT32 numItems)
{
CONVOYSTRUCT* list;
UINT32 i;

	if (numItems == 0)
		return NULL;

	list = (CONVOYSTRUCT*)listInitCList(numItems, sizeof(CONVOYSTRUCT));

	//Order the list
	for (i=0; i<numItems; i++)
	{
		list[i].nextEvent= i;
	}

	return list;
}

//Removes the first item from an convoyList
CONVOYSTRUCT* cnvyRemoveFirstConvoy(CONVOYSTRUCT** convoyList)
{
CONVOYSTRUCT* current = NULL;

	current = (CONVOYSTRUCT*) listPopItem((LISTHEADER **) convoyList);

	return current;
}

//Inserts a nextEvent into the convoyList
LOGICAL cnvyInsertConvoy(CONVOYSTRUCT** convoyList, CONVOYSTRUCT* convoy)
{
CONVOYSTRUCT * current;
	if ((convoyList == NULL) || (convoy == NULL))
		return FALSE;

	if ((*convoyList) == NULL)
	{
		//Its the first convoy
		*convoyList = convoy;
	}
	else if ((*convoyList)->nextEvent >= convoy->nextEvent)
	{
		//Belongs at the head of the list
		listPushItem((LISTHEADER **) convoyList, (LISTHEADER*) convoy);
	}
	else if ((*convoyList)->prev->nextEvent < convoy->nextEvent)
	{
		//Belongs at the end of the list
		listAddItemAfter((LISTHEADER **) convoyList, (LISTHEADER*) (*convoyList)->prev, (LISTHEADER*) convoy);
	}
	else
	{
		current = *convoyList;

		//Find the spot where it belongs on the list
		while ((current!=NULL) && (current->nextEvent < convoy->nextEvent))
			current=current->next;
		if (current!=NULL)
			listAddItemBefore((LISTHEADER**) convoyList, (LISTHEADER*) current, (LISTHEADER*) convoy);
	}

	return TRUE;
}

//Deletes a nextEvent from the convoyList
CONVOYSTRUCT* cnvyDeleteConvoy(CONVOYSTRUCT** convoyList, CONVOYSTRUCT* convoy)
{
	if ((convoyList == NULL) || (*convoyList == NULL) || (convoy == NULL))
		return NULL;

	listExtractItem((LISTHEADER **) convoyList, (LISTHEADER*) convoy);

	return convoy;
}

//Finds someone in the convoy list
CONVOYSTRUCT* cnvyFindConvoy(CONVOYSTRUCT* convoyList, UINT16 nextEvent)
{
CONVOYSTRUCT* current;

	current = convoyList;

	while ((current != NULL) && (current->nextEvent != nextEvent))
		current = current->next;

	if ((current!=NULL) && (current->nextEvent == nextEvent))
		return current;
	else
		return NULL;
}

//Prints an convoy list
LOGICAL cnvyPrintConvoys(CONVOYSTRUCT* convoyList)
{
CONVOYSTRUCT* current;

	current = convoyList;
	while (current != NULL)
	{
		fprintf(stderr,"convoy: 0x%x\n",current->nextEvent);
		current = current->next;
	}

	return TRUE;
}

