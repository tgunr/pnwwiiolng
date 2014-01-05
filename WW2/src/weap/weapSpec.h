#pragma once
// Created: Tue Sep  6 18:49:28 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "wwii/wwiiOwnershipEnums.h"

#pragma pack(push, 1)
//! @struct WEAPON_SPEC
//! @brief Weapon (vehicle/trooper) type specification.
//! @detail A specific vehicle, such as a British Spitfire or
//! a German Rifleman, are identified using four values
//! rather than a flat linear value as this helps provide
//! sanity checks but also helps perform various masking
//! operations. E.g. you can quickly check if a given vehicle
//! type is a trooper by checking if spec.wClass == _INFANTRY.
//tolua_begin
struct WEAPON_SPEC
{
	WEAPON_SPEC() : wType(0), wClass(0), wCategory(0), wCountry(0) {}
	WEAPON_SPEC(const UINT16& rhs) { *(UINT16*)this = rhs; }
	WEAPON_SPEC(const WEAPON_SPEC& rhs) : wType(rhs.wType), wClass(rhs.wClass), wCategory(rhs.wCategory), wCountry(rhs.wCountry) {}
	WEAPON_SPEC(UINT32 country_, UINT32 category_, UINT32 class_, UINT32 type_)
		: wType((UINT8)type_), wClass((UINT8)class_), wCategory((UINT8)category_), wCountry((UINT8)country_)
		{}

	// OGS: Reversed the order of fields to make 'asUINT32' cheaper,
	// OGS: while maintaining network order.
	//! Unique per country+category, from the th_blitz/weapon/XX_XX_weap.csv file.
//tolua_end
	UINT16		wType 			//tolua_export
				: 5
				;				//tolua_export
	//! Descriptive of the vehicle's classification, e.g tank, truck, troop, paratroop, etc. From th_blitz/weapon/weapClasses.csv
	UINT16		wClass			//tolua_export
				: 5
				;				//tolua_export
	//! Partitions vehicle by branch, e.g. aircraft, ground vehicle, infantry. See weap/weapDefs.h
	UINT16		wCategory		//tolua_export
				: 3
				;				//tolua_export
	//! Which country this type belongs to.
	UINT16		wCountry		//tolua_export
				: 3
				;				//tolua_export
//tolua_begin

	//! But I don't want a complicated structure. Just gimme a number.
	UINT32 asUINT32() const { return ((UINT32)wCountry << 24) | ((UINT32)wCategory << 16) | ((UINT32)wClass << 8) | (UINT32)wType; }
	UINT16 asUINT16() const { return *(UINT16*)this; }
	//! Compare two weapon specs, but do it real cheap, because we do it frequently.
	bool operator==(const WEAPON_SPEC& rhs) const { return this->asUINT16() == rhs.asUINT16(); }

//tolua_end
	//! Make network fly.
	void packer(LOGICAL mode, ACTION_OBJECT* a);

private:
	operator UINT32() const;	//<! Use asUINT32() instead.
	operator SINT32() const;	//<! Use asUINT32() instead.
	operator UINT64() const;	//<! Use asUINT32() instead.
	operator SINT64() const;	//<! Use asUINT32() instead.
	operator UINT8() const;	//<! Use asUINT32() instead.
	operator SINT8() const;	//<! Use asUINT32() instead.
public:
	operator UINT16() const { return asUINT16(); }

	//! When 16-bits just aren't enough.
	void fromShort(const UINT16 key)
	{
		*(UINT16*)this = key;
	}

	//! Because != != !(=). For some reason.
	//! (not-equals is not-equal-to not(equals))
	bool operator!=(const WEAPON_SPEC& rhs) const { return this->asUINT16() != rhs.asUINT16(); }

	//! Does this weapon spec make my ass look thin?
	bool operator<(const WEAPON_SPEC& rhs) const { return this->asUINT16() < rhs.asUINT16(); }
	//! Does this weapon spec make my ass look fat?
	bool operator>(const WEAPON_SPEC& rhs) const { return this->asUINT16() > rhs.asUINT16(); }
//tolua_begin

	//! Check for a null spec.
	bool isNull() const { return asUINT16() == 0; }

	WWII_COUNTRY country() const { return (WWII_COUNTRY)wCountry; }
	int side() const { return wwiiSide((WWII_COUNTRY)wCountry); }
};

extern WEAPON_SPEC NULL_WEAP_SPEC; // removed const to work with broken MW Linker
//tolua_end
#pragma pack(pop)

#define COMPARE_SPECS(s1,s2) (*((UINT32*)&(s1)) == *((UINT32*)&(s2)))
#define	SPEC_IS_NULL(spec) COMPARE_SPECS(spec, NULL_WEAP_SPEC)

//tolua_begin

// find a weapon spec based on the weapon's name
extern const WEAPON_SPEC& weapFindWeaponByName(const char* weaponName);

static inline const char* weapPrintSpecC(const WEAPON_SPEC* spec)
{
	if (PN_UNLIKELY(spec == nullptr))
		return "<null>";

	static char buffer[24];
	sprintf(buffer, "%u:%u:%u:%u", (UINT8)spec->wCountry, (UINT8)spec->wCategory, (UINT8)spec->wClass, (UINT8)spec->wType);

	return buffer;
}

// create a spec from a string
extern WEAPON_SPEC weapCreateSpec(const char* weapString);

// create a spec from its dotted quad
extern WEAPON_SPEC weapCreateSpec(UINT32 country, UINT32 category, UINT32 klass, UINT32 type);

// find *just* a weapon's class number
extern UINT8 weapFindClassIndex(const std::string& className);

// turn a class string into a weap spec
static inline WEAPON_SPEC weapFindClass(const std::string& className)
{
	return WEAPON_SPEC(0, 0, weapFindClassIndex(className), 0);
}


//tolua_end
