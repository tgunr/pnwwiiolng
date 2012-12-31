#pragma once

#include "weap/weapons.h"

#define ML_MAX_WEAPONS_PER_MOUNT 8
#define ML_MAX_MOUNTS 16
#define ML_MAX_CONFIGURATIONS 16

// Trigger Types
enum
{
	ML_TRIGGERTYPE_NONE = 0,
	ML_TRIGGERTYPE_PRIMARY,
	ML_TRIGGERTYPE_SECONDARY,
	ML_TRIGGERTYPE_BOTH,
	ML_MAX_TRIGGERTYPE
};

/************************************************************************************************
	mlWeaponMount
*************************************************************************************************/

struct mlWeaponMount
{
	weapWeapon	Weapons[ML_MAX_WEAPONS_PER_MOUNT]; 				// weapons data

	UINT32 	TriggerType; 	// which trigger fires this weapon
	UINT32 	SelectorNum; 	// which selector number to activate weapon
	UINT32 	ConfigNum; 		// which weapons configuration this mount is active on
	UINT32 	MountNum; 		// which mount varnum to unhide for this mount
	UINT32 	NumberOfWeaponsInMount; 	// number of weapons in the mount

	UCHAR 	MountName[32]; 	// name of the mount subobject;
	OBJECT* MountObject; 	// the mount in question in the 3d model

	UCHAR 		WeaponNames[ML_MAX_WEAPONS_PER_MOUNT][32]; 		// object names for the weapon shapes for the weapons
	OBJECT*		WeaponConnectPoints[ML_MAX_WEAPONS_PER_MOUNT]; 	// weapon connection points
	OBJECT*		WeaponFirePoints[ML_MAX_WEAPONS_PER_MOUNT]; 	// weapon fire points
	OBJECT*		WeaponCrapPoints[ML_MAX_WEAPONS_PER_MOUNT]; 	// weapon crap points
	SCHAR 		WeaponSounds[ML_MAX_WEAPONS_PER_MOUNT][64]; 	// weapons sound data
	SCHAR 		WeaponSoundsReload[ML_MAX_WEAPONS_PER_MOUNT][64];	// weapon reload sound data
	UINT32 		WeaponSoundId[ML_MAX_WEAPONS_PER_MOUNT];

	UINT32 	CollideType; 	// ground object collide list type
	FLOAT64 AmmoDamage; 	// damage to the ammo-feed system
	UINT32 	WeaponJammed; 	// weapon or ammo feed damaged
	UCHAR 	AmmoExplosionDamageComponent[8][64];
	UINT32 	PositionNum; 	// which position can fire?
	UINT32 	MountSerialNum;
	FLOAT32	GunFireSmokeLevel;

	UINT32	MultiCrewPos; 	// multicrew position required for use of weapon

	UINT32 		UsesConvergence;
	FLOAT32		MinConvergence;
	piMATRIX 	ConvergenceMat;
	LOGICAL 	ConvergenceMatSetup;

	UINT32	PickleMount; 	// does the mount use pickle-style firing?
	UINT32	AddToGroupAmmoCount;

	LOGICAL	LastUpdateWasReloading[ML_MAX_WEAPONS_PER_MOUNT];

	UINT32 	UseBombBay; 	// weapon in bomb bay, must be open to deploy
	UINT32	BombMountType;	// Type of bomb mount internal or external
	FLOAT32 DiveAngle;		// Max angle bombs can be released
	LOGICAL	CheckIfBlocked;	// Checks to see if firing weapon would hit vehicle
	LOGICAL	CanJettison[ML_MAX_CREW_POSITIONS]; // Determines if position can jettison

	mlWeaponMount()
	{
		memset(&TriggerType, 0x0, sizeof(mlWeaponMount)-sizeof(Weapons));
	};
};

/************************************************************************************************
	mlweapons
*************************************************************************************************/

extern UINT32 mlWeaponMountAddSpecialVariable(void* SpecialData,  const char* VarName, const char* VarContents,
										mlComponentList* ComponentList,  mlComponent* Component,
										void* VehicleMasterPtr);

extern UINT32 mlUpdateWeaponMount(FLOAT64VECTOR* Force, FLOAT64ATT* Torque,
						   mlComponent* Component, mlVehicleData* VehicleData,
						   FLOAT64 DeltaTime, FLOAT64 CurrentTime,
						   UINT32* PrimaryIsFiring, UINT32* SecondaryIsFiring, OBJECT* PlaneObject,
						   mlComponentList* ComponentList, FLOAT64 ConvergenceRange, UINT32* MountFired);

extern UINT32 mlLoadBeltFile(weapWeapon* Weapon,UCHAR* VarContents,SINT32 AmmoGroupNum);

extern void mlWeaponGetFireAttPosVel(mlWeaponMount* Mount, int i, mlVehicleData* VehicleData, piMATRIX* FireAtt,
								FLOAT64VECTOR* RelativePnt, FLOAT64VECTOR* FirePnt,  FLOAT64VECTOR* FireVel);

