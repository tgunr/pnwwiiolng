#pragma once

#include "ob/obcd.h"
#include "strt/strtDTL.h"
#include "weap/weapDefs.h"
#include "weap/weapSpec.h"
#include "include/vars.h"

#include <map>
#include <vector>
#include <string>

#define TRN_MAX_LOD 16

#define TRN_MAX_SPAWN_LOCATIONS 16

#define TRN_WEAPON_DISTANCEBYTIME_TABLE_SIZE 100

#define TRN_MAX_AUTOGUN_TRACK 16
#define TRN_MAX_AUTOGUN_ENGAGE 4

#define TRN_OBJ_FIRE_TIME1	20000
#define TRN_OBJ_FIRE_TIME2	120000

// Flags for trnCDTerrainObject uFlags
#define TRN_OBJECT_BUILDER  0x00000001  // Builder Statue
#define TRN_OBJECT_ONMEDIUMLIST  0x00000002  // on the medium collide list
#define TRN_OBJECT_ONSHORTLIST  0x00000004  // on the short collide list
#define TRN_OBJECT_INACTIVE  0x00000008  // not currently collidable
#define TRN_OBJECT_DESTROYABLE  0x00000010  // destroyable scenario object

class BoundBox
{
	private:

		// This is in object space
		piVECTOR	minBounds;
		piVECTOR	maxBounds;


		// This is in world space (call to Update() required when accessing these at the current world position)
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//	corners[0] : left,	back,	bottom
		//	corners[1] : right, back,	bottom
		//	corners[2] : right,	front,	bottom
		//	corners[3] : left,	front,	bottom
		//	corners[4] : left,	back,	top
		//	corners[5] : right,	back,	top
		//	corners[6] : right,	front,	top
		//	corners[7] : left,	front,	top
		//////////////////////////////////////////////////////////////////////////////////////////////////
		piVECTOR	corners[8];

		//////////////////////////////////////////////////////////////////////////////////////////////////
		//	sideNormals[0] : bottom
		//	sideNormals[1] : top
		//	sideNormals[2] : front
		//	sideNormals[3] : back
		//	sideNormals[4] : left
		//	sideNormals[5] : right
		//////////////////////////////////////////////////////////////////////////////////////////////////
		piVECTOR	sideNormals[6];

		void	CreateBounds(const OBJECT* o, piVECTOR& min, piVECTOR& max, LOGICAL singleObject);
		void	Update();

	public:

		BoundBox(const OBJECT* o, LOGICAL singleObject);
//		BoundBox(int fd);

//		void	SaveToFile(int hfile);
//		void	Transform(const piMATRIX* m);
//		void	Translate(const piVECTOR* v);

		const piVECTOR& Min(){ return minBounds; }
		const piVECTOR& Max(){ return maxBounds; }

#ifdef	_DEBUG
		void	Draw()	const;
#endif
};

struct trnAutoGunType
{
	UINT32 Type;

	FLOAT32 TraverseRate[MAX_COUNTRIES]; // in rad/sec
	FLOAT32 ElevateRate[MAX_COUNTRIES]; // in rad/sec

	SINT8 WeaponName[MAX_COUNTRIES][64]; // name of weapon autogun is associated with (per country)
	SINT8 ObjectName[MAX_COUNTRIES][64]; // object name for autogun per country
	SINT8 BeltFile[MAX_COUNTRIES][64]; // belt file used for the gun

	FLOAT32 MaxEffectiveRange[MAX_COUNTRIES]; // maximum effective range of gun
	FLOAT32 MaxConeAngle[MAX_COUNTRIES]; // maximum cone angle of gun
	FLOAT32 MinConeAngle[MAX_COUNTRIES]; // minimum cone angle of gun
	FLOAT32 TimeToMinCone[MAX_COUNTRIES]; // time engaging to minimum cone angle

	FLOAT32 VisAdditionalAngle[MAX_COUNTRIES]; // additional angle from arc of fire that gun can target

	LOGICAL GunFiresContinuously[MAX_COUNTRIES]; // is this gun a sustained-fire gun?
	FLOAT32 AngleOffMult[MAX_COUNTRIES];  // angle-off speed required to neutralize cone shrink
	FLOAT32 DegreesOffMult[MAX_COUNTRIES]; // number of degrees off of target to neutralize cone shrink
	FLOAT32 MaxEngagementRange[MAX_COUNTRIES]; // maximum range that gun cares about targets

	UINT32 EngageVehiclePriority[MAX_COUNTRIES][3]; // first 3 vehicle types engaged

	SINT8 FireSound[MAX_COUNTRIES][64]; // sound file used for firing
	SINT8 ReloadSound[MAX_COUNTRIES][64]; // sound file used for reloads

};

struct SpawnData
{
    std::map<UINT32, std::vector<OBJECT*> > classToObject;
    std::string buildingname;

    inline OBJECT* findSpawnObject(WEAPON_SPEC& spec)
    {
        WEAPON_SPEC ws = NULL_WEAP_SPEC;
        ws.wClass = spec.wClass;

        std::map<UINT32, std::vector<OBJECT*> >::iterator itr = classToObject.find(ws.asUINT32());

        // Did not find spec by class only (some dofs are specified for classes only
        if(itr == classToObject.end())
        {
            // Some dofs specify weapons too. So look it up if the class is not found.
            itr = classToObject.find(spec.asUINT32());
        }

        if(itr != classToObject.end())
        {
            int size = itr->second.size();
            int index = frametime % size;
            return itr->second[index];
        }

        return nullptr;
    }

    inline void addSpawnObject(unsigned int key, OBJECT* o)
    {
        // We can have multiple spawn objects for a specific weap spec.
        std::map<UINT32, std::vector<OBJECT*> >::iterator itr = classToObject.find(key);

        if(itr != classToObject.end())
        {
            // More than one dof in this building can spawn the same vehicle
            itr->second.push_back(o);
        }
        else
        {
            // Only one dof in this building can spawn this vehicle
            std::vector<OBJECT*> v;
            v.push_back(o);
            classToObject.insert(make_pair(key, v));
        }
    }
};

namespace StratDB { class StratObject; }

struct trnCDTerrainObject
{
	trnCDTerrainObject* Next;
	trnCDTerrainObject* Prev;

	OBJECT * TerrainObject;
	OBJECT * TerrainMasterObject;
	OBJECT * CollisionObject;
	OBJECT * DamageCollisionObject;
	OBJECT * LOD[TRN_MAX_LOD];
	SINT32 LODDamageLevel[TRN_MAX_LOD];
	FLOAT32 LODSavedMin[TRN_MAX_LOD];
	FLOAT32 LODSavedMax[TRN_MAX_LOD];
	SINT32 DamageLevel;
	SINT32 LastFXDamageLevel;
	UINT32 InUse;
	piVECTOR BBox1; // note: these are in world space
	piVECTOR BBox2; // note: these are in world space
	piVECTOR Center;
	piVECTOR OrigCenter;
	FLOAT32 Radius;
	FLOAT32 Alpha; // the alpha transparancy of the point of collision, 0 = , 1.0 =
//	piMATRIX InvMat;

	piVECTOR AimOffset; // used to compute lead

	// strat system stuff
	SINT32 StratSupercellX;
	SINT32 StratSupercellY;
	UINT32 StratId;

	UINT32 Country;
	UINT32 FirstUpdate;
	SINT32 FireStartTime;
	LOGICAL LevelInited;

	SINT16 SpawnEntries;
	OBJECT* SpawnLocations[TRN_MAX_SPAWN_LOCATIONS];
	SINT32 DamThreshold;

	REL_OBJ_ID ObjId;

	SINT32 EnemyReportTime; // for STRAT_ENEMY_REPORT updates

    UINT32 uFlags;

	struct trnAutoGun * TrnAutoGun;


	void* physicsBoxCollider;
	void* physicsColliderSpace;
	SINT32 physicsColliderDamageLevel;

    SpawnData* spawndata;

	// start lua compatibility functions with OSG
	StratDB::StratObject* StratObj();
	const char* ObjName();
	unsigned char ObjFlags(){ return TerrainObject->flags; }

	int SupercellX(){ return StratSupercellX; }
	int SupercellY(){ return StratSupercellY; }
	// end compatibility

	bool IsSTO()
	{
		return (TerrainMasterObject && (TerrainMasterObject->flag3 & OBJECT_STO) && TerrainMasterObject->lpspare);
	}

	bool IsCollidable(UINT32 cflags);

};

struct trnAutoGun
{
	struct trnAutoGun * prev;
	struct trnAutoGun * next;

	FLOAT32 DistSqr;

	UINT32 AGDeadOrHidden;

	UINT32 AGRunModel;

	trnAutoGunType * Type;
	SINT32 Country;

	OBJECT * Object;
	OBJECT * FirePnt;
	OBJECT * ViewPnt;
	OBJECT * CollisionObject;

	FLOAT32 Elevate;
	FLOAT32 Traverse;

	FLOAT64VECTOR Pos;
	piVECTOR EyePos;
	piMATRIX rotateMatrix;

	FLOAT32 MinTraverse;
	FLOAT32 MaxTraverse;
	FLOAT32 CenterTraverse;
	FLOAT32 MinElevate;
	FLOAT32 MaxElevate;
	FLOAT32 LeftRightAngle;

	LOGICAL GunIsFiring;
	LOGICAL FireAtTarget;

	FLOAT32 ConeSize;

	piVECTOR AimPntDisplacement;
	piVECTOR AimPntLead;

	UINT32 StormTrooperFire;

	SINT32 NextBurstStartTime;
	SINT32 BurstEndTime;

	SINT32 ReAimStartTime;

	FLOAT32 TimeToTarget;
	FLOAT32 LastTargetAngleOff;

	void*  TrackingList[TRN_MAX_AUTOGUN_TRACK];
//	void*  EngageList[TRN_MAX_AUTOGUN_ENGAGE];
	SINT32 NextTrackingAdjustmentTime;
	SINT32 NextTargetSelectTime;

//	weapWeapon Weapon;
	struct weapWeapon* Weapon; // weapWeapon structure
	void*  Target; // vehVehicle structure
	void*  LastTarget; // vehVehicle structure

	struct trnCDTerrainObject * TO;

	SINT32 FireSoundId; // firing sound id, for non-singleshot weapons

	SINT32 LastGunUpdateTime;

	SINT32 LastLeadCalcTime;

};

	extern LOGICAL trncdCheckTerrainCollision(OBJECT* lpobj, COLLISION* collision);
	extern LOGICAL trncdCheckTerrainDown(COLLISION* collision);
	extern LOGICAL trncdCheckTerrain(COLLISION* collision);
	extern LOGICAL trncdCheckOriginalTerrain(COLLISION* collision);
	extern LOGICAL trncdPlayerToTerrain(piVECTOR* oldpos, piVECTOR* newpos);
	extern LOGICAL trncdObjectToTerrain(OBJECT* object, piVECTOR* oldpos, piVECTOR* newpos);

	extern LOGICAL trncdCheckObjectCollision(OBJECT* lpobj, COLLISION* collision);
	extern LOGICAL trncdCheckObject(COLLISION* collision);
	extern LOGICAL trncdPlayerToObject1(piVECTOR* oldpos, piVECTOR* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly);
	extern LOGICAL trncdPlayerToObject(piVECTOR* oldpos, piVECTOR* newpos);
	extern LOGICAL trncdObjectToObject(OBJECT* object, piVECTOR* oldpos, piVECTOR* newpos);


	extern LOGICAL trncdPlayerOnGround(piVECTOR* player, piVECTOR* oldpos, piVECTOR* newpos);
	extern LOGICAL trncdCollideOnGround(piVECTOR* oldpos, piVECTOR* newpos, piVECTOR* retpos, OBJECT** lpobj, SINT32* poly);

	extern UINT32 trncdInit(void);
	extern UINT32 trncdKill(void);

	extern LOGICAL trncdAddTerrainObject(OBJECT * Object,SINT32 SuperCellX,SINT32 SuperCellY,UINT32 Id,
									REL_OBJ_ID * ObjId);
	extern LOGICAL trncdRemoveTerrainObject(OBJECT * Object,SINT32 DummyNum1,SINT32 DummyNum2,UINT32 Id,
									REL_OBJ_ID * ObjId);

	extern UINT32 trncdGetTOHead(trnCDTerrainObject ** TOHead);

	extern void trncdRemoveODEColliderImmediate(trnCDTerrainObject& TO, bool release);
	extern void trncdRemoveODEColliderQueued(trnCDTerrainObject& TO);
	extern void trncdProcessODEQueue(bool all = false);

	extern UINT32 trncdGetNumTOs(void);

	// steps through the list until out of objects or a Terrain object is collided
	extern UINT32 trncdCollideWithListOfTerrainObjects(FASTCOLLIDE ** CollisionList,
												UINT32 * NumCollisionSlotsLeft,
												trnCDTerrainObject ** TOList,
												UINT32 MaxNumInTOList,
												piVECTOR const* OldPnt,
												piVECTOR const* NewPnt,
												UINT32 DontIgnoreBackface,
												UINT32 Flags = 0);

	// steps through the object list until out of objects or a collision occurs
	// different from other collider of similar name in that the exact objects to check are passed in
	extern UINT32 trncdCollideWithListOfTerrainObjects1(FASTCOLLIDE ** CollisionList,
												UINT32 * NumCollisionSlotsLeft,
												OBJECT ** CollideObjects,
												trnCDTerrainObject ** TOList,
												UINT32 MaxNumInTOList,
												piVECTOR* OldPnt,
												piVECTOR* NewPnt,
												UINT32 DontIgnoreBackface,
												UINT32 Flags = 0);

	extern UINT32 trncdCollideLineCollisionObjectWithTerrain(FASTCOLLIDE ** CollisionList,
													  UINT32 * NumCollisionSlotsLeft,
													  trnCDTerrainObject ** TOList,
													  UINT32 MaxNumInTOList,
													  OBJECT * CollideObject,
													  UINT32 Flags = 0);

	extern UINT32 trncdCollideWithSpecificTerrainObject(FASTCOLLIDE ** CollisionList,
												UINT32 * NumCollisionSlotsLeft,
												OBJECT * CurrentObject,
												OBJECT * TopObject,
												trnCDTerrainObject* TO,
												const piVECTOR * OldPnt,
												const piVECTOR * NewPnt,
												UINT32 DontIgnoreBackface,
												UINT32 Flags = 0);

	extern UINT32 trncdCollideWithAllTerrainObjects(FASTCOLLIDE** CollisionList, UINT32* NumCollisionSlotsLeft,
											piVECTOR* OldPnt, piVECTOR* NewPnt, UINT32 DontIgnoreBackface, UINT32 Flags = 0);

	extern UINT32 trncdAdjustTerrainObjects(piVECTOR * CameraAdjustment);
	extern UINT32 trncdInitSingleTerrainObject(trnCDTerrainObject* TO);

	// generates a list of poly/object pairs of the terrain that intersect
	// the specified bounding box
	extern LOGICAL trncdGetListOfTerrainPolysNearBoundingBox(piVECTOR * BBox,
													  OBJECT ** CollideObj,
													  UINT32 * CollidePoly,
	  												  UINT32 * NumCollisionSlotsLeft,
													  UINT32 Flags = 0);

	// collides with a specific list of object/poly list
	extern UINT32 trncdCollideWithPolyList(piVECTOR * OldPos,
									piVECTOR * NewPos,
									piVECTOR * CollidePnt,
									OBJECT ** CollideObject,
									UINT32 * PolyNum,
									OBJECT ** CollideObjectList,
									UINT32 * PolyNumList,
									UINT32 NumPolys,
									UINT32 Flags = 0);

	// collide with ode colliders
	/*
	extern bool trncdCollideUsingODE(const Vec3d& p1,
									const Vec3d& p2,
									FASTCOLLIDE** list,
									unsigned int& SlotsLeft,
									const OBJECT& o,
									const OBJECT& TopObject,
									const trnCDTerrainObject& to,
									const unsigned int collidebits);
	*/

	extern UINT32 trnUpdateTerrainObjectLODs(UINT32 Override_);
	extern UINT32 trnSetDamageLevel(trnCDTerrainObject * TO,UINT32 DamageLevel_);
	extern UINT32 trnSetCountry(trnCDTerrainObject * TO_, UINT32 Country_);

	extern UINT32 trnUpdateObjectStatus(SINT32 x, SINT32 y, UINT32 id, SINT32 country = -1, SINT32 vislevel = -1);

	extern UINT32 trnGetSpawnObject(OBJECT ** ObjectPtrPtr,
							 SINT32 SuperCellX,
							 SINT32 SuperCellY,
							 UINT32 Id,
							 UINT32 SpawnNum,
                             WEAPON_SPEC& spec);
	extern LOGICAL trncdCollideDownMediumList(piVECTOR* OldPos, piVECTOR* NewPos, UINT32 Flags = 0);
	extern LOGICAL trncdCollideDownShortList(COLLISION& c, FASTCOLLIDE list[32],  UINT32& numcollides, UINT32 Flags = 0);

	extern void trnFindCollisionObject(OBJECT ** const CollisionObject, OBJECT* Object);

	extern UINT32 trncdUpdateStratMsgQue(void);

	extern LOGICAL trncdCollideWithVehiclesAndTerrain(piVECTOR* V1,
											   piVECTOR* V2,
											   piVECTOR* CollisionPnt,
											   OBJECT ** CollisionObject,
											   SINT32* CollisionPolyNum,
											   void* CollisionVehicle,
											   trnCDTerrainObject** TO,
											   LOGICAL CollideWithAllTerrainObjects,
											   LOGICAL IgnoreVehicles,
											   LOGICAL CollideWithInfantry,
											   UINT32 Flags = 0);

	extern SINT32 trncdReplacementTileTextureID();

// new collider stuff
struct trncdDTLCollider;
struct TERRAIN_DETAIL;

extern trncdDTLCollider* trncdDTLColliderAlloc(struct TERRAIN_DETAIL*);
extern void trncdDTLColliderFree(trncdDTLCollider*);

extern SINT32 trncdCollideWithBaseTerrain(FASTCOLLIDE* collisionList, SINT32 maxcol, piVECTOR const* start, piVECTOR const* end);
extern SINT32 trnSetBuilderFlag(SINT32 x, SINT32 y, UINT32 iID);

extern SINT32 trncdCollideSpeedTree(OBJECT* lpObj,
                             OBJECT* lpTop,
                             FASTCOLLIDE** ppCollisionList,
                             UINT32* pNumSlotsLeft,
                             piVECTOR const* lpPt1,
                             piVECTOR const* lpPt2);

extern void trncdIgnoreTreeCollide(LOGICAL bIgnore);
extern void trncdCheckTreeBox(LOGICAL bIgnore);

extern LOGICAL trncdCheckTerrainDownWithDTL(COLLISION* collision,
                                     struct TERRAIN_DETAIL* pDTL);

extern LOGICAL trncdSphereTest(trnCDTerrainObject* to, OBJECT* o);
extern bool trncdSpawnDataExists(const char* buildingname);

// Update the flags on a CP object so Rick's map knows about contention/objectives/etc
extern UINT32 trnUpdateObjectFlags(SINT32 x, SINT32 y, UINT32 id, SINT16 ownership, SINT32 flags);
extern trnCDTerrainObject* trncdFindTerrainObject(OBJECT* o);

// calling this function resets the check
extern bool trnCDUpdateCollideLists(bool set = false);

extern bool trncdCollideWithMouse(int sx, int sy);
extern bool trncdToggleDamageWithMouseClick(int sx, int sy);
