#pragma once
// This module provides Circular Linked List utility functions.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Marty Poulin

#ifndef FRONT_END
struct LISTHEADER
{
	LISTHEADER* prev;
	LISTHEADER* next;

};
#else
#include "list/list.h"
#endif

#ifdef WALK_LISTS

// list walker to verify CLList integity
// maxItems is a high enough number to allow operation but low enough to find endless loops
// printType is the ERROR_TYPE to print errors with, ERR_FATAL combined with FORCE_ABORT
//	will provide a core or break.
static inline LOGICAL listWalk(LISTHEADER** list, SINT32 maxItems, ERROR_TYPE printType)
{
LISTHEADER* node;
SINT32 i;

	for(	node = *list, i=0
		; i< maxItems
		; node = node->next, i++)
	{
		if (node == NULL)
			return TRUE;

		if (	node->prev == NULL
			|| (*list == node && node->prev->next != NULL)
			|| (*list != node && node->prev->next != node) )
		{
			dbugLog(_FL, printType, "illegal list node %d, prev %d"
						, (UINT32) node,(UINT32) node->prev);
		}
	}

	dbugLog(_FL, printType, "list %d has no end or is over %d nodes"
				, (UINT32) list, maxItems);
	return FALSE;

}

#else

// when complied out returns TRUE
#define listWalk(x, y, z)	TRUE

#endif

//listPopItem removes the head of the list that has been passed to it
//	<list> is a handle to the list you wish to pop the top item off of
//	the function returns a pointer to the previous head of the list
//
//	This function is often used to pull the first item off of a pre-allocated
//	list of free items.  The item is then initialized and placed on another
//	list.
extern	LISTHEADER* listCLPopItem( LISTHEADER** list );

//listExtractItem extracts a node from a CList
//	<list> is a handle to the list with which we are working
//	<node> is a pointer to the item we wish to extract
//	the function returns a pointer to the node extracted
extern 	LISTHEADER* listCLExtractItem(LISTHEADER** list, LISTHEADER* node);

//listPushItem puts an item on the head of a list
//	<list> is a handle to the list we are working with
//	<node> is the node to be placed on the list
extern 	LOGICAL listCLPushItem(LISTHEADER** list, LISTHEADER* node);

// same as above, buts puts it at the end of the list
extern	LOGICAL listCLPushItemEnd(LISTHEADER** list, LISTHEADER* node);

//listAddItemBefore adds an item to a list* before* a certain node
//	<inpoint> is the node that you want to add the item before
//	<node> is the item you wish to add to the list
extern	LOGICAL listCLAddItemBefore( LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node);

//listAddItemAfter adds an item to a list* after* a certain node
//	<inpoint> is the node that you want to add the item after
//	<node> is the item you wish to add to the list
extern	LOGICAL listCLAddItemAfter(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node);

//listMoveItemAfter works like listAddItemAfter, except that it takes an
//	item that is* already* part of another list
//	<inpoint> is the node that you want to move the item after
//	<node> is the node off of another list that you want to move
extern	LOGICAL listCLMoveItemAfter(LISTHEADER* inpoint, LISTHEADER* node);

//listMoveItemBefore works like listAddItemBefore, except that it takes an
//	item that is* already* part of another list
//	<inpoint> is the node that you want to move the item before
//	<node> is the node off of another list that you want to move
extern	LOGICAL listCLMoveItemBefore(LISTHEADER* inpoint, LISTHEADER* node);

//listInitCList allocates a CL list for use with all the CL list functions
//	<numitems> is the size of the list you want
//	<itemsize> is the size of the individual items
//	typical use:
//		myItemList = listInitCList(100, sizeof(ITEMTYPE));
extern	void* listCLInit( SINT32 numitems, size_t itemsize );

//Last time I heard, you don't want to use this function
extern	void* listCLInitStatic( void* free, SINT32 numitems, size_t itemsize );

// append source list to dest list (sets* source to NULL)
extern 	LOGICAL listCLAppendList(LISTHEADER** dest, LISTHEADER** source);
