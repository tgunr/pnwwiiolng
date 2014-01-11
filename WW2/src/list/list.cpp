#include <syscfg.h> // Path is project specific.
//

#include "list/list.h"
#include <string.h>

void* listGetItem(LISTHEADER* const list)
{
  if (list == nullptr)
  {
    return(nullptr);
  }

  LISTHEADER* const head = list->next;
  if (head != nullptr)
  {
    list->next = head->next;
    if (list->next != nullptr)
    {
      ((LISTHEADER*)list->next)->prev = list;
    }
  }

  return(head);
}

LOGICAL listAddItem(LISTHEADER* list, LISTHEADER* part)
{
LISTHEADER*  head;

  if (list == nullptr || part == nullptr || list == part)
  {
    return(FALSE);
  }

  head = list->next;
  part->next = head;
  part->prev = list;
  list->next = part;
  if (head != nullptr)
  {
    head->prev = part;
  }

  return(TRUE);
}

LOGICAL listAddItemAfter(LISTHEADER* part, LISTHEADER* after)
{
  if (part == nullptr || after == nullptr || part == after)
  {
    return(FALSE);
  }

  after->next = part->next;
  if (after->next != nullptr)
  {
    ((LISTHEADER*)after->next)->prev = after;
  }
  part->next = after;
  after->prev = part;

  return(TRUE);
}

LOGICAL listAddItemBefore(LISTHEADER* part, LISTHEADER* before)
{
  if (part == nullptr || before == nullptr || part == before)
  {
    return(FALSE);
  }

  before->prev = part->prev;
  if (before->prev != nullptr)
  {
    ((LISTHEADER*)before->prev)->next = before;
  }
  part->prev = before;
  before->next = part;

  return(TRUE);
}

LOGICAL listDeleteItem(LISTHEADER* part)
{
  if (part == nullptr)
  {
    return(FALSE);
  }

	if ((LISTHEADER*)part->prev != nullptr)
	{
		((LISTHEADER*)part->prev)->next = part->next;
	}

  if (part->next != nullptr)
  {
    ((LISTHEADER*)part->next)->prev = part->prev;
  }

// i can't believe this wasn't in here already
	part->prev = part->next = nullptr;

  return(TRUE);
}

LOGICAL listInitList(LISTHEADER* free, int numitems, int itemsize)
{
int   i;
LISTHEADER*  part;

  if (free == nullptr || free->next == nullptr || numitems <= 0 || itemsize <= 0)
  {
    return(FALSE);
  }

  part = free->next;
  free->next = nullptr;
  for (i=0; i < numitems; i++)
  {
    memset(part, 0, itemsize);
    listAddItem(free, part);
    part = (LISTHEADER*)((char*)part + itemsize);
  }

  return(TRUE);
}
