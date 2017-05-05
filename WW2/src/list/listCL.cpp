#include <syscfg.h> // Path is project specific.
// This module provides circular linked list management routines.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin
// Supporting Programmer: Russell Mirabelli

#include "mem/mem.h"
#include "list/listCL.h"

#define _FLX _FLA

static inline LOGICAL _listAddItemAfter(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node);

static inline LOGICAL
_listAddItemAfter(LISTHEADER** list, LISTHEADER* inpoint, LISTHEADER* node)
{
	const char* reason = nullptr;
	if ( PN_UNLIKELY(list == nullptr) )
		reason = "NULL list";
	else if ( PN_UNLIKELY(node == nullptr) )
		reason = "NULL node";
	else if ( PN_UNLIKELY(inpoint == nullptr) )
		reason = "NULL inpoint";
	else if ( PN_UNLIKELY(*list == nullptr) )
		reason = "empty list";
	else if ( PN_UNLIKELY(node == inpoint) )
		reason = "node == inpoint";
	else if ( PN_UNLIKELY(inpoint->next == node) )
		reason = "node already inplace";
	else if ( PN_UNLIKELY(inpoint->prev == node) )
		reason = "node already present";
	if ( PN_UNLIKELY(reason != nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "can't add item %p after node %p to list %p: %s", node, inpoint, list, reason);
		return FALSE;
	}

	node->next = inpoint->next;

	if (inpoint->next != nullptr)
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
	const char* reason = nullptr;
	if (PN_UNLIKELY(list == nullptr))
		reason = "NULL list";
	else if (PN_UNLIKELY(node == nullptr))
		reason = "NULL node";
	else if (PN_UNLIKELY(inpoint == nullptr))
		reason = "NULL inpoint";
	else if (PN_UNLIKELY(*list == nullptr))
		reason = "empty list";
	else if (PN_UNLIKELY(node == inpoint))
		reason = "node == inpoint";
	else if (PN_UNLIKELY(inpoint->next == node))
		reason = "node already inplace";
	else if (PN_UNLIKELY(inpoint->prev == node))
		reason = "node already present";
	if (PN_UNLIKELY(reason != nullptr))
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "can't add item %p before node %p to list %p: %s", node, inpoint, list, reason);
		return FALSE;
	}

	if (*list == nullptr || *list == inpoint)
		return listCLPushItem(list, node);

	node->next = inpoint;
	node->prev = inpoint->prev;
	inpoint->prev = node;

	if (node->prev != nullptr)
		node->prev->next = node;
	else
		*list = node;

	return(TRUE);
}

LISTHEADER*
listCLPopItem( LISTHEADER** list )
{
	LISTHEADER* newNode = *list;

	if (newNode != nullptr)
	{
		*list = newNode->next;

		if (newNode->next != nullptr)
		{
			newNode->next->prev = newNode->prev;
		}

		newNode->prev = newNode;
		newNode->next = nullptr;
	}

	return newNode;
}

LOGICAL
listCLPushItem(LISTHEADER** list, LISTHEADER* node)
{
	const char* reason = nullptr;
	if ( PN_UNLIKELY(list == nullptr) )
		reason = "NULL list";
	else if ( PN_UNLIKELY(node == nullptr) )
		reason = "NULL node";
	else if ( PN_UNLIKELY(node == *list) )
		reason = "already in list";
	if ( PN_UNLIKELY(reason != nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "can't add item %p to list %p: %s", node, list, reason);
		return FALSE;
	}

	if ( node->prev != nullptr && node->prev->next == node )
	{
		WriteLog(LOG_TEUL, LOG_ABORT,"illegal pointer chain node %p list %p!",node,list);
		return FALSE;
	}

	node->prev = node;
	node->next = nullptr;

	if ( (node->next = (*list) ) != nullptr)
	{
		node->prev = (*list)->prev;
		(*list)->prev = node;
	}
#ifdef EXTREME_DEBUG
	else
	{
		dbugWrite(_FLA,"pushing to empty list %p node %p\n", list, node);
	}
#endif

	*list = node;

	return TRUE;
}

LOGICAL listCLAppendList(LISTHEADER** dest, LISTHEADER** source)
{
	LISTHEADER* temp;

	if (dest == nullptr ||  source == nullptr )
	{
		WriteLog(LOG_TEUL, LOG_ABORT,"invalid params to listCLAppendList dest %p  source %p "
					, dest, source);
		return FALSE;
	}

	// dont append null lists
	if (*source == nullptr)
		return FALSE;

	// destination is empty just copy the pointer
	if (*dest == nullptr)
		*dest = *source;

	if (PN_UNLIKELY((*dest)->prev == nullptr || (*source)->prev == nullptr))
		LogAbort("Illegal null prev pointer for dest %p or source %p", (*dest)->prev, (*source)->prev);

	temp = (*dest)->prev;
	(*dest)->prev->next = (*source);
	(*dest)->prev = (*source)->prev;
	(*source)->prev = temp;

	*source = nullptr;

	return TRUE;
}

LOGICAL
listCLPushItemEnd(LISTHEADER** list, LISTHEADER* node)
{
	const char* reason = nullptr;
	if ( PN_UNLIKELY(list == nullptr) )
		reason = "NULL list";
	else if ( PN_UNLIKELY(node == nullptr) )
		reason = "NULL node";
	else if ( PN_UNLIKELY(node == *list) )
		reason = "already in list";
	if ( PN_UNLIKELY(reason != nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "can't add item %p to list %p: %s", node, list, reason);
		return FALSE;
	}

	if (*list == nullptr)
		return listCLPushItem( list, node);
	else
		return _listAddItemAfter(list,(*list)->prev,node);
}

LISTHEADER
*listCLExtractItem(LISTHEADER** list, LISTHEADER* node)
{
	const char* reason = nullptr;
	if ( PN_UNLIKELY(list == nullptr) )
		reason = "NULL list";
	else if ( PN_UNLIKELY(*list == nullptr) )
		reason = "*NULL list";
	else if ( PN_UNLIKELY(node == nullptr) )
		reason = "NULL item";
	else if ( PN_UNLIKELY(node->prev == nullptr) )
		reason = "NULL prev";
	else if ( *list != node )
	{
		if ( PN_UNLIKELY(node->prev == node) )
			reason = "invalid backref";
		if ( PN_UNLIKELY(node->prev->next == nullptr) )
			reason = "invalid fwdref";
	}
	if ( PN_UNLIKELY(reason != nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "cant extract item %p to list %p: %s", node, list, reason);
		return nullptr;
	}

	// This shouldn't be happening, but somehow, somewhere, it is.
	if ( node->next == node )
		node->next = nullptr;

	if (node->prev->next == nullptr || *list == node)
	{
		*list = node->next;
		if (*list != nullptr)
			(*list)->prev = node->prev;
	}
	else
	{
		node->prev->next = node->next;

		if (node->next != nullptr)
			node->next->prev = node->prev;
		else
			(*list)->prev = node->prev;
	}


	node->prev = node;
	node->next = nullptr;

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
	if ( PN_UNLIKELY(inpoint == nullptr || node == nullptr || inpoint == node) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "cant move item %p after node %p",node,inpoint);
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
	if ( PN_UNLIKELY(inpoint == nullptr || node == nullptr || inpoint == node) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "cant move item %p before node %p",node,inpoint);
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

	if ( PN_UNLIKELY(numitems <= 0 || itemsize <= 0) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT, "illegal params num %d size %" PRSizeT "",numitems,itemsize);
		return(FALSE);
	}

	LISTHEADER* free = (LISTHEADER*)memCalloc(itemsize, numitems);
	if ( PN_UNLIKELY(free == nullptr))
	{
		WriteLog(LOG_TEUL, LOG_ABORT,"couldnt allocate memory for new list!!!");
		return nullptr;
	}

	part = free;
	part->prev = part;
	part->next = nullptr;				// make the list circular

	for (i = 1; i < numitems; ++i)		// init and add the rest of the nodes
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
		WriteLog(LOG_TEUL, LOG_ABORT, "illegal params num %d size %" PRSizeT "",numitems,itemsize);
		return(FALSE);
	}

	if ( PN_UNLIKELY(free == nullptr) )
	{
		WriteLog(LOG_TEUL, LOG_ABORT,"NULL memory for new list!!!");
		return nullptr;
	}

	list = part = (LISTHEADER*)free;
	part->prev = part;
	part->next = nullptr;				// make the list circular

	for (i = 1; i < numitems; ++i)		// init and add the rest of the nodes
	{
		nextPart = (LISTHEADER*)((char*)part + itemsize);
		_listAddItemAfter(&list, part, nextPart);
		part = nextPart;
	}

	return free;
}

#undef _FLX
