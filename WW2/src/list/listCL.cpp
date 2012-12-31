#include <syscfg.h> // Path is project specific.
// This module provides circular linked list managment routines.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin
// Supporting Programmer: Russell Mirabelli

#include "dbug/dbug.h"

#include "mem/mem.h"
#include "list/listCL.h"

#define _FLX _FLA

static inline LOGICAL _listAddItemAfter(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node);

/*
static void breakHere(void)
{
	return;
}
*/

static inline LOGICAL
_listAddItemAfter(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node)
{
	try
	{
		if ( list == NULL ) throw "NULL list" ;
		if ( node == NULL ) throw "NULL node" ;
		if ( inpoint == NULL ) throw "NULL inpoint" ;
		if ( *list == NULL ) throw "empty list" ;
		if ( node == inpoint ) throw "node == inpoint" ;
		if ( inpoint->next == node ) throw "node already inplace" ;
		if ( inpoint->prev == node ) throw "node already present" ;
	}
	catch ( const char* error )
	{
		dbugLog(_FLX, "can't add item %p after node %p to list %p: %s", node, inpoint, list, error) ;
		return FALSE ;
	}

	node->next = inpoint->next;

	if (inpoint->next != NULL)
		inpoint->next->prev = node;			// insert into list
	else
		(*list)->prev = node;

	inpoint->next = node;
	node->prev = inpoint;

	return(TRUE);
}

static inline LOGICAL
_listAddItemBefore(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node)
{
	try
	{
		if ( list == NULL ) throw "NULL list" ;
		if ( node == NULL ) throw "NULL node" ;
		if ( inpoint == NULL ) throw "NULL inpoint" ;
		if ( *list == NULL ) throw "empty list" ;
		if ( node == inpoint ) throw "node == inpoint" ;
		if ( inpoint->prev == node ) throw "node already inplace" ;
		if ( inpoint->next == node ) throw "node already present" ;
		if ( inpoint->prev == NULL ) throw "NULL prev" ;
	}
	catch ( const char* error )
	{
		dbugLog(_FLX, "can't add item %p before node %p to list %p: %s", node, inpoint, list, error) ;
		return FALSE ;
	}

	if (*list == NULL || *list == inpoint)
		return listCLPushItem(list, node);

	node->next = inpoint;
	node->prev = inpoint->prev;
	inpoint->prev = node;

	if (node->prev->next != NULL)
		node->prev->next = node;
	else
		*list = node;

	return(TRUE);
}

LISTHEADER*
listCLPopItem( LISTHEADER** list )
{
	LISTHEADER* newNode = *list ;

	if (newNode != NULL)
	{
		*list = newNode->next;

		if (newNode->next != NULL)
		{
			newNode->next->prev = newNode->prev;
		}

		newNode->prev = newNode;
		newNode->next = NULL;
	}

	return newNode;
}

LOGICAL
listCLPushItem(LISTHEADER** list, LISTHEADER* node)
{
	try
	{
		if ( list == NULL ) throw "NULL list" ;
		if ( node == NULL ) throw "NULL node" ;
		if ( node == *list ) throw "already in list" ;
	}
	catch ( const char* error )
	{
		dbugLog(_FLX, "can't add item %p to list %p: %s", node, list, error) ;
		return FALSE ;
	}

	if ( node->prev != NULL && node->prev->next == node )
	{
		dbugLog(_FLX,"illegal pointer chain node %p list %p!",node,list);
		return FALSE;
	}

	node->prev = node;
	node->next = NULL;

	if ( (node->next = (*list) ) != NULL)
	{
		node->prev = (*list)->prev;
		(*list)->prev = node;
	}
#ifdef EXTREME_DEBUG
	else
	{
		dbugWrite(_FLA,"pushing to empty list %x node %x\n"
					,(UINT32) list, (UINT32) node);
	}
#endif

	*list = node;

	return TRUE;
}

LOGICAL listCLAppendList(LISTHEADER** dest, LISTHEADER** source)
{
LISTHEADER* temp;

	if (dest == NULL ||  source == NULL )
	{
		dbugLog(_FLX,"invalid params to listCLAppendList dest %p  source %p "
					, dest, source);
		return FALSE;
	}

	// dont append null lists
	if (*source == NULL)
		return FALSE;

	// destination is empty just copy the pointer
	if (*dest == NULL)
		*dest = *source;

	if ((*dest)->prev == NULL || (*source)->prev == NULL)
	{
		dbugLog(_FLX, "illegal NULL prev pointer for dest %p or source %p"
			, (*dest)->prev, (*source)->prev);
	}

	temp = (*dest)->prev;
	(*dest)->prev->next = (*source);
	(*dest)->prev = (*source)->prev;
	(*source)->prev = temp;

	*source = NULL;

	return TRUE;
}

LOGICAL
listCLPushItemEnd(LISTHEADER** list, LISTHEADER* node)
{
	try
	{
		if ( list == NULL ) throw "NULL list" ;
		if ( node == NULL ) throw "NULL node" ;
		if ( node == *list ) throw "already in list" ;
	}
	catch ( const char* error )
	{
		dbugLog(_FLX, "can't add item %p to list %p: %s", node, list, error) ;
		return FALSE ;
	}

	if (*list == NULL)
		return listCLPushItem( list, node);
	else
		return _listAddItemAfter(list,(*list)->prev,node);
}

LISTHEADER
*listCLExtractItem(LISTHEADER** list, LISTHEADER* node)
{
	try
	{
		if ( list == NULL ) throw "NULL list" ;
		if ( *list == NULL ) throw "*NULL list" ;
		if ( node == NULL ) throw "NULL item" ;
		if ( node->prev == NULL ) throw "NULL prev" ;

		if ( *list != node )
		{
			if ( node->prev == node ) throw "invalid backref" ;
			if ( node->prev->next == NULL ) throw "invalid fwdref" ;
		}
	}
	catch ( const char* error )
	{
		dbugLog(_FLX, "cant extract item %p to list %p: %s", node, list, error) ;
		return NULL;
	}

	// This shouldn't be happening, but somehow, somewhere, it is.
	if ( node->next == node )
		node->next = NULL ;

	if (node->prev->next == NULL || *list == node)
	{
		*list = node->next;
		if (*list != NULL)
			(*list)->prev = node->prev;
	}
	else
	{
		node->prev->next = node->next;

		if (node->next != NULL)
			node->next->prev = node->prev;
		else
			(*list)->prev = node->prev;
	}


	node->prev = node;
	node->next = NULL;

	return node;
}

LOGICAL listCLAddItemAfter(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node)
{
	return _listAddItemAfter(list, inpoint, node);

}

LOGICAL listCLAddItemBefore( LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node)
{
	return _listAddItemBefore(list, inpoint, node);
}

LOGICAL listCLMoveItemAfter(LISTHEADER* inpoint, LISTHEADER* node)
{
	if (inpoint == NULL || node == NULL || inpoint == node)
	{
		dbugLog(_FLX, "cant move item %p after node %p",node,inpoint);
		return FALSE;
	}

	node->prev->next = node->next;		// extract from list
	node->next->prev = node->prev;

	node->next = inpoint->next;
	inpoint->next->prev = node;			// insert into list
	inpoint->next = node;
	node->prev = inpoint;

	return(TRUE);
}

LOGICAL listCLMoveItemBefore(LISTHEADER* inpoint, LISTHEADER* node)
{
	if (inpoint == NULL || node == NULL || inpoint == node)
	{
		dbugLog(_FLX, "cant move item %p before node %p",node,inpoint);
		return FALSE;
	}

	node->prev->next = node->next;		// extract from list
	node->next->prev = node->prev;


	node->prev = inpoint->prev;
	inpoint->prev->next = node;			// insert into list
	inpoint->prev = node;
	node->next = inpoint;

	return(TRUE);
}

void* listCLInit( SINT32 numitems, size_t itemsize )
{
int   i;
LISTHEADER*  part, *nextPart;

	if ( numitems <= 0 || itemsize <= 0)
	{
		dbugLog(_FLX, "illegal params num %d size %zu",numitems,itemsize);
		return(FALSE);
	}

	LISTHEADER* free = (LISTHEADER*)memCalloc(itemsize, numitems);
	if (free == NULL)
	{
		dbugLog(_FLX,"couldnt allocate memory for new list!!!");
		return NULL;
	}

	part = free;
	part->prev = part;
	part->next = NULL;					// make the list circular

	for (i = 1 ; i < numitems ; ++i)		// init and add the rest of the nodes
	{
		nextPart = (LISTHEADER*)((char*)part + itemsize);
		_listAddItemAfter(&free, part, nextPart);
		part = nextPart;
	}

	return free;
}

void* listCLInitStatic( void* free, SINT32 numitems, size_t itemsize )
{
int   i;
LISTHEADER*  part, *nextPart, *list;

	if ( numitems <= 0 || itemsize <= 0)
	{
		dbugLog(_FLX, "illegal params num %d size %zu",numitems,itemsize);
		return(FALSE);
	}

	if (free == NULL)
	{
		dbugLog(_FLX,"NULL memory for new list!!!");
		return NULL;
	}

	list = part = (LISTHEADER*)free;
	part->prev = part;
	part->next = NULL;					// make the list circular

	for (i = 1 ; i < numitems ; ++i)		// init and add the rest of the nodes
	{
		nextPart = (LISTHEADER*)((char*)part + itemsize);
		_listAddItemAfter(&list, part, nextPart);
		part = nextPart;
	}

	return free;
}

#undef _FLX
