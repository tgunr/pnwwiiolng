#pragma once
// Created: April 17 2002
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Marty "Vandal" Poulin

extern SINT32 wwiiPackBrigadeID(PackMode mode, UCHAR* const data, SINT32 offset, UINT32& brigadeID, SINT32 size=0);

extern SINT32 wwiiPackFullObjID(PackMode mode, UCHAR* data, SINT32 offset, struct FULL_OBJ_ID* const object, SINT32 /*size*/);

static inline SINT32 wwiiPackCountry(PackMode mode, UCHAR* const data, SINT32 offset, WWII_COUNTRY* country, SINT32 /*size*/)
{
	return teulPackUINT32(mode, data, offset, (UINT32*)country, 4);
}

extern SINT32 wwiiPackWeaponSpec(PackMode mode, UCHAR* data, SINT32 offset, struct WEAPON_SPEC* const object, SINT32 /*size*/);

extern SINT32 wwiiPackWeaponSerial(PackMode mode, UCHAR* data, SINT32 offset, struct WEAPON_SERIAL* const object, SINT32 /*size*/);

extern SINT32 wwiiPackText(PackMode mode, UCHAR* data, SINT32 offset, char* text, SINT32 maxLen);

static inline SINT32 wwiiPackRankNo(PackMode mode, UCHAR* const data, SINT32 offset, RANK_NO* const rank, SINT32 size)
{
	return teulPackUCHAR(mode, data, offset, (UCHAR*)rank, size);
}

static inline SINT32 wwiiPackHandle(PackMode mode, UCHAR* const data, SINT32 offset, ChatHandle* const handle, SINT32 /*size*/)
{
	return teulPackUINT64(mode, data, offset, (UINT64*)handle, GAME_HANDLE_BITS);
}
