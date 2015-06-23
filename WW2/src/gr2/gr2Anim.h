#pragma once

#include "gr2/gr2layer.h"
#include "fp/fpTPSdata.h"			// For TPS_NUM_LODS
#include "math/Vec3d.h"
#include "veh/vehmain.h"

#ifdef GR2_BLEND_EDITOR
namespace phui
{
	class View;
};
#endif

#define	GR2ANIM_USE_DEFAULT_WEAPON	9999
#define GR2ANIM_AVG_YAWS 5
#define GR2ANIM_CORPSE_LOD 15.0f

#define GR2ANIM_SHADOW_DISTANCE 20.0f
#define GR2ANIM_CULL_DISTANCE 700.0f

#define USE_CORPSES
//#undef USE_CORPSES

class fpTPSRagdoll;
class gr2PhysicsModel;
struct Corpse;
struct Kit;

#ifdef __cplusplus

class gr2AnimBitPacker
{
public:
	gr2AnimBitPacker(unsigned int bits);
	gr2AnimBitPacker();

	int walking;
	int parade;
	int useAltTrooperMesh;
	int chuteOpen;
	int freeFall;
	int altWeaponModel;
	int emoteID;
	int deathState;
	int headright;
	int headleft;
	int point;
	int movement;
	int pose;
	int weapon;
	int attacking1;
	int attacking2;

	unsigned int pack();
	gr2AnimBitPacker& unpack(unsigned int bits);
};
#endif

struct gr2AnimTwist
{
	gr2AnimTwist() :
		predictedYaw(0),
		vehicleYaw(0),
		yawRate(0),
		headYaw(0),
		torsoYaw(0),
		legsYaw(0),
		headPitch(0),
		headTwist(0),
		torsoTwist(0),
		modelTwist(0),
		headTwistTarget(0),
		torsoTwistTarget(0),
		modelTwistTarget(0),
		headPitchTarget(0),
		headTwistRate(0),
		torsoTwistRate(0),
		modelTwistRate(0),
		maxAngle(0),
		headAngle(0),
		turning(false),
		facing(false),
		straighten(false),
		lastChange(0),
		lastUpdate(0)
	{
	}

	// smooth our yaw
	void UpdateYaw(vehVehicle* v, FLOAT64 delta);

	// normal update function
	void Update(gr2AnimTrooper* model, bool reset);

	// entry point for calculations
	//void Calculate2(FLOAT64 delta, gr2AnimTrooper* model);
	void Calculate(FLOAT64 delta, gr2AnimTrooper* model);

	// turn in place calculations
	void CalculateTurn(gr2AnimTrooper* model, FLOAT64 yaw);

	// movement calculations
	void CalculateMoveLocked(gr2AnimTrooper* model, FLOAT64 angle);
	void CalculateMovePrimary(gr2AnimTrooper* model, FLOAT64 angle);
	void CalculateMoveSecondary(gr2AnimTrooper* model, FLOAT64 angle);

	void Smooth(FLOAT64 delta, gr2AnimTrooper* model);
	void Copy(gr2AnimTwist& t);
	bool InLine() const;
	void SetTwists(gr2Model* skeleton);
	void Reset();
	void Straighten(gr2AnimTrooper* model);

	FLOAT64 Twist(FLOAT64& target, FLOAT64& twist, FLOAT64& speed, FLOAT64 delta, FLOAT64 offset = 0);

	// vehicle rotations (used to smooth vehicle yaw)
	// results in an updated vehicle attitude yaw that
	// can be used to do the anim roations
	FLOAT64			predictedYaw;					// our predicted yaw
	FLOAT64			vehicleYaw;						// last vehicle yaw
	FLOAT64			yawRate;

	// used to calculate turn in place rotations (not used when moving)
	FLOAT64			headYaw;						// absolute head yaw
	FLOAT64			torsoYaw;						// absolute torso yaw
	FLOAT64			legsYaw;						// absolute legs yaw

	// current bone rotations
	FLOAT64			headPitch;						// look up/down
	FLOAT64			headTwist;						// vehicle yaw
	FLOAT64			torsoTwist;						// relative torso yaw
	FLOAT64			modelTwist;						// relative legs yaw

	// rotation smoothing (relative)
	FLOAT64			headTwistTarget;				// vehicle yaw
	FLOAT64			torsoTwistTarget;				// relative torso yaw
	FLOAT64			modelTwistTarget;				// relative legs yaw
	FLOAT64			headPitchTarget;				// look up/down target

	// how fast we move over time
	FLOAT64			headTwistRate;
	FLOAT64			torsoTwistRate;
	FLOAT64			modelTwistRate;

	// new twist variables
	FLOAT64 maxAngle;								// max angle we'll twist either the torso or head
	FLOAT64 headAngle;								// max amount of maxAngle the head twisting will account for

	bool			turning;
	bool			facing;
	bool			straighten;

	SINT32			lastChange;
	SINT32			lastUpdate;
};

struct gr2AnimSound
{
	gr2AnimSound() :
		model(nullptr),
		pose(-1),
		move(-1),
		ID(-1),
		loopID(-1),
		name(nullptr),
		footNextTime(0),
		footOKTime(0)
	{
	}

	void Init(gr2AnimTrooper* m = nullptr);
	void Update();
	void Stop();

	gr2AnimTrooper* model;
	SINT32 pose;
	SINT32 move;
	SINT32 ID;
	SINT32 loopID;
	const char* name;

	SINT32 footNextTime;	// next timed foot sound
	SINT32 footEventType;	// next timed type
	SINT32 footOKTime;		// next time its okay to play any foot sounds (prevents shuffling)
};

// base gr2 animated object
struct gr2AnimModel
{
	UINT32 animState;
	bool active;

	inline bool HasFlag(UINT32 flag)
	{
		return (animState & flag) > 0;
	}

	inline bool SetFlag(UINT32 flag, UINT32 source)
	{
		if(source & flag)
			return SetFlag(flag, true);
		else
			return ClearFlag(flag);
	}

	inline bool SetFlag(UINT32 flag, bool enable = true)
	{
		if(enable)
		{
			if(!HasFlag(flag))
			{
				animState |= flag;
				return true;
			}
		}
		else
		{
			return ClearFlag(flag);
		}

		return false;
	}

	inline bool ClearFlag(UINT32 flag)
	{
		if(HasFlag(flag))
		{
			animState &= ~flag;
			return true;
		}

		return false;
	}
};

// animated trooper subclass
struct gr2AnimTrooper : public gr2AnimModel
{
	gr2AnimTrooper()
	{
		Reset();
	}

	enum GR2_ANIM_STATE
	{
		ACTIVE				= (1<<0),
		HIDE_GUN			= (1<<1),
		HIDE_CHUTE			= (1<<2),
		PERSONALIZED		= (1<<3),
		USE_ALT_WEAPON		= (1<<4),
		USE_ALT_MODEL		= (1<<5),
		USE_SELF_MODEL		= (1<<6),
		FREE_FALL			= (1<<7),
		CHUTE_OPEN			= (1<<8),
		SPLASHED			= (1<<9),
		PARADE				= (1<<10),
		WALKING				= (1<<11),
		BACKWARDS			= (1<<12),
		DEAD				= (1<<13),
		TO_PRONE			= (1<<14),
		TO_CROUCH			= (1<<15),
		TO_STAND			= (1<<16),
		TO_COMPLETE			= (1<<17),
		RECOIL				= (1<<18),
		READYING			= (1<<19),
		ATTACK1				= (1<<20),
		ATTACK2				= (1<<21),
		ATTACKING			= (1<<22),
		TRANSITIONS			= (TO_PRONE|TO_CROUCH|TO_STAND)
	};

	vehVehicle*			myVehicle;

	gr2Model*			skeleton;
	gr2Model*			collisionModel;

	gr2ModelAlt			model;
	gr2ModelAlt			weapons[MAX_NUM_FPS_WEAPONS];
	//gr2ModelList		optional;
	gr2Model*			selfModel;
	gr2Model*			chuteModel;

	// kit we're currently using
	Kit*				kit;

	fpsWeaponTypes		weaponArray[MAX_NUM_FPS_WEAPONS];

	fpTPSWeaponType		currentWeaponType;
	SINT32				currentWeapon;
	SINT32				positioningBone;

	UINT32				lastFlags;
	UINT32				currentFlags;

	fpTPSLODType		currentLOD;
	fpTPSPoseType		currentPose;
	fpTPSMoveType		currentMove;
	fpTPSPointType		currentPoint;

	// animation override
	SINT32				overrideTime;
	SINT32				overridePose;
	SINT32				overrideMove;
	SINT32				overrideWeapon;
	SINT32				overridePoint;

	SINT32				headPoint;
	SINT32				deathState;
	SINT32				emoteID;

	FLOAT32				animationOffsetPercent;

	FLOAT32				upDownWeight, targetUpDownWeight;	// this is updated per frame, ranges from 0 to 1
	FLOAT32				lastTargetUpDownWeight;				// do we need to update?
	FLOAT32				upDownWeightAdjust;
	FLOAT32				upDownWeightOverride;
	FLOAT32				currentUpWeight;					// actual weight for the up animation
	FLOAT32				currentDownWeight;					// actual weight for the down animation
	FLOAT32				currentMiddleWeight;				// actual weight for the middle animation
	FLOAT32				modelSpeed;
	FLOAT32				lastModelSpeed;

	// 2d stuff point stuff
	FLOAT32				rectUpDownWeight, rectTargetUpDownWeight;
	FLOAT32				lastRectTargetUpDownWeight;
	FLOAT32				rectLeftRightWeight, rectTargetLeftRightWeight;
	FLOAT32				lastRectTargetLeftRightWeight;

	// timers
	SINT32				lastUpdateTime;
	SINT32				lastDrawTime;
	SINT32				nextLODCheck;
	SINT32				nextSurfaceCheck;
	SINT32				lastWeaponFireTime;
	SINT32				lastColliderUpdate;
	SINT32				turnInPlace;

	SINT32				countryID;

	// motion
	fpTPSMotionType		currentMotion;
	SINT32				strafe;

	// sound stuff
	gr2AnimSound		moveSound;

	// attacking
	SINT32				attacking;
	SINT32				lastVisualAttack, nextVisualAttack;
	SINT32				nextAttack, attackUntil, readyUntil;

	// stowed
	UINT32				stowedWeapon;

	// matrix used to terrainize world oriented
	// matrices and vectors; only set when
	// tps troopers are prone; get the world
	// oriented vector or matrix into the gr2AnimTrooper's
	// object space and multiple it by this matrix
	piMATRIX			terrainMat;
	Vec3d				terrainTarget;
	FLOAT64				terrainTargetZ;
	SINT32				terrainUpdate;

	// pose transition blend
	gr2AnimationBlend*  transition;

	// attack blend
	gr2AnimationBlend*  attack;

	// used to handle twist smoothing
	gr2AnimTwist		twist;

	void Reset();
	void SetKit(SINT32 index = -1, const char* vid = nullptr);

	// is this trooper within a certain range of me?
	inline bool InRange(FLOAT32 distance, bool applyMod = false)
	{
        if(myVehicle && myVehicle->VehicleObjectData && myVehicle->VehicleObjectData->PlaneObject)
        {
		    FLOAT32 dist = myVehicle->VehicleObjectData->PlaneObject->distsqr * ((applyMod) ? (gTrooperLOD * gTrooperMOD) : 1.0f);
		    return (dist <= distance) ? true : false;
        }

        return false;
	}

	inline bool InMotion()
	{
		switch(currentMove)
		{
			case TPS_MOVETYPE_WALK:
			case TPS_MOVETYPE_FORWARD:
			case TPS_MOVETYPE_SPRINT:
			case TPS_MOVETYPE_BACKWARD:
			case TPS_MOVETYPE_TURN:
			case TPS_MOVETYPE_STRAFE_LEFT:
			case TPS_MOVETYPE_STRAFE_RIGHT:
				return true;
            default:
                return false;
		}
	}

#ifdef GR2_MEMORY_PAD
	//char pad[0];
#endif
};

struct Corpse
{
	gr2Model* skeleton;
	gr2Model* model;

    piMATRIX world;
    FLOAT64VECTOR pos;
	piVECTOR gr2Offset;

	trnCDTerrainObject* TO;
	int TOLevel;
	int expires;
    int inittime;
    int material;
	bool shadow;
	bool update;

    Corpse() :
		skeleton(nullptr),
		model(nullptr),
		TO(nullptr),
		TOLevel(0),
		expires(-1),
		inittime(0),
		material(0),
		shadow(true),
		update(true)
	{
		reset();
    }

	~Corpse()
	{
		reset();
	}

	void reset()
	{
		if(skeleton)
			gr2FreeModel(skeleton);

		if(model && model != skeleton)
			gr2FreeModel(model);

		memset(&world, 0, sizeof(world));
		memset(&pos, 0, sizeof(pos));
		memset(&gr2Offset, 0, sizeof(gr2Offset));

		skeleton = nullptr;
		model	= nullptr;
		TO = nullptr;
		TOLevel = 0;
		expires = -1;
		inittime = 0;
		material	= -1;
		shadow = true;
		update = true;
	}

};

struct vehVehicle;

extern struct vehVehicle*gr2AnimGetVehicle (struct OBJECT* pObject);
extern struct gr2AnimTrooper* gr2AnimCreateModel (struct vehVehicle*vehicle, UINT32 vehicleTypeNum, struct OBJECT* pObject);

extern OBJECT* gr2AnimCreateObject(struct vehVehicle*vehicle, UINT32 vehicleTypeNum);
extern void	gr2AnimHandleEvent(struct OBJECT* pObject, gr2Model* model, SINT32 eventID);
extern void	gr2AnimExecuteEvent(struct OBJECT* pObject, gr2Model* model, SINT32 typeID, const char* data = nullptr);

// old stuff
extern LOGICAL gr2AnimDoWeaponCollision (struct OBJECT* pObject, piVECTOR* startPnt, piVECTOR* endPnt, SINT32* location, piVECTOR* collisionPnt);
extern LOGICAL gr2AnimGetCurrentWeapon (struct OBJECT* pObject, SINT32* weapon);

// pass true to match the visual model
// pass false to filter out visual model terrain adjustments
extern LOGICAL gr2AnimGetBoneMatrix (struct OBJECT* pObject, piMATRIX* pMatrix, const char* bone, bool visual = false);
extern LOGICAL gr2AnimGetFirePointMatrix (struct OBJECT* pObject, piMATRIX* pMatrix, bool visual = false);
extern LOGICAL gr2AnimGetEjectPointMatrix (struct OBJECT* pObject, piMATRIX* pMatrix, bool visual = false);

extern bool gr2AnimVisualAttack(gr2AnimTrooper* model, SINT32 attack); // visual attacks play an animation, sound or fx
extern bool gr2AnimAttack(gr2AnimTrooper* model); // real attacks fire off an ammo object
extern void	gr2AnimStopAttacking(gr2AnimTrooper* model, SINT32 mount = -1);
extern fpTPSAttackType gr2AnimGetAttack(gr2AnimTrooper* model);
extern void	gr2AnimPlayAttackSound(vehVehicle* Vehicle, SINT32 mount, FLOAT64VECTOR* pos);
extern void	gr2AnimStopAttackSound(vehVehicle* Vehicle, SINT32 mount);
extern fpTPSWeaponType gr2AnimWeaponNumToTPSType (struct gr2AnimTrooper* model, SINT32 weaponNum);

extern bool gr2AnimDrawCallback (OBJECT* pObject);
extern bool gr2AnimCullCallback (OBJECT* pObject);
extern LOGICAL gr2AnimIsDead (OBJECT* pObject, LOGICAL* isDead);
extern SINT32 gr2AnimDeathFlags();

extern LOGICAL	gr2AnimInit (void);
extern void	gr2AnimKill (void);

extern LOGICAL gr2AnimKillObject (OBJECT* pObject);
extern LOGICAL gr2AnimInitShadow(OBJECT* pObject);
extern LOGICAL gr2AnimProcessAnimationFlags(OBJECT* pMain, UINT32 Flags);

/*
extern LOGICAL gr2AnimSetModelState (OBJECT* pObject, SINT32 LOD, SINT32 pose, SINT32 movement, SINT32 point,
							  UINT32 currentWeapon, SINT32 deathState, SINT32 emoteID, SINT32 altWeaponModel,
							  LOGICAL chuteOpen, LOGICAL freeFall, LOGICAL useAltTrooperMesh, LOGICAL parade);
*/
extern LOGICAL gr2AnimSetModelState(OBJECT* pObject,
									SINT32 LOD,
									SINT32 pose,
									SINT32 move,
									SINT32 point,
									UINT32 weapon,
									UINT32 animState);

extern LOGICAL gr2AnimIsTrooperSitting (OBJECT* pObject);
extern LOGICAL gr2AnimIsTrooperProne (OBJECT* pObject);

extern void gr2AnimUpdateSurfaceID(struct gr2AnimTrooper* model);
extern void gr2AnimUpdateLOD(struct gr2AnimTrooper* model, FLOAT32 distance);
extern void gr2AnimForceLOD(FLOAT32 lodValue);
extern void gr2AnimAddTwitch(OBJECT* pObject);
extern void gr2AnimPersonalize(OBJECT* pObject, char* handle);
extern void gr2AnimSetWeaponFireTime(OBJECT* pObject, SINT32 time);

extern SINT32 gr2AnimGetAmmoTypeNum (struct vehVehicle*vehicle);
extern SINT32 gr2AnimHitMapLocation(SINT32 boneNum);
extern FLOAT64 gr2AnimGetLocationThickness(SINT32 boneNum);
extern LOGICAL gr2AnimIsVehicleHC (struct vehVehicle*vehicle);
extern FLOAT32 gr2AnimDetermineAnimationFootspeed (struct gr2AnimTrooper* model, fpTPSWeaponType tpsWeaponType);
extern void	gr2AnimDetermineMotionType (struct gr2AnimTrooper* model, FLOAT64 angle);
extern LOGICAL gr2AnimGunPointStraight (struct gr2AnimTrooper* model);
extern bool gr2AnimFaceVelocity(struct gr2AnimTrooper* model);
extern bool gr2AnimFaceYaw(struct gr2AnimTrooper* model);
extern void gr2AnimTrooperAdjustments(struct gr2AnimTrooper* model, float offset = 0);
/*
extern void gr2AnimTrooperTwists(OBJECT* object,
								  FLOAT64& finalModelTwist,
								  FLOAT64& finalModelTwistSpeed,
								  FLOAT64& finalTorsoTwist,
								  FLOAT64& finalTorsoTwistSpeed,
								  FLOAT64& finalHeadTwist,
								  FLOAT64& finalHeadTwistSpeed,
								  FLOAT64& finalHeadPitch,
								  FLOAT64& finalHeadPitchSpeed);
*/
extern LOGICAL gr2AnimIsRagdoll(struct gr2AnimTrooper* model);

extern SINT32 gr2AnimGetDeathState();

extern bool gr2AnimDrawCorpses();
extern void gr2AnimExpireCorpses();

// check to see if this trooper has a particular weapon type
extern bool gr2AnimHasWeapon(gr2AnimTrooper* model, fpFPSWeaponType type);

// lifetime of 0 means use the default; greater than 0 is the lifetime; -1 means don't expire
extern Corpse* gr2AnimAddCorpse(gr2AnimTrooper* a, const char* offset = nullptr, bool blood = false, SINT32 lifeTime = 0, SINT32 material = -1);
extern Corpse* gr2AnimAddCorpse(gr2PhysicsModel* m, const char* offset = nullptr, bool blood = false, SINT32 lifeTime = 0, SINT32 material = -1);
extern Corpse* gr2AnimAddCorpse(OBJECT* o, gr2Model* s, const char* m, const char* offset = nullptr, bool blood = false, SINT32 lifeTime = 0, SINT32 material = -1);
extern Corpse* gr2AnimAddCorpse(const char* modelPath, FLOAT64VECTOR& pos, piMATRIX& mat, SINT32 lifeTime = 0, SINT32 material = -1);
extern void gr2AnimCorpseTO(Corpse* c);

extern bool gr2AnimTerrainize(gr2AnimTrooper* model);
extern LOGICAL gr2AnimUpdateModel (OBJECT* pObject, LOGICAL isFPS);
extern void gr2AnimUpdate();
extern void gr2AnimReset(gr2AnimTrooper* model);

#ifdef GR2_BLEND_EDITOR

phui::View* uiAnimWeightCreate(_xmlNode*, phui::View* parent);

class GR2EditBlend
{
public:
	enum TYPE
	{
		TPS = 0,
		FPS = 1
	};

	typedef std::vector<int> BLEND_STATES;
	typedef std::vector<FLOAT32> BLEND_WEIGHTS;

	bool selected;
	bool modified; // have we been modified?
	bool play; // should we keep playing even if we're not the selected blend
	bool active;
	FLOAT32 weight; // current blend weight
	FLOAT32 mod1;
	FLOAT32 mod2;
	TYPE type;
	BLEND_STATES states; // max of 4 states for now
	BLEND_WEIGHTS weights; // max of 4 for now
	gr2AnimationBlend* blend;
};

extern void initBlendEditor(phui::View* editor, gr2AnimTrooper* model);
extern void killBlendEditor();
extern void updateBlendEditor();
extern gr2AnimTrooper* getEditModel();
extern void setRotateEditModel(bool on);
extern void setEditModelOscillation(int duration);
extern int addEditBlend(GR2EditBlend::TYPE stateType, GR2EditBlend::BLEND_STATES states);
extern bool validEditBlend(GR2EditBlend::TYPE stateType, GR2EditBlend::BLEND_STATES states);
extern const char* getEditBlendName(GR2EditBlend::TYPE stateType, GR2EditBlend::BLEND_STATES states);
extern void removeEditBlend(int index);
extern void saveEditBlend(int index);
extern void selectEditBlend(int index);
extern void reorderEditBlend(int index, int animation, int direction);
extern void updateEditBlends();
extern GR2EditBlend* getEditBlend(int index = 0);
extern GR2EditBlend* getSelectedBlend();
extern void setEditBlendWeapon(int type);
//extern int getEditBlendWeapon();
extern int getEditBlendWeaponType();
extern int getEditBlendWeaponNum();
extern int getEditWeaponTypeToNum(int type);
extern void setEditBlendWeights(int index, FLOAT32 weight1, FLOAT32 weight2, FLOAT32 weight3, FLOAT32 weight4);
extern void setEditAnimation(const char* path);
extern void clearAnimCache();
#endif

#ifdef BETA_RELEASE
extern void			gr2ToggleStateTest();
extern void			gr2AnimMemStats();
#endif
