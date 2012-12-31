#pragma once

struct LISTHEADER
{
	LISTHEADER* prev;
	LISTHEADER* next;
};

//! Pop the first element off the list, if any is available.
//! @return NULL if list is NULL/empty, otherwise returns the first element.
extern void* listGetItem(LISTHEADER* list);
//! Add an item to a list.
extern LOGICAL listAddItem(LISTHEADER* list, LISTHEADER* part);
extern LOGICAL listAddItemAfter(LISTHEADER* part, LISTHEADER* after);
extern LOGICAL listAddItemBefore(LISTHEADER* part, LISTHEADER* before);
//! Remove an element from a list.
extern LOGICAL listDeleteItem(LISTHEADER* part);
extern LOGICAL listInitList(LISTHEADER* free, int numitems, int itemsize);

