#pragma once

#include "ml/mltypes.h"			// For ML_MAX_TRACK_VERTICES etc
#include "pi/pi.h"					// For piVECTOR etc

struct mlObjectTrackSection
{
	struct OBJECT* TrackSegment;
	struct OBJECT* TrackSegmentObjectToLinkTo;
	UINT32 LinkToVertex[ML_MAX_TRACK_VERTICES][2];
	UINT32 NumVertices;

};

struct mlObjectTrackAnimation
{
	struct OBJECT * TrackAnimationObject;
	UINT32 TrackNum;
	FLOAT32 TrackMoveRate;
};


struct mlObjectCollidePnt
{
	struct OBJECT * CollideObject; // the collide object;
	piVECTOR SlideVel; // the slide velocity of the contact object
	piVECTOR ForceArm; // the force arm of the contact object
	piVECTOR Force; // the force velocity of the contact object
	piVECTOR LastForce; // the last force the contact object exerted
	FLOAT32  FrictionCo; // the saved collision point friction coefficient
	FLOAT32  BumpDensity; // the saved collision point bump density
	FLOAT32  BumpHeight; // the saved collision point bump height
	FLOAT64  ShockDisplacement; // the impact shock contact point displacement
	FLOAT64  ShockVel; // the current velocity of the contact point relative to vehicle
	FLOAT32  RollFrictionCo;
	FLOAT64VECTOR ForceDir;
	FLOAT64VECTOR RatioForce; // for internal contact point use
	FLOAT64VECTOR LastContactPnt;
	FLOAT64 Length;
	char VisDamage[32];
	LOGICAL IsSpike;
};


struct mlObjectData
{
	struct OBJECT* PlaneObject;

	struct mlObjectTrackSection* TrackSection;
	UINT32 TrackCount;

	struct mlObjectTrackAnimation* TrackAnimation;
	UINT32 TrackAnimationCount;

	struct OBJECT* HitMap; // used for ballistic collisions
	struct OBJECT* HitBox; // used for vehicle-vehicle collisions
	struct OBJECT* CollideExternObject; // the point-line collision group for external stuff
	struct OBJECT* CollideInternObject; // the point-line collision group for internal stuff
	struct OBJECT* CollideWalkObject; // the point-line collision group for walk stuff
	struct OBJECT* EffectsObject; // for emitter DOFs

	struct OBJECT* MassObject; // mass distribution model

// visdamage
	struct OBJECT* VisDamageGroup; 	// top level visdamage group
	struct OBJECT* VisDamageCrapGroup; // for crap flying off
	struct OBJECT* VisDamageStubGroup; // for stubs
	struct OBJECT* VisDamageHulkLOD; 	// the hulk LOD

	struct OBJECT* LineCollideObject; // line collide object;
	UINT32 NumCollideObjects; // number of collision objects
	struct mlObjectCollidePnt* CollidePntList; // collide pnt list

	struct OBJECT* LODObjects[ML_MAX_LODS]; // lod objects

	FLOAT32 SavedMinLODRange[ML_MAX_LODS]; // saved minimum LOD ranges
	FLOAT32 SavedMaxLODRange[ML_MAX_LODS]; // saved maximum LOD ranges

	struct OBJECT* LoadoutLODObject; // weapons lod for imported weapons
	FLOAT32 SavedMinLODLoadoutRange;
	FLOAT32 SavedMaxLODLoadoutRange;

	FLOAT32 Radius; // radius of object
	piVECTOR BBox1,BBox2; // Vehicle bounding box
	FLOAT32 LowestPoint; // the lowest point on the vehicle z value
	FLOAT32 HighestPoint; // the heigest point on the vehicle z value

	// wheel stuff for host-controlled vehicles
	FLOAT32* WheelRadius;

	struct OBJECT* TowObject;
	UINT32 TowType;
	struct OBJECT* HoistObject;

	UINT32 NumFPSSeats;
	struct OBJECT* FPSSeats[ML_MAX_FPS_SEATS];

	UINT32 NumVehSeats;
	struct OBJECT* VehSeats[ML_MAX_VEH_SEATS];

	struct OBJECT* ShadowObject;

	struct OBJECT*	GearUpGroup[ML_MAX_GEARUPDOWN_GROUPS];
	UINT32 	GearUpCount;
	struct OBJECT*	GearDownGroup[ML_MAX_GEARUPDOWN_GROUPS];
	UINT32 	GearDownCount;
	struct OBJECT*	FlapsUpGroup[ML_MAX_FLAPSUPDOWN_GROUPS];
	UINT32 	FlapsUpCount;
	struct OBJECT*	FlapsDownGroup[ML_MAX_FLAPSUPDOWN_GROUPS];
	UINT32 	FlapsDownCount;
	struct OBJECT*	EngineFastGroup[ML_MAX_ENGINES][ML_MAX_ENGINEFASTSLOW_GROUPS];
	UINT32 	EngineFastCount[ML_MAX_ENGINES];
	struct OBJECT*	EngineSlowGroup[ML_MAX_ENGINES][ML_MAX_ENGINEFASTSLOW_GROUPS];
	UINT32 	EngineSlowCount[ML_MAX_ENGINES];

	// Turret Convergence, would be nice to put this in the mlweapon struct
	// but there's no easy way to tie the turret positions to the weapons
	struct OBJECT* FireDirectorObj[ML_MAX_CREW_POSITIONS];
	struct OBJECT* TurretConvObj[ML_MAX_CREW_POSITIONS][VEH_MAX_MOUNTS];
	struct OBJECT* FirePoint[ML_MAX_CREW_POSITIONS][VEH_MAX_MOUNTS];
	LOGICAL TraverseReversed[ML_MAX_CREW_POSITIONS];

	// For hoist collision detection
	struct OBJECT* HoistHitBox;
	struct OBJECT* HoistLineCollideObject;
	UINT32 NumHoistCollideObjects;
	struct mlObjectCollidePnt* HoistCollidePntList;

	struct OBJECT*	WeaponFirePoints	[VEH_MAX_MOUNTS][4];
	struct OBJECT* WeaponConnectPoints	[VEH_MAX_MOUNTS][4]; // used for hiding weapons

    // Mobile Spawn Object
    struct OBJECT* spawnObject;

    LOGICAL ProcessContactPointObject;

    mlObjectData()
    {
        Reset();
    }

    void Reset();

	static mlObjectData* Get();
	static void Release(mlObjectData* object);
};

typedef void (*mlForEachCallback)(struct OBJECT*, void*);
struct vehVehicle;
struct vehType;

extern UINT32 mlobjectKillVehicleObject(struct mlObjectData ** ObjectData);


extern UINT32 mlobjectLoadVehicleObject(struct mlObjectData ** ObjectData,
								 UCHAR * ObjectName,
								 UINT32 NumDamageObjects,
								 struct vehVehicle*Vehicle,
								 UINT32 LoadoutNum,
								 UCHAR * DamageObjectList,
								 LOGICAL LoadCockpit);

extern UINT32 mlMoveAndRotateObject(struct OBJECT * Object,struct FLOAT64VECTOR * inPos,struct FLOAT64ATT * InAtt);
extern UINT32 mlObjectSetInplaneObject(struct OBJECT * BaseObject,UINT32 Position);
extern UINT32 mlObjectSetExternObject(struct OBJECT * BaseObject);
extern UINT32 mlSetModelSpecialPoints(void* InVehicleData,struct mlObjectData * VehicleObjectData);
extern UINT32 mlApplyVelocityToObject(struct OBJECT * PlaneObject,struct FLOAT64VECTOR * Vel);
extern UINT32 mlDrawProps(void);
extern UINT32 mlForEachSubObject(struct OBJECT*, mlForEachCallback, void* data, LOGICAL IncludeSib);
extern UINT32 mlProcessContactPointObject(struct mlObjectData* ObjectData);
extern UINT32 mlInitVehicleObject(struct mlObjectData** ObjectData, struct vehType const* VehicleType,
							LOGICAL LoadCockpit);
extern UINT32 mlCalculateObjectTreeRadius(FLOAT32 * Radius,OBJECT * Object, UINT32 IncludeSibs);

