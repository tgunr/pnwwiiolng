#pragma once
// This module provides Circular Linked List
// utility functions used in the teul API.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Marty Poulin

#include "list/listCL.h"

//listPopItem removes the head of the list that has been passed to it
//	<list> is a handle to the list you wish to pop the top item off of
//	the function returns a pointer to the previous head of the list
//
//	This function is often used to pull the first item off of a pre-allocated
//	list of free items.  The item is then initialized and placed on another
//	list.
#define listPopItem listCLPopItem

//listExtractItem extracts a node from a CList
//	<list> is a handle to the list with which we are working
//	<node> is a pointer to the item we wish to extract
//	the function returns a pointer to the node extracted
#define listExtractItem listCLExtractItem

//listPushItem puts an item on the head of a list
//	<list> is a handle to the list we are working with
//	<node> is the node to be placed on the list
#define  listPushItem listCLPushItem

//listAddItemBefore adds an item to a list* before* a certain node
//	<inpoint> is the node that you want to add the item before
//	<node> is the item you wish to add to the list
#define listAddItemBefore listCLAddItemBefore

//listAddItemAfter adds an item to a list* after* a certain node
//	<inpoint> is the node that you want to add the item after
//	<node> is the item you wish to add to the list
#define listAddItemAfter listCLAddItemAfter

//listMoveItemAfter works like listAddItemAfter, except that it takes an
//	item that is* already* part of another list
//	<inpoint> is the node that you want to move the item after
//	<node> is the node off of another list that you want to move
#define listMoveItemAfter listCLMoveItemAfter

//listMoveItemBefore works like listAddItemBefore, except that it takes an
//	item that is* already* part of another list
//	<inpoint> is the node that you want to move the item before
//	<node> is the node off of another list that you want to move
#define listMoveItemBefore listCLMoveItemBefore

//listInitCList allocates a CL list for use with all the CL list functions
//	<numitems> is the size of the list you want
//	<itemsize> is the size of the individual items
//	typical use:
//		myItemList = listInitCList(100, sizeof(ITEMTYPE));
#define listInitCList listCLInit

//Last time I heard, you don't want to use this function
#define listInitCListStatic listCLInitStatic
