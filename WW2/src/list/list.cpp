#include <syscfg.h> // Path is project specific.
//

#include "list/list.h"
#include <string.h>

void* listGetItem(LISTHEADER* const list)
{
  if (list == NULL)
  {
    return(NULL);
  }

  LISTHEADER* const head = list->next;
  if (head != NULL)
  {
    list->next = head->next;
    if (list->next != NULL)
    {
      ((LISTHEADER*)list->next)->prev = list;
    }
  }

  return(head);
}

LOGICAL listAddItem(LISTHEADER* list, LISTHEADER* part)
{
LISTHEADER*  head;

  if (list == NULL || part == NULL || list == part)
  {
    return(FALSE);
  }

  head = list->next;
  part->next = head;
  part->prev = list;
  list->next = part;
  if (head != NULL)
  {
    head->prev = part;
  }

  return(TRUE);
}

LOGICAL listAddItemAfter(LISTHEADER* part, LISTHEADER* after)
{
  if (part == NULL || after == NULL || part == after)
  {
    return(FALSE);
  }

  after->next = part->next;
  if (after->next != NULL)
  {
    ((LISTHEADER*)after->next)->prev = after;
  }
  part->next = after;
  after->prev = part;

  return(TRUE);
}

LOGICAL listAddItemBefore(LISTHEADER* part, LISTHEADER* before)
{
  if (part == NULL || before == NULL || part == before)
  {
    return(FALSE);
  }

  before->prev = part->prev;
  if (before->prev != NULL)
  {
    ((LISTHEADER*)before->prev)->next = before;
  }
  part->prev = before;
  before->next = part;

  return(TRUE);
}

LOGICAL listDeleteItem(LISTHEADER* part)
{
  if (part == NULL)
  {
    return(FALSE);
  }

	if ((LISTHEADER*)part->prev != NULL)
	{
		((LISTHEADER*)part->prev)->next = part->next;
	}

  if (part->next != NULL)
  {
    ((LISTHEADER*)part->next)->prev = part->prev;
  }

// i can't believe this wasn't in here already
	part->prev = part->next = NULL;

  return(TRUE);
}

LOGICAL listInitList(LISTHEADER* free, int numitems, int itemsize)
{
int   i;
LISTHEADER*  part;

  if (free == NULL || free->next == NULL || numitems <= 0 || itemsize <= 0)
  {
    return(FALSE);
  }

  part = free->next;
  free->next = NULL;
  for (i=0 ; i < numitems ; i++)
  {
    memset(part, 0, itemsize);
    listAddItem(free, part);
    part = (LISTHEADER*)((char*)part + itemsize);
  }

  return(TRUE);
}
