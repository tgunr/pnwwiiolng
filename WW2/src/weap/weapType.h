#pragma once

#include "ob/obmain.h"

#define WEAP_MAX_AMMO_TYPE 8

// Ammo subtypes
enum
{
	WEAPAMMO_SUBTYPE_BALLISTIC,
	WEAPAMMO_SUBTYPE_GRENADE,
	WEAPAMMO_SUBTYPE_AIRBURST,
	WEAPAMMO_SUBTYPE_BOMB,
	WEAPAMMO_SUBTYPE_TORPEDO,
	WEAPAMMO_SUBTYPE_ROCKET,
	WEAPAMMO_SUBTYPE_PLACED,
	WEAPAMMO_SUBTYPE_NUKE,
	WEAPAMMO_SUBTYPE_KNIFE,
	WEAPAMMO_SUBTYPE_MORTAR,
	WEAPAMMO_SUBTYPE_MAX
};

enum
{
	WEAPAMMO_CLIPTYPE_NA = -1 // we don't use clips
,	WEAPAMMO_CLIPTYPE_CLIP // this is the WeaponType default (0)
,	WEAPAMMO_CLIPTYPE_MAGAZINE
,	WEAPAMMO_CLIPTYPE_BELT
,	WEAPAMMO_CLIPTYPE_RELOAD // generic clip
,	WEAPAMMO_CLIPTYPE_DRUM
,	WEAPAMMO_CLIPTYPE_MAX
};

enum
{
	WEAP_FIRETYPE_NONE
,	WEAP_FIRETYPE_SINGLESHOT
,	WEAP_FIRETYPE_AUTO_SYNC
,	WEAP_FIRETYPE_AUTO_NORMAL
,	WEAP_MAX_FIRETYPE
};

enum
{
    WEAP_MUZZLEFLASH_DONTUSE,
    WEAP_MUZZLEFLASH_GUN, // rifle
    WEAP_MUZZLEFLASH_BIGGUN, // destroyer guns
    WEAP_MUZZLEFLASH_TANK,
    WEAP_MUZZLEFLASH_GENERIC, // same as gun
    WEAP_MUZZLEFLASH_MORTAR,
    WEAP_MUZZLEFLASH_SMALLTANK,
    NUM_WEAP_MUZZELFLASH
};

#define WEAP_AMMOFLAG_FIRSTFRAME				0x00000001
#define WEAP_AMMOFLAG_CANDAMAGE					0x00000002
#define WEAP_AMMOFLAG_USE_MEDCOLLIDELIST		0x00000004 // use medium terrain collide list
#define WEAP_AMMOFLAG_USE_MED60DEG_COLLIDELIST	0x00000008 // use medium front 60 degrees collide list
#define WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST		0x00000010 // use short collide list
#define WEAP_AMMOFLAG_DONTCHECKGROUNDOBJECTS	0x00000020 // don't bother checking ground objects
#define WEAP_AMMOFLAG_DONTCHECKVEHICLES			0x00000040 // don't bother checking vehicles
#define WEAP_AMMOFLAG_CANTHITPLAYER				0x00000080 // don't include player's vehicle in vehicle collisions
#define WEAP_AMMOFLAG_NOMUZZLEFLASH				0x00000100 // don't display a muzzle flash
#define WEAP_AMMOFLAG_DONTCHECKTERRAIN			0x00000200 // don't check the terrain
#define WEAP_AMMOFLAG_NOFX						0x00000400 // don't do any FX on this ammo
#define WEAP_AMMOFLAG_IS_SHRAPNEL				0x00000800 // is a piece of shrapnel
#define WEAP_AMMOFLAG_IS_GHOSTROUND				0x00001000 // is fired from a ghost vehicle
#define WEAP_AMMOFLAG_DONT_SEND_DAMAGE			0x00002000 // don't send any damage to the host, unless player's vehicle
#define WEAP_AMMOFLAG_ONLY_CHECKPLAYER			0x00004000 // only check player when checking vehicle collisions
#define WEAP_AMMOFLAG_IS_AUTOGUNROUND			0x00008000 // is an autogun round
#define WEAP_AMMOFLAG_ISATTACHED_TO_OBJECT		0x00010000 // ammo is attached to an object
#define WEAP_AMMOFLAG_NOMUZZLEFLASH_SMOKE		0x00020000 // muzzle flash has no smoke component
#define WEAP_AMMOFLAG_FPS_WEAPON				0x00040000 // fps weapon, means some fpFPS calls to notify the fps system
#define WEAP_AMMOFLAG_IS_SPALL					0x00080000 // shrapnel from spall
#define WEAP_AMMOFLAG_IS_SHAPED_CHARGE			0x00100000 // shaped-charge jet "projectile"
#define WEAP_AMMOFLAG_DONTCOLLIDEWITHFIRER		0x00200000 // don't collide with vehicle that fired round
#define WEAP_AMMOFLAG_TPS_WEAPON				0x00400000 // third person infantry weapon
#define WEAP_AMMOFLAG_DAMAGE_SPECIFIC_OBJECT	0x00800000 // ammo can only hurt specific object but collides with all
#define WEAP_AMMOFLAG_SPECIALACTIVE             0x01000000 // ammo object active until finished doing a specific behavior (smoke grenades)

/************************************************************************************************
	weapType
*************************************************************************************************/
struct weapAmmoType
{
	UCHAR Name[32];
	char hiteffect[32];

	UINT16 LocalIndex;
	UINT32 SubType;
	FLOAT32 Diameter;
	FLOAT32 ProjectileWeight;
	FLOAT32 AmmoWeight;
	FLOAT32 InitVelocity;
	FLOAT32 TracerTime;
	FLOAT32 TracerScale;
	FLOAT32 BallisticCoefficient;
	FLOAT32 LifeTime;
	UINT32 StretchObject;
	UINT32 IgnoreFirstFrame;
	UINT32 UseParticlesForTracer;
	FLOAT32 PenetrationConstant;
	FLOAT32 RichochetAngle;
	UINT32 ModelSpall;

	UINT32 NumShrapnelPieces;
	FLOAT32 TotalShrapnelMass;
	FLOAT32 ShrapnelPenetrationConstant;
	FLOAT32 ExplosiveEnergy;

	FLOAT32 FuzeTime;
	FLOAT32 FuseStartSpeedLoss;
	UINT32	FuzeType; // for variable fuzing
	
	FLOAT32 ArmingTime; // for variable arming timers

	FLOAT32 SelfDestructTime;

	SINT8 ExplosionNameNear[64];
	SINT8 ExplosionNameFar[64];
	FLOAT32 NearFarSoundDist; // threshold for using far sound instead of near

	LOGICAL IsPlaced; // ammo is placed on something to work

	FLOAT32 GrenadeRotateRate;
	SINT32 HasSmoke; // 1 for only smoke, 2 for smoke without disabling other functionality

	LOGICAL UseGhostRoundSTO; // do we use an sto for the ghost round so everyone is sycned up?

	FLOAT32 ShapedChargePen; // penetration distance for shaped charge

	FLOAT32 TorpedoRunDepth;
	FLOAT32 TorpedoRunSpeed;

	// new HE model exposed variables
	SINT32	UseNewHE;
	FLOAT32	ShrapProjDiameter;
	FLOAT32	ShrapProjWeight;
	FLOAT32	ShrapProjMinPenConst;
	FLOAT32 ShrapProjRicochetAngle;
	FLOAT32	ShrapEnergyMult;
	SINT32	MinShrapPieces;
	SINT32	MaxShrapPieces;
	UINT32	MaxShrapPerVehicle;
	FLOAT32	AddedParticleSpeed;
	FLOAT32	ParticleTimeout;
	FLOAT32	MaxMassMultiplier;

	SINT32 MuzzleFlashType;

	FLOAT64VECTOR ModelCM;
	FLOAT64 Ixx, Iyy, Izz, Ixz, Ixy, Iyz;
	FLOAT32 GrenadeOrient;
	FLOAT32 GrenadeRestitution;

	OBJECT* Object;
};

struct weaponType
{
	UCHAR Name[32];
	char muzzleflash[32];

	FLOAT32 Mass;
	UINT32 FireType;
	FLOAT32 RateOfFire; // in rounds/minute
	FLOAT32 GhostRateOfFire; // rate of fire when on a vehicle controlled by host
	FLOAT32 Dispersion; // this is* RADIUS* not* DIAMETER*
	UINT32 HideOnLastDrop; // this indicated to hide the weapon when last round fired
	UINT32 NumRoundsInClip; // number of rounds in clip, 0 for continuous
	FLOAT32 ReloadTime; // time to reload the gun, in seconds
	UINT32 IsPlacedWeapon; // is weapon a "placed" weapon?
	UINT32 NoMuzzleSmoke; // weapon fire generates no smoke
	LOGICAL bShockDust;     // weapon firing causes shock smoke
	UINT32 MaxOverSupply; // number of times start ammo this weapon can be oversupplied
	FLOAT32 InfResupply; // 0 - 1 float that indicates a percent of total ammo to fill on inf resupply
	SINT32 ClipType; // defaults to clip (0)

	weapAmmoType* AmmoType[WEAP_MAX_AMMO_TYPE];
	weapAmmoType* GhostAmmoType;
};

extern void weapTypeInit();
extern weaponType const* weapTypeFindByName(const char*);
extern weapAmmoType const* weapAmmoTypeFindByName(const char*);
