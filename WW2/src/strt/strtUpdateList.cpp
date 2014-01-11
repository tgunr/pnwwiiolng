#include <syscfg.h> // Path is project specific.
// Date: Fri Mar 23 16:14:11 2001
//
// List management routines for a strat push persona updates.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Eric "Meat" Hall

#include "strt/strtTypes.h"
#include "teul/teulCLList.h"

#include "strt/strtUpdateList.h"
#include "strt/strtEventQueue.h"

//Creates an stratUpdate list of numItems size
STRAT_UPDATE* strtCreateStratUpdateList(UINT32 numItems)
{
	if (numItems == 0)
		return nullptr;

	STRAT_UPDATE* list = (STRAT_UPDATE*)listInitCList(numItems, sizeof(STRAT_UPDATE));

	//Order the list
	for (UINT32 i=0; i<numItems; i++)
	{
		list[i].updateTime = i;
	}

	return list;
}

//Removes the first item from an stratUpdatelist
STRAT_UPDATE* strtRemoveFirstStratUpdate(STRAT_UPDATE** stratUpdateList)
{
	return (STRAT_UPDATE*) listPopItem((LISTHEADER **) stratUpdateList);
}

//Inserts a updateTime into the stratUpdatelist
LOGICAL
strtInsertStratUpdate(STRAT_UPDATE** stratUpdateList, STRAT_UPDATE* stratUpdate)
{
	if ((stratUpdateList == nullptr) || (stratUpdate == nullptr))
		return FALSE;

	if (*stratUpdateList == nullptr)
	{
		//Its the first stratUpdate
		*stratUpdateList = stratUpdate;
	}
	else if ((*stratUpdateList)->updateTime >= stratUpdate->updateTime)
	{
		//Belongs at the head of the list
		listPushItem((LISTHEADER **) stratUpdateList, (LISTHEADER*) stratUpdate);
	}
	else if ((*stratUpdateList)->prev->updateTime < stratUpdate->updateTime)
	{
		//Belongs at the end of the list
		listAddItemAfter((LISTHEADER **) stratUpdateList, (LISTHEADER*) (*stratUpdateList)->prev, (LISTHEADER*) stratUpdate);
	}
	else
	{
		STRAT_UPDATE* current = *stratUpdateList;

		//Find the spot where it belongs on the list
		while ((current!=nullptr) && (current->updateTime < stratUpdate->updateTime))
			current=current->next;
		if (current!=nullptr)
			listAddItemBefore((LISTHEADER **) stratUpdateList, (LISTHEADER*) current, (LISTHEADER*) stratUpdate);
	}

	return TRUE;
}

//Deletes a updateTime from the stratUpdatelist
STRAT_UPDATE*
strtDeleteStratUpdate(STRAT_UPDATE** stratUpdateList, STRAT_UPDATE* stratUpdate)
{
	if ((stratUpdateList == nullptr) || (*stratUpdateList == nullptr) || (stratUpdate == nullptr))
		return nullptr;

	listExtractItem((LISTHEADER **) stratUpdateList, (LISTHEADER*) stratUpdate);

	return stratUpdate;
}

//Prints an stratUpdate list
void
strtPrintStratUpdates(STRAT_UPDATE* stratUpdateList)
{
	STRAT_UPDATE* current = stratUpdateList;
	while (current != nullptr)
	{
		fprintf(stderr,"stratUpdate: %d\n",current->updateTime);
		current = current->next;
	}
}
