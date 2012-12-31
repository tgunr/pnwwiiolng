/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991-1994  Riley Rainey
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
#include <sys/types.h>
#include <X11/Intrinsic.h>

#include <client.h>
#define MIN_THROTTLE	8192
#define MAX_THROTTLE	32768
#define FL_TRIGGER	0

extern void PostTriggerDown PARAMS((void));
extern void PostTriggerUp PARAMS((void));
extern void PostLaunchDrone PARAMS((void));

/* ARGSUSED */
void
SetThrottle (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	register char	*p;
	register long	value;


	if (*num_params == 1) {

		p = *params;
		value = atol(p);

		if (player_id >= 0) {
			if (*p == '+' || *p == '-')
				ptbl[player_id].throttle += value;
			else
				ptbl[player_id].throttle = value;

			if (ptbl[player_id].throttle < MIN_THROTTLE)
				ptbl[player_id].throttle = MIN_THROTTLE;

			if (ptbl[player_id].throttle > MAX_THROTTLE)
				ptbl[player_id].throttle = MAX_THROTTLE;
		} 
	}
	else
		fprintf (stderr,"bad number of parameters on set-throttle()\n");

}

/* ARGSUSED */
void
SetRudder (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	register char	*p;
	register long	value, current;


	if (*num_params == 1) {

		p = *params;
		value = atol(p);

		if (player_id >= 0) {

			current = ptbl[player_id].Sr * 32768.0;
		
			if (*p == '+' || *p == '-')
				current += value;
			else
				current = value;

			if (ptbl[player_id].throttle < -32768)
				ptbl[player_id].throttle = -32768;

			if (ptbl[player_id].throttle > 32768)
				ptbl[player_id].throttle = 32768;

			ptbl[player_id].Sr = (double) current / 32768.0;

		} 
	}
	else
		fprintf (stderr,"bad number of parameters on set-rudder()\n");

}

/* ARGSUSED */
void
Afterburner (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	char	*p;

	if (*num_params == 0) {

		if (player_id >= 0) {

			ptbl[player_id].flags ^= FL_AFTERBURNER;

		} 
	}
	else if (*num_params == 1) {
		p = *params;
		if (player_id >= 0) {

			if (strcmp (p, "1") == 0)
				ptbl[player_id].flags |= FL_AFTERBURNER;
			else if (strcmp (p, "0") == 0)
				ptbl[player_id].flags &= ~FL_AFTERBURNER;
			else
				fprintf (stderr,
					"bad afterburner parameter: %s\n", p);
		} 
	}
	else {
		fprintf (stderr,"bad number of parameters on afterburner()\n");
	}

}

/* ARGSUSED */
void
ToggleGear (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	if (*num_params == 0) {

		if (player_id >= 0) {

			ptbl[player_id].flags ^= FL_GHANDLE_DN;

		} 
	}
	else
		fprintf (stderr,"bad number of parameters on toggle-gear()\n");

}

/* ARGSUSED */
void
ToggleBrakes (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	if (*num_params == 0) {

		if (player_id >= 0) {

			ptbl[player_id].flags ^= FL_BRAKES;

		} 
	}
	else
		fprintf(stderr,"bad number of parameters on toggle-brakes()\n");

}

static	VPoint	up =	{0.0, 0.0, -1.0};
static	VPoint	back =	{-1.0, 0.0, 0.0};
static	VPoint	zero =	{ 0.0, 0.0, 0.0};

/* ARGSUSED */
void
SetView (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {


	craft	*c;
	char	*s;

	s = *params;

	if (*num_params == 1) {

		if (player_id >= 0) {

			c = &ptbl[player_id];
			c->viewDirection = zero;
			c->viewUp = up;

			if (strcmp (s, "forward") == 0)
				c->viewDirection.x = 1.0;
			if (strcmp (s, "aft") == 0)
				c->viewDirection.x = -1.0;
			else if (strcmp (s, "right") == 0)
				c->viewDirection.y = 1.0;
			else if (strcmp (s, "left") == 0)
				c->viewDirection.y = -1.0;
			else if (strcmp (s, "up") == 0) {
				c->viewDirection = up;
				c->viewUp = back;
			}

		} 
	}
	else
		fprintf (stderr,"bad number of parameters on look()\n");

}

/* ARGSUSED */
void
Trigger (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	register unsigned char	*p;

	p = *params;

	if (*num_params == 1) {

		if (player_id >= 0) {

			if (strcmp (p, "press") == 0)
				PostTriggerDown();
			else if (strcmp (p, "release") == 0)
				PostTriggerUp();
			else
				fprintf (stderr,
					"bad trigger parameter: %s\n", p);

		}
	}
	else
		fprintf (stderr,"bad number of parameters on trigger()\n");

}

/* ARGSUSED */
void
SelectWeapon (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params; {

	if (*num_params == 0) {
		if (player_id >= 0) {
			selectWeapon (&ptbl[player_id]);
		}
	}
	else
		fprintf (stderr,"bad number of parameters on select-weapon()\n");

}


/* ARGSUSED */
void
LaunchDrone (w, event, params, num_params)
Widget		w;
XEvent		*event;
String		*params;
Cardinal	*num_params;
{
	register unsigned char	*p;

	p = *params;

	if (*num_params == 0) {

		if (player_id >= 0) {

			PostLaunchDrone();

		}
	}
	else
		fprintf (stderr,"bad number of parameters on launch-drone()\n");

}

