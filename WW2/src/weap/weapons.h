#pragma once
// Weapons system weapons module
// Handles all the weapons-related stuff for WW2 Online

#include "wwii/wwiiCovert.h"
#include "weap/weapammo.h"

#define WEAP_MAX_BELT_SEQUENCE_ROUNDS 8

enum
{
	WEAP_VAR_NOTUSED1,
	WEAP_VAR_MASS,
	WEAP_VAR_FIRETYPE,
	WEAP_VAR_RATEOFFIRE,
	WEAP_VAR_NOTUSED4,
	WEAP_VAR_AMMOTYPE1,
	WEAP_VAR_AMMOTYPE2,
	WEAP_VAR_AMMOTYPE3,
	WEAP_VAR_AMMOTYPE4,
	WEAP_VAR_AMMOTYPE5,
	WEAP_VAR_AMMOTYPE6,
	WEAP_VAR_AMMOTYPE7,
	WEAP_VAR_AMMOTYPE8,
	WEAP_VAR_DISPERSION,
	WEAP_VAR_GHOST_ROF,
	WEAP_VAR_GHOST_AMMOTYPE,
	WEAP_VAR_HIDEONLASTDROP,
	WEAP_VAR_NUMROUNDSINCLIP,
	WEAP_VAR_RELOADTIME,
	WEAP_VAR_WEAPONPLACED,
	WEAP_VAR_NOMUZZLESMOKE,
	WEAP_VAR_MAXOVERSUPPLY,
	WEAP_VAR_INFRESUPPLY,
	WEAP_VAR_CLIPTYPE,
	WEAP_VAR_MUZZLEFLASH,
	WEAP_VAR_MAXVAR
};


typedef void (*WEAP_FIRE_CALLBACK)(struct weapWeapon * Weapon);

struct weapWeapon
{
	weaponType * WeaponType;
	weapAmmoType * BeltSequence[4][WEAP_MAX_BELT_SEQUENCE_ROUNDS];
	Covert<UINT16> NumRoundsInBeltSequence[4];
	Covert<SINT16> NumRoundsAmmoLeft[4];
	Covert<UINT16> NumTotalRounds[4];
	Covert<UINT16> NumOverSupplyRounds[4];
	Covert<UINT32> BeltSequencePosition[4];
	SCHAR BeltName[4][32];

	Covert<UINT16> AmmoTypeInUse; // which belt are we drawing from
	Covert<UINT16> NextAmmoToUse; // which belt to use on next reload

	FLOAT64 LastFireTime;
	FLOAT64 ReloadStartTime;
	Covert<FLOAT64> FireAccumulator; // used for spacing rounds properly
	UINT32 WeaponFireType;

	FLOAT32 Displacement; // displacement of bullet vs muzzle flash

	weapWeapon()
		: WeaponType(NULL)
		, LastFireTime(0.0f)
		, ReloadStartTime(0.0f)
		, WeaponFireType(0)
		, Displacement(0.0f)
	{
		memset(BeltSequence, 0x0, sizeof(BeltSequence));
		memset(BeltName, 0x0, sizeof(BeltName));
	}
};
struct vehVehicle;

extern SINT32 weaponsInit(void);
extern UINT32 weaponsKill(void);
extern UINT32 weapInitializeWeapon(weapWeapon * Weapon,weaponType * WeaponType);
extern UINT32 weapFindWeaponTypeByName(weaponType ** WeaponType,UCHAR * WeaponName);
extern UINT32 weapCreateWeaponByName(weapWeapon * Weapon,
									  UCHAR * WeaponName);
extern UINT32 weapFireWeapon(FLOAT64VECTOR * RecoilForce,
							  weapWeapon * Weapon,
							  FLOAT64VECTOR * Pos,
							  FLOAT64VECTOR * Vel,
							  piMATRIX * FireAtt,
							  FLOAT64 FireDelta,
							  FLOAT64 CurrentTime,
							  UINT32 CanCauseDamage,
							  UINT32 FiredFromGhost,
							  UINT32 Flags,
							  OBJECT * EmitterObject,
							  FLOAT64VECTOR * RelativePos,
								FLOAT64 * LastFireTime,
								void* FireVehicle); // note: FireDelta is in seconds

extern LOGICAL weapIsWeaponReloading(weapWeapon * Weapon, FLOAT64 CurrentTime);
extern UINT32 weapResupplyWeapon(weapWeapon * Weapon, FLOAT32 percentOfTotal = 0.0f, bool allowOversupply = false);
extern void weapAddFireCallback(WEAP_FIRE_CALLBACK callback);
