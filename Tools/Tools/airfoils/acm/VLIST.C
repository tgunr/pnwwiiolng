#include <pm.h>

/*
 *  Get the number of local viewers in the simulation
 */

int
getViewerCount(void)
{
	return vl_count;
}

/*
 *  Add a new view to the head of the viewer list
 */

void
addViewer ( viewer *v )
{
	v->vl_next = vl_head;
	v->vl_prev = NULL;

	if (vl_head == NULL) {
		vl_tail = v;
	}

	vl_head = v;
	++vl_count;
}

/*
 *   Remove a viewer from the viewer list
 *   Returns zero on success; one if the entry wasn't found.
 */

int
removeViewer ( viewer *v )
{
	viewer *cur, *last = NULL;

	for (cur=vl_head; cur != NULL; cur=cur->vl_next) {
		if (cur == v) {
			/* not the first entry on the list ? */
			if (last) {
				last->vl_next = cur->vl_next;
			}
			/* first entry on the list */
			else {
				vl_head = cur->next;
			}
			/* not the last entry on the list */
			if (cur->vl_next) {
				cur->vl_next->vl_prev = last;
			}
			/* last entry on the list */
			else {
				vl_tail = last;
			}
			-- vl_count;
			return 0;
		}
		last = cur;
	}
	return 1;
}
