/*
 *  acm : an aerial combat simulator for X
 *  Copyright (C) 1991,1992  Riley Rainey
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
typedef struct {
        WorldCoordinates w;
	VPoint    Sg;
	VPoint    Cg;
	double    heading, pitch, roll;
} _BBShortState;

typedef struct {
	short     type;				/* craft type */
	char      name[64];			/* craft name */
} _BBNewObject;

typedef struct {
	unsigned  char rectype;		/* black box record type */
	unsigned  char table;			/* is it ptbl(0) or mtbl(1)? */
	unsigned  short id;			/* player or missile index */
	union {
		_BBShortState short_state;
		_BBNewObject object;
	} u;
} BBRecord;
#define BB_HDR_SIZE (sizeof(char) + sizeof(char) + sizeof(short))

#define BB_TYPE_SHORT_STATE		0x00
#define BB_TYPE_ADD_OBJECT		0x01
#define BB_TYPE_DELETE_OBJECT		0x02
#define BB_TYPE_END_OF_FRAME		0x03
