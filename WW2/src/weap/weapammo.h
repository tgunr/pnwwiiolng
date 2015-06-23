#pragma once

#include "ob/obmain.h"
#include "ml/mltypes.h"
#include "weap/weapType.h"

#define G1_CHART_MACH_DELTA 0.005
#define G1_CHART_NUM_DIVISIONS 1001

#define _AMMOCD_FLAG_ISABUILDING				0x00000001
#define _AMMOCD_FLAG_ISAVEHICLE					0x00000002
#define _AMMOCD_FLAG_ISTERRAIN					0x00000004
#define _AMMOCD_FLAG_INTERNALCOMPONENT			0x00000008
#define _AMMOCD_FLAG_EXTERNALCOMPONENT			0x00000010
#define _AMMOCD_FLAG_ISPERSONA					0x00000020
#define _AMMOCD_FLAG_ISAUTOGUN					0x00000040
#define _AMMOCD_FLAG_ISTREE                     0x00000080
#define ODE_TREE_COLLISION                      0x00000100

#define max_trajectory_steps    64

class physicsBody;

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
	piMATRIX FireAtt; // Used for torpedo to stay in same direction

	FLOAT32 VarFuzeInterval;
};

struct AmmoTrajectory
{
	void Reset()
	{
		points.clear();
		fireMatrix.identity();
		impactPosition.set(0,0,0);
		impactNormal.set(0,0,0);
	}

	Matrixd fireMatrix;
	Vec3d impactPosition;
	Vec3d impactNormal;
	std::vector<Vec3d> points;
};


struct vehVehicle;

extern FLOAT32 G1MachChart[G1_CHART_NUM_DIVISIONS];

UINT32 	weapAmmoInit(void);
UINT32 	weapAmmoKill(void);

UINT32	weapAmmoKillAllAmmo(void);
void weapAmmoClearVehicle(vehVehicle* v);

UINT32 	weapAmmoObjectCeasingToExist(struct OBJECT* Object);

UINT32 	weapFindAmmoTypeByName(struct weapAmmoType ** AmmoType,UCHAR * VarContents);
struct weapAmmoType* 	weapFindAmmoTypeByIndex(UINT32 i);

void weapAmmoSetTorpedoRunDepth(FLOAT32);
void weapAmmoSetFuzeInterval(FLOAT32);
FLOAT32 weapAmmoGetFuzeInterval(void);

// ammo is the ammo to track
// fire pos is the position to fire from
// fire att is a vector with positive y pointing in the direction of fire
AmmoTrajectory& weapAmmoTrajectory(const weapAmmoType* ammo,
								   const Matrixd& fireMat,
								   FLOAT64 speed,
								   FLOAT64 timeStep,
								   SINT32 maxSteps);

void weapAmmoTrajectory(AmmoTrajectory& trajectory,
						const weapAmmoType* ammo,
						const Matrixd& fireMat,
						FLOAT64 speed,
						FLOAT64 timeStep,
						SINT32 maxSteps);


UINT32 	weapUpdate(void);
UINT32 	weapUpdateVehicleHitmap(struct vehVehicle* InVehicle);

UINT32  weapAmmoSetRoundPos(struct piVECTOR* Pos);
LOGICAL weapDrawTracers(void);

UINT32 	weapFireRound(struct FLOAT64VECTOR* RecoilForce, struct weapAmmoType* AmmoType,
						struct FLOAT64VECTOR* Pos, struct FLOAT64VECTOR* Vel,
						union piMATRIX* FireAtt, FLOAT64 LaunchTime, UINT32 Flags,
						struct OBJECT* EmitterObject, struct FLOAT64VECTOR* RelativePnt, struct weapWeapon* weapon, void* FireVehicle);

UINT32 	weapPlaceRoundOnObject(struct piVECTOR* CollidePnt, struct OBJECT* CollideObject,
								struct piVECTOR const* CollideNorm, struct piVECTOR const* OrigNorm,
								UINT32 Flags, FLOAT64 LaunchTime,
								struct weapAmmoType* AmmoType,
								void* Vehicle, void* TO);

UINT32 	weapCollideWithHitmapObject(struct FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
									struct OBJECT* CurrentObject, struct OBJECT* TopObject,
									struct piVECTOR const* OldPnt, struct piVECTOR const* NewPnt,
									UINT32 DontIgnoreBackface, UINT32 DontCheckSibs);

UINT32 	weapCollideRoundWithVehicles(struct FASTCOLLIDE** CollideListPtr, UINT32* NumCollisionSlotsLeft,
									 struct piVECTOR const* OldPos, struct piVECTOR const* NewPos,
									 FLOAT64 OldPosTime, FLOAT64 NewPosTime,
									 UINT32 CanHitPlayer,  UINT32 CheckOnlyPlayer,
									 struct FLOAT64VECTOR const* WorldOriginOffset,
									 UINT32 UseHitBoxInstead, SINT32 Side);

#if USE_STATESET
void DrawWeaponsInFlight();
#endif