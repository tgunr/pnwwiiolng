/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1998  Riley Rainey
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 dated June, 1991.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
 */

#include <pm.h>

#define RESULT_REQUEST_OK 0
#define RESULT_UNABLE     1

/*
 *
 *  t r a n s f e r C o n t r o l R e q u e s t H a n d l e r 
 *
 *  This routine is responsible for handling entity transfer control requests
 *  at the simulation level.  It determines if the control transfer is
 *  feasible, adjusts simulation level data structures as needed, and returns
 *  an indication to the caller (the DIS interface) whether the request
 *  should proceed, or not.
 */

int
transferControlRequestHandler (Entity_t *e, dis_transfer_control_pdu *pdu)
{
	int result = RESULT_UNABLE;
	craftType *cinfo;

	switch (pdu->transfer_type) {

	/*
	 *  Someone would like use to take control of an entity
     *
     *  If it is an aircraft we can model, then make it a drone.
     */

	case DISTransferTypeEntityControllerRequest:
		cinfo = lookupCraftByEntityType( &e->entityType );
		if ( cinfo != NULL ) {
			e->c->type = CT_DRONE;
			e->c->cinfo = cinfo;
			/* TODO: provision the aircraft; landing gear, etc */
			result = RESULT_REQUEST_OK;
		}
		break;

	/*
	 *  Control of this entity is requested by someone else.
     *
     *  Change type to DIS aircraft and we're done.
	 */

	case DISTransferTypeEntityRequest:
		result = RESULT_REQUEST_OK;
		e->c->type = CT_DIS_PLANE;
	}

	return result;
}
