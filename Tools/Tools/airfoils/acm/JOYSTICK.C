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

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static int min[4] = {999, 999, 999, 999};
static int max[4] = {-1, -1, -1, -1};
static int home[4];
static int value[5] = {128, 128, 128, 128, 0};

static char *joystick_port = NULL;

void
SetJoystickPort (char * name)
{
    joystick_port = name;
}

void
CalibrateJoystick()
{
	int i;

	printf("\n\nCenter the joystick, click when ready!\n\n");
	for (;;)
		if (ProcessJoystickInput()) 
			if (value[4] != 0) break;
	for (i = 0; i < 4; i++) home[i] = value[i];
	for (;;) if (ProcessJoystickInput()) if (value[4] == 0) break;

	printf("\n\nPosition stick in minimum and maximum values, then click\n\n");
	for (;;)
		{
			if (ProcessJoystickInput())
				{
					for (i = 0; i < 4; i++)
						{
							if (value[i] < min[i]) min[i] = value[i];
							if (value[i] > max[i]) max[i] = value[i];
						}
					if (value[4] != 0) break;
				}
		}
	for (;;) if (ProcessJoystickInput()) if (value[4] == 0) break;
	printf("\n\nJoystick Calibrated\n");
}

double fixJoy(int i)
{
  // Forward joysticks.
  if (min[i] < max[i])
  {      
    if (value[i] <= home[i])
      return (double) (value[i] - min[i]) / (double)(home[i] - min[i]) - 1.0;
    else
      return (double) (value[i] - home[i]) / (double)(max[i] - home[i]);
  }
  // Backwards joysticks.
  else 
    if (value[i] >= home[i])
      return ((double)(min[i] - value[i]) / (double)(min[i] - home[i]) - 1.0);
    else 
      return ((double)(home[i] - value[i]) / (double)(home[i] - max[i]));
}

void
GetJoystickPosition(double *x1, double *y1,
		    double *x2, double *y2, int *switches)
{
  if (min[0] == 999)
  {
    *x1 = 0.0;  *y1 = 0.0;  *x2 = 0.0;  *y2 = 0.0; *switches = value[4];
    return;
  }
  
  *x1 = fixJoy(0);
  if (*x1 < -1.0) *x1 = -1.0;

  *y1 = fixJoy(1);
  if (*y1 < -1.0) *y1 = -1.0;

  *x2 = fixJoy(2);
  if (*x2 < -1.0) *x2 = -1.0;

  *y2 = fixJoy(3);
  if (*y2 < -1.0) *y2 = -1.0;
  *switches = value[4];
}

/*
 *  ProcessJoystickInput()
 *
 *  This procedure should be called just before you call GetJoystickInput().
 *  In processes any pending input from the Workstation Gameport.
 */

static int init = 0, state;

int
ProcessJoystickInput()
{
    static int fd;
    static int x1, y1, x2, y2, switches;
    int on = 1;
    int flags, n, i, updated = 0;
    unsigned char bytes[512], *p;

    if (init < 0) {
	return init;
    } else if (init == 0) {
	struct termios term;

	init = -1;

	if (!joystick_port) {
	    return init;
	}

	if ((fd = open(joystick_port, O_RDWR)) < 0) {
	    perror("joystick port open failed");
	    return -1;
	}

/*
 *  Set POSIX non-blocking I/O
 */

	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
	    perror("F_GETFL failed");
	    return -1;
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0) {
	    perror("F_SETFL failed");
	    return -1;
	}

/*
 *  Assert both DTR and RTS (these pins supply power to
 *  the Workstation Gameport).
 */

#ifdef TIOCM_RTS
	flags = TIOCM_RTS | TIOCM_DTR | TIOCM_LE;
	if (ioctl(fd, TIOCMSET, &flags) == -1) {
	    perror("ioctl failed");
	    return -1;
	}
#endif

/*
 *  Condition the TTY line to talk to the converter
 */

	if (tcgetattr(fd, &term) != 0) {
	    perror("tcgetattr failed");
	    return -1;
	}
	term.c_iflag = IGNBRK | IGNPAR;
	term.c_oflag = 0;
	term.c_cflag = CLOCAL | CS8 | CSTOPB | CREAD;
	term.c_lflag &= ~(ECHO | ICANON);
	for (i = 0; i < NCCS; ++i) {
	    term.c_cc[i] = -1;
	}
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	cfsetospeed(&term, B9600);
	cfsetispeed(&term, B9600);
	if (tcsetattr(fd, TCSAFLUSH, &term) != 0) {
	    perror("tcsetattr failed");
	    return -1;
	}
	state = 0;
	init = 1;
    }
/*
 *  Read a whole bunch of characters from the tty
 */

    n = read(fd, bytes, sizeof bytes);
	if (n != -1) {
		printf("%d ->", n);
		for (state = 0; state < n; state++) printf(" %d", bytes[state]);
		printf("\n");
	}

/*
 *  And then process them ...
 */

    for (p = bytes; n > 0; --n) {

	switch (state) {

	case 0:
	  if ((*p & 0x80) != 0) {
	    switches = (((int) *p) >> 3) & 0x0F;
	    x1 = (((int) *p) & 0x07) << 5;
	    state = 1;
	  }
	  break;

	case 1:
	  if ((*p & 0x80) != 0) { state = 0; break; }
	  x1 += (((int) *p) >> 2) & 0x1F;
	  y1 = (((int) *p) & 0x03) << 6;
	  state = 2;
	  break;

	case 2:
	  if ((*p & 0x80) != 0) { state = 0; break; }
	  y1 += (((int) *p) >> 1) & 0x3F;
          x2 = (((int) *p) & 0x01) << 7;
	  state = 3;
	  break;

	case 3:
	  if ((*p & 0x80) != 0) { state = 0; break; }
	  x2 += ((int) *p) & 0x7F;
	  state = 4;
	  break;

	case 4:
	  if ((*p & 0x80) != 0) { state = 0; break; }
	  y2 = ((int) *p) << 1;
	  state = 5;
	  break;

	case 5:
	  if ((*p & 0x80) != 0) { state = 0; break; }
	  y2 += ((int) *p) & 0x01;
	  value[0] = 255 - x1;
	  value[1] = 255 - y1;
	  value[2] = x2;
	  value[3] = y2;
	  value[4] = switches;
	  state = 0;
	  updated = 1;
	  // printf("%d %d %d -> %d %d\n", switches, x1, y1, x2, y2);
	  break;
	}
	++p;
    }
    return updated;
}
