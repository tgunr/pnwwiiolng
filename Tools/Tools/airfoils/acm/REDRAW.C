#include "pm.h"

int
redrawItem(drawnItem * p)
{
	p->redraw = 4;
	return 0;
}

_BOOL
isRedrawRequired(drawnItem * p)
{
	_BOOL      result = FALSE;

	if (p->redraw > 0) {
		--p->redraw;
		result = TRUE;
	}
	return result;
}

void
initializeDrawnItemController(drawnItemController * c)
{
	c->head = NULL;
}

void
addDrawnItem(drawnItemController * c, drawnItem * p)
{
	p->next = c->head;
	p->redraw = 0;
	c->head = p;
}

void
redrawAllItems(drawnItemController * c)
{
	drawnItem *p;

	for (p = c->head; p; p = p->next) {
		redrawItem(p);
	}
}
