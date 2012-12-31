#pragma once
/*
	This is where all declarations for pack routine prototypes for
	universal game specific object structures thatwill be transmited
	accross the network must reside or be included into.  All pack
	routines must have an entry for a reference prototype in this file.

	Copyright 1999 Playnet Inc, All rights reserved

	Originating Programmer : Russell Mirabelli

	Rev 0.1  11/16/99
 */

extern SINT32 commPackInit(void);

extern SINT32 commPackPLAYER_LOGIN(LOGICAL mode, UCHAR* const data,
							 SINT32 offset, struct PLAYER_LOGIN* object, SINT32 size);

extern SINT32 commPackPERSONA_REQUEST(LOGICAL mode, UCHAR* const data,
								  SINT32 offset, struct PERSONA_REQUEST* object, SINT32 size);

extern SINT32 commPackVEHICLE_REQUEST(LOGICAL mode, UCHAR* const data,
							  SINT32 offset, struct VEHICLE_REQUEST* object, SINT32 size);

extern SINT32 commPackVEHICLE_REPLY(LOGICAL mode, UCHAR* const data,
							SINT32 offset, struct VEHICLE_REPLY* object, SINT32 size);

extern SINT32 commPackPlayerExit(LOGICAL mode, UCHAR* const data,
							  SINT32 offset, struct PLAYER_EXIT* pe, SINT32 size);
