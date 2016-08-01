#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>

#define FLOAT32 float
#define FLOAT64 double
#define UCHAR unsigned char
#define UINT16 uint16_t
#define UINT32 uint32_t
#define SINT8 int8_t
#define SINT32 int32_t
#define LOGICAL bool

struct FLOAT64VECTOR {
	FLOAT64 x;
	FLOAT64 y;
	FLOAT64 z;
};

struct piVECTOR
{
	FLOAT32 x;
	FLOAT32 y;
	FLOAT32 z;

	void set(float X, float Y, float Z)
	{
		x = X;
		y = Y;
		z = Z;
	}
};

struct FLOAT64ATT {
	FLOAT64 Roll;
	FLOAT64 Pitch;
	FLOAT64 Yaw;
};

#define mathExactVectorLength(A) \
			(sqrt((A)->x*(A)->x + (A)->y*(A)->y + (A)->z*(A)->z))

//------------------------------

struct weapAmmoType {
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

	FLOAT32 SelfDestructTime;

	char ExplosionNameNear[64];
	char ExplosionNameFar[64];
	FLOAT32 NearFarSoundDist; // threshold for using far sound instead of near

	LOGICAL IsPlaced; // ammo is placed on something to work

	FLOAT32 GrenadeRotateRate;
	LOGICAL IsSmokeGrenade;

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

	//FLOAT64VECTOR ModelCM;
	FLOAT64 Ixx, Iyy, Izz, Ixz, Ixy, Iyz;
	FLOAT32 GrenadeOrient;
	FLOAT32 GrenadeRestitution;

	//OBJECT* Object;
};

struct weapAmmo
{
	weapAmmo * Next;
	weapAmmo * Prev;

	UINT32 Id;
	UINT32 Flags;
	SINT32 SoundID;
	SINT32 iParticleID;

	bool inuse;

	struct weapAmmoType * AmmoType;
	struct mlVehicleData * VehicleData;

	struct OBJECT * AttachedObject;
	void* AttachedToVehicle;
	void* TO;

	void* FireVehicle; // vehicle firing the round
	void* SpecificDamageObject; // specific thing (vehicle OR AG .. hence void*) that's to be checked
	void* LastVehicleHit;

	class physicsBody* Body;

	LOGICAL TorpedoIsRunning;

	LOGICAL FirstUpdate : 1;
	LOGICAL KillAmmo : 1;

	FLOAT64VECTOR Position;
	FLOAT64VECTOR LastCollidePos;
	FLOAT64VECTOR PrevRenderPos;
	FLOAT64VECTOR Velocity;
	FLOAT64VECTOR LastAcc;
	FLOAT64VECTOR LastVel;
	FLOAT64ATT Att;

	FLOAT64 LaunchTime;
	FLOAT64 LastUpdateTime;
	FLOAT64 PrevUpdateTime; // used for stretching tracers
	FLOAT64 LastCollideTime;

	FLOAT64 FuseStartTime;

	piVECTOR AttachedPnt;
	piVECTOR AttachedNorm;
	piVECTOR OrigNorm;

	FLOAT32 TorpedoRunDepth;
	//piMATRIX FireAtt; // Used for torpedo to stay in same direction

	FLOAT32 VarFuzeInterval;
};


void ReadBallisticChart(void);
weapAmmoType ReadAmmoType(const std::string& name);
weapAmmo CreateRound(weapAmmoType& ammo_type);
void UpdateProjectile(weapAmmo* Ammo, double CurrentTime);
FLOAT64 PiercingPotential(const weapAmmo& round);