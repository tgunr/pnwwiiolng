#pragma once

#include <map>
#include <string>

#include "weap/weapSpec.h"

enum
{
	VEH_TYPE_NONE,
	VEH_TYPE_FLOATER,
	VEH_TYPE_PERSON,
	VEH_TYPE_AIR,
	VEH_TYPE_GROUND,
	VEH_TYPE_SEA,
	VEH_MAXTYPE,
};

enum
{
	VEH_TURRET_TYPE_360 = 1,
	VEH_TURRET_TYPE_LIMITED,
	VEH_TURRET_TYPE_FLEXIBLE,
	VEH_TURRET_TYPE_GRAVITY
};

enum
{
	VEH_TURRET_NEUTRAL_ON_CANOPYOPEN = 1,
	VEH_TURRET_NEUTRAL_ON_CANOPYCLOSED,
	VEH_TURRET_NEUTRAL_ON_HATCHOPEN,
	VEH_TURRET_NEUTRAL_ON_HATCHCLOSED,
	VEH_TURRET_NEUTRAL_ON_BOMBBAYOPEN,
	VEH_TURRET_NEUTRAL_ON_BOMBBAYCLOSED,
};

#define VEH_MAX_CREW_POSITIONS 	            16
#define VEH_MAX_MOUNTS 			            16
#define VEH_MAX_INERTIA_COMPONENTS          32
#define VEH_MAX_WEAPONS_PER_MOUNT           4
#define VEH_MAX_LOADOUTS                    8
#define VEH_MAX_STATE_BITS                  64
#define VEH_MAX_ENGINESOUND_SPEED_RANGES    8
#define VEH_MAX_MOUNTSOUNDS                 8
#define VEH_MAX_GEARNUM                     32

#include "updt/updtEnums.h"
#define VEH_MAX_TURRETS                     MAX_TURRETS

struct vehInertiaComponent
{
	char name[64];
	FLOAT64 mass;
	FLOAT64 vol;
	FLOAT64VECTOR cm;
	FLOAT64 I[3][3]; // per unit mass moment of inertia, multiply times mass to get moment
};

struct vehanimVehicleStateType
{
	UINT32 	StateType;
	UINT32 	StateSubType;
	UINT32 	Data;
	UINT32 	Data1;
	UINT32 	MultiCrewPos;
	char  	Name[32];
	char  	Emitter[32];
	char 	MassObject[32];
	char  	ParentVisDamage[32];
	FLOAT64 EjectSpeed;
};

struct weaponType;
struct vehLoadout
{
	UCHAR		MountObject		[VEH_MAX_MOUNTS][32];
	SINT32 		MountSoundNum	[VEH_MAX_MOUNTS];
	UINT32 		RecoilNum		[VEH_MAX_MOUNTS];	//!< recoil DOF number to use for recoil, if applicable
	FLOAT32 	RecoilTime		[VEH_MAX_MOUNTS]; 	//!< time to recoil to the full "back" position
	weaponType*	WeaponTypes		[VEH_MAX_MOUNTS][VEH_MAX_WEAPONS_PER_MOUNT];
	UCHAR 		WeaponObject	[VEH_MAX_MOUNTS][VEH_MAX_WEAPONS_PER_MOUNT][32];

};

//! @struct vehType
//! @brief Data about a type of vehicle (as opposed to an instance of it).
struct vehType
{
	vehType()
	{
		memset(&VehicleTypeNum, 0x0, (size_t)this + sizeof(vehType) - (size_t)&VehicleTypeNum);
	}
	std::map<std::string, std::string> turretSound[VEH_MAX_CREW_POSITIONS]; //<! turret traverse, elevate, general
	std::string InfoText;

	UINT32 	VehicleTypeNum; 		//!< vehicle type number (never change the position of this--all class members go before)
	UINT32 	Type; 					//!< vehicle type
	WEAPON_SPEC WeaponSpec;

	LOGICAL OnlineOnly; 		    //!< is the vehicle visible offline, or online only?
	LOGICAL VehicleIsMultiCrew; 	//!< is vehicle a multi-crew vehicle?
	LOGICAL bHasTreadSound;         //!< Vehicle has tread sound.
    LOGICAL bHasBrakeSound;         //!< Vehicle has brake sound.
	vehLoadout Loadout[VEH_MAX_LOADOUTS]; 					//!< weapon loadouts

	SINT32 NumStates;
	vehanimVehicleStateType StateType[VEH_MAX_STATE_BITS]; 	//!< used for state change stuff
	FLOAT32 VehicleRealMass;							//!< hack until we actually model the concussive damage correctly

	vehInertiaComponent Inertia[VEH_MAX_INERTIA_COMPONENTS];

// Config Files
	UCHAR 	Shape[32]; 				//!< 3d shape filename to use for this vehicle
	UCHAR 	Id[32]; 				//!< id used to refer to this vehicle type
	UCHAR 	Icon[32]; 				//!< icon the player sees
	UCHAR 	Description[64]; 		//!< "short" description (like "Supermarine Spitfire Mk Ia")
	UCHAR 	ModelDirectory[64];		//!< which directory in "vehicles" directory is the "flight" model
	UCHAR 	ModelFile[64]; 			//!< vehicle "flight" model filename
	UCHAR 	ControlFile[64]; 		//!< which file contains the enabled control list for this vehicle
	UCHAR 	DamageFile[64];
	UCHAR 	StateFile[64];
	UCHAR 	TurretFile[64];
	UCHAR 	LoadoutFile[64];
	UCHAR 	SoundFile[64];

// Sounds
	UCHAR 	EngineSound[4][32]; 									//!< sounds used for the external engine
    UCHAR   szTreadSound[32];   //!< Vehicle tread sound
    UCHAR   szBrakeSound[32];   //!< Vehicle brake sound

    FLOAT32	EngineSpeedStart[VEH_MAX_ENGINESOUND_SPEED_RANGES];		//!< speed range start speed
	FLOAT32	EngineSpeedEnd	[VEH_MAX_ENGINESOUND_SPEED_RANGES]; 	//!< speed range end speed
	FLOAT32	EnginePitchStart[VEH_MAX_ENGINESOUND_SPEED_RANGES]; 	//!< pitch at start speed
	FLOAT32	EnginePitchEnd	[VEH_MAX_ENGINESOUND_SPEED_RANGES]; 	//!< pitch at end speed

	UCHAR 	StukaSound[32]; 										//!< sound used for stuka siren
	UCHAR 	MountSounds[VEH_MAX_MOUNTSOUNDS][32]; 					//!< mount sounds

	SCHAR 	HitSounds[3][32]; 										//!< player's vehicle hit sounds
	SCHAR   ShrapHitSounds[3][32];

    FLOAT32	HitMaxVolumeEnergy; 									//!< energy required to get maximum hit sound volume

// Damage
	UINT32	NumDamageObjects;											//!< number of damage objects for vehicle
	UCHAR 	DamageObjects				[VEH_MAX_DAMAGE_OBJECTS][32];	//!< names of damage objects in vehicle
	FLOAT32	DamageArmorThickness		[VEH_MAX_DAMAGE_OBJECTS];		//!< equivelant armor thickness
	FLOAT32	DamageFaceHardness			[VEH_MAX_DAMAGE_OBJECTS]; 		//!< used for ammo breakup
	FLOAT32	DamageAverageHardness		[VEH_MAX_DAMAGE_OBJECTS];		//!< used for ammo breakup
	SINT32 	DamageConcussionResistance	[VEH_MAX_DAMAGE_OBJECTS];	 	//!< used for concussion damage resistance
	FLOAT32	DamageThreshold				[VEH_MAX_DAMAGE_OBJECTS]; 		//!< lowest energy that can apply damage
	LOGICAL	DamageImportant				[VEH_MAX_DAMAGE_OBJECTS];		//!< should damage count towards scoring?

    UINT32 	MultiCrewPos[VEH_MAX_CREW_POSITIONS];

// Turret
	FLOAT32	TurretTraverseRate		[VEH_MAX_CREW_POSITIONS];	//!< turret traverse rate for each turret (in seconds)
	FLOAT32	TurretElevateRate		[VEH_MAX_CREW_POSITIONS]; 	//!< turret elevation rate for each turret (in seconds)
	UINT32 	TurretType				[VEH_MAX_CREW_POSITIONS]; 	//!< turret type
	SINT32 	TurretNum				[VEH_MAX_CREW_POSITIONS]; 	//!< turret number for update system
	SINT32 	TurretNeutral			[VEH_MAX_CREW_POSITIONS]; 	//!< what forces the turret to neutral position
	FLOAT32	TurretTraverseNeutral	[VEH_MAX_CREW_POSITIONS]; 	//!< neutral position for turret
	FLOAT32	TurretElevateNeutral	[VEH_MAX_CREW_POSITIONS]; 	//!< neutral position for turret
	SINT32	TurretAux				[VEH_MAX_CREW_POSITIONS]; 	//!< additional turret that can be controlled by position, 1-based

// Gunsight
	SINT8	GunsightFile			[VEH_MAX_CREW_POSITIONS][96]; 	//!< gunsight files for crew positions
	SINT8 	InstGunsightFile		[VEH_MAX_CREW_POSITIONS][96]; 	//!< instrument view gunsight files for crew positions
	SINT8 	GunsightAlignmentObj	[VEH_MAX_CREW_POSITIONS][32]; 	//!< gunsight alignment object
	SINT8 	InstGunsightAlignmentObj[VEH_MAX_CREW_POSITIONS][32];	//!< instrument view gunsight alignment object
	FLOAT32	GunsightAlignmentRange	[VEH_MAX_CREW_POSITIONS]; 		//!< range at which gunsight is aligned
	SINT32 	GunsightBlocked			[VEH_MAX_CREW_POSITIONS]; 		//!< when is the gunsight view blocked?
	SINT32 	InstGunsightBlocked		[VEH_MAX_CREW_POSITIONS]; 		//!< when is the instrument view blocked?

// Bombsight
	SINT32	BombSightType; 		//!< what bombsight type, if any, is onboard?
	SINT8 	BombSightMount[32];	//!< what mount does the bombsight use to determine it's bomb table?
	UINT32 	BombSightPosition; 	//!< which position is the bombsight used?
};


/************************************************************************************************
	vehtype
*************************************************************************************************/

extern vehType const* vehTypeGetBySpec(WEAPON_SPEC);
extern vehType const* vehtypeGetByIndex(UINT32);
extern vehType const* vehTypeGetById(const char*);
extern UINT32 vehtypeCount();
extern WEAPON_SPEC vehtypeWeaponSpec(UINT32);
extern const wchar_t* vehTypeLoc(int type, const vehType*);

extern int vehtypeInit();
extern int vehtypeKill();

struct _xmlNode;
typedef void (*vehTypeXMLHandler)(vehType*, _xmlNode*);

extern void vehTypeRegisterXMLHandler(const char* xmlTag, vehTypeXMLHandler);

