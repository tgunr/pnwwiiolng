#include <syscfg.h> // Path is project specific.

#include "gr2/gr2anim.h"
#include "ob/obmain.h"
#include "include/vars.h"
#include "veh/vehmain.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "math/matheul.h"
#include "math/Vec3d.h"
#include "fp/fpFPS.h"
#include "fp/fpFPSdata.h"
#include "fp/fpTPS.h"
#include "fp/fpTPSdata.h"
#include "fp/fpTPSPhysics.h"
#include "fp/fpMain.h"
#include "pers/persmain.h"
#include "ml/mltypes.h"
#include "ob/obpool.h"
#include "fx/fxMgr.h"
#include "trn/trnmain.h"
#include "fp/fpFPSinternal.h"
#include "rn/rnvis.h"
#include "cfg/cfgPrefs.h"
#include "fx/fxBlt.h"
#include "trn/trnvars.h"
#include "rn/rnmain.h"
#include "locl/loclApp.h"
#include "veh/vehupdate.h"
#include "ui/uiChat.h"

#include "mm/mmEnums.h"
#include "math/mathnum.h"
#include "draw/draw3d.h"
#include "fx/fxMgr.h"
#include "ml/mlutility.h"
#include "ob/obMgr.h"
#include <stack>
#include "trn/trncd.h"
#include "physics/physicsMain.h"
#include "fp/fpTPSPhysics.h"
#include "math/mathUnitConversion.h"
#include "ob/obdraw.h"
#include "osgImport/ShadowMap.h"
#include "fx/TransformEffect.h"
#include "spt/Random.h"
#include "imap/ww2func.h"
#include "ui/uiMain.h"
#include "fp/fpTPSKits.h"
#include "wwii/wwiiBranchEnums.h"

#include "fx/fxSnd.h"

#define STOWED_WEAPON_POSITION 0
#define STOWED_WEAPON_ATTACH "StowedAttach"

// distance over which we don't get gravy features (idles, tps sounds, etc)
#define POLISH_VERY_NEAR 10.0f
#define POLISH_NEAR 50.0f
#define POLISH_FAR 100.0f

static LOGICAL animInited;

SINT32 gr2AnimWeapBits, gr2AnimPoseBits, gr2AnimMoveBits, gr2AnimPointBits, gr2AnimHeadBits, gr2AnimFPSWeapBits;
SINT32 gr2AnimDeathBits, gr2AnimAltWeapBits, gr2AnimParachuteBits, gr2AnimStateBits; //gr2AnimEmoteBits

UINT32	bitFields[12] =
{
	0x00000000,
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000F,
	0x0000001F,
	0x0000003F,
	0x0000007F,
	0x000000FF,
	0x000001FF,
	0x000003FF,
	0x000007FF
};


// this is the big granddaddy structure to hold all of the state and info needed for a
// third person model

#ifdef GM_RELEASE
static FLOAT32		LODForceValue;
#endif

#ifdef BETA_RELEASE
static LOGICAL stateTestFlag;
#endif

#ifdef LOD_ANIMATIONS
static granny_real32 AllowedErrorMax = 20.0f;
static granny_real32 AllowedErrorInPixels = 20.0f;
static granny_real32 AllowedErrorAtUnitDistance = 0;
#endif

#ifdef DEBUG_DEPLOY_CRAP
extern void _fpsdrawline(piMATRIX&);
#endif

#ifdef BETA_RELEASE
static void _drawfpscollider();
#endif

// gr2AnimTrooper used to be defined here
// moved it to the header for easier member access

typedef GR2_POOL<gr2AnimTrooper> GR2_TROOPER_POOL;
static GR2_TROOPER_POOL _gr2TrooperPool = GR2_TROOPER_POOL(128, 32, GR2_TROOPER_POOL::NEW);

#ifdef USE_CORPSES
typedef GR2_POOL<Corpse> GR2_CORPSE_POOL;
static GR2_CORPSE_POOL _gr2CorpsePool = GR2_CORPSE_POOL(32, 32, GR2_CORPSE_POOL::NEW);
#endif

struct RayColliders
{
	int count;
	dGeomID rays[5];
};

struct BlendSpec
{
	gr2AnimTrooper* model;
	SINT32 pose;
	SINT32 move;
	SINT32 weapon;
	SINT32 fpsWeapon;
	SINT32 point;
	UINT32 loopCount;
	FLOAT32 easeIn;
	FLOAT32 easeOut;
	FLOAT32 removeEaseOut;
	FLOAT32 weight;
	FLOAT32 weightMod;
	FLOAT32 startOffset; // percentage 0 to 1.0f
	FLOAT32 endOffset; // percentage 0 to 1.0f
	bool start;
	bool removeWhenDone;
	bool allowRessurect;
	CONTROL_CALLBACK onBlendComplete;

	void Set(gr2AnimTrooper* t, fpTPSWeaponType wt, SINT32 cw)
	{
		model = t;

		if(model)
		{
			pose	= model->currentPose;
			move	= model->currentMove;
			weapon	= wt;
			point	= model->currentPoint;
		}
		else
		{
			pose	= 0;
			move	= 0;
			weapon	= wt;
			point	= 0;
		}

		if(weapon == TPS_WEAPONTYPE_EMPTY)
			fpsWeapon = NO_WEAPON;
		else
			fpsWeapon = (SINT32)model->weaponArray[cw];

		loopCount		= 0;
		easeIn			= 1.0f;
		easeOut			= 1.0f;
		removeEaseOut	= -1.0f;
		weight			= 1.0f;
		weightMod		= 1.0f;
		startOffset		= 0.0f;
		endOffset		= 0.0f;
		start			= false;
		removeWhenDone	= false;
		allowRessurect	= true;
		onBlendComplete	= nullptr;
	}
};

// default motion
struct gr2AnimMotionBase
{
	virtual void Calculate(gr2AnimTrooper* t)
	{

	}
};

typedef GR2_POOL<RayColliders> GR2_RAY_POOL;
static GR2_RAY_POOL _gr2RayPool = GR2_RAY_POOL(128);

typedef std::map<gr2AnimTrooper*,RayColliders*> GR2RayMap;

static void gr2AnimHandleAttack(gr2AnimTrooper* model, int pose, int move, int point, int weapon);
static void gr2AnimHandlePoseChange(gr2AnimTrooper* model, int pose, int move, int point, int weapon);
static void gr2AnimHandleTransitions(gr2AnimTrooper* model, int pose, int move, int point, int weapon);
static void gr2AnimHandleStowing(gr2AnimTrooper* model);
static void gr2AnimHandleEmotes(gr2AnimTrooper* model);
static void gr2AnimHandleRecoil(gr2AnimTrooper* model);

const char* gr2AnimDetermineMovementSound(fpTPSPoseType pose, fpTPSMoveType move, SINT32 surfaceID, FLOAT32* soundHalfDistance, FLOAT32* soundMaxDistance);
FLOAT32 gr2AnimDetermineBaseOffset(gr2AnimTrooper* model, fpTPSPoseType pose, fpTPSMoveType movement);
static LOGICAL gr2AnimGetFirePointMatrixByBone(OBJECT* pObject, piMATRIX* pMatrix, bool visual);

static	void gr2AnimSoundStoppedCallback(void* callbackPtr);
SINT32	gr2GetSurfaceIDUnderfoot(vehVehicle*vehicle, FLOAT64VECTOR* offset);
static void _freemodel(gr2AnimTrooper* model);

#if(0)
static bool _idleok(gr2AnimTrooper* model);
#endif

static FLOAT32 _getTransitionWeight(gr2AnimTrooper* model, gr2AnimationBlend* blend, FLOAT32 balance = -1.0f);
static gr2AnimationBlend* _setBlend(BlendSpec& spec, fpTPSBlendTypes type = TPS_BLENDTYPE_TPS);
#if(0)
static void _clearTransition(void*);
static void _clearAttack(void*);
#endif
static	gr2AnimationBlend*	defaultAnimation;
/*
static gr2Model* helmet1;
static gr2Model* helmet2;
static gr2Model* helmet;
*/

static FLOAT32 polishNear = POLISH_NEAR;

/*--------------------------------
DELAY DEATH CRAP
---------------------------------*/

#ifdef USE_CORPSES
#include "map/mapArenaDescription.h"
#include <set>

// 10 minutes max
#define CORPSE_TIME 10 * 60 * 1000

// 100 corpses max
#define CORPSE_COUNT 100

static std::set<Corpse*> _corpses; // active corpses
//static int _corpsetime = CORPSE_TIME;
//static int _corpsecount = CORPSE_COUNT;
static BlendSpec _blendSpec;

#endif // USE_CORPSES

#ifdef GR2_BLEND_EDITOR
static phui::View* _editAnimSlider = nullptr;
static phui::View* _editModelControl = nullptr;
static gr2AnimTrooper* _editModel = nullptr;
static gr2Control* _editAnimation = nullptr;
static gr2Model* _editWeapon = nullptr;
static SINT32 _editWeaponType = 0;
static SINT32 _editWeaponNum = 0;
static std::vector<GR2EditBlend> _editBlends;
static bool _rotateEditModel = false;
static double _editModelFacing = 0;
static int _editModelOscillation = 0;
static int _editModelOscillationExecute	= frametime;
static int _editModelOscillationFirst	= 0;
static int _editModelOscillationSecond = 1;

static Vec3d _editModelPos;

class AnimWeightView : public phui::View
{
public:
	AnimWeightView();
	virtual void DrawSelf();
	virtual bool MouseDrag (float x, float y, int button, bool start);
	bool dragdead, dragfull, dragcontrol;
};

/************************************************************************************************
	uiJoystickCalibrationCreate
*************************************************************************************************/

phui::View* uiAnimWeightCreate(_xmlNode*, phui::View* parent)
{
	return new AnimWeightView;
}

#endif

// default trooper state
void gr2AnimTrooper::Reset()
{
	active				= true;
	animState			= 0;

	myVehicle			= nullptr;
	skeleton			= nullptr;
	collisionModel		= nullptr;
	selfModel			= nullptr;
	chuteModel			= nullptr;

	//gr2ModelAlt			model;
	//gr2ModelAlt			weapons[MAX_NUM_FPS_WEAPONS];

	kit					= nullptr;

	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
		weaponArray[i] = NO_WEAPON;

	currentWeapon		= -1;
	positioningBone		= 0;

	lastFlags			= 0;
	currentFlags		= 0;

	currentLOD			= TPS_LOD_1;
	currentPose			= TPS_POSETYPE_STAND;
	currentMove			= TPS_MOVETYPE_STILL;
	currentPoint		= TPS_POINTTYPE_RELAXED;

	overrideTime		= 0;
	overridePose		= -1;
	overrideMove		= -1;
	overrideWeapon		= -1;
	overridePoint		= -1;

	headPoint			= 0;
	deathState			= 0;
	emoteID				= 0;

	animationOffsetPercent = 1.0f;

	upDownWeight					= 0;
	targetUpDownWeight				= 0;
	lastTargetUpDownWeight			= 0;
	upDownWeightAdjust				= 0;
	upDownWeightOverride			= 0;
	currentUpWeight					= 0;
	currentDownWeight				= 0;
	currentMiddleWeight				= 0;
	modelSpeed						= 1.0f;
	lastModelSpeed					= 1.0f;
	rectUpDownWeight				= 0;
	rectTargetUpDownWeight			= 0;
	lastRectTargetUpDownWeight		= 0;
	rectLeftRightWeight				= 0;
	rectTargetLeftRightWeight		= 0;
	lastRectTargetLeftRightWeight	= 0;

	lastUpdateTime		= 0;
	lastDrawTime		= 0;
	nextLODCheck		= 0;
	nextSurfaceCheck	= 0;
	lastWeaponFireTime	= 0;
	lastColliderUpdate	= 0;
	countryID			= 0;
	turnInPlace			= 0;
	strafe				= 0;
	currentMotion		= TPS_MOTION_F;

	// sound stuff
	moveSound.Init(this);

//	firing				= 0;
//	readyToFire			= 0;
//	fired				= 0;
//	fireStartTime		= 0;
//	lastFireTime		= 0;
//	nextFireTime		= 0;
	attacking			= 0;
	lastVisualAttack	= 0;
	nextVisualAttack	= 0;
	readyUntil			= 0;
	attackUntil			= 0;
	nextAttack			= 0;
	stowedWeapon		= 0;

	terrainMat			= IdentityMatrix;
	terrainTarget.set(0,0,0);
	terrainTargetZ		= 0;
	terrainUpdate		= 0;

	transition			= nullptr;
	attack				= nullptr;

	twist.Reset();

	/*
	if(collisionModel)
		gr2SetModelActive(collisionModel, true);

	if(skeleton)
	{
		gr2ClearBlendedAnimations(skeleton);

		if(gr2PlayBlendedAnimation(skeleton, fpTPSBasePose))
		{
			gr2BlendControlEaseIn(skeleton, fpTPSBasePose, 0.0);
			gr2BlendControlEaseOut(skeleton, fpTPSBasePose, 0.1f);
			gr2BlendControlSetWeight(skeleton, fpTPSBasePose, 1.0f);
		}

		// make sure the model gets rotated
		gr2ModelSetModelRotation(skeleton, .01f);
	}
	*/

	// reset our target twists
	//twist.Update(this, true);

	//gr2AnimReset(this);
}

void gr2AnimTrooper::SetKit(SINT32 index, const char* vid)
{
	fpTPSKitsReset(this);

	if(vid)
		fpTPSKitsSetByName(this, vid);
	else
		fpTPSKitsSetByVehicle(this, index);
}

vehVehicle*gr2AnimGetVehicle (OBJECT* pObject)
{
	if (!pObject)
		return nullptr;
	if (!pObject->animModel)
		return nullptr;

	return pObject->animModel->myVehicle;
}

static void _addInfantryLODs(gr2AnimTrooper* anim, SINT32 modelIndex, SINT32 fileIndex)
{
	gr2Model* lastLod = nullptr;

	for(int i = 0; i < TPS_NUM_LODS; ++i)
	{
		const char* path = fpTPSInfantryMeshFiles[fileIndex][i];

		if(path && *path)
		{
			gr2Model* lod = gr2Model::Load(path, anim->skeleton, nullptr, 1);

			if(lod)
			{
				lastLod = lod;

				switch(i)
				{
					case TPS_LOD_1: lod->maxVis = 2; break;
					case TPS_LOD_2: lod->maxVis = 8; break;
					case TPS_LOD_3: lod->maxVis = 20; break;
					case TPS_LOD_4: lod->maxVis = 40; break;
					case TPS_LOD_5: lod->maxVis = 0; lod->minVis = 40; break; // vary low poly; don't let us get to close to them
				}

				gr2Model* model = anim->model.AddModel(modelIndex, lod);

				if(model && i < TPS_LOD_5)
					model->MakeMutable();
			}
		}
	}

	// make sure the last lod can be seen out to the cull distance
	if(lastLod)
		lastLod->maxVis = 0;
}

static void _addWeaponLODs(gr2AnimTrooper* anim)
{
	fpTPSWeaponType	type;

	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
	{
		if (anim->weaponArray[i] == NO_WEAPON)
			continue;

		type = gr2AnimWeaponNumToTPSType(anim, i);

		if(!tpsWeapons[type].attachBone)
			continue;

		// adjust lod distances by weapon size
		FLOAT32 weaponVisMod = 1.0f;

		switch(type)
		{
			case TPS_WEAPONTYPE_ROCKET:
			case TPS_WEAPONTYPE_RIFLE:
			case TPS_WEAPONTYPE_SMG:
			case TPS_WEAPONTYPE_LTMORTAR:
			case TPS_WEAPONTYPE_LMG:
			case TPS_WEAPONTYPE_ATR:
			case TPS_WEAPONTYPE_PISTOL:
			{
				weaponVisMod = 2.0f;
			}
			break;
			case TPS_WEAPONTYPE_SATCHEL:
			case TPS_WEAPONTYPE_BINOCS:
			case TPS_WEAPONTYPE_MAP:
			case TPS_WEAPONTYPE_BUILD:
			case TPS_WEAPONTYPE_KNIFE:
			{
				weaponVisMod = .5f;
			}
			break;

			case TPS_WEAPONTYPE_GRENADE:
			{
				weaponVisMod = .1f;
			}
			break;
            default:
                // other weapon types, leave default
                break;
		}

		// rangewe must be able to see the weapon at
		UINT32 maxWeaponRange = 200;
		UINT32 maxVisibleWeaponRange = (UINT32)(maxWeaponRange * weaponVisMod); // meters

		gr2Model* lastPrimaryLod = nullptr;
		gr2Model* lastSecondaryLod = nullptr;

		for(int j = 0; j < TPS_NUM_LODS; j++)
		{
			// primary weapon model
			const char* path = fpTPSWeaponMeshFiles[anim->weaponArray[i]][j];

			if(path && *path)
			{
				gr2Model* lod = gr2Model::Load(path, nullptr, nullptr, 1);

				if(lod)
				{
					lastPrimaryLod = lod;

					// these ranges are based on the large weapon distances
					switch(i)
					{
						case TPS_LOD_1: lod->maxVis = (UINT32)(20 * weaponVisMod); break;
						case TPS_LOD_2: lod->maxVis = (UINT32)(75 * weaponVisMod); break;
						case TPS_LOD_3: lod->maxVis = (UINT32)(125 * weaponVisMod); break;
						case TPS_LOD_4: lod->maxVis = maxVisibleWeaponRange; break;
					}

					// save our weapon instance
					anim->weapons[i].AddModel(0, lod);

					// attach it to our skeleton
					lod->Attach(anim->skeleton, (SCHAR *)tpsWeapons[type].attachToBone);
				}
			}

			// check for an alternate weapon model
			path = fpTPSWeaponMeshAltFiles[anim->weaponArray[i]][j];

			if(path && *path)
			{
				gr2Model* lod = gr2Model::Load(path, nullptr, nullptr, 1);

				if(lod)
				{
					lastSecondaryLod = lod;

					switch(i)
					{
						case TPS_LOD_1: lod->maxVis = (UINT32)(20 * weaponVisMod); break;
						case TPS_LOD_2: lod->maxVis = (UINT32)(75 * weaponVisMod); break;
						case TPS_LOD_3: lod->maxVis = (UINT32)(125 * weaponVisMod); break;
						case TPS_LOD_4: lod->maxVis = maxVisibleWeaponRange; break;
					}

					if(anim->weaponArray[i] == PIAT)
						lod->SetMeshActive("PiatShell", false);

					// save our weapon instance
					anim->weapons[i].AddModel(1, lod);

					// attach it to our skeleton
					lod->Attach(anim->skeleton, (SCHAR *)tpsWeapons[type].attachToBone);
				}
			}
		}

		// guarantee we can see the weapon out to the desired max visible weapon range
		if(lastPrimaryLod && lastPrimaryLod->maxVis < maxVisibleWeaponRange)
			lastPrimaryLod->maxVis = maxVisibleWeaponRange;

		if(lastSecondaryLod && lastSecondaryLod->maxVis < maxVisibleWeaponRange)
			lastSecondaryLod->maxVis = maxVisibleWeaponRange;
	}
}

gr2AnimTrooper* gr2AnimCreateModel (vehVehicle*vehicle, UINT32 vehicleTypeNum, OBJECT* pObject)
{
	gr2AnimTrooper* newModel;
	SINT32	countryID;
	SINT32	boneCount;

	if(!animInited) gr2AnimInit();

	newModel = _gr2TrooperPool.Pop();

	// Try to avoid malloc when possible.
	if(PN_UNLIKELY(newModel == nullptr))
		piExitProgram (_FL, "gr2AnimCreateModel: Failed to allocate memory for animModel");

	// get everything to a default state
	newModel->Reset();

	pObject->animModel = newModel;

	newModel->myVehicle = vehicle;
	newModel->active = true;
	newModel->animState = 0;

	// always enable binos for 3rd person (duh) and pass in nullptr for last resort, as we don't care
	fpFPSDetermineTrooperLoadout (vehicle, vehicleTypeNum, 1, (SINT32*)newModel->weaponArray, &countryID, nullptr, nullptr);

	newModel->kit= nullptr;
    newModel->countryID = countryID;

	newModel->skeleton = gr2Model::Load(fpTPSSkeletonFile, nullptr, &boneCount, 1);

	// this doesn't work because COM bone isn't the root bone of the base skeleton
	// gr2SetPositioningBone (newModel->skeleton, "COM");

	newModel->collisionModel = gr2Model::Load(fpTPSDamageFile, newModel->skeleton, nullptr, 1);
	newModel->chuteModel = nullptr;
	newModel->selfModel = nullptr;

	_addInfantryLODs(newModel, 0, countryID);

	switch(countryID)
	{
		case BRITISH_PARA_PACK:
		{
			newModel->chuteModel	= gr2Model::Load(fpTPSParachuteFile, nullptr, nullptr, 1);
			//newModel->selfModel		= gr2Model::Load(fpTPSInfantryMeshFiles[BRITISH_PARA_PACK_SELF][TPS_LOD_1], newModel->skeleton, nullptr, 1);
			_addInfantryLODs(newModel, 1, BRITISH_PARA_NOPACK);
		}
		break;
		case FRENCH_PARA_PACK:
		{
			newModel->chuteModel	= gr2Model::Load(fpTPSParachuteFile, nullptr, nullptr, 1);
			//newModel->selfModel		= gr2Model::Load(fpTPSInfantryMeshFiles[FRENCH_PARA_PACK_SELF][TPS_LOD_1], newModel->skeleton, nullptr, 1);
			_addInfantryLODs(newModel, 1, FRENCH_PARA_NOPACK);
		}
		break;
		case GERMAN_PARA_PACK:
		{
			newModel->chuteModel	= gr2Model::Load(fpTPSParachuteFile, nullptr, nullptr, 1);
			//newModel->selfModel		= gr2Model::Load(fpTPSInfantryMeshFiles[GERMAN_PARA_PACK_SELF][TPS_LOD_1], newModel->skeleton, nullptr, 1);
			_addInfantryLODs(newModel, 1, GERMAN_PARA_NOPACK);
		}
		break;
	}

#ifndef GR2_BLEND_EDITOR
	_addWeaponLODs(newModel);
#endif

	// set up base pose
	newModel->currentPose = TPS_POSETYPE_STAND;
	newModel->currentMove = TPS_MOVETYPE_IDLE;
	newModel->currentPoint = TPS_POINTTYPE_RELAXED;
	newModel->currentWeapon = -1;		// this forces an update in update state
	newModel->currentWeaponType = TPS_WEAPONTYPE_EMPTY;

	newModel->ClearFlag(gr2AnimTrooper::HIDE_GUN);
	newModel->ClearFlag(gr2AnimTrooper::HIDE_CHUTE);

	newModel->stowedWeapon = 0; // stow weapon array

    gr2SetModelActive(newModel->collisionModel, true);
	newModel->terrainMat = IdentityMatrix;
	newModel->terrainTarget = Vec3d(0,0,-1.0f);
	newModel->terrainTargetZ = 0;
	newModel->twist.SetTwists(newModel->skeleton);
	newModel->transition = nullptr;
	newModel->attack = nullptr;
	newModel->skeleton->lodError = 0;

	if(newModel->chuteModel)
		newModel->chuteModel->Attach(newModel->skeleton, (SCHAR *)"Bip01 Spine1");

	if (!gr2PlayBlendedAnimation (newModel->skeleton, fpTPSBasePose))
	{
		Write_Log (LOG_FPS, LOG_INFO, "Failed to play base pose blend!");
	}

//	gr2BlendControlEaseIn (newModel->skeleton, fpTPSAnimLib[TPS_POSETYPE_STAND][TPS_MOVETYPE_IDLE][TPS_WEAPONTYPE_EMPTY][TPS_POINTTYPE_RELAXED], 0.0);
//	gr2BlendControlSetWeight (newModel->skeleton, fpTPSAnimLib[TPS_POSETYPE_STAND][TPS_MOVETYPE_IDLE][TPS_WEAPONTYPE_EMPTY][TPS_POINTTYPE_RELAXED], 1.0f);
	gr2BlendControlEaseIn(newModel->skeleton, fpTPSBasePose, 0.0);
	gr2BlendControlEaseOut(newModel->skeleton, fpTPSBasePose, 0.1f);
	gr2BlendControlSetWeight(newModel->skeleton, fpTPSBasePose, 1.0f);

	// add mixin blends

	// set up the event handler
	gr2ModelSetEventHandler(newModel->skeleton, pObject, gr2AnimHandleEvent);

	// make sure the model gets rotated
	gr2ModelSetModelRotation(newModel->skeleton, .01f);

	// lookup and attach our kit objects
	newModel->SetKit();

	// make sure models are active
	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
		newModel->weapons[i].Set(0);

	newModel->model.Set(0);
	newModel->moveSound.Init(newModel);

	// make sure our lods are correct initially
	gr2AnimUpdateLOD(newModel, 0.0f);

	return newModel;
}

OBJECT* gr2AnimCreateObject (vehVehicle*vehicle, UINT32 vehicleTypeNum)
{
	OBJECT* newObject, *childObject;

	obmainCreateObjectFull (&newObject, "trooperObject", 0, 0);

	newObject->flags = 0;
	newObject->flags |= OBJECT_VISIBLE;
	newObject->flags |= OBJECT_LITE;
	newObject->flags |= OBJECT_DYNALITE;
	newObject->flags |= OBJECT_INDEXED;
	newObject->flags |= OBJECT_TRIANGLELIST;
	newObject->flags |= OBJECT_USERDRAW;
	// copied from anim.c?
	newObject->flags |= OBJECT_COLLISION;
	newObject->flags |= OBJECT_USEMATRIX;

	newObject->flag2 |= OBJECT_ANIMATION;
	newObject->flag3 |= OBJECT_CAST_SHADOW;

	newObject->drawcallback = gr2AnimDrawCallback;
	newObject->cullcallback = gr2AnimCullCallback;

	newObject->bbox[0].x = -0.5f;
	newObject->bbox[0].y = -0.5f;
	newObject->bbox[0].z = 0;

	newObject->bbox[1].x = 0.5f;
	newObject->bbox[1].y = 0.5f;
	newObject->bbox[1].z = 2.0f;

	newObject->maxrad = 4.0f;

	// newObject->child make "lod1"

	obmainCreateObjectFull (&childObject, "lod1", 0, 0);

	childObject->flags |= OBJECT_VISIBLE;
	childObject->flags |= OBJECT_LITE;
	childObject->flags |= OBJECT_DYNALITE;
	childObject->flags |= OBJECT_INDEXED;
	childObject->flags |= OBJECT_TRIANGLELIST;
	childObject->flags |= OBJECT_USERDRAW;
	childObject->flags |= OBJECT_LOD;

	childObject->flag2 |= OBJECT_ANIMATION;
    childObject->flag3 |= OBJECT_CAST_SHADOW;

	newObject->child = childObject;

	// now hook the created object into the model *
	gr2AnimCreateModel(vehicle, vehicleTypeNum, newObject);

    TransformEffect::Apply(newObject->animModel, newObject);

	return newObject;
}

// helper function!
static void gr2AnimStartBlendedAnimation (gr2AnimTrooper* model, gr2AnimationBlend* blend, FLOAT32 easeInTime, FLOAT32 easeOutTime, bool allowResurrect = true)
{
	if (!gr2PlayBlendedAnimationFull (model->skeleton, blend, easeInTime, easeOutTime, allowResurrect))
	{
		Write_Log (LOG_FPS, LOG_INFO, "Failed to play blend!");
	}

	if (model->animationOffsetPercent > 0.0f)
	{
		gr2BlendControlSetStartOffset (model->skeleton, blend, model->animationOffsetPercent);
	}
}

// how fast (unitless/s) does the weight transition
 #define TPS_UPDOWN_RATE 1.0f

// called every frame
LOGICAL gr2AnimUpdateModel (OBJECT* pObject, LOGICAL isFPS)
{
	if(!pObject)
		return FALSE;

	gr2AnimTrooper* model = pObject->animModel;

	if(!model ||
	   !model->active ||
	   !model->myVehicle ||
	   !model->myVehicle->InUse)
		return FALSE;


	if(model->lastUpdateTime == frametime)
		return TRUE;

	FLOAT32 distToMe = (model->myVehicle->ModelData) ? 0.0f : model->myVehicle->VehicleObjectData->PlaneObject->distsqr;

	// FIXME
	if (model->myVehicle && model->myVehicle->Handle[0] == 0) {
		strlcpy(reinterpret_cast<char *>(model->myVehicle->Handle), " ", sizeof(model->myVehicle->Handle));
	}

	//if (model->myVehicle && model->myVehicle->Handle[0] && model->SetFlag(gr2AnimTrooper::PERSONALIZED))
	//	gr2AnimPersonalize(pObject, (char*)model->myVehicle->Handle);

	FLOAT32 deltaTime = (frametime - model->lastUpdateTime)/1000.0f;
	model->lastUpdateTime = frametime;

	//fpTPSLODType lastLOD = model->currentLOD;

	// determine LOD
#ifdef GM_RELEASE
	if (LODForceValue) {
		gr2AnimUpdateLOD(model, LODForceValue);
	} else
#endif
	{
		// FPS always sees itself at LOD1
		if (isFPS) {
			gr2AnimUpdateLOD(model, 0.0f);
		} else if (frametime >= model->nextLODCheck) {
			gr2AnimUpdateLOD(model, distToMe);
		}
	}

	// if we're dead, all we need to do is LOD .. so stop here
	if (model->HasFlag(gr2AnimTrooper::DEAD))
	{
		model->moveSound.Stop();
		return TRUE;
	}

	//if(frametime >= model->nextSurfaceCheck)
	//	gr2AnimUpdateSurfaceID(model);

	//fpTPSWeaponType	tpsWeaponType;
	//piVECTOR		vec, viewPos;
	//FLOAT64			distSqr, distSqrNoFOV;
	//FLOAT64			FOVFactor;
	//fpTPSPointType	midPoint, upPoint, downPoint;


	// modify point
	fpTPSWeaponType tpsWeaponType = model->currentWeaponType;

	// model speed .. based on the pose/movement adjust the model speed accordingly
	// set the default, then modify it if needed
	model->modelSpeed = 1.0f;

#ifdef GR2_BLEND_EDITOR
	if(model != _editModel)
	{
#endif
	if (model->myVehicle && model->InRange(200.0f))
	{
		FLOAT32 playerSpeed, baseSpeed;

		baseSpeed = gr2AnimDetermineAnimationFootspeed(model, tpsWeaponType);

		if (baseSpeed <= 0)
		{
			if(false && model->currentPose != TPS_POSETYPE_PRONE && frametime <= abs(model->turnInPlace))
				model->modelSpeed = (model->turnInPlace < 0) ? -2.0f : 2.0f;
			else
				model->modelSpeed = 1.0f;
		}
		else
		{
			if (pObject->animModel->myVehicle->ModelData)
			{
				FLOAT64VECTOR& vel = pObject->animModel->myVehicle->ModelData->VehicleVel;
				playerSpeed = (FLOAT32)mathExactVectorLength(&vel);
			}
			else
			{
				playerSpeed = (FLOAT32)pObject->animModel->myVehicle->interpolator.Interpolate(frametime).speed;
			}

			if (playerSpeed > 0)
				model->modelSpeed = playerSpeed / baseSpeed;
			else
				model->modelSpeed = 1.0f;
		}

//		Write_Log (LOG_FPS, LOG_INFO, "Speeds = %f / %f ->>>> %f", playerSpeed, baseSpeed, model->modelSpeed);
	}

	if(model->currentPose == TPS_POSETYPE_PRONE && model->modelSpeed >= 1.4f)
		model->modelSpeed = 1.4f;

	if(model->HasFlag(gr2AnimTrooper::BACKWARDS) ||
	  (model->currentPose == TPS_POSETYPE_CROUCH && model->currentMove == TPS_MOVETYPE_STRAFE_RIGHT))
	{
		model->modelSpeed *= -1.0f;
	}
	else if(model->currentPose == TPS_POSETYPE_STAND &&
			model->currentMove == TPS_MOVETYPE_STRAFE_RIGHT &&
			model->strafe == TPS_MOVETYPE_STRAFE_LEFT &&
			model->currentPoint == TPS_POINTTYPE_READY &&
			fpTPSConvertWeaponClass(model->currentWeaponType) == CLASS_FIREARM)
	{
		model->modelSpeed *= -1.0f;
	}

#ifdef GR2_BLEND_EDITOR
	}
#endif


	// determine point up/down weight from turret values
	// turret gives me -1 to +1 .. translate this to 0.0 to 1.0
	if (false && isFPS)
	{
		mlVehicleData*	pVehicleData = persGetPersonaModelDataPtr();

		model->upDownWeight = ((float)(pVehicleData->TurretElevate[0]) + 1.0f)/ 2.0f;

		if(model->upDownWeightAdjust)
			model->upDownWeight += model->upDownWeightAdjust;

		if(model->upDownWeightOverride)
			model->upDownWeight = model->upDownWeightOverride;

		//if(model->upDownWeight != model->targetUpDownWeight)
		//	uiChatPrintLine("New Up/Down weight is: %.2f", model->upDownWeight);

		model->targetUpDownWeight = model->upDownWeight;

		model->rectUpDownWeight = ((float)(pVehicleData->TurretElevate[2]) + 1.0f)/ 2.0f;
		model->rectTargetUpDownWeight = model->rectUpDownWeight;

		model->rectLeftRightWeight = ((float)(pVehicleData->TurretTraverse[2]) + 1.0f)/ 2.0f;

		// fudge for now (FIXME) because FPS uses 20 degrees and TPS art is 30 degrees
		//model->rectLeftRightWeight *= 2.0f/3.0f;
		model->rectTargetLeftRightWeight = model->rectLeftRightWeight;

		/*
		static FLOAT64 minUD = 1.0f;
		static FLOAT64 maxUD = 0.0f;
		static FLOAT64 minLR = 1.0f;
		static FLOAT64 maxLR = 0.0f;

		if(model->rectTargetUpDownWeight < minUD) minUD = model->rectTargetUpDownWeight;
		if(model->rectTargetUpDownWeight > maxUD) maxUD = model->rectTargetUpDownWeight;
		if(model->rectTargetLeftRightWeight < minLR) minLR = model->rectTargetLeftRightWeight;
		if(model->rectTargetLeftRightWeight > maxUD) maxLR = model->rectTargetLeftRightWeight;
		*/

		//uiChatPrintLine("Turrets: %f %f : %f %f", pVehicleData->TurretElevate[2], pVehicleData->TurretTraverse[2], model->rectUpDownWeight, model->rectLeftRightWeight);
//		Write_Log (LOG_FPS, LOG_INFO, "%f %f : %f %f", pVehicleData->TurretElevate[1], pVehicleData->TurretTraverse[1], model->rectUpDownWeight, model->rectLeftRightWeight);
	}
	else
	{
		static FLOAT32 rate = TPS_UPDOWN_RATE;
		FLOAT32 upDownMove = deltaTime * rate;

		if(model->myVehicle->IsMe())
		{
			mlVehicleData* pVehicleData			= persGetPersonaModelDataPtr();
			model->targetUpDownWeight			= ((float)(pVehicleData->TurretElevate[0]) + 1.0f)/ 2.0f;
			model->rectTargetUpDownWeight		= ((float)(pVehicleData->TurretElevate[2]) + 1.0f)/ 2.0f;
			model->rectTargetLeftRightWeight	= ((float)(pVehicleData->TurretTraverse[2]) + 1.0f)/ 2.0f;
		}
		else
		{
			model->targetUpDownWeight			= (model->myVehicle->StateAnimation.TurretElev[0] + 1.0f) / 2.0f;
			model->rectTargetUpDownWeight		= (model->myVehicle->StateAnimation.TurretElev[2] + 1.0f) / 2.0f;
			model->rectTargetLeftRightWeight	= (model->myVehicle->StateAnimation.TurretTraverse[2] + 1.0f) / 2.0f;
		}

		if(model->upDownWeightAdjust)
			model->targetUpDownWeight += model->upDownWeightAdjust;

		if(model->upDownWeightOverride)
			model->targetUpDownWeight = model->upDownWeightOverride;

		if (model->targetUpDownWeight < model->upDownWeight)
		{
			if (model->upDownWeight - upDownMove <= model->targetUpDownWeight)
			{
				model->upDownWeight = model->targetUpDownWeight;
			}
			else
			{
				model->upDownWeight = model->upDownWeight - upDownMove;
			}
		}
		else if(model->targetUpDownWeight > model->upDownWeight)
		{
			if (model->upDownWeight + upDownMove >= model->targetUpDownWeight)
			{
				model->upDownWeight = model->targetUpDownWeight;
			}
			else
			{
				model->upDownWeight = model->upDownWeight + upDownMove;
			}
		}

		if (model->rectTargetUpDownWeight < model->rectUpDownWeight)
		{
			if (model->rectUpDownWeight - upDownMove <= model->rectTargetUpDownWeight)
			{
				model->rectUpDownWeight = model->rectTargetUpDownWeight;
			}
			else
			{
				model->rectUpDownWeight = model->rectUpDownWeight - upDownMove;
			}
		}
		else if(model->rectTargetUpDownWeight > model->rectUpDownWeight)
		{
			if (model->rectUpDownWeight + upDownMove >= model->rectTargetUpDownWeight)
			{
				model->rectUpDownWeight = model->rectTargetUpDownWeight;
			}
			else
			{
				model->rectUpDownWeight = model->rectUpDownWeight + upDownMove;
			}
		}
		// else must be the same

		if (model->rectTargetLeftRightWeight < model->rectLeftRightWeight)
		{
			if (model->rectLeftRightWeight - upDownMove <= model->rectTargetLeftRightWeight)
			{
				model->rectLeftRightWeight = model->rectTargetLeftRightWeight;
			}
			else
			{
				model->rectLeftRightWeight = model->rectLeftRightWeight - upDownMove;
			}
		}
		else if(model->rectTargetLeftRightWeight > model->rectLeftRightWeight)
		{
			if (model->rectLeftRightWeight + upDownMove >= model->rectTargetLeftRightWeight)
			{
				model->rectLeftRightWeight = model->rectTargetLeftRightWeight;
			}
			else
			{
				model->rectLeftRightWeight = model->rectLeftRightWeight + upDownMove;
			}
		}
		// else must be the same
	}

	if (model->upDownWeight < 0)
		model->upDownWeight = 0.0f;
	if (model->upDownWeight > 1.0f)
		model->upDownWeight = 1.0f;

	if (model->rectLeftRightWeight < 0)
		model->rectLeftRightWeight = 0.0f;
	if (model->rectLeftRightWeight > 1.0f)
		model->rectLeftRightWeight = 1.0f;

	if (model->rectUpDownWeight < 0)
		model->rectUpDownWeight = 0.0f;
	if (model->rectUpDownWeight > 1.0f)
		model->rectUpDownWeight = 1.0f;

	if (model->HasFlag(gr2AnimTrooper::CHUTE_OPEN)) //(model->freeFall) ||
		tpsWeaponType = TPS_WEAPONTYPE_EMPTY;

	if(model->HasFlag(gr2AnimTrooper::RECOIL))
	{
		if(tpsWeaponType != TPS_WEAPONTYPE_EMPTY && model->InRange(20.0f))
			gr2AnimHandleRecoil(model);

		model->ClearFlag(gr2AnimTrooper::RECOIL);
	}

	if(model->transition)
	{
		bool keep = false;

		if(model->transition->type == gr2AnimationBlend::TRANSITION)
		{
			FLOAT32 weight = _getTransitionWeight(model, model->transition, 1.0f);

			if(weight)
			{
				gr2BlendControlSetWeight(model->skeleton, model->transition, weight);
				keep = true;
			}
			else
			{
				gr2BlendControlSetWeight(model->skeleton, model->transition, 0.0f);
			}
		}

		if(!keep)
			model->transition = nullptr;
	}

	//gr2AnimationBlend* blend = fpTPSGetBlend(model->currentPose, model->currentMove, tpsWeaponType, model->currentMove);

#ifdef GR2_BLEND_EDITOR
	if(model != _editModel)
	{
#endif

	_blendSpec.Set(model, tpsWeaponType, model->currentWeapon);

	_setBlend(_blendSpec);

#ifdef GR2_BLEND_EDITOR
	}
#endif

	/*
	if(blend)
	{
		switch(blend->numBlends)
		{
			case 5: // 4 blend (base + 4 corners)
			{
				if(model->rectTargetLeftRightWeight != model->lastRectTargetLeftRightWeight)
				{
					// value = up/down * left/right value
					// 1,1 is upper right
					FLOAT32 weightUL = (model->rectUpDownWeight) * (1.0f - model->rectLeftRightWeight);
					FLOAT32 weightUR = (model->rectUpDownWeight) * (model->rectLeftRightWeight);
					FLOAT32 weightDL = (1.0f - model->rectUpDownWeight) * (1.0f - model->rectLeftRightWeight);
					FLOAT32 weightDR = (1.0f - model->rectUpDownWeight) * (model->rectLeftRightWeight);

					// Write_Log (LOG_FPS, LOG_INFO, "%f %f : %f %f %f %f", model->rectUpDownWeight, model->rectLeftRightWeight, weightUL, weightUR, weightDL, weightDR);

					gr2BlendControlSetWeight(model->skeleton, blend, weightUL, weightUR, weightDL, weightDR);

					model->lastRectTargetLeftRightWeight = model->rectTargetLeftRightWeight;
					model->lastModelSpeed = model->modelSpeed;
				}
			}
			break;
			case 4: // 3 blend (base + up,mid,down)
			{
				if(model->targetUpDownWeight != model->lastTargetUpDownWeight || model->modelSpeed != model->lastModelSpeed)
				{
					// calculate weights
					if (model->upDownWeight >= 0.5f)
					{
						model->currentDownWeight = 0.0f;
						model->currentUpWeight = (2.0f * model->upDownWeight) - 1.0f;  // 2x - 1
						model->currentMiddleWeight = 1.0f - model->currentUpWeight;
					}
					else
					{
						model->currentUpWeight = 0.0f;
						model->currentDownWeight = 1.0f - (2.0f * model->upDownWeight);  // 1 - 2x
						model->currentMiddleWeight = 1.0f - model->currentDownWeight;
					}

					if (tpsWeaponType == TPS_WEAPONTYPE_GRENADE)
					{
						model->currentUpWeight *= 99.0;
						model->currentMiddleWeight *= 99.0;
						model->currentDownWeight *= 99.0;
					}

					// set the weights
					gr2BlendControlSetWeight(model->skeleton, blend, model->currentUpWeight, model->currentMiddleWeight, model->currentDownWeight);

					// set the speeds
					gr2BlendControlSetSpeed (model->skeleton, blend, model->modelSpeed);

					if(tpsWeaponType == TPS_WEAPONTYPE_GRENADE)
						gr2BlendControlSetLoopCountForGrenade(model->skeleton, blend, 1);

					model->lastTargetUpDownWeight = model->targetUpDownWeight;
					model->lastModelSpeed = model->modelSpeed;
				}
			}
			break;
			case 3: // 2 blend (base + up,down)
			{
				if(model->targetUpDownWeight != model->lastTargetUpDownWeight || model->modelSpeed != model->lastModelSpeed)
				{
					model->currentUpWeight = model->upDownWeight;
					model->currentDownWeight = 1.0f - model->upDownWeight;

					if (tpsWeaponType == TPS_WEAPONTYPE_GRENADE)
					{
						model->currentUpWeight *= 99.0;
						model->currentMiddleWeight *= 99.0;
						model->currentDownWeight *= 99.0;
					}

					gr2BlendControlSetWeight(model->skeleton, blend, model->currentUpWeight, model->currentDownWeight);
					gr2BlendControlSetSpeed(model->skeleton, blend, model->modelSpeed);

					if(tpsWeaponType == TPS_WEAPONTYPE_GRENADE)
						gr2BlendControlSetLoopCount(model->skeleton, blend, 1);

					model->lastTargetUpDownWeight = model->targetUpDownWeight;
					model->lastModelSpeed = model->modelSpeed;
				}
			}
			break;
			default: // 1 blend (base + mid)
			{
				if(model->modelSpeed != model->lastModelSpeed)
				{
					gr2BlendControlSetSpeed(model->skeleton, blend, model->modelSpeed);
					model->lastModelSpeed = model->modelSpeed;
				}
			}
			break;
		}
	}
	else
	{
		gr2BlendControlSetWeight(model->skeleton, defaultAnimation, 999.0);
	}
	*/

	if(model->currentPose == TPS_POSETYPE_SWIM)
	{
		model->SetFlag(gr2AnimTrooper::HIDE_GUN);
	}
	else if((tpsWeaponType == TPS_WEAPONTYPE_GRENADE) && ((frametime - model->lastWeaponFireTime) < 2000))
	{
		model->SetFlag(gr2AnimTrooper::HIDE_GUN);
	}
	/*
	else if(model->emoteID && fpTPSGetEmoteType(model->emoteID) == (fpTPSBodyEmoteType)TPS_EMOTETYPE_FULLBODY) // casting gets rid of compile warning, but this is s bit skanky
	{
		model->SetFlag(gr2AnimTrooper::HIDE_GUN);
	}
	*/
	else if(model->HasFlag(gr2AnimTrooper::CHUTE_OPEN))
	{
		model->SetFlag(gr2AnimTrooper::HIDE_GUN);
	}
	else
	{
		model->ClearFlag(gr2AnimTrooper::HIDE_GUN);
	}

	// check for and update movement sounds here
	// make sure we're not FPS
	// if either the states have changed or the sound isn't currently playing, go for it
	if (distToMe <= polishNear)
		model->moveSound.Update();

	return TRUE;
}

/* // can't do this right now since kit objects are shared
void gr2AnimKitFixups(gr2AnimTrooper* model, SINT32 nextWeapon)
{
	if(!model->active || model->deathState)
		return;

	switch(model->myVehicle->Country)
	{
		case 2:
		{
			if(model->currentWeapon != nextWeapon)
			{
				if(model->currentWeaponType == TPS_WEAPONTYPE_PISTOL)
				{
					model->kit
				}
				else if(gr2AnimWeaponNumToTPSType(model, nextWeapon) == TPS_WEAPONTYPE_PISTOL)
				{

				}
			}
		}
		break;
	}
}
*/

LOGICAL gr2AnimSetModelState (OBJECT* pObject,
							  SINT32 LOD,
							  SINT32 pose,
							  SINT32 move,
							  SINT32 point,
							  UINT32 weapon,
							  UINT32 animState)
{
	if(!pObject || !(pObject->animModel) || !pObject->animModel->active)
		return FALSE;

	gr2AnimTrooper* model = pObject->animModel;

#ifdef GR2_BLEND_EDITOR
	if(model == _editModel)
		return FALSE;
#endif

	LOGICAL forceUpdate = 0;
	SINT32	currentWeapon = (SINT32)weapon;

	// this needs to be done before error checking!
	if((currentWeapon == GR2ANIM_USE_DEFAULT_WEAPON) || (model->currentWeapon == -1))
	{
		currentWeapon = 0;
		forceUpdate = 1;
	}

	// error check inputs .. if they're bad, ignore them .. could cause trouble, but not as bad as a crash

	if((pose < 0) || (pose >= TPS_POSETYPE_NUM_TYPES))
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimSetModelState: Pose out of bounds %d!", pose);
		return TRUE;
	}

	if((move < 0) || (move >= TPS_MOVETYPE_NUM_TYPES))
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimSetModelState: Movement out of bounds %d!", move);
		return TRUE;
	}

	if((point < 0) || (point >= TPS_POINTTYPE_NUM_TYPES))
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimSetModelState: Point out of bounds %d!", point);
		return TRUE;
	}

	if((currentWeapon < 0) || (currentWeapon >= MAX_NUM_FPS_WEAPONS))
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimSetModelState: Weapon out of bounds %d!", currentWeapon);
		return TRUE;
	}

	// do some basic state processing based on the weapon type
	model->currentWeaponType = gr2AnimWeaponNumToTPSType(model, currentWeapon);
	fpTPSWeaponType tpsWeaponType = model->currentWeaponType;
	fpTPSWeaponClass tpsWeaponClass = fpTPSConvertWeaponClass(tpsWeaponType);

	// prevent changing to binocs or map while moving; could cause Jesus pose from missing states
	if(move != TPS_MOVETYPE_STILL && (tpsWeaponType == TPS_WEAPONTYPE_BINOCS || tpsWeaponType == TPS_WEAPONTYPE_MAP))
	{
		currentWeapon				= model->currentWeapon;
		model->currentWeaponType		= gr2AnimWeaponNumToTPSType(model, currentWeapon);
		tpsWeaponType				= model->currentWeaponType;
		tpsWeaponClass				= fpTPSConvertWeaponClass(tpsWeaponType);
	}

	if(model->overrideTime >= frametime)
	{
		if(model->overridePose >= 0)
			pose = model->overridePose;
		if(model->overrideMove >= 0)
			move = model->overrideMove;
		if(model->overrideWeapon >= 0)
			weapon = model->overrideWeapon;
		if(model->overridePoint >= 0)
			point = model->overridePoint;
	}
	else if(model->overrideTime)
	{
		model->overrideTime		= 0;
		model->overridePose		= -1;
		model->overrideMove		= -1;
		model->overrideWeapon	= -1;
		model->overridePoint	= -1;
	}

	// manipulate our state here based on the incoming flags
	// switch to idle animations as needed

	if ((move == TPS_MOVETYPE_BACKWARD) ||
		(model->currentMotion == TPS_MOTION_RFB) ||
		(model->currentMotion == TPS_MOTION_LFB) ||
		(model->currentMotion == TPS_MOTION_B))
	{
		model->SetFlag(gr2AnimTrooper::BACKWARDS);
	}
	else
	{
		model->ClearFlag(gr2AnimTrooper::BACKWARDS);
	}

	if(pose == TPS_POSETYPE_SIT && move > TPS_MOVETYPE_IDLE)
		move = TPS_MOVETYPE_STILL;

	if((animState & gr2AnimTrooper::WALKING) && pose == TPS_POSETYPE_STAND && (point == TPS_POINTTYPE_RELAXED || point == TPS_POINTTYPE_READY))
	{
		if(!model->HasFlag(gr2AnimTrooper::ATTACK1|gr2AnimTrooper::ATTACK2) && !model->attacking && frametime > model->attackUntil && frametime > model->readyUntil)
		{
			if(move == TPS_MOVETYPE_FORWARD || move == TPS_MOVETYPE_BACKWARD)
				move = TPS_MOVETYPE_WALK;

			if(point != TPS_POINTTYPE_RELAXED)
				point = TPS_POINTTYPE_RELAXED;
		}
	}

	if(move == TPS_MOVETYPE_BACKWARD)
		move = TPS_MOVETYPE_FORWARD;

	//if(point == TPS_POINTTYPE_AIM && pose == TPS_POSETYPE_STAND && move != TPS_MOVETYPE_STILL)
	//	move = TPS_MOVETYPE_WALK;

	if(pose == TPS_POSETYPE_STAND && move == TPS_MOVETYPE_FORWARD && model->currentMotion != TPS_MOTION_F && !model->strafe)
		pose = TPS_POSETYPE_CROUCH;
	else if(pose == TPS_POSETYPE_STAND && move > TPS_MOVETYPE_IDLE && point == TPS_POINTTYPE_AIM)
		pose = TPS_POSETYPE_CROUCH;

	if(pose == TPS_POSETYPE_PRONE && move > TPS_MOVETYPE_STILL && move != TPS_MOVETYPE_DEPLOYED && point != TPS_POINTTYPE_RELAXED)
	{
		if(move == TPS_MOVETYPE_IDLE)
			move = TPS_MOVETYPE_STILL;

		point = TPS_POINTTYPE_RELAXED;
	}

	if(pose != TPS_POSETYPE_SWIM && move != TPS_MOVETYPE_STILL && model->strafe)
		move = model->strafe;

	// movement test .. quick test to see how it responds if I make it so movement is based on actual
	// velocity instead of state bits. conclusion:  smoother craps the data so I can't do this yet
	/*
	if ((pObject->animModel) && (pObject->animModel->myVehicle) && (pose == TPS_POSETYPE_STAND))
	{
		FLOAT64VECTOR	vel;
		FLOAT32			speed2;

		if ((movement == TPS_MOVETYPE_IDLE) ||
			(movement == TPS_MOVETYPE_FORWARD) ||
			(movement == TPS_MOVETYPE_WALK) ||
			(movement == TPS_MOVETYPE_SPRINT))
		{
			if (pObject->animModel->myVehicle->ModelData)
				vel = pObject->animModel->myVehicle->ModelData->VehicleVel;
			else
				vel = pObject->animModel->myVehicle->PosData.Vel;

			speed2 = (FLOAT32)mathExactVectorLength2(&vel);

			if (speed2 > 16.0f)
			{
				movement = TPS_MOVETYPE_SPRINT;
			}
			else if (speed2 > 4.0f)
				movement = TPS_MOVETYPE_FORWARD;
			else if (speed2 > 0)
				movement = TPS_MOVETYPE_WALK;
			else
				movement = TPS_MOVETYPE_IDLE;
		}
	}
	*/

	// regardless of anything else, binocs always are ready
	if(tpsWeaponType == TPS_WEAPONTYPE_BINOCS || tpsWeaponType == TPS_WEAPONTYPE_MAP)
		point = TPS_POINTTYPE_READY;
	else if(tpsWeaponType == TPS_WEAPONTYPE_ROCKET && move > TPS_MOVETYPE_STILL)
		point = TPS_POINTTYPE_RELAXED;

	// switch from reload to reload_cup if we're dealing with a rifle grenade
	//if (tpsWeaponType == TPS_WEAPONTYPE_RIFLE && (animState & gr2AnimTrooper::USE_ALT_WEAPON) && point == TPS_POINTTYPE_RELOAD)
	//	point = TPS_POINTTYPE_ACTION1;

	// if we're idle and we've determined we're turning in place, override the move type
	if(pose != TPS_POSETYPE_SWIM && frametime <= abs(model->turnInPlace) && model->myVehicle->RideVehicleHostIndex == 0)
	{
		if(move == TPS_MOVETYPE_STILL)
		{
			SINT32 fpsWeapon = (SINT32)model->weaponArray[model->currentWeapon];

			// if we have a turn, use it
			if(fpTPSGetBlend(pose, TPS_MOVETYPE_TURN, tpsWeaponType, point) || fpTPSGetBlend(pose, TPS_MOVETYPE_TURN, fpsWeapon, point, 1))
			{
				move = TPS_MOVETYPE_TURN;
			}
			else
			{
				move = TPS_MOVETYPE_FORWARD;

				if(pose == TPS_POSETYPE_STAND && point == TPS_POINTTYPE_AIM)
					move = TPS_MOVETYPE_WALK;
				else if(pose == TPS_POSETYPE_PRONE && fpsWeapon == PIAT && point == TPS_POINTTYPE_AIM)
					point = TPS_POINTTYPE_RELAXED;
				else if(pose == TPS_POSETYPE_PRONE && (point == TPS_POINTTYPE_READY || point == TPS_POINTTYPE_RELOAD))
					point = TPS_POINTTYPE_RELAXED;
			}
		}
	}

	// switch to alternate models as necessary
	UINT32 altFlags = gr2AnimTrooper::USE_ALT_WEAPON|gr2AnimTrooper::USE_ALT_MODEL;

	if((model->animState & altFlags) != (animState & altFlags))
	{
		model->SetFlag(gr2AnimTrooper::USE_ALT_WEAPON, animState);
		model->SetFlag(gr2AnimTrooper::USE_ALT_MODEL, animState);

		// set our alt models as necessary
		model->model.Set(model->HasFlag(gr2AnimTrooper::USE_ALT_MODEL));
		model->weapons[model->currentWeapon].Set(model->HasFlag(gr2AnimTrooper::USE_ALT_WEAPON));

		model->nextLODCheck = frametime;
	}

	// set whether we are attacking or not if we are past our last attackUntil time
	if(true)//frametime >= model->attackUntil)
	{
		model->SetFlag(gr2AnimTrooper::ATTACK1, animState);
		model->SetFlag(gr2AnimTrooper::ATTACK2, animState);
	}

	// handle any attacking states
	gr2AnimHandleAttack(model, pose, move, point, tpsWeaponType);

	if(model->readyUntil > frametime && point == TPS_POINTTYPE_RELAXED)
	{
		if(pose == TPS_POSETYPE_STAND && move != TPS_MOVETYPE_SPRINT && move != TPS_MOVETYPE_WALK)
		{
			point = TPS_POINTTYPE_READY;
		}
		else if(pose == TPS_POSETYPE_CROUCH && move == TPS_MOVETYPE_FORWARD)
		{
			point = TPS_POINTTYPE_READY;
		}

		if(pose == TPS_POSETYPE_STAND && move == TPS_MOVETYPE_STRAFE_LEFT && point == TPS_POINTTYPE_READY && tpsWeaponClass == CLASS_FIREARM)
			move = TPS_MOVETYPE_STRAFE_RIGHT;

		/*
		if(pose == TPS_POSETYPE_STAND && model->strafe && point == TPS_POINTTYPE_READY)
		{
			pose = TPS_POSETYPE_CROUCH;
		}
		*/
	}

	/*
	static LOGICAL lastFireCheck = FALSE;

	if(model->myVehicle->ModelData)
	{
		if(model->myVehicle->ModelData->MountIsFiring[0] != lastFireCheck)
		{
			lastFireCheck = model->myVehicle->ModelData->MountIsFiring[0];
			uiChatPrintLine("Fire Flag is %s", (lastFireCheck) ? "on" : "off");
		}
	}
	else
	{
		if(model->myVehicle->MountIsFiring[0] != lastFireCheck)
		{
			lastFireCheck = model->myVehicle->MountIsFiring[0];
			uiChatPrintLine("Fire Flag is %s", (lastFireCheck) ? "on" : "off");
		}
	}
	*/

	if(animState & gr2AnimTrooper::CHUTE_OPEN)
	{
		pose = TPS_POSETYPE_STAND;
		move = TPS_MOVETYPE_FALL;
		point = TPS_POINTTYPE_PARA;
		tpsWeaponType = TPS_WEAPONTYPE_EMPTY;
	}
	else if(animState & gr2AnimTrooper::FREE_FALL)
	{
		move = TPS_MOVETYPE_FALL;
		pose = TPS_POSETYPE_STAND;
		point = TPS_POINTTYPE_RELAXED;
	}

	pObject->animModel->ClearFlag(gr2AnimTrooper::USE_SELF_MODEL);

	if(animState & gr2AnimTrooper::CHUTE_OPEN)
	{
		model->ClearFlag(gr2AnimTrooper::HIDE_CHUTE);

		if ((pObject->flag2 & OBJECT_ISFPS) && !fpFPSIsExternalView())
			pObject->animModel->SetFlag(gr2AnimTrooper::USE_SELF_MODEL);
	}
	else
	{
		model->SetFlag(gr2AnimTrooper::HIDE_CHUTE);
	}

	gr2AnimUpdateModel(pObject, pObject->flag2 & OBJECT_ISFPS);

	// states to look for changes in
	const UINT32 changeMask = (gr2AnimTrooper::CHUTE_OPEN|
							   gr2AnimTrooper::FREE_FALL|
 							   gr2AnimTrooper::DEAD);

	// do we have a notable state change?
	bool animStateChange = ((changeMask & model->animState) != (changeMask & animState));

	// ok, now that we've done all the processing that needs to happen ..
	// see if anything important actually changed if it didn't, we're done here
	if ((model->currentPose == ((fpTPSPoseType)pose)) &&
		(model->currentMove == ((fpTPSMoveType)move)) &&
		(model->currentPoint == ((fpTPSPointType)point)) &&
		(model->currentWeapon == currentWeapon) &&
		(!animStateChange) &&
		(!forceUpdate))
	{
		return TRUE;
	}

#ifdef BETA_RELEASE
	{
		SCHAR tempStr[255];

		if(stateTestFlag)// && !pObject->animModel->myVehicle->ModelData) // stateTestFlag &&
		{
			sprintf (tempStr, "TPS STATE: %s, %s, %s, %s, %s, %d, %d", (const char*)pObject->animModel->myVehicle->Handle, fpTPSPoseKeyStrings[pose], fpTPSMoveKeyStrings[move], fpTPSWeaponKeyStrings[tpsWeaponType], fpTPSPointKeyStrings[point], currentWeapon, (changeMask & gr2AnimTrooper::DEAD) ? 1 : 0);
			rnmainListText (tempStr);
		}
	}
#endif	// BETA_RELEASE

	//gr2AnimKitFixups(model, currentWeapon);

	//model->upDownWeightAdjust = 0;
	model->lastModelSpeed = 0;
	model->lastTargetUpDownWeight = 99.0f;
	model->lastRectTargetUpDownWeight = 99.0f;
	model->lastRectTargetLeftRightWeight = 99.0f;

	// death first
	if(animState & gr2AnimTrooper::DEAD)
	{
		SINT32 deathState = (point == TPS_POINTTYPE_PARA) ? TPS_DEATH_PARA : TPS_DEATH1;

		if(model->deathState != deathState)
		{
			/*
			SINT32	deathSoundID;

			gr2UpdateModel(model->skeleton, true, true);

			FLOAT64VECTOR& pos = (model->myVehicle->ModelData) ?
									model->myVehicle->ModelData->VehiclePos:
									model->myVehicle->PosData.Pos;

			bool inFoV		= (model->myVehicle->ModelData != nullptr || rnvisFOVCheck(model->myVehicle->VehicleObjectData->PlaneObject));
			bool ragdolled	= false;

			if(inFoV && model->HasFlag(gr2AnimTrooper::CHUTE_OPEN)) // deathState != TPS_DEATH_PARA)
			{
				bool addForce = (model->deathState != TPS_DEATH_PARA);

				FLOAT64VECTOR& v = (model->myVehicle->ModelData) ?
													model->myVehicle->ModelData->VehicleVel:
													model->myVehicle->PosData.Vel;

				// clear any velocity if it was a para landing death
				if(!addForce)
					v.x = v.y = v.z = 0.0f;

				// add our ragdoll
				ragdolled = (fpTPSAddRagdoll(model, -1, -1, addForce) != nullptr);

				// stop any sounds
				pObject->animModel->moveSound.Stop();
			}

			model->SetFlag(gr2AnimTrooper::DEAD, true);
			*/

			gr2UpdateModel(model->skeleton, false, false);

			if(deathState == TPS_DEATH_PARA)
			{
				gr2RemoveAllBlendedAnimations (model->skeleton, -1);

				if(model->active && fpTPSDeathBlends[TPS_POSETYPE_STAND][TPS_DEATH_PARA])
				{
					gr2AnimationBlend* deathBlend = fpTPSDeathBlends[TPS_POSETYPE_STAND][TPS_DEATH_PARA];

					if(gr2PlayBlendedAnimation (model->skeleton, deathBlend))
					{
						gr2BlendControlSetMixin(model->skeleton, deathBlend);
						gr2BlendControlSetLoopCount(model->skeleton, deathBlend, 1);
					}
					else
					{
						Write_Log (LOG_FPS, LOG_INFO, "Failed to play death blend %d (%s)!", TPS_DEATH_PARA, fpTPSDeathFiles[TPS_POSETYPE_STAND][TPS_DEATH_PARA]);
					}
				}
			}
			else if(model->myVehicle->ModelData != nullptr || rnvisFOVCheck(model->myVehicle->VehicleObjectData->PlaneObject))
			{
				// add our ragdoll
				fpTPSAddRagdoll(model, -1, -1, (model->deathState != TPS_DEATH_PARA));
			}

			// stop any sounds
			pObject->animModel->moveSound.Stop();

			// save our death state
			model->deathState = deathState;

			// stop any attacks we were doing
			gr2AnimStopAttacking(model);

			if(model->deathState == TPS_DEATH1)
				model->SetFlag(gr2AnimTrooper::DEAD, true);

			// add a death icon
			/*
			if(cfgPrefsGetBool("app.deathicons", true))
			{
				ChatHandle& handle = model->myVehicle->Chat_Handle;
				FLOAT64VECTOR dpos = pos;
				dpos.z += 1.0f;
				PersHUDManager::GetInstance()->AddObject(new PersHUDManager::HUDDeath(handle, dpos));
			}
			*/
		}

		return TRUE;
	}

	if(model->InRange(100.0f))
		gr2AnimHandleEmotes(model);

	_blendSpec.Set(model, tpsWeaponType, currentWeapon);

	// setup our blend spec with the correct easein/easeout times and
	// whatever else is needed to make this state change look good
	gr2AnimHandleTransitions(model, pose, move, point, tpsWeaponType);

	if(model->InRange(100.0f))
		gr2AnimHandlePoseChange(model, pose, move, point, tpsWeaponType);

	gr2AnimHandleStowing(model);

	if(pose == TPS_POSETYPE_SWIM && pose != model->currentPose)
		model->ClearFlag(gr2AnimTrooper::SPLASHED);

	model->SetFlag(gr2AnimTrooper::CHUTE_OPEN, animState);
	model->SetFlag(gr2AnimTrooper::FREE_FALL, animState);

	model->currentWeaponType		= tpsWeaponType;
	model->currentWeapon			= currentWeapon;
	model->currentPose			= (fpTPSPoseType)pose;
	model->currentMove			= (fpTPSMoveType)move;
	model->currentPoint			= (fpTPSPointType)point;

	gr2RemoveAllBlendedAnimations(model->skeleton, _blendSpec.removeEaseOut, model->transition);

	_blendSpec.start	= true;
	_blendSpec.pose		= pose;
	_blendSpec.move		= move;
	_blendSpec.weapon	= tpsWeaponType;
	_blendSpec.point	= point;

#ifdef GR2_BLEND_EDITOR
	if(model != _editModel)
	{
#endif

	_setBlend(_blendSpec);
#ifdef GR2_BLEND_EDITOR
	}
#endif

	// add in a little motion while standing or kneeling and still
	if(move == TPS_MOVETYPE_STILL && !model->attacking && model->InRange(10.0f))
	{
		gr2AnimationBlend* motion = fpTPSGetBlend(pose, move, tpsWeaponType, TPS_POINTTYPE_MOTION);

		if(motion)
		{
			gr2AnimStartBlendedAnimation(model, motion, .1f, .1f);
			gr2BlendControlSetStartOffset(model->skeleton, motion, GetRandom(0.0f,1.0f));

			if(model->currentPoint != TPS_POINTTYPE_AIM)
				gr2BlendControlSetWeight(model->skeleton, motion, GetRandom(1.0f,3.0f));
			else
				gr2BlendControlSetWeight(model->skeleton, motion, 1.0f);
		}
	}

	model->animationOffsetPercent = 0.0f;

	return TRUE;
}

LOGICAL gr2AnimKillObject (OBJECT* pObject)
{
	// stop any sounds
	pObject->animModel->moveSound.Stop();

	/*
#ifdef USE_CORPSES
    LOGICAL sitting = gr2AnimIsTrooperSitting(pObject);
	LOGICAL nochute = pObject->animModel->HasFlag(gr2AnimTrooper::HIDE_CHUTE);

    // Only leave dead body if we are...dead
	// Only leave a corpse if the death animation completed
    if(!sitting &&
		nochute &&
		pObject->animModel->deathState &&
		pObject->animModel->active &&
		cfgPrefsGetBool("app.corpses", true))
    {
		// don't leave corpses for non-ragdolls for now
		gr2AnimTrooper* model = pObject->animModel;

		if (fpTPSDeathBlends[model->currentPose][model->deathState])
		{
			gr2ClearBlendedAnimations(model->skeleton);

			if (gr2PlayBlendedAnimation (model->skeleton, fpTPSDeathBlends[model->currentPose][model->deathState]))
			{
				gr2BlendControlSetLoopCount(model->skeleton, fpTPSDeathBlends[model->currentPose][model->deathState], 1);
				gr2BlendControlSetLastFrame(model->skeleton, fpTPSDeathBlends[model->currentPose][model->deathState]);
				gr2BlendControlSetEaseOutDuration(model->skeleton, fpTPSDeathBlends[model->currentPose][model->deathState], 0);
				gr2BlendControlSetEaseInDuration(model->skeleton, fpTPSDeathBlends[model->currentPose][model->deathState], 0);

				// make sure our visual models are properly updated
				gr2UpdateModel(model->skeleton, true, true);

				Corpse* dk = gr2AnimAddCorpse(pObject->animModel, "Bip01 Spine", true);

				// look up its TO object
				gr2AnimCorpseTO(dk);
			}
		}
    }
#endif
	*/

	_freemodel(pObject->animModel);
    pObject->animModel = nullptr;

	return TRUE;
}

LOGICAL gr2AnimDoWeaponCollision (OBJECT* pObject, piVECTOR* startPnt, piVECTOR* endPnt, SINT32* location, piVECTOR* collisionPnt)
{
	piMATRIX		inverseMat;
	piVECTOR		rayStart, rayEnd, rayNormal;
	LOGICAL			collisionOccured = FALSE;
	gr2AnimTrooper*	model;

	if (location == nullptr || pObject == nullptr || startPnt == nullptr || endPnt == nullptr)
	{
		return FALSE;
	}

	model = pObject->animModel;

	piMATRIX world = pObject->mat;

	world._31 = -world._31;
	world._32 = -world._32;
	world._33 = -world._33;

	// take model pitch into account
	math3dMatrixMultiply(&world, &model->terrainMat, &world);

	// get the inverse world mat
	math3dInvertMatrix(&inverseMat, &world);

	// bring the world oriented rays into the model's object space
	math3dVectorMultiply(&rayStart, startPnt, &inverseMat);
	//rayStart.z = -rayStart.z;

	math3dVectorMultiply(&rayEnd, endPnt, &inverseMat);
	//rayEnd.z = -rayEnd.z;

	// adjust it by any pitch adjustments
	//math3dVectorMultiply(&rayStart, &rayStart, &model->terrainMat);
	//math3dVectorMultiply(&rayEnd, &rayEnd, &model->terrainMat);

	mathSubVectors(&rayNormal, &rayEnd, &rayStart);

	/*
	collisionModel = model->collisionModel;

	if(model->lastColliderUpdate != frametime)
	{
		collisionModel->Update(false, false);
		model->lastColliderUpdate = frametime;
	}
	*/

	collisionOccured = gr2CollideRayWithModel(model->collisionModel, &rayStart, &rayNormal, location, collisionPnt);

	return collisionOccured;
}

LOGICAL gr2AnimGetCurrentWeapon (OBJECT* pObject, SINT32* weapon)
{
	if (PN_UNLIKELY(pObject == nullptr))
	{
		return (FALSE);
	}

	if (pObject->animModel == nullptr)
	{
		return (FALSE);
	}

	*weapon = pObject->animModel->currentWeapon;

	return(TRUE);
}

LOGICAL gr2AnimGetBoneMatrix (struct OBJECT* pObject, piMATRIX* pMatrix, const char* bone, bool visual)
{
	if((pObject == nullptr) || (pObject->animModel == nullptr))
		return(FALSE);

	if(pMatrix == nullptr)
		return(FALSE);

	if(pObject->animModel->currentWeapon < 0 || pObject->animModel->currentWeapon >= MAX_NUM_FPS_WEAPONS)
		return FALSE;

	gr2Model* weapModel = pObject->animModel->weapons[pObject->animModel->currentWeapon].GetActive();

	if (!weapModel)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimGetBoneMatrix: Null weapon model");
		return FALSE;
	}

	LOGICAL fireFind = gr2GetBonePosition (weapModel, (SCHAR*)bone, pMatrix);

	if(!fireFind)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimGetBoneMatrix: Failed to find %s bone, using IdentityMatrix!", bone);
		*pMatrix = IdentityMatrix;
	}

	if(visual)
		math3dMatrixMultiply(pMatrix, pMatrix, &pObject->animModel->terrainMat);

	fpTPSWeaponType tpsWeaponType = pObject->animModel->currentWeaponType;

	// fix up the grenades based on the trooper's up down angle .. this makes the 3rd person throw represenation more accurate
	// since grenades are just "launched"
	if (tpsWeaponType == TPS_WEAPONTYPE_GRENADE)
	{
		piMATRIX finalMatrix;

		generateRotationMatrix (AROUND_X, 180.0 + (80.0 * (pObject->animModel->myVehicle->StateAnimation.TurretElev[0])), &finalMatrix);

		finalMatrix._41 = pMatrix->_41;
		finalMatrix._42 = pMatrix->_42;
		finalMatrix._43 = pMatrix->_43;

		*pMatrix = finalMatrix;
	}

	pObject->animModel->lastWeaponFireTime = frametime;

	return(TRUE);
}

LOGICAL gr2AnimGetFirePointMatrixByBone(OBJECT* pObject, piMATRIX* pMatrix, bool visual)
{
	return gr2AnimGetBoneMatrix(pObject, pMatrix, "firepoint", visual);
}

LOGICAL gr2AnimGetFirePointMatrix(OBJECT* pObject, piMATRIX* pMatrix, bool visual)
{
	if ((pObject == nullptr) || (pObject->animModel == nullptr) || (pMatrix == nullptr))
		return(FALSE);

	gr2AnimTrooper* anim	= pObject->animModel;
	fpTPSWeaponType wtype	= anim->currentWeaponType;
	bool useFirePoint		= visual;

	switch(wtype)
	{
		case TPS_WEAPONTYPE_GRENADE:
		//case TPS_WEAPONTYPE_SATCHEL:
		case TPS_WEAPONTYPE_KNIFE:
		case TPS_WEAPONTYPE_LTMORTAR:
		{
			//if(anim->myVehicle->IsMe())
			//	return false;

			useFirePoint = true;
		}
		break;
        default:
            // all others use default of what was passed in
            break;
	}

	if(useFirePoint) // use old fire point bone method
		return gr2AnimGetFirePointMatrixByBone(pObject, pMatrix, visual);

	// this method, takes the eye heigh, plus yaw plus turret 0 traverse and elevate to calculate the proper fire point matrix
	// makes it much more accurate in a network environment
	vehVehicle* vehicle	= anim->myVehicle;

	Vec3f firePos		= Vec3f(0, 0, fpGetZAdjust(anim->currentPose, anim->currentMove));
	Vec3d forward		= Vec3d(0.0f, 1.0f, 0.0f);

	Matrixf fireMat;
	Matrixf rotate;

	FLOAT32 trav		= 0;
	FLOAT32 elev		= 0;

	bool deployed		= (anim->currentMove == TPS_MOVETYPE_DEPLOYED);
	bool pivot			= (wtype == TPS_WEAPONTYPE_LMG || wtype == TPS_WEAPONTYPE_ATR); // wtype == TPS_WEAPONTYPE_LMG_MG34 ||
	int turret			= (deployed && pivot) ? 1: 0;

	// a few additional pose and move based z adjustments to firepoint
	if(anim->currentPose == TPS_POSETYPE_PRONE)
	{
		firePos.z() -= .1f;

		if(deployed)
		{
			// defaults
			firePos.z() += .2f; // move it back up slightly to get over obstacles
			forward.x() -= .65f;

			// ideally use the firepoint bone for the position
			if(anim->currentWeapon >= 0 && anim->currentWeapon < MAX_NUM_FPS_WEAPONS)// && anim->currentLOD >= 0 && anim->currentLOD < TPS_NUM_LODS)
			{
				gr2Model* weapModel = nullptr;

				/*
				if (anim->HasFlag(gr2AnimTrooper::USE_WEAPON_MODEL))
					weapModel = anim->weaponAltModels[anim->currentWeapon][anim->currentLOD];
				else
					weapModel = anim->weaponModels[anim->currentWeapon][anim->currentLOD];
				*/

				weapModel = anim->weapons[anim->currentWeapon].GetActive();

				if(weapModel && gr2GetBonePosition (weapModel, "firepoint", pMatrix))
				{
					// set out fire point position
					firePos.set(pMatrix->_41, pMatrix->_42, pMatrix->_43);

					// no need to adjust if we have a proper fire point
					forward.set(0,0,0);
				}
			}
		}
	}
	else if(anim->currentPose == TPS_POSETYPE_CROUCH)
	{
		if(anim->currentPoint == TPS_POINTTYPE_AIM)
			firePos.z() -= .1f;
		else if(anim->currentMove == TPS_MOVETYPE_DEPLOYED)
			firePos.z() += .05f; // use eye height, but move it up slightly to get over obstacles
		else
			firePos.z() = (firePos.z() * .5f) + .1f; // around the waist
	}
	else
	{
		if(anim->currentPoint == TPS_POINTTYPE_AIM)
			firePos.z() -= .1f;
		else if(anim->currentMove == TPS_MOVETYPE_DEPLOYED)
			firePos.z() += .05f; // use eye height, but move it up slightly to get over obstacles
		else
			firePos.z() -= .4f; // around the waist
	}

	// get the turret info
	if(vehicle->ModelData)
	{
		trav = (float)vehicle->ModelData->TurretTraverse[turret];
		elev = (float)vehicle->ModelData->TurretElevate[turret];
	}
	else
	{
		vehanimVehicleStateAnimations& anims = vehicle->StateAnimation;

		trav = anims.TurretTraverse[turret];
		elev = anims.TurretElev[turret];
	}

	// special casing for odd deploy offsets for lmgs
	// couldn't figure out where these are coming from
	if(deployed && pivot && wtype != TPS_WEAPONTYPE_ATR)
	{
		switch(anim->myVehicle->VehicleType->WeaponSpec.country())
		{
			case WWII_ENGLAND:
				elev += .02f;
			break;
			case WWII_FRANCE:
				trav += .01f;
			break;
			case WWII_GERMANY:
				elev += .01f;
				trav += .08f;
			break;
            default:
                // undefined country, leave alone.
                break;
		}
	}

	fireMat.makeIdentity();

	// start off rotating around the z 180 degrees because troopers are like that, wtf?
	if(!vehicle->IsMe() && anim->currentWeaponType != TPS_WEAPONTYPE_SATCHEL)
		rotate.makeRotate(mlDEG_TO_RAD(180.0f), 0.0f, 0.0f, 1.0f);

	// rotate to our trooper front
	fireMat = fireMat * rotate;

	if(elev)
	{
		FLOAT32 maxUp = 80.0f; //(turret == 1) ? fpsTrooper.maxProneDeployedUpAngle: 80.0f;
		FLOAT32 maxDown = 70.0f; //(turret == 1) ? fpsTrooper.maxProneDeployedDownAngle: 70.0f;

		// an adjustment for some offset I can't find
		//elev += (deployed) ? .03f: 0;

		// fps elevation ranges from 70 degrees down to 80 degrees up
		FLOAT32 elevDeg = elev * ((elev < 0) ? maxDown: maxUp);

		// rotate the elevation degrees around the x axis
		rotate.makeRotate(mlDEG_TO_RAD(-elevDeg), 1.0f, 0.0f, 0.0f);

		// rotate a forward vector to translate use out in front of the eyepoint by a meter
		forward = forward * rotate;

		// rotate us elev 0 degrees
		fireMat = fireMat * rotate;
	}

	if(trav)
	{
		FLOAT32 maxLeft = fpsTrooper.maxProneDeployedLeftAngle;
		FLOAT32 maxRight = fpsTrooper.maxProneDeployedRightAngle;

		// fps traverse ranges from 70 degrees down to 80 degrees up
		FLOAT32 travDeg = trav * ((trav < 0) ? maxLeft: maxRight);

		// rotate the elevation degrees around the x axis
		rotate.makeRotate(mlDEG_TO_RAD(-travDeg), 0.0f, 0.0f, 1.0f);

		// rotate a forward vector to translate use out in front of the eyepoint by a meter
		forward = forward * rotate;

		// rotate us elev 0 degrees
		fireMat = fireMat * rotate;
	}

	// move it out along the barrel some
	firePos += forward;

	// set our attitude position, relative to our vehicle position
	fireMat.setTrans(firePos);

	// save our result
	fireMat.get(*pMatrix);

	// update our last fire time
	pObject->animModel->lastWeaponFireTime = frametime;

	return(TRUE);
}

LOGICAL gr2AnimGetEjectPointMatrix (OBJECT* pObject, piMATRIX* pMatrix, bool visual)
{
	gr2Model*	weapModel;

	if ((pObject == nullptr) || (pObject->animModel == nullptr))
	{
		return(FALSE);
	}

	if (pMatrix == nullptr)
	{
		return(FALSE);
	}

	/*
	if (pObject->animModel->HasFlag(gr2AnimTrooper::USE_WEAPON_MODEL))
	{
		weapModel = pObject->animModel->weaponAltModels[pObject->animModel->currentWeapon][pObject->animModel->currentLOD];
	}
	else
	{
		weapModel = pObject->animModel->weaponModels[pObject->animModel->currentWeapon][pObject->animModel->currentLOD];
	}
	*/

	weapModel = pObject->animModel->weapons[pObject->animModel->currentWeapon].GetActive();

	if (!weapModel)
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2AnimGetEjectPointMatrix: Null weapon model");
		return FALSE;
	}

	LOGICAL ok = gr2GetBonePosition (weapModel, "ejectpoint", pMatrix);

	if(ok && visual)
		math3dMatrixMultiply(pMatrix, pMatrix, &pObject->animModel->terrainMat);

	return ok;
}

bool gr2AnimHasWeapon(gr2AnimTrooper* model, fpFPSWeaponType type)
{
	if(!model)
		return false;

	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
	{
		if(fpsWeapons[model->weaponArray[i]].weaponType == type)
			return true;
	}

	return false;
}

LOGICAL gr2AnimIsRagdoll(gr2AnimTrooper* model)
{
    return 0;
}

inline void _drawThirdPersonModel(gr2AnimTrooper* model)
{
    if(obdrawIsRenderPass(RENDER_REFLECTION))
    {
        if(model->currentPose == TPS_POSETYPE_SWIM)
        {
            // Do nothing. Avoid underwater anomalies.
            return;
        }
    }

	if(model->selfModel && model->HasFlag(gr2AnimTrooper::USE_SELF_MODEL))
	{
		model->selfModel->Render();
	}
	else
	{
		model->model.Render();
	}

	if(!model->HasFlag(gr2AnimTrooper::HIDE_GUN))
		model->weapons[model->currentWeapon].Render();

	if(model->chuteModel && !model->HasFlag(gr2AnimTrooper::HIDE_CHUTE))
		model->chuteModel->Render();

	model->skeleton->RenderAttachments();
}

bool gr2AnimCullCallback (OBJECT* pObject)
{
	if(!pObject->animModel)
		return true;

	gr2AnimTrooper* model = pObject->animModel;

	if(!model->InRange(GR2ANIM_CULL_DISTANCE))
		return true;

    if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
    {
		if(!model->InRange(GR2ANIM_SHADOW_DISTANCE, true))
			return true;

        if(pObject->animModel)
        {
            gr2AnimTrooper* model = pObject->animModel;

            if(model->myVehicle && model->myVehicle->RideVehicleHostIndex > 0)
            {
                vehVehicle* veh = vehFindVehicle(model->myVehicle->RideVehicleHostIndex);

                if(veh)
                {
                    // Don't shadow if riding on a plane
                    if(veh->VehicleType && veh->VehicleType->Type == VEH_TYPE_AIR)
                    {
                        return true;
                    }
                }
            }
        }
    }

	return false;
}

#if USE_STATESET
bool gr2AnimDrawCallback (OBJECT* pObject)
{
	if (!pObject->animModel || !pObject->animModel->active)
		return (FALSE);

    bool shadowfinal = obdrawIsRenderPass(RENDER_FINAL_SHADOW);
    bool shadowmap = obdrawIsRenderPass(RENDER_SHADOW_MAP);

    LOGICAL	drawTPS = TRUE, retVal = TRUE;
	gr2AnimTrooper*	model = nullptr;

    static SINT32 interiorMaterialID = -1;
    static SINT32 defaultInfantryMaterialID = -1;

    if(defaultInfantryMaterialID == -1)
        defaultInfantryMaterialID = piFindMaterialName ("forest1");

    if(interiorMaterialID == -1)
        interiorMaterialID = piFindMaterialName("interior");

	if(!(pObject->flags & OBJECT_VISIBLE) ||
		pObject->flags & OBJECT_SKIPDRAWING)
	{
		return(TRUE);
	}

	model = pObject->animModel;

    // Object is inside building
    if(pObject->flag3 & OBJECT_IS_INTERIOR)
    {
        oglSetActiveMaterial(interiorMaterialID);
    }
    else
    {
        oglSetActiveMaterial(defaultInfantryMaterialID);
    }

	if (pObject->flags & OBJECT_USEMATRIX)
	{
		// FIXME
		// Since we are using left handed matrices and granny doesn't support them
		// we must kludge our way around that fact
		piMATRIX world = pObject->mat;

		world._31 = -world._31;
        world._32 = -world._32;
        world._33 = -world._33;

		math3dMatrixMultiply(&world, &model->terrainMat, &world);

		oglSetWorldMatrix(&world);
	}

	if (pObject->flag2 & OBJECT_ISFPS && !(shadowfinal || shadowmap))
	{
		// fpFPSDraw needs to be called every frame regardless because that's where the FPS system updates itself
		// check for "no draw FPS cases" .. namely external cam or when we're parachuting
		if (fpFPSIsExternalView() || fpFPSIsTrooperParachuteOpen())
		{
			if (!fpFPSDraw(pObject, FALSE))
				retVal = FALSE;
		}
		else
		{
			drawTPS = FALSE;

			if (!fpFPSDraw(pObject, TRUE))
			{
				retVal = FALSE;
			}

			model->twist.Update(model, false);
		}

#ifdef TESTING_FPS
		drawTPS = FALSE;
#endif
	}

	if (drawTPS)
	{
		if(model->lastDrawTime != frametime)
		{
			// do some updates
			bool resetTwists = true;

#define	ROTATE_TO_MOVE
#ifdef	ROTATE_TO_MOVE
			// force facing to velocity vector .. problems with smoother make this less good than it should be
			// only do this for crouch and stand .. prone leave alone
			if ((model) && (model->myVehicle) && ((model->currentPose == TPS_POSETYPE_STAND) ||
												  (model->currentPose == TPS_POSETYPE_CROUCH)) ||
												  (model->currentPose == TPS_POSETYPE_PRONE))
			{
				if(!model->HasFlag(gr2AnimTrooper::CHUTE_OPEN|gr2AnimTrooper::FREE_FALL))
					resetTwists = false;
			}
#endif

			model->twist.Update(model, resetTwists);

			// determine LOD, then update the appropriate models
			gr2AnimUpdateModel(pObject, pObject->flag2 & OBJECT_ISFPS);

			// update our skeleton
			model->skeleton->Update(false, (pObject->distsqr <= polishNear));

			// we always do this because the shadow pass uses it to render
			//model->collisionModel->Update(false, false);

			if (model->selfModel && model->HasFlag(gr2AnimTrooper::USE_SELF_MODEL))
				model->selfModel->Update(false, false);
			else
				model->model.Update(false, false);

			// update our weapon
			model->weapons[model->currentWeapon].Update(false, false);

			// update our chute if we have one
			if(model->chuteModel && !(model->HasFlag(gr2AnimTrooper::HIDE_CHUTE)))
				model->chuteModel->Update(false, false);

			model->lastDrawTime = frametime;
		}

		//gr2AnimTrooperAdjustments(model, -.01f);
#ifdef BETA_RELEASE
		if((gr2DrawFlags & (1<<0)) && !model->HasFlag(gr2AnimTrooper::DEAD))
			_drawThirdPersonModel(model);

		static bool drawdebug = false;

		if(drawdebug)
		{
			piVECTOR start;
			start.x = 0;
			start.y = 0;
			start.z = 1.0f;

			piVECTOR stop;
			stop.x = 0;
			stop.y = 1.0f;
			stop.z = 1.0f;

			// draw our yaw line
			draw3dLine(&start, &stop, colorBLUEALPHA);
		}

		if(gr2DrawFlags & (1<<4))
			gr2DrawSkeleton(model->skeleton, piRGBA(0, 1, 0, 1));

		if((gr2DrawFlags & ((1<<5)|(1<<6)|(1<<6))) && (pObject->flag2 & OBJECT_ISFPS))
			_drawfpscollider();
#else
		if(!model->HasFlag(gr2AnimTrooper::DEAD))
			_drawThirdPersonModel(model);
#endif
    }

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis)
		if (!gr2RenderDamageModel(model->collisionModel))
			return (FALSE);
#endif // BETA_RELEASE

#ifdef DEBUG_DEPLOY_CRAP
    _fpsdrawline(pObject->mat);
#endif

    oglSetWorldMatrix(&IdentityMatrix);

	return(retVal);
}
#else
bool gr2AnimDrawCallback (OBJECT* pObject)
{
	if (!pObject->animModel || !pObject->animModel->active)
		return (FALSE);

    bool shadowfinal = obdrawIsRenderPass(RENDER_FINAL_SHADOW);
    bool shadowmap = obdrawIsRenderPass(RENDER_SHADOW_MAP);

    LOGICAL	drawTPS = TRUE, retVal = TRUE;
	gr2AnimTrooper*	model = nullptr;

    static SINT32 interiorMaterialID = -1;
    static SINT32 defaultInfantryMaterialID = -1;

    if(defaultInfantryMaterialID == -1)
        defaultInfantryMaterialID = piFindMaterialName ("forest1");

    if(interiorMaterialID == -1)
        interiorMaterialID = piFindMaterialName("interior");

	if (!(pObject->flags & OBJECT_VISIBLE) ||
		pObject->flags & OBJECT_SKIPDRAWING)
	{
		return(TRUE);
	}

	model = pObject->animModel;

    // Object is inside building
    if(pObject->flag3 & OBJECT_IS_INTERIOR)
    {
        piSetActiveMaterial(interiorMaterialID);
    }
    else
    {
        piSetActiveMaterial(defaultInfantryMaterialID);
    }

	if (pObject->flags & OBJECT_USEMATRIX)
	{
		// FIXME
		// Since we are using left handed matrices and granny doesn't support them
		// we must kludge our way around that fact
		piMATRIX world = pObject->mat;

		world._31 = -world._31;
        world._32 = -world._32;
        world._33 = -world._33;

		math3dMatrixMultiply(&world, &model->terrainMat, &world);

		/* scale testing
		if (false && fpFPSIsExternalView())
		{
			static FLOAT32 scale = .01f;
			Vec3f pos = Vec3f(world._41, world._42, world._43);

			Matrixf w = Matrixf(world);

			Matrixf s;
			s.makeScale(scale, scale, scale);

			w = w * s;

			w.get(world);

			world._41 = pos.x();
			world._42 = pos.y();
			world._43 = pos.z() - .9f;
		}
		*/

		piSetWorldMatrix(&world);
		piSetRenderState(piRENDERSTATE_CULLMODE, piCULL_CW);
	}

	if (pObject->flag2 & OBJECT_ISFPS && !(shadowfinal || shadowmap))
	{
		// fpFPSDraw needs to be called every frame regardless because that's where the FPS system updates itself
		// check for "no draw FPS cases" .. namely external cam or when we're parachuting
		if (fpFPSIsExternalView() || fpFPSIsTrooperParachuteOpen())
		{
			if (!fpFPSDraw(pObject, FALSE))
				retVal = FALSE;
		}
		else
		{
			drawTPS = FALSE;

			if (!fpFPSDraw(pObject, TRUE))
			{
				retVal = FALSE;
			}

			model->twist.Update(model, false);
		}
/*
		if (!fpFPSIsExternalView() && !fpFPSIsTrooperParachuteOpen())
		{
			drawTPS = FALSE;

			if (!fpFPSDraw(pObject, TRUE))
			{
				retVal = FALSE;
			}
		}
		else if (!fpFPSDraw(pObject, FALSE))
		{
			retVal = FALSE;
		}
*/
#ifdef TESTING_FPS
		drawTPS = FALSE;
#endif
	}

	if (drawTPS)
	{
		if(model->lastDrawTime != frametime)
		{
			// do some updates
			bool resetTwists = true;

#define	ROTATE_TO_MOVE
#ifdef	ROTATE_TO_MOVE
			// force facing to velocity vector .. problems with smoother make this less good than it should be
			// only do this for crouch and stand .. prone leave alone
			if ((model) && (model->myVehicle) && ((model->currentPose == TPS_POSETYPE_STAND) ||
												  (model->currentPose == TPS_POSETYPE_CROUCH) ||
												  (model->currentPose == TPS_POSETYPE_PRONE)))
			{
				if(!model->HasFlag(gr2AnimTrooper::CHUTE_OPEN|gr2AnimTrooper::FREE_FALL))
					resetTwists = false;
			}
#endif

			model->twist.Update(model, resetTwists);

			// determine LOD, then update the appropriate models
			gr2AnimUpdateModel(pObject, pObject->flag2 & OBJECT_ISFPS);

			// update our skeleton
			model->skeleton->Update(false, (pObject->distsqr <= polishNear));

			// we always do this because the shadow pass uses it to render
			//model->collisionModel->Update(false, false);

			if (model->selfModel && model->HasFlag(gr2AnimTrooper::USE_SELF_MODEL))
				model->selfModel->Update(false, false);
			else
				model->model.Update(false, false);

			// update our weapon
			model->weapons[model->currentWeapon].Update(false, false);

			// update our chute if we have one
			if(model->chuteModel && !(model->HasFlag(gr2AnimTrooper::HIDE_CHUTE)))
				model->chuteModel->Update(false, false);

			model->lastDrawTime = frametime;
		}

		//gr2AnimTrooperAdjustments(model, -.01f);
#ifdef BETA_RELEASE
		if((gr2DrawFlags & (1<<0)) && !model->HasFlag(gr2AnimTrooper::DEAD))
			_drawThirdPersonModel(model);

		static bool drawdebug = false;

		if(drawdebug)
		{
			piVECTOR start;
			start.x = 0;
			start.y = 0;
			start.z = 1.0f;

			piVECTOR stop;
			stop.x = 0;
			stop.y = 1.0f;
			stop.z = 1.0f;

			// draw our yaw line
			draw3dLine(&start, &stop, colorBLUEALPHA);
		}

		if(gr2DrawFlags & (1<<4))
			gr2DrawSkeleton(model->skeleton, piRGBA(0, 1, 0, 1));

		if((gr2DrawFlags & ((1<<5)|(1<<6)|(1<<6))) && (pObject->flag2 & OBJECT_ISFPS))
			_drawfpscollider();
#else
		if(!model->HasFlag(gr2AnimTrooper::DEAD))
			_drawThirdPersonModel(model);
#endif
    }

#ifdef BETA_RELEASE
	if(VehicleObjectAnalysis)
		if (!gr2RenderDamageModel(model->collisionModel))
			return (FALSE);
#endif // BETA_RELEASE

	if (pObject->flags & OBJECT_USEMATRIX)
	{
		piSetRenderState(piRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
        oglSetWorldMatrix(&IdentityMatrix);
	}

#ifdef DEBUG_DEPLOY_CRAP
    _fpsdrawline(pObject->mat);
#endif

	return(retVal);
}
#endif

void gr2AnimTrooperAdjustments(struct gr2AnimTrooper* model, float offset)
{
	if(model->currentPose == TPS_POSETYPE_SWIM)
		return;

	if(model->currentMove == TPS_MOVETYPE_DEPLOYED)
		return;

	if(model->currentPose == TPS_POSETYPE_STAND &&
	  (model->currentMove == TPS_MOVETYPE_FORWARD ||
	   model->currentMove == TPS_MOVETYPE_SPRINT))
		return;

	if(model->currentPose == TPS_POSETYPE_PRONE)// && model->currentMove > TPS_MOVETYPE_IDLE_AIM)
		return;

	// find the foot with the lowest z
	piMATRIX mat;
	FLOAT32 z = 0;

	// check left tow
	if(gr2GetBonePosition(model->skeleton, "Bip01 L Toe0", &mat))
		z = mat._43;

	// check right tow
	if(gr2GetBonePosition(model->skeleton, "Bip01 R Toe0", &mat) && mat._43 < z)
		z = mat._43;

	//uiChatPrintLine("TPS Lowest Point is %.5f", z);

	piVECTOR adjust;
	adjust.x = 0;
	adjust.y = 0;
	adjust.z = (z * -1.0f) + offset;

	gr2SetModelPosition(model->skeleton, &adjust);
}

/*
void gr2AnimTrooperAdjustments(struct gr2AnimTrooper* model, double offset)
{
	// find the foot with the lowest z
	piMATRIX mat;
	double sx, sy, sz;
	sx = sy = sz = 0;

	// check left tow
	if(gr2GetBonePosition(model->skeleton, "Bip01 L Toe0", &mat) && mat._43 < sz)
	{
		sx = mat._41;
		sy = mat._42;
		sz = mat._43;
	}

	// check right tow
	if(gr2GetBonePosition(model->skeleton, "Bip01 R Toe0", &mat) && mat._43 < sz)
	{
		sx = mat._41;
		sy = mat._42;
		sz = mat._43;
	}

	// add it to our start position;
	sx += model->myVehicle->PosData.Pos.x;
	sy += model->myVehicle->PosData.Pos.y;
	sz += model->myVehicle->PosData.Pos.z;

	FLOAT64VECTOR p = model->myVehicle->PosData.Pos;

	if(physicsRayCollide(sx, sy, sz + 1.0f, 0, 0, -1, 3, p))
	{
		FLOAT64VECTOR adjust;
		adjust.x = 0;
		adjust.y = 0;
		adjust.z = 0;

		mathSubVectors(&adjust, &model->myVehicle->PosData.Pos, &p);

		FLOAT64 a = adjust.z;

		static FLOAT64 lastAdjust = 0;

		if(lastAdjust != a)
		{
			lastAdjust = a;

			if(a)
				uiChatPrintLine("TPS Model Adjust is %.5f", (FLOAT32)a);
		}

		piVECTOR newPos;
		newPos.x = 0;
		newPos.y = 0;
		newPos.z = (FLOAT32)adjust.z;

		gr2SetModelPosition(model->skeleton, &newPos);
	}
}
*/


LOGICAL gr2AnimIsDead (OBJECT* pObject, LOGICAL* isDead)
{
	if ((!pObject) || (!pObject->animModel))
	{
		return FALSE;
	}

	*isDead = (pObject->animModel->HasFlag(gr2AnimTrooper::DEAD));

	return (TRUE);
}

SINT32 gr2AnimDeathFlags()
{
	unsigned int flags = 1;
	flags <<= (gr2AnimAltWeapBits + 3); // gr2AnimEmoteBits
	return flags;
}

extern void rnlogoUpdateStatus(const char*, ...);

LOGICAL gr2AnimInit (void)
{
	if(animInited) return TRUE;

#ifdef BETA_RELEASE
#if GR2_MEMORY_PAD
	{
		char buf[256];
		size_t size = sizeof(gr2AnimTrooper);

		if(size % GR2_MEMORY_MULTIPLE > 0)
		{
			snprintf(buf, 255, "gr2AnimTrooper is not byte aligned: size(%ld) %% multiple(%d) = %ld bytes leftover", size, GR2_MEMORY_MULTIPLE, (size % GR2_MEMORY_MULTIPLE));
			piExitProgram(_FL, buf);
		}

#ifdef USE_CORPSES
		size = sizeof(Corpse);

		if(size % GR2_MEMORY_MULTIPLE > 0)
		{
			snprintf(buf, 255, "Corpse is not byte aligned: size(%ld) %% multiple(%d) = %ld bytes leftover", size, GR2_MEMORY_MULTIPLE, (size % GR2_MEMORY_MULTIPLE));
			piExitProgram(_FL, buf);
		}
#endif // USE_CORPSES

	}
#endif //GR2_MEMORY_PAD
#endif //BETA_RELEASE

	rnlogoUpdateStatus("loading.animations.tps");
	fpTPSLoad();
	rnlogoUpdateStatus("loading.animations.fps");
	fpFPSDataInit();

	rnlogoUpdateStatus("loading.animations.grandma");
	gr2Init();

	for (gr2AnimWeapBits = 31; gr2AnimWeapBits >= 0; gr2AnimWeapBits--)
	{
		if ((1<<gr2AnimWeapBits) & (VEH_MAX_MOUNTS-1))
		{
			gr2AnimWeapBits++;
			break;
		}
	}

	for (gr2AnimPoseBits = 31; gr2AnimPoseBits >= 0; gr2AnimPoseBits--)
	{
		if ((1<<gr2AnimPoseBits) & (TPS_POSETYPE_SHARED-1))
		{
			gr2AnimPoseBits++;
			break;
		}
	}

	for (gr2AnimMoveBits = 31; gr2AnimMoveBits >= 0; gr2AnimMoveBits--)
	{
		if ((1<<gr2AnimMoveBits) & (TPS_MOVETYPE_SHARED-1)) // 2 turn movetypes + num types not used
		{
			gr2AnimMoveBits++;
			break;
		}
	}

	for (gr2AnimPointBits = 31; gr2AnimPointBits >= 0; gr2AnimPointBits--)
	{
		if ((1<<gr2AnimPointBits) & (TPS_POINTTYPE_SHARED-1))
		{
			gr2AnimPointBits++;
			break;
		}
	}

	gr2AnimHeadBits = 1;
	gr2AnimDeathBits = 1; //TPS_NUM_DEATH_BITS;

	/*
	for (gr2AnimEmoteBits = 31; gr2AnimEmoteBits >= 0; gr2AnimEmoteBits--)
	{
		// this isn't -1 because we want the 0 state to be "no emote"
		if ((1<<gr2AnimEmoteBits) & (TPS_BODYEMOTE_NUM_TYPES+TPS_COMBATEMOTE_NUM_TYPES))
		{
			gr2AnimEmoteBits++;
			break;
		}
	}
	*/

	//gr2AnimEmoteBits = 1;
	gr2AnimAltWeapBits = 1;
	gr2AnimParachuteBits = 3;

	// walking, parade rest, attack1
	gr2AnimStateBits = 3;

	defaultAnimation = fpTPSGetBlend(TPS_POSETYPE_STAND, TPS_MOVETYPE_IDLE, TPS_WEAPONTYPE_RIFLE, TPS_POINTTYPE_RELAXED);

	/*
	helmet1 = gr2Model::Load("TPS\\meshes\\british_inf\\Kit_UK_Helmet_LOD1", nullptr, nullptr, 1);
	helmet2 = gr2Model::Load("TPS\\meshes\\german_inf\\Kit_DE_Helmet_LOD1", nullptr, nullptr, 1);
	*/

	Write_Log (LOG_FPS, LOG_INFO, "Total bits used = %d",
		gr2AnimWeapBits + gr2AnimPoseBits + gr2AnimMoveBits + gr2AnimPointBits + gr2AnimHeadBits + gr2AnimDeathBits + gr2AnimAltWeapBits + gr2AnimParachuteBits + gr2AnimStateBits); // gr2AnimEmoteBits

	rnlogoUpdateStatus("loading.animations.fx");
    if(!fpFPSfxInit())
    {
        Write_Debug("[gr2AnimInit] fpFPSfxInit FAILED!");
        return FALSE;
    }

	rnlogoUpdateStatus("loading.animations.kits");
	fpTPSKitsInit();

	animInited = TRUE;

#ifdef USE_CORPSES
	/*
     
     TBD - is this really meant to be implemented?
     
    ARENA_VARS::iterator itr = g_arenaDescription.find("dead_body_delay");

    if(itr != g_arenaDescription.end())
    {
        int delay = atol(itr->second.c_str());

        if(false && delay) // force our default for now
        {
            _corpsetime = delay;
        }
    }
	*/
#endif

    return TRUE;
}

void gr2AnimKill (void)
{
	_gr2TrooperPool.Free();

#ifdef USE_CORPSES
	std::set<Corpse*>::iterator it = _corpses.begin();

    for(; it != _corpses.end(); ++it)
		_gr2CorpsePool.Push(*it);

    _corpses.clear();
	_gr2CorpsePool.Free();
#endif

	_gr2RayPool.Free();

	fpFPSKill();
	fpTPSKill();
	fpTPSRagdollKill();
	gr2Kill();
    fpFPSfxKill();
	fpTPSKitsKill();

    animInited = FALSE;

    Write_Log(LOG_OLD, LOG_INFO, "[gr2AnimKill]");
}

void _freemodel(gr2AnimTrooper* model)
{
    gr2FreeModel(model->skeleton);
    model->skeleton = nullptr;

    gr2FreeModel(model->collisionModel);
    model->collisionModel = nullptr;

    if(model->selfModel)
    {
        gr2FreeModel(model->selfModel);
        model->selfModel = nullptr;
    }

    if(model->chuteModel)
    {
        gr2FreeModel(model->chuteModel);
        model->chuteModel = nullptr;
    }


	model->model.Free();

	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
		model->weapons[i].Free();

	/*
	for(int i = 0; i < TPS_NUM_LODS; i++)
	{
		if(model->model[i])
		{
			gr2FreeModel(model->model[i]);
			model->model[i] = nullptr;
		}

        if(model->trooperAltModel[i])
        {
            gr2FreeModel(model->trooperAltModel[i]);
            model->trooperAltModel[i] = nullptr;
        }

		for(int j = 0; j < MAX_NUM_FPS_WEAPONS; j++)
		{
			if(model->weaponModels[j][i])
			{
				gr2FreeModel(model->weaponModels[j][i]);
				model->weaponModels[j][i] = nullptr;
			}

			if(model->weaponAltModels[j][i])
			{
				gr2FreeModel(model->weaponAltModels[j][i]);
				model->weaponAltModels[j][i] = nullptr;
			}
		}
	}
	*/

	model->active = false;

	_gr2TrooperPool.Push(model);
}

bool _idleok(gr2AnimTrooper* model)
{
	if(model->myVehicle->VehicleObjectData->PlaneObject->distsqr <= polishNear)
		return false;

	if(model->emoteID)
		return false;

	//if(model->currentPoint >= TPS_POINTTYPE_IDLE_01 && model->currentPoint <= TPS_POINTTYPE_IDLE_08)
	//	return true;

	if(!model->twist.InLine()) // is our legs, torso and head aligned?
		return false;

	if(frametime <= abs(model->turnInPlace)) // are we turning
		return false;

	SINT32 timediff = frametime - combattime;

	if(timediff <= 120000) // have we had a combat event in the last 2 minutes
	{
		if(timediff > 2000 || GetRandom(0.0f, 1.0f) > .5f)
			return false;
	}

	if(model->HasFlag(gr2AnimTrooper::CHUTE_OPEN|gr2AnimTrooper::FREE_FALL)) // are we falling
		return false;

	// don't play any animations if we're at parade rest; need to redo idle handling to account for regular idle and parade idle
	/*
	if(model->HasFlag(gr2AnimTrooper::PARADE) &&
	   model->currentPose == TPS_POSETYPE_STAND &&
	   model->currentMove == TPS_MOVETYPE_IDLE &&
	   model->currentPoint == TPS_POINTTYPE_RELAXED)
	{
		return false;
	}
	*/

	return true;
}

void _clearTransition(void* data)
{
	if(data)
	{
		gr2AnimTrooper* anim = (gr2AnimTrooper*)data;

		// clear any transition flags
		anim->SetFlag(gr2AnimTrooper::TO_COMPLETE);
	}
}

void _clearAttack(void* data)
{
	if(data)
	{
		gr2AnimTrooper* anim = (gr2AnimTrooper*)data;

		// clear any transition flags
		anim->ClearFlag(gr2AnimTrooper::ATTACKING);
	}
}

FLOAT32 _getTransitionWeight(gr2AnimTrooper* model, gr2AnimationBlend* blend, FLOAT32 balance)
{
	gr2ControlBlend* blendControl = gr2BlendControlFindRunningBlend(model->skeleton, blend);

	if(!blend->numBlends || !blendControl)
		return 0.0f;

	float weight = 0.0f;

	// Find out how long this control plays for, total
    //float duration = (float)GrannyGetControlDuration(blendControl->controls[0]->control);

    // Find out how long this control will continue to play for,
    // accounting for however much time has already elapsed
    //float passed = GrannyGetControlRawLocalClock(blendControl->controls[0]->control);

	float passed   = gr2BlendControlGetTime(model->skeleton, blend);

	float y = blend->mod2;

    float cy = y * blendControl->startWeight;

    float y1 = 0;
    float y2 = cy;
    float y3 = y2;
    float y4 = blendControl->startWeight;

    //float delta = 1.0f / 64.0f;
	// time passed could be many loops for any blend that runs more than one loop
	//SINT32 animLength	= SINT32(duration * 1000);
	//SINT32 animPassed	= passed %

    float t = passed; //float(current) / 1000.0f; // time passed

    weight = (1 - t) * (1 - t) * (1 - t) * y1 +
	          3 * t * (1 - t) * (1 - t) * y2 +
	          3 * t * t * (1 - t) * y3 +
	          t * t * t * y4;

	//uiChatPrintLine("Transition Weight: duration = %.2f, passed = %.2f, weight = %.2f", duration, t, weight);

	weight = blendControl->startWeight - weight;

	if(weight < 0)
		weight = 0;

	return weight;
}

gr2AnimationBlend* _setBlend(BlendSpec& spec, fpTPSBlendTypes type)
{
#ifdef GR2_BLEND_EDITOR
	gr2AnimationBlend* blend = fpTPSGetBlend(spec.pose, spec.move, spec.weapon, spec.point, type);
#else
	// try the weapon specific version 1st
	gr2AnimationBlend* blend = fpTPSGetBlend(spec.pose, spec.move, spec.fpsWeapon, spec.point, TPS_BLENDTYPE_FPS);

	// if no found, try the generic tps weapon type version
	if(!blend)
		blend = fpTPSGetBlend(spec.pose, spec.move, spec.weapon, spec.point);
#endif

	if(blend)
	{
		bool update = false;
		FLOAT32 weight1, weight2, weight3, weight4;
		weight1 = weight2 = weight3 = weight4 = 0.0f;

		UINT32 blendCount = 1;

		if(blend->type == gr2AnimationBlend::ARBITRARY)
			blendCount = blend->numBlends;
		else if(blend->type == gr2AnimationBlend::BASE)
			blendCount = blend->numBlends - 1;

		switch(blendCount)
		{
			case 5: // 4 blend (base + 4 corners)
			case 4: // 4 blend (base + 4 corners)
			{
				if(spec.start ||
				   spec.model->rectLeftRightWeight != spec.model->rectTargetLeftRightWeight ||
				   spec.model->rectTargetLeftRightWeight != spec.model->lastRectTargetLeftRightWeight)
				{
					// value = up/down * left/right value
					// 1,1 is upper right
					weight1 = (spec.model->rectUpDownWeight) * (1.0f - spec.model->rectLeftRightWeight);
					weight2 = (spec.model->rectUpDownWeight) * (spec.model->rectLeftRightWeight);
					weight3 = (1.0f - spec.model->rectUpDownWeight) * (1.0f - spec.model->rectLeftRightWeight);
					weight4 = (1.0f - spec.model->rectUpDownWeight) * (spec.model->rectLeftRightWeight);

					//uiChatPrintLine("45 Blend: w1 = %.2f, w2 = %.2f, w3 = %.2f, w4 = %.2f", weight1, weight2, weight3, weight4);

					// set this once the smoothed value hits the target
					if(spec.model->rectLeftRightWeight == spec.model->rectTargetLeftRightWeight)
						spec.model->lastRectTargetLeftRightWeight = spec.model->rectTargetLeftRightWeight;

					spec.model->lastModelSpeed = spec.model->modelSpeed;
					update = true;
				}
			}
			break;
			case 3: // 3 blend (base + up,mid,down)
			{
				if(spec.start ||
				   spec.model->upDownWeight != spec.model->targetUpDownWeight ||
				   spec.model->targetUpDownWeight != spec.model->lastTargetUpDownWeight ||
				   spec.model->modelSpeed != spec.model->lastModelSpeed)
				{
					// calculate weights
					if (spec.model->upDownWeight >= 0.5f)
					{
						spec.model->currentDownWeight = 0.0f;
						spec.model->currentUpWeight = (2.0f * spec.model->upDownWeight) - 1.0f;  // 2x - 1
						spec.model->currentMiddleWeight = 1.0f - spec.model->currentUpWeight;
					}
					else
					{
						spec.model->currentUpWeight = 0.0f;
						spec.model->currentDownWeight = 1.0f - (2.0f * spec.model->upDownWeight);  // 1 - 2x
						spec.model->currentMiddleWeight = 1.0f - spec.model->currentDownWeight;
					}

					/*
					if (spec.weapon == TPS_WEAPONTYPE_GRENADE)
					{
						spec.model->currentUpWeight *= 99.0;
						spec.model->currentMiddleWeight *= 99.0;
						spec.model->currentDownWeight *= 99.0;
					}
					*/

					weight1 = spec.model->currentUpWeight;
					weight2 = spec.model->currentMiddleWeight;
					weight3 = spec.model->currentDownWeight;

					//uiChatPrintLine("3 Blend: w1 = %.2f, w2 = %.2f, w3 = %.2f", weight1, weight2, weight3);

					if(spec.model->upDownWeight == spec.model->targetUpDownWeight)
						spec.model->lastTargetUpDownWeight = spec.model->targetUpDownWeight;

					spec.model->lastModelSpeed = spec.model->modelSpeed;
					update = true;
				}
			}
			break;
			case 2: // 2 blend (base + up,down)
			{
				if(spec.start ||
				   spec.model->upDownWeight != spec.model->targetUpDownWeight ||
				   spec.model->targetUpDownWeight != spec.model->lastTargetUpDownWeight ||
				   spec.model->modelSpeed != spec.model->lastModelSpeed)
				{
					spec.model->currentUpWeight = spec.model->upDownWeight;
					spec.model->currentDownWeight = 1.0f - spec.model->upDownWeight;

					/*
					if (spec.weapon == TPS_WEAPONTYPE_GRENADE)
					{
						spec.model->currentUpWeight *= 99.0;
						spec.model->currentMiddleWeight *= 99.0;
						spec.model->currentDownWeight *= 99.0;
					}
					*/

					weight1 = spec.model->currentUpWeight;
					weight2 = spec.model->currentDownWeight;

					//uiChatPrintLine("2 Blend: w1 = %.2f, w2 = %.2f", weight1, weight2);

					if(spec.model->upDownWeight == spec.model->targetUpDownWeight)
						spec.model->lastTargetUpDownWeight = spec.model->targetUpDownWeight;

					spec.model->lastModelSpeed = spec.model->modelSpeed;
					update = true;
				}
			}
			break;
			default: // 1 blend (base + mid)
			{
				if(spec.start || spec.model->modelSpeed != spec.model->lastModelSpeed)
				{
					weight1 = 1.0f;
					//uiChatPrintLine("1 Blend: w1 = %.2f", weight1);
					spec.model->lastModelSpeed = spec.model->modelSpeed;
					update = true;
				}
			}
			break;
		}

		if(update)
		{
			if(spec.start)
			{
				gr2AnimStartBlendedAnimation(spec.model, blend, spec.easeIn, spec.easeOut, spec.allowRessurect);

				if(spec.loopCount > 0)
					gr2BlendControlSetLoopCount(spec.model->skeleton, blend, spec.loopCount);

				if(spec.removeWhenDone)
					gr2BlendControlRemoveWhenDone(spec.model->skeleton, blend, spec.onBlendComplete, spec.model);

				if(spec.startOffset)
					gr2BlendControlSetStartOffset(spec.model->skeleton, blend, spec.startOffset);

				if(spec.endOffset)
					gr2BlendControlSetEndOffset(spec.model->skeleton, blend, spec.endOffset);
			}

			if(spec.weightMod != 1.0f)
			{
				weight1 *= spec.weightMod;
				weight2 *= spec.weightMod;
				weight3 *= spec.weightMod;
				weight4 *= spec.weightMod;
			}

			gr2BlendControlSetWeight(spec.model->skeleton, blend, weight1, weight2, weight3, weight4);
			gr2BlendControlSetSpeed(spec.model->skeleton, blend, spec.model->modelSpeed);

			/*
			if(spec.weapon == TPS_WEAPONTYPE_GRENADE)
			{
				gr2BlendControlSetLoopCountForGrenade(spec.model->skeleton, blend, 1);
				gr2BlendControlSetEaseInDuration(spec.model->skeleton, blend, 0.0f);
				gr2BlendControlSetEaseOutDuration(spec.model->skeleton, blend, 1.0f);
			}
			*/

			// set the offsets
			//if(spec.start && spec.model->animationOffsetPercent)
			//	gr2BlendControlSetStartOffset(spec.model->skeleton, blend, spec.model->animationOffsetPercent);
		}
	}
	else
	{
		/*
		blend = defaultAnimation;

		if(spec.start)
			gr2AnimStartBlendedAnimation(spec.model, blend, spec.easeIn, spec.easeOut, spec.allowRessurect);

		gr2BlendControlSetWeight(spec.model->skeleton, blend, 999.0);
		*/

/*
		const char* key = fpTPSCreateBlendKey(spec.pose, spec.move, spec.weapon, spec.point);

		static std::map<std::string,bool> notFoundMap;

		if(notFoundMap.find(key) == notFoundMap.end())
		{
			uiChatPrintLine("Blend not found: %s", key);
			notFoundMap[key] = true;
		}
*/
	}

	return blend;
}

void _addCorpseBlood(OBJECT* o, piVECTOR* offset)
{
	if(!o) return;

	COLLISION c;
	piVECTOR pos;

	pos = o->pos;

	if(false && offset)
		mathAddVectors(&pos, &pos, offset);

	math3dVectorMultiply(&c.oldpos, &pos, &o->mat);
	c.newpos = c.oldpos;

	c.oldpos.z -= 2.0f;
	FASTCOLLIDE list[32];

	unsigned int numcollides = 0;

	if(trncdCollideDownShortList(c, list, numcollides))
	{
		if(numcollides > 1)
			mlUtilitySortCollideList(list, numcollides);

		FXBULLETCREATE decal;
		memset(&decal, 0, sizeof(FXBULLETCREATE));

		decal.vecPos.x = list[0].CollidePnt.x;
		decal.vecPos.y = list[0].CollidePnt.y;
		decal.vecPos.z = list[0].CollidePnt.z;

		mathVectorMultiply64(&decal.vecPos, &decal.vecPos, &list[0].CollideObject->mat);
		decal.vecPos.z = -decal.vecPos.z;

		decal.vecPos.x += WorldOriginOffset.x;
		decal.vecPos.y += WorldOriginOffset.y;
		decal.vecPos.z += WorldOriginOffset.z;

		mathSubVectors(&decal.vecDelta, &c.newpos, &c.oldpos);
		mathReallyFastNormalize(decal.vecDelta);

		decal.fRadius = 0.25f;
		decal.lpAttachedObj = list[0].CollideObject;
		decal.lpTopLevelObj = list[0].TopLevelObject;
		decal.iPolyNum = list[0].PolyNum;
		decal.flags = blt_decal_is_ground_blood;

		fxBltCreate(&decal);
	}
}

void gr2AnimExpireCorpses()
{
    if(_corpses.empty())
        return;

	std::set<Corpse*>::iterator it = _corpses.begin();
	Corpse* dk = nullptr;

	for(; it != _corpses.end(); ++it)
	{
		dk = *it;
		dk->reset();
        _gr2CorpsePool.Push(dk);
	}

	_corpses.clear();
}

#if USE_STATESET
bool gr2AnimDrawCorpses()
{
#ifdef USE_CORPSES
#define MAX_DIST2 40000 // 200 meters

    if(_corpses.empty())
        return false;

	std::vector<std::set<Corpse*>::iterator> freelist;
    std::set<Corpse*>::iterator itr = _corpses.begin();

	// position for vis check
	piVECTOR visPos;

	// defaults to 2 minutes (1 - 10 minutes)
	SINT32 corpsetime = SINT32(cfgPrefsGetFloat("app.corpsetime", .2f) * (CORPSE_TIME - 60000)) + 60000; // min of 1 minute

	// defaults to 20 corpses (1 - 100)
	SINT32 corpsecount = SINT32(cfgPrefsGetFloat("app.corpsecount", .2f) * (CORPSE_COUNT - 1)) + 1; // min of 1 corpse

	// non-expiring count (corpses that don't have a specific expire time set)
	SINT32 necount = 0;

    float fovscale = FieldOfView / 90.0f;

    Corpse* dk = nullptr;
	Corpse* oldest = nullptr;

    for(; itr != _corpses.end(); ++itr)
    {
        dk = *itr;

        piVECTOR pos =
        {
            (float)(dk->pos.x - WorldOriginOffset.x),
            (float)(dk->pos.y - WorldOriginOffset.y),
            (float)(dk->pos.z)
        };

		// setup visual position check
		mathAddVectors(&visPos, &pos, &dk->gr2Offset);

        piVECTOR playPos = {-playpos.x, -playpos.y, -playpos.z};
        // Don't draw over 200 m away.
        piVECTOR diff;
        mathSubVectors(&diff, &visPos, &playPos);

		//piVECTOR diff2;
        //mathSubVectors(&diff2, &pos, &playPos);

        float dist2 = math3dDotProduct(&diff, &diff) * fovscale;

        dk->world._41 = pos.x;
        dk->world._42 = pos.y;
        dk->world._43 = pos.z;

		//float dist = mathExactVectorLength(&diff);

		// some sort of wierd offset here; not sure where it's coming from but it's probably some sort of
		// sign flippage thing combined with the gr2Offset...so I just increased the radius to a relatively large
		// value so they show up; this problem really only shows up when the ragdoll corpse ends up a
		// long way from the original vehicle position
        if(dist2 < MAX_DIST2 && rnvisVectorFOVCheck(&visPos, 100.0f))
        {
			if(dk->material == -1)
				dk->material = piFindMaterialName("forest1");

            piSetWorldMatrix(&dk->world);
            piSetActiveMaterial(dk->material);

            if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                if(!dk->shadow)
                {
                    continue;
                }

                if(math3dFastSqrt(dist2) > _shadowmap.DynamicShadowDistance() * 0.75f)
                {
                    continue;
                }
            }

            if(obdrawIsRenderPass(RENDER_FINAL_SHADOW))
            {
                // Don't want corpses to shadow themselves.
                if(!_shadowmap.SetShadowStuff(pos, SHADOWED_CORPSE))
                {
                    continue;
                }
            }

			// only deform the first frametime
			if(!dk->update)
				dk->model->lastDeformTime = (float)frametime;

			// render our corpse
			dk->model->Render(false, true);

			// only do it on the first draw call
			if(!obdrawIsRenderPass(RENDER_FINAL_SHADOW | RENDER_SHADOW_MAP))
				dk->update = false;

#ifdef _DEBUG
            static int drawline = 0;
            // Better to see for debugging
            if(drawline)
            {
                piVECTOR pt1 = {0, 0, 0};
                piVECTOR pt2 = {0, 0, 50.0f};
                draw3dLine(&pt1, &pt2, colorBLUEALPHA);
            }
#endif
        }

		// make sure and expire the corpse right away if the TO its on changes state
		if(dk->TO && (!dk->TO->InUse || dk->TO->DamageLevel != dk->TOLevel) && dk->expires < 1)
		{
			dk->expires = 0; // default lifetime
			dk->inittime = 1; // make it the oldest
		}

		if(true || Online)
		{
			if(dk->expires > 0 && frametime >= dk->expires)  // if we have expired
				freelist.push_back(itr);
			else if(frametime >= dk->inittime + corpsetime) // we are older than the time limit
				freelist.push_back(itr);
			else if(dk->expires == 0 && (!oldest || dk->inittime < oldest->inittime)) // save the oldest in case we are over the count limit
				oldest = dk;
		}

		if(dk->expires == 0)
			++necount;
    }

	if(!freelist.empty()) // free any completed corpses
	{
		for(unsigned int i = 0; i < freelist.size(); i++)
		{
			dk = *freelist[i];
			dk->reset();

			_gr2CorpsePool.Push(dk);
			_corpses.erase(freelist[i]);
		}
	}
	else if(necount > corpsecount && oldest) // knock the oldest off the list if we're over the count
	{
		oldest->reset();
		_gr2CorpsePool.Push(oldest);
		_corpses.erase(oldest);
	}

    // Restore states
    oglSetWorldMatrix(&IdentityMatrix);

    return true;

#endif // USE_CORPSES
}
#else
bool gr2AnimDrawCorpses()
{
#ifdef USE_CORPSES
#define MAX_DIST2 40000 // 200 meters

    if(_corpses.empty())
        return false;

	std::vector<std::set<Corpse*>::iterator> freelist;
    std::set<Corpse*>::iterator itr = _corpses.begin();

	// position for vis check
	piVECTOR visPos;

    UINT32 cull = piGetEngineRenderState(piRENDERSTATE_CULLMODE);
    piSetRenderState(piRENDERSTATE_CULLMODE, piCULL_CW);

	// defaults to 2 minutes (1 - 10 minutes)
	SINT32 corpsetime = SINT32(cfgPrefsGetFloat("app.corpsetime", .2f) * (CORPSE_TIME - 60000)) + 60000; // min of 1 minute

	// defaults to 20 corpses (1 - 100)
	SINT32 corpsecount = SINT32(cfgPrefsGetFloat("app.corpsecount", .2f) * (CORPSE_COUNT - 1)) + 1; // min of 1 corpse

	// non-expiring count (corpses that don't have a specific expire time set)
	SINT32 necount = 0;

    float fovscale = FieldOfView / 90.0f;

    Corpse* dk = nullptr;
	Corpse* oldest = nullptr;

    for(; itr != _corpses.end(); ++itr)
    {
        dk = *itr;

        piVECTOR pos =
        {
            (float)(dk->pos.x - WorldOriginOffset.x),
            (float)(dk->pos.y - WorldOriginOffset.y),
            (float)(dk->pos.z)
        };

		// setup visual position check
		mathAddVectors(&visPos, &pos, &dk->gr2Offset);

        piVECTOR playPos = {-playpos.x, -playpos.y, -playpos.z};
        // Don't draw over 200 m away.
        piVECTOR diff;
        mathSubVectors(&diff, &visPos, &playPos);

		//piVECTOR diff2;
        //mathSubVectors(&diff2, &pos, &playPos);

        float dist2 = math3dDotProduct(&diff, &diff) * fovscale;

        dk->world._41 = pos.x;
        dk->world._42 = pos.y;
        dk->world._43 = pos.z;

		//float dist = mathExactVectorLength(&diff);

		// some sort of wierd offset here; not sure where it's coming from but it's probably some sort of
		// sign flippage thing combined with the gr2Offset...so I just increased the radius to a relatively large
		// value so they show up; this problem really only shows up when the ragdoll corpse ends up a
		// long way from the original vehicle position
        if(dist2 < MAX_DIST2 && rnvisVectorFOVCheck(&visPos, 100.0f))
        {
			if(dk->material == -1)
				dk->material = piFindMaterialName("forest1");

            piSetWorldMatrix(&dk->world);
            piSetActiveMaterial(dk->material);

            if(obdrawIsRenderPass(RENDER_SHADOW_MAP))
            {
                if(!dk->shadow)
                {
                    continue;
                }

                if(math3dFastSqrt(dist2) > ShadowMap::GetInstance().DynamicShadowDistance() * 0.75f)
                {
                    continue;
                }
            }

            if(obdrawIsRenderPass(RENDER_FINAL_SHADOW))
            {
                // Don't want corpses to shadow themselves.
                if(!ShadowMap::GetInstance().SetShadowStuff(pos, SHADOWED_CORPSE))
                {
                    continue;
                }
            }

			// only deform the first frametime
			if(!dk->update)
				dk->model->lastDeformTime = (float)frametime;

			// render our corpse
			dk->model->Render(false, true);

			// only do it on the first draw call
			if(!obdrawIsRenderPass(RENDER_FINAL_SHADOW | RENDER_SHADOW_MAP))
				dk->update = false;

#ifdef _DEBUG
            static int drawline = 0;
            // Better to see for debugging
            if(drawline)
            {
                piVECTOR pt1 = {0, 0, 0};
                piVECTOR pt2 = {0, 0, 50.0f};
                draw3dLine(&pt1, &pt2, colorBLUEALPHA);
            }
#endif
        }

		// make sure and expire the corpse right away if the TO its on changes state
		if(dk->TO && (!dk->TO->InUse || dk->TO->DamageLevel != dk->TOLevel) && dk->expires < 1)
		{
			dk->expires = 0; // default lifetime
			dk->inittime = 1; // make it the oldest
		}

		if(true || Online)
		{
			if(dk->expires > 0 && frametime >= dk->expires)  // if we have expired
				freelist.push_back(itr);
			else if(frametime >= dk->inittime + corpsetime) // we are older than the time limit
				freelist.push_back(itr);
			else if(dk->expires == 0 && (!oldest || dk->inittime < oldest->inittime)) // save the oldest in case we are over the count limit
				oldest = dk;
		}

		if(dk->expires == 0)
			++necount;
    }

	if(!freelist.empty()) // free any completed corpses
	{
		for(unsigned int i = 0; i < freelist.size(); i++)
		{
			dk = *freelist[i];
			dk->reset();

			_gr2CorpsePool.Push(dk);
			_corpses.erase(freelist[i]);
		}
	}
	else if(necount > corpsecount && oldest) // knock the oldest off the list if we're over the count
	{
		oldest->reset();
		_gr2CorpsePool.Push(oldest);
		_corpses.erase(oldest);
	}

    // Restore states
    oglSetWorldMatrix(&IdentityMatrix);
    piSetRenderState(piRENDERSTATE_CULLMODE, cull);

    return true;

#endif // USE_CORPSES
}
#endif

Corpse* gr2AnimAddCorpse(gr2AnimTrooper* a, const char* offset, bool blood, SINT32 lifeTime, SINT32 material)
{

	OBJECT* o = a->myVehicle->VehicleObjectData->PlaneObject;
	gr2Model* m = a->model.GetActive();

	Corpse* c = gr2AnimAddCorpse(o, m, gr2GetPath(m), offset, blood, lifeTime, material);
	if(!c) return nullptr;

	if(c->material == -1)
		c->material = (a->myVehicle->isInside) ? piFindMaterialName("interior") : piFindMaterialName("forest1");

	// make this corpse have the same face and decals as the incoming model
	gr2ModelPersonalize(c->model, a->countryID, BRANCH_LAND, (SCHAR*)a->myVehicle->Handle, a->myVehicle->GetDecal());

	return c;
}

Corpse* gr2AnimAddCorpse(const char* modelPath, FLOAT64VECTOR& pos, piMATRIX& mat, SINT32 lifeTime, SINT32 material)
{
	gr2Model* model = gr2Model::Load(modelPath, nullptr, nullptr, 1);
	if(!model) return nullptr;

    // Hold off freeing trooper model for immersive effect.
    Corpse* dk = _gr2CorpsePool.Pop();
	if(!dk) return nullptr;

	dk->reset();

	// force an update to get to a good start state
	gr2UpdateModel(model, true, false);

	// setup gr2 model
	dk->skeleton	= model;
	dk->model		= model;

	// update gr2 bone matrices
	granny_skeleton* toSkeleton = gr2GetSkeleton(dk->model);
	granny_world_pose* toWorldPose = gr2GetWorldPose(dk->skeleton);
	int boneCount = gr2GetBoneCount(dk->skeleton);

	// build the model's world pose
	if(GrannyGetWorldPoseComposite4x4Array(toWorldPose) != nullptr)
		GrannyBuildWorldPoseComposites(toSkeleton, 0, boneCount, toWorldPose);

	// setup matrix
    dk->world = mat;
	dk->world._31 = -dk->world._31;
	dk->world._32 = -dk->world._32;
	dk->world._33 = -dk->world._33;

	// setup pos
	dk->pos.x = pos.x;
    dk->pos.y = pos.y;
	dk->pos.z = -pos.z;

	dk->inittime = frametime;
	dk->expires = (lifeTime < 0 ) ? -1 : (lifeTime > 0) ? frametime + lifeTime : 0;
	dk->material = material;

    // Forget weapons
    dk->shadow = false;

    _corpses.insert(dk);

	return dk;
}

Corpse* gr2AnimAddCorpse(OBJECT* o, gr2Model* s, const char* m, const char* offset, bool blood, SINT32 lifeTime, SINT32 material)
{
    // Hold off freeing trooper model for immersive effect.
    Corpse* dk = _gr2CorpsePool.Pop();
	if(!dk) return nullptr;

	dk->reset();

	dk->material = material;

	gr2Model* skeleton = gr2GetWornOnModel(s);

	// setup gr2 model
	dk->skeleton	= gr2Model::Load(gr2GetPath(skeleton), nullptr, nullptr, 1);
	dk->model		= (*m) ? gr2Model::Load(m, dk->skeleton, nullptr, 1) : dk->skeleton;

	// make this model have its own vertex buffer copy
	dk->model->MakeMutable();

	// let our meshes manage a local copy of the vertix data
	/*
	for(int i = 0; i < dk->model->meshes.size(); i++)
	{
		if(!dk->model->meshes.at(i)->data->rigidMesh)
			dk->model->meshes.at(i)->Stamp();
	}
	*/

	// get a good start state
	gr2UpdateModel(dk->skeleton, true, false);

	if(dk->model != dk->skeleton)
		gr2UpdateModel(dk->model, true, false);

	// update gr2 bone matrices
	granny_skeleton* toSkeleton = gr2GetSkeleton(dk->skeleton);

	granny_world_pose* fromWorldPose = gr2GetWorldPose(skeleton);
	granny_world_pose* toWorldPose = gr2GetWorldPose(dk->skeleton);

	granny_matrix_4x4* fromWorldMatrices = GrannyGetWorldPose4x4Array(fromWorldPose);
	granny_matrix_4x4* toWorldMatrices = GrannyGetWorldPose4x4Array(toWorldPose);
	int boneCount = gr2GetBoneCount(skeleton);

	for(int i = 0; i < boneCount; i++)
	{
		granny_real32* fromBone = (float*)fromWorldMatrices[i];
		granny_real32* toBone = (float*)toWorldMatrices[i];

		for(int b = 0; b < 16; b++)
			toBone[b] = fromBone[b];
	}

	// build the model's world pose
	if(GrannyGetWorldPoseComposite4x4Array(toWorldPose) != nullptr)
		GrannyBuildWorldPoseComposites(toSkeleton, 0, boneCount, toWorldPose);

	//gr2SetWorldPose(skeleton, dk->skeleton);

	// setup matrix
    dk->world = o->mat;
	dk->world._31 = -dk->world._31;
	dk->world._32 = -dk->world._32;
	dk->world._33 = -dk->world._33;

	// setup pos
    dk->pos.x = (FLOAT64)o->mat._41 + WorldOriginOffset.x;
    dk->pos.y = (FLOAT64)o->mat._42 + WorldOriginOffset.y;
    dk->pos.z = (FLOAT64)o->mat._43;

	if(blood)
	{
		COLLISION c;

		math3dVectorMultiply(&c.oldpos, &o->pos, &o->mat);
		c.newpos = c.oldpos;

		c.oldpos.z -= 2.0f;
		FASTCOLLIDE list[32];

		unsigned int numcollides = 0;

		if(trncdCollideDownShortList(c, list, numcollides))
		{
			if(numcollides > 1)
				mlUtilitySortCollideList(list, numcollides);

			FXBULLETCREATE decal;
			memset(&decal, 0, sizeof(FXBULLETCREATE));

			decal.vecPos.x = list[0].CollidePnt.x;
			decal.vecPos.y = list[0].CollidePnt.y;
			decal.vecPos.z = list[0].CollidePnt.z;

			mathVectorMultiply64(&decal.vecPos, &decal.vecPos, &list[0].CollideObject->mat);
			decal.vecPos.z = -decal.vecPos.z;

			decal.vecPos.x += WorldOriginOffset.x;
			decal.vecPos.y += WorldOriginOffset.y;
			decal.vecPos.z += WorldOriginOffset.z;

			mathSubVectors(&decal.vecDelta, &c.newpos, &c.oldpos);
			mathReallyFastNormalize(decal.vecDelta);

			decal.fRadius = 0.25f;
			decal.lpAttachedObj = list[0].CollideObject;
			decal.lpTopLevelObj = list[0].TopLevelObject;
			decal.iPolyNum = list[0].PolyNum;
			decal.flags = blt_decal_is_ground_blood;

			fxBltCreate(&decal);
		}
	}

	piMATRIX boneMat;

	// granny model may be some distance from game position
	if(offset && gr2GetBonePosition(dk->skeleton, (SCHAR*)offset, &boneMat))
	{
		dk->gr2Offset.x = boneMat._41;
		dk->gr2Offset.y = boneMat._42;
		dk->gr2Offset.z = -boneMat._43;
	}

	dk->inittime = frametime;
	dk->expires = (lifeTime < 0 ) ? -1 : (lifeTime > 0) ? frametime + lifeTime : 0;

    // martini is a dweeb.
    dk->shadow = o->flag3 & OBJECT_CAST_SHADOW;

    _corpses.insert(dk);

	return dk;
}

Corpse* gr2AnimAddCorpse(gr2PhysicsModel* m, const char* offset, bool blood, SINT32 lifeTime, SINT32 material)
{
	Corpse* dk = gr2AnimAddCorpse(m->GetModelNode(), m->GetModel(), m->GetCorpse(), nullptr, blood, lifeTime, material);

	if(dk)
	{
		dk->world = m->GetRootMatrix();
		dk->world._31 = -dk->world._31;
		dk->world._32 = -dk->world._32;
		dk->world._33 = -dk->world._33;

		dk->pos.x = (FLOAT64)dk->world._41 + WorldOriginOffset.x;
		dk->pos.y = (FLOAT64)dk->world._42 + WorldOriginOffset.y;
		dk->pos.z = (FLOAT64)dk->world._43;

		piMATRIX boneMat;

		// granny model may be some distance from game position
		if(offset && gr2GetBonePosition(dk->skeleton, (SCHAR*)offset, &boneMat))
		{
			dk->gr2Offset.x = boneMat._41;
			dk->gr2Offset.y = boneMat._42;
			dk->gr2Offset.z = -boneMat._43;
		}

		// add some blood splat
		//_addCorpseBlood(m->getModelNode(), &dk->gr2Offset);

		//m->getRootPos().get(dk->pos);
		//dk->pos.z = -dk->pos.z;

		/*
		const Vec3d& root = m->getRootPos();

		dk->pos.x += root.x();
		dk->pos.y += root.y();
		dk->pos.z += root.z();
		*/

		if(dk->material == -1)
			dk->material = m->GetMaterial();

		// lets transfer all attachments from the ragdoll to the corpse
		m->GetSkeleton()->TransferAttachments(dk->skeleton);
	}

	return dk;
}

void gr2AnimCorpseTO(Corpse* c)
{
	if(!c)
		return;

	Vec3d dir	= Vec3d(0, 0, -1.0f);
	Vec3d pos	= Vec3d(c->pos);

	pos.x() += c->gr2Offset.x;
	pos.y() += c->gr2Offset.y;
	pos.z() += c->gr2Offset.z;

	pos.z() = -pos.z();

	// move it to the physical world
	pos = physicsGameToPhysical(pos);

	const colliderQuery::CollisionList& hits = collideRayList(COLLIDER_TRN_BUILDING|COLLIDER_TRN_BRIDGE, 1.0f, pos, dir);

	// save the TO we're on
	if(!hits.empty())
	{
		// save our TO
		c->TO = colliderSpaceData<trnCDTerrainObject>(hits[0].g2);

		// save our damage level
		c->TOLevel = (c->TO) ? c->TO->DamageLevel: 0;
	}
}

extern LOGICAL gr2AnimInitShadow (OBJECT* pObject)
{
#if(1)
	if (true || ShadowSizeSelect == 0) // disable trooper shadows for now
	{
		return(TRUE);
	}
#endif

	OBJECT*	pShadowObject, *pCurrentObject;

	obmgrCreateModelInstance("troopshadow");
	pShadowObject = obmainFindNamedObject("troopshadow");

	if (pShadowObject == nullptr)
	{
		Write_Debug("Shadow object not found for troops");
		// no shadow, no error
		return TRUE;
	}

	pCurrentObject = pObject;
	while (pCurrentObject->child != nullptr)
	{
		pCurrentObject = pCurrentObject->child;
	}

	pCurrentObject->child = pShadowObject;

	//pTopMost->flag2 |= OBJECT_TOPMOST;
	pShadowObject->flag2 &= ~OBJECT_TOPMOST;

    OBJECT* o = pShadowObject;

    while(o->child)
    {
        o = o->child;
    }

    o->flags |= OBJECT_DYNASHADOW;
    o->flag2 |= OBJECT_ANIMATION;

	obmainMoveToNotObjectTree(pShadowObject);

	strcpy(pShadowObject->name, "shadow");
	return TRUE;
}

gr2AnimBitPacker::gr2AnimBitPacker(unsigned int bits)
{
	unpack(bits);
}

gr2AnimBitPacker::gr2AnimBitPacker()
            : walking(0)
            , parade(0)
			, useAltTrooperMesh(0)
			, chuteOpen(0)
			, freeFall(0)
			, altWeaponModel(0)
			, emoteID(0)
			, deathState(0)
			, headright(0)
			, headleft(0)
			, point(0)
			, movement(0)
            , pose(0)
            , weapon(0)
			, attacking1(0)
{
}

unsigned int gr2AnimBitPacker::pack()
{
	unsigned int pFlags = 0;

	pFlags |= weapon;

	pFlags <<= gr2AnimPoseBits;
	pFlags |= pose;

	pFlags <<= gr2AnimMoveBits;
	pFlags |= movement;

	pFlags <<= gr2AnimPointBits;
	pFlags |= point;

	pFlags <<= gr2AnimHeadBits;
	pFlags |= headleft;

	pFlags <<= gr2AnimHeadBits;
	pFlags |= headright;

	pFlags <<= gr2AnimDeathBits;
	pFlags |= deathState;

	//pFlags <<= gr2AnimEmoteBits;
	//pFlags |= emoteID;

	pFlags <<= gr2AnimAltWeapBits;
	pFlags |= altWeaponModel;

	pFlags <<= 1;
	pFlags |= freeFall;

	pFlags <<= 1;
	pFlags |= chuteOpen;

	pFlags <<= 1;
	pFlags |= useAltTrooperMesh;

	pFlags <<= 1;
	pFlags |= walking;

	pFlags <<= 1;
	pFlags |= parade;

	pFlags <<= 1;
	pFlags |= attacking1;

	//pFlags <<= 1;
	//pFlags |= attacking2;

	return pFlags;
}

gr2AnimBitPacker& gr2AnimBitPacker::unpack(unsigned int flags)
{
	//attacking2 = flags & bitFields[1];
	//flags >>= 1;

	attacking1 = flags & bitFields[1];
	flags >>= 1;

	parade = flags & bitFields[1];
	flags >>= 1;

	walking = flags & bitFields[1];
	flags >>= 1;

	useAltTrooperMesh = flags & bitFields[1];
	flags >>= 1;

	chuteOpen = flags & bitFields[1];
	flags >>= 1;

	freeFall = flags & bitFields[1];
	flags >>= 1;

	altWeaponModel = flags & bitFields[gr2AnimAltWeapBits];
	flags >>= gr2AnimAltWeapBits;

	//emoteID = flags & bitFields[gr2AnimEmoteBits];
	//flags >>= gr2AnimEmoteBits;

	deathState = flags & bitFields[gr2AnimDeathBits];
	flags >>= gr2AnimDeathBits;

	headright = flags & bitFields[gr2AnimHeadBits];
	flags >>= gr2AnimHeadBits;

	headleft = flags & bitFields[gr2AnimHeadBits];
	flags >>= gr2AnimHeadBits;

	point = flags & bitFields[gr2AnimPointBits];
	flags >>= gr2AnimPointBits;

	movement = flags & bitFields[gr2AnimMoveBits];
	flags >>= gr2AnimMoveBits;

	pose = flags & bitFields[gr2AnimPoseBits];
	flags >>= gr2AnimPoseBits;

	weapon = flags & bitFields[gr2AnimWeapBits];

	return *this;
}

LOGICAL gr2AnimProcessAnimationFlags(OBJECT* pObject, UINT32 flags)
{
	SINT32	pose		= 0;
	SINT32	move		= 0;
	SINT32	point		= 0;
	SINT32	weapon		= 0;
	SINT32	death		= 0;
#ifdef BETA_RELEASE
	SINT32	emote		= 0;
#endif
	SINT32	altWeapon	= 0;
	LOGICAL altTrooper	= 0;
	LOGICAL	chuteOpen	= 0;
	LOGICAL freeFall	= 0;
	LOGICAL walking		= 0;
	LOGICAL parade		= 0;
	LOGICAL attack1		= 0;
	//LOGICAL attack2		= 0;

	// pmm sanity check
	if ((pObject == nullptr) || (pObject->animModel == nullptr))
	{
		return (FALSE);
	}

	gr2AnimTrooper* anim = pObject->animModel;

	if (gr2AnimStateBits)
	{
		//attack2 = flags & bitFields[1];
		//flags = flags >> 1;

		attack1 = flags & bitFields[1];
		flags = flags >> 1;

		//if(attacking && !pObject->animModel->myVehicle->ModelData)
			//uiChatPrintLine("Received attack!");

		parade = flags & bitFields[1];
		flags = flags >> 1;

		walking = flags & bitFields[1];
		flags = flags >> 1;
	}

	if (gr2AnimParachuteBits)
	{
		altTrooper = flags & bitFields[1];
		flags = flags >> 1;

		chuteOpen = flags & bitFields[1];
		flags = flags >> 1;

		freeFall = flags & bitFields[1];
		flags = flags >> 1;
	}

	// alt weapon model
	if (gr2AnimAltWeapBits)
	{
		altWeapon = flags & bitFields[gr2AnimAltWeapBits];
		flags = flags >> gr2AnimAltWeapBits;
	}
	else
		altWeapon = 0;

	// emote
	//emoteID = flags & bitFields[gr2AnimEmoteBits];
	//flags = flags >> gr2AnimEmoteBits;

	// death
	death = flags & bitFields[gr2AnimDeathBits];
	flags = flags >> gr2AnimDeathBits;

	// head
	anim->headPoint = TPS_HEAD_STRAIGHT;

	// right, then left
	if (flags & bitFields[gr2AnimHeadBits])
	{
		anim->headPoint = TPS_HEAD_RIGHT;
	}
	flags = flags >> gr2AnimHeadBits;

	if (flags & bitFields[gr2AnimHeadBits])
	{
		anim->headPoint = TPS_HEAD_LEFT;
	}
	flags = flags >> gr2AnimHeadBits;

	// point
	point = flags & bitFields[gr2AnimPointBits];
	flags = flags >> gr2AnimPointBits;

	// move
	move = flags & bitFields[gr2AnimMoveBits];
	flags = flags >> gr2AnimMoveBits;

	// pose
	pose = flags & bitFields[gr2AnimPoseBits];
	flags = flags >> gr2AnimPoseBits;

	// weapon
	weapon = flags & bitFields[gr2AnimWeapBits];
//	flags = flags >> gr2AnimPoseBits;

	// the weapon provided is the weapon # in the weapon array .. we need to translate this to a real weapon ID
	if ((weapon < 0) || (weapon > MAX_NUM_FPS_WEAPONS))
	{
		Write_Log (LOG_FPS, LOG_INFO, "Weapon out of bounds! %d", weapon);
		// for live this shouldn't be a CTD .. just set to weapon 0 (weapon 0 will always be a valid weapon)
#ifdef BETA_RELEASE
		piExitProgram (_FL, "Weapon out of bounds in animation flags");
#else
		weapon = 0;
#endif
	}

#ifdef BETA_RELEASE
	{
		SCHAR tempStr[255];

		if (stateTestFlag && !anim->myVehicle->ModelData)
		{
			sprintf (tempStr, "%s, %s, %s, %d, %d, %d, %d", fpTPSPoseTypeStrings[pose], fpTPSMoveTypeStrings[move], fpTPSPointTypeStrings[point], weapon, death, emote, altWeapon);
//			rnmainListText (tempStr);
			Write_Log (LOG_FPS, LOG_INFO, tempStr);
		}
	}
#endif	// BETA_RELEASE

	if(freeFall && anim->InRange(200.0f))
	{
		static Vec3d dir = Vec3d(0, 0, -1.0f);
		static Vec3d start;
		vehVehicle* v = anim->myVehicle;

		start.set((v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos);
		start.z() += .5f; // move it up a little
		start = physicsGameToPhysical(start);

		// update the freefall flag
		freeFall = (collideRay(COLLIDER_TERRAIN_MASK, 2.5f, start, dir) > 0) ? 0 : 1;
	}

	UINT32 animState = 0;

	if(altTrooper)	animState |= gr2AnimTrooper::USE_ALT_MODEL;
	if(altWeapon)	animState |= gr2AnimTrooper::USE_ALT_WEAPON;
	if(chuteOpen)	animState |= gr2AnimTrooper::CHUTE_OPEN;
	if(freeFall)	animState |= gr2AnimTrooper::FREE_FALL;
	if(death)		animState |= gr2AnimTrooper::DEAD;
	if(walking)		animState |= gr2AnimTrooper::WALKING;
	if(parade)		animState |= gr2AnimTrooper::PARADE;
	if(attack1)		animState |= gr2AnimTrooper::ATTACK1;
	//if(attack2)		animState |= gr2AnimTrooper::ATTACK2;

	/*
	if(anim->myVehicle->IsMe())
	{
		if(anim->myVehicle->ModelData->MountIsFiring[0])
			animState |= gr2AnimTrooper::ATTACK1;

		if(anim->myVehicle->ModelData->MountIsFiring[1])
			animState |= gr2AnimTrooper::ATTACK2;
	}
	else
	{
		if(anim->myVehicle->MountIsFiring[0])
			animState |= gr2AnimTrooper::ATTACK1;

		if(anim->myVehicle->MountIsFiring[1])
			animState |= gr2AnimTrooper::ATTACK2;
	}
	*/

	// save our flags
	anim->currentFlags = flags;

	// set our state
	gr2AnimSetModelState(pObject, anim->currentLOD, pose, move, point, weapon, animState);

	return TRUE;
}

void gr2AnimHandleRecoil(gr2AnimTrooper* model)
{
	return;

	piMATRIX fireBone;

	if(gr2AnimGetFirePointMatrixByBone(model->myVehicle->VehicleObjectData->PlaneObject, &fireBone, true))
	{
		SINT32 hand = gr2ModelFindBone(model->skeleton, "Bip01 L Hand");
		if(hand < 0) return;

		Matrixf fireMat = Matrixf(fireBone);
		Vec3f firePos = fireMat.getTrans();

		Matrixf handMat;
		handMat.set(gr2GetBoneTransformByIndex(model->skeleton, hand));

		Vec3f handPos = handMat.getTrans();

		Vec3f diff = handPos - firePos;
		diff.normalize();

		handPos += (diff * .01f);

		// add bone adjustment here
		//model->skeleton->AddBoneAdjust(hand, handPos, 100);

		hand = gr2ModelFindBone(model->skeleton, "Bip01 R Hand");
		if(hand < 0) return;

		handMat.set(gr2GetBoneTransformByIndex(model->skeleton, hand));

		handPos = handMat.getTrans();
		handPos += (diff * .01f);

		model->skeleton->AddBoneAdjust(hand, handPos, 100);
	}
}

void gr2AnimHandleEmotes(gr2AnimTrooper* model)
{
	// check for fullbody emotes
	/*
	if (emoteID)
	{
		if (model->emoteID != emoteID)
		{
			gr2AnimationBlend*	emoteAnim;

			if (fpTPSGetEmoteType(emoteID) == (fpTPSBodyEmoteType)TPS_EMOTETYPE_FULLBODY)
			{
				gr2RemoveAllBlendedAnimations (model->skeleton, -1);

				emoteAnim = fpTPSBodyEmotes[emoteID-TPS_COMBATEMOTE_NUM_TYPES-1];
				if (emoteAnim)
				{
					if (!gr2PlayBlendedAnimation (model->skeleton, emoteAnim))
					{
						Write_Log (LOG_FPS, LOG_INFO, "Failed to play body emote blend %d (%s)!", emoteID-TPS_COMBATEMOTE_NUM_TYPES-1, fpTPSBodyEmoteFiles[emoteID-TPS_COMBATEMOTE_NUM_TYPES-1]);
					}

					gr2BlendControlSetEaseInDuration (model->skeleton, emoteAnim, 0.4f);
					gr2BlendControlSetEaseOutDuration (model->skeleton, emoteAnim, 1.0f);
					gr2BlendControlSetLoopCount (model->skeleton, emoteAnim, 1);
				}

				model->SetFlag(gr2AnimTrooper::HIDE_GUN);
				model->emoteID = emoteID;

				// if we're online, and we're on the same side, play the emote text
//				if (Online)
				if (1)
				{
					vehVehicle*		playerVehicle = persGetPersonaVehiclePtr();
					if (!playerVehicle)
						return TRUE;

					if (!model->myVehicle)
						return TRUE;

					if (!model->myVehicle->Chat_Handle)
						return TRUE;

					if (playerVehicle == model->myVehicle)
						return TRUE;

					// this is really the original enum value of the emote in the FPS system
					SINT32 emoteNum = emoteID-TPS_COMBATEMOTE_NUM_TYPES-1;
					WCHAR			prependStr[255];

					if (playerVehicle->Side == model->myVehicle->Side)
					{
						FLOAT64VECTOR	posDiff;

						mathSubVectors (&posDiff, &(playerVehicle->PosData.RenderPos), &(model->myVehicle->PosData.Pos));

						if (mathExactVectorLength2(&posDiff) <= (tpsTroopData.emoteCombatMaxDistance * tpsTroopData.emoteCombatMaxDistance))
						{
							swprintf (prependStr, 255, L"%ls ", chatGetWideCallsign(model->myVehicle->Chat_Handle));
							if (emoteNum == TPS_BODYEMOTE_DRESSDOWN)
							{
								SCHAR	appendStr[255];

								memset (appendStr, 0, 255);

								if (model->myVehicle->InfantryTarget)
								{
									vehVehicle*	targetVehicle;

									targetVehicle = vehFindVehicle (model->myVehicle->InfantryTarget);

									if (targetVehicle && targetVehicle->Handle && targetVehicle->Handle[0])
									{
										strncpy (appendStr, (char*)targetVehicle->Handle, 255);
									}
								}

								rnmainListEmoteText (IDE_LANG_UI, TPS_BODYEMOTE_OFFSET + emoteNum, colorGREENALPHA, prependStr, appendStr);
							}
							else
							{
								rnmainListEmoteText (IDE_LANG_UI, TPS_BODYEMOTE_OFFSET + emoteNum, colorGREENALPHA, prependStr, nullptr);
							}
						}
					}
				}
				// full body emotes take over, so we're done
				return TRUE;
			}
			else
			{
				// combat emotes are mixins, complete when done, short ease in, short ease out, single loop count
				// first, determine handedness.  then determine pose.  then get the animation pointer, and start it
				fpTPSHandType			handedness;
				fpTPSMixinPoseType		mixinPose;

				handedness = tpsWeapons[tpsWeaponType].emoteHand;
				if ((pose == TPS_POSETYPE_PRONE) || (pose == TPS_POSETYPE_SWIM))
				{
					mixinPose = TPS_MIXINPOSE_PRONE;
				}
				else
				{
					mixinPose = TPS_MIXINPOSE_STAND;
				}

				emoteAnim = fpTPSCombatEmotes[emoteID-1][handedness][mixinPose];

				if (emoteAnim)
				{
					if (!gr2PlayBlendedAnimation (model->skeleton, emoteAnim))
					{
						Write_Log (LOG_FPS, LOG_INFO, "Failed to play combat emote blend %d (%s)!", emoteID-1, fpTPSCombatEmoteFiles[emoteID-1][handedness][mixinPose]);
					}
					gr2BlendControlSetEaseInDuration (model->skeleton, emoteAnim, 1.0f);
					gr2BlendControlSetEaseOutDuration (model->skeleton, emoteAnim, 1.0f);
					gr2BlendControlSetLoopCount (model->skeleton, emoteAnim, 1);
					gr2BlendControlSetMixin (model->skeleton, emoteAnim);
					gr2BlendControlRemoveWhenDone (model->skeleton, emoteAnim);
//					gr2BlendControlSetWeight (model->skeleton, emoteAnim, 10.0);
				}

				model->emoteID = emoteID;

				// if we're online, and we're on the same side, play the emote text
//				if (Online)
				if (1)
				{
					vehVehicle*		playerVehicle = persGetPersonaVehiclePtr();

					if (!playerVehicle)
						return TRUE;

					if (!model->myVehicle)
						return TRUE;

					if (!model->myVehicle->Chat_Handle)
						return TRUE;

					if (playerVehicle == model->myVehicle)
						return TRUE;

					// this is really the original enum value of the emote in the FPS system
					SINT32			emoteNum = emoteID-1;
					WCHAR			prependStr[255];

					if (playerVehicle->Side == model->myVehicle->Side)
					{
						FLOAT64VECTOR	posDiff;

						mathSubVectors (&posDiff, &(playerVehicle->PosData.RenderPos), &(model->myVehicle->PosData.Pos));

						if (mathExactVectorLength2(&posDiff) <= (tpsTroopData.emoteFullBodyMaxDistance * tpsTroopData.emoteFullBodyMaxDistance))
						{
							swprintf (prependStr, 255, L"%ls ", chatGetWideCallsign(model->myVehicle->Chat_Handle));
							rnmainListEmoteText (IDE_LANG_UI, TPS_COMBATEMOTE_OFFSET + emoteNum, colorGREENALPHA, prependStr, nullptr);
						}

						// let us handle other's emotes
						fpFPSProcessTPSEmote(model->myVehicle->Chat_Handle, emoteNum);
					}
				}
			}
		}
	}
	*/
}

void gr2AnimHandleStowing(gr2AnimTrooper* model)
{
	// handle stowed weapons
	/* none for now
	if(model->currentWeapon >= 0 && currentWeapon  >= 0 && model->currentWeapon != currentWeapon)
	{
		const char* mountPoint = "ftmount2";
		UINT32 useModels = 1;
		fpTPSWeaponType weaponType = gr2AnimWeaponNumToTPSType(model, STOWED_WEAPON_POSITION);

		switch (weaponType)
		{
			case TPS_WEAPONTYPE_RIFLE:
				mountPoint = "riflemount";
			break;
			case TPS_WEAPONTYPE_SMG:
				mountPoint = "smgmount";
			break;
			case TPS_WEAPONTYPE_GRENADE:
			case TPS_WEAPONTYPE_KNIFE:
				mountPoint = "beltmount";
			break;
			case TPS_WEAPONTYPE_LTMORTAR:
				useModels = 2; // use alt models
			break;
			default:
				mountPoint = "ftmount2";
				useModels = 1;
			break;
		}

		if(currentWeapon > 0)
		{
			// attach
			if(!model->stowedWeapon)
			{
				GR2_MODEL_LIST weapList	= model->weapons->GetAllModels();
				const char* attach		= STOWED_WEAPON_ATTACH;

				for(int w = 0; w < weapList.size(); ++w)
				{
					gr2Model* weapModel = weapList.at(w);

					if(weapModel->FindBone((SCHAR*)attach))
					{
						weapModel->Detach();
						weapModel->Attach(model->skeleton, (SCHAR*)mountPoint, attach);
					}
				}

				for(int i = 0; i < TPS_NUM_LODS; i++)
				{
					gr2Model* stowedWeapon = (useModels == 2) ? model->weaponAltModels[STOWED_WEAPON_POSITION][i]:
																  model->weaponModels[STOWED_WEAPON_POSITION][i];

					const char* attach = STOWED_WEAPON_ATTACH;

					if(gr2ModelHasBone(stowedWeapon, (SCHAR*)attach))
					{
						model->stowedWeapon = useModels; // if we attach at least one lod, we've stowed
						gr2AttachModel(stowedWeapon,
									   (SCHAR*)attach,
									   gr2GetWornOnModel(model->model[i]),
									   (SCHAR*)mountPoint);
					}
				}
			}
		}
		else
		{
			if(model->stowedWeapon)
			{
				// detach
				for(int i = 0; i < TPS_NUM_LODS; i++)
				{
					gr2Model* stowedWeapon = (model->stowedWeapon == 2) ? model->weaponAltModels[STOWED_WEAPON_POSITION][i]:
																			model->weaponModels[STOWED_WEAPON_POSITION][i];

					// reattach it to the original mount point
					if(stowedWeapon)
					{
						gr2AttachModel(stowedWeapon,
									   tpsWeapons[weaponType].attachBone,
									   gr2GetWornOnModel(model->model[i]),
									   tpsWeapons[weaponType].attachToBone);

						// clear out the offset
						gr2SetModelRotation(stowedWeapon, &IdentityMatrix);
					}
				}

				model->stowedWeapon = 0;
			}
		}
	}
	*/
}

void gr2AnimHandleAttack(gr2AnimTrooper* model, int pose, int move, int point, int weapon)
{
	// see if we need to do a tps attack (fire an ammo object, etc...)
	gr2AnimAttack(model);

	// remember the last time we tried to do either
	model->myVehicle->PosData.LastFireTime = frametime;

	// see if we need to visually attack (throw grenade, place satchel, etc...)
	if(!model->HasFlag(gr2AnimTrooper::ATTACK1|gr2AnimTrooper::ATTACK2))
		return;

	// some of these types have visual attacks but are started from the weapon firing code to keep
	// the real attack and visual attack in sync
	switch(model->currentWeaponType)
	{
		case TPS_WEAPONTYPE_RIFLE:
		case TPS_WEAPONTYPE_SMG:
		case TPS_WEAPONTYPE_PISTOL:
		case TPS_WEAPONTYPE_EMPTY:
		case TPS_WEAPONTYPE_BINOCS:
		case TPS_WEAPONTYPE_LMG:
		case TPS_WEAPONTYPE_ATR:
		case TPS_WEAPONTYPE_MAP:
			return;
        default:
            // everything else we accept
            break;
	}

	// which attack should we do?
	SINT32 attack = (model->HasFlag(gr2AnimTrooper::ATTACK1)) ? TPS_POINTTYPE_ATTACK1 : TPS_POINTTYPE_ATTACK2;

	// do the visual attack
	gr2AnimVisualAttack(model, attack);

	// clear these flags out once we initiate the attack
	model->ClearFlag(gr2AnimTrooper::ATTACK1|gr2AnimTrooper::ATTACK2);
}

void gr2AnimHandleTransitions(gr2AnimTrooper* model, int pose, int move, int point, int weapon)
{
	bool newPose		= (model->currentPose != pose);
	bool newMove		= (model->currentMove != move);
	bool newPoint		= (model->currentPoint != point);

	// movement changes
	if(newMove)
	{
		if(move == TPS_MOVETYPE_FALL)
		{
			// starting fall
			_blendSpec.easeIn			= 1.0f;
			_blendSpec.easeOut			= .25f;
			_blendSpec.removeEaseOut	= .25f;
		}
		else if(model->currentMove == TPS_MOVETYPE_FALL)
		{
			if(newPoint && model->currentPoint == TPS_POINTTYPE_PARA)
			{
				if(model->InRange(10.0f))
					fpFPSfxPlayTPSSingleSound(fpsTrooper.moveSounds[FPS_SOUND_PARACHUTE_FALL], model->myVehicle->VehicleObjectData->PlaneObject, 4.0f, 40.0f);
			}
			else if(model->InRange(7.0f))
			{
				fpFPSfxPlayTPSSingleSound(fpsTrooper.moveSounds[FPS_SOUND_FALL_NORMAL], model->myVehicle->VehicleObjectData->PlaneObject, 4.0f, 30.0f);
			}

			// landing
			_blendSpec.easeIn			= .25f;
			_blendSpec.removeEaseOut	= .25f;
		}
		else if(model->currentMove == TPS_MOVETYPE_STILL && move == TPS_MOVETYPE_IDLE)
		{
			_blendSpec.easeIn			= 1.0f;
			_blendSpec.removeEaseOut	= 1.0f;
		}
		else if(model->currentMove <= TPS_MOVETYPE_IDLE && move > TPS_MOVETYPE_IDLE)
		{
			// we're going somewhere, get to it
			_blendSpec.easeIn			= .25f;
			_blendSpec.removeEaseOut	= .25f;
		}
		else if(model->currentMove > TPS_MOVETYPE_IDLE && move <= TPS_MOVETYPE_IDLE)
		{
			// we're stopping, ease into it
			_blendSpec.easeIn			= .5f;
			_blendSpec.removeEaseOut	= .5f;
		}
		else if(model->currentMove >= TPS_MOVETYPE_IDLE && move >= TPS_MOVETYPE_IDLE)
		{
			// we're already moving; transition to it fast
			_blendSpec.easeIn			= .2f;
			_blendSpec.removeEaseOut	= .2f;
		}
		else if(move == TPS_MOVETYPE_STRAFE_LEFT || TPS_MOVETYPE_STRAFE_RIGHT)
		{
			_blendSpec.easeIn			= .1f;
			_blendSpec.easeOut			= .2f;
			_blendSpec.removeEaseOut	= .2f;
		}
	}

	// pose changes
	if(newPose && pose == TPS_POSETYPE_SIT)
	{
		// sitting jumps into place; no transition
		_blendSpec.easeIn			= 0.0f;
		_blendSpec.easeOut			= 0.5f;
		_blendSpec.removeEaseOut	= 0.0f;
	}

	// point changes
	if(newPoint)
	{
		if(point == TPS_POINTTYPE_AIM || point == TPS_POINTTYPE_READY)
		{
			// aim and get ready fast
			_blendSpec.easeIn			= .2f;
			_blendSpec.removeEaseOut	= .2f;
		}
		else if(point == TPS_POINTTYPE_ATTACK1 || point == TPS_POINTTYPE_ATTACK2)
		{
			// when attacking, get to it
			_blendSpec.easeIn			= .1f;
			_blendSpec.easeOut			= .2f;
			_blendSpec.removeEaseOut	= .2f;
		}
	}

	// special fixups

	/*
	if(pose == TPS_POSETYPE_STAND && point == TPS_POINTTYPE_READY && model->attacking)
	{
		// bring up the weapon fast; needs work
		_blendSpec.easeIn			= tpsTroopData.fireSnapUpTime;
		_blendSpec.removeEaseOut	= tpsTroopData.fireSnapUpTime;
	}
	*/

	// these animation states only have left animations; play them backwards when we're going right
	if(point == TPS_POINTTYPE_AIM && move == TPS_MOVETYPE_STRAFE_RIGHT)
	{
		model->modelSpeed = -1.0f;
	}
	else if(pose == TPS_POSETYPE_CROUCH && move == TPS_MOVETYPE_STRAFE_RIGHT)
	{
		model->modelSpeed = -1.0f;
	}
	else if(pose == TPS_POSETYPE_STAND &&
			move == TPS_MOVETYPE_STRAFE_RIGHT &&
			model->strafe == TPS_MOVETYPE_STRAFE_LEFT &&
			point == TPS_POINTTYPE_READY &&
			fpTPSConvertWeaponClass(model->currentWeaponType) == CLASS_FIREARM)
	{
		model->modelSpeed = -1.0f;
	}
}

// from current (inside object) to new state transition
void gr2AnimHandlePoseChange(gr2AnimTrooper* model, int pose, int move, int point, int weapon)
{
	gr2AnimationBlend* transition = nullptr;

	FLOAT32 weight	= 4.0f;
	FLOAT32 easeIn	= 0.1f;
	FLOAT32 easeOut	= 0.5f;
	FLOAT32 speed	= 1.25f;
	FLOAT32 specIn	= 0.5f;

	// stand to prone
	if(model->currentPose == TPS_POSETYPE_STAND && pose == TPS_POSETYPE_PRONE)
	{
		switch(model->currentMove)
		{
			case TPS_MOVETYPE_STILL:
			{
				transition = fpTPSGetBlend(TPS_POSETYPE_STAND, TPS_MOVETYPE_STILL, weapon, TPS_POINTTYPE_TOPRONE);
			}
			break;
			case TPS_MOVETYPE_WALK:
			case TPS_MOVETYPE_FORWARD:
			{
				transition = fpTPSGetBlend(TPS_POSETYPE_STAND, TPS_MOVETYPE_FORWARD, weapon, TPS_POINTTYPE_TOPRONE);
			}
			break;
			case TPS_MOVETYPE_SPRINT:
			{
				//speed = 1.5f;
				easeOut = 1.0f;
				transition = fpTPSGetBlend(TPS_POSETYPE_STAND, TPS_MOVETYPE_SPRINT, weapon, TPS_POINTTYPE_TOPRONE);
			}
			break;
            default:
                // otherwise keep defaults
                break;
		}
	}
	else if(model->currentPose == TPS_POSETYPE_CROUCH && pose == TPS_POSETYPE_PRONE)
	{
		switch(model->currentMove)
		{
			case TPS_MOVETYPE_STILL:
			{
				transition = fpTPSGetBlend(TPS_POSETYPE_CROUCH, TPS_MOVETYPE_STILL, weapon, TPS_POINTTYPE_TOPRONE);
			}
			break;
			case TPS_MOVETYPE_FORWARD:
			{
				transition = fpTPSGetBlend(TPS_POSETYPE_CROUCH, TPS_MOVETYPE_FORWARD, weapon, TPS_POINTTYPE_TOPRONE);
			}
			break;
            default:
                // otherwise keep default
                break;
		}
	}
	else if(model->currentPose == TPS_POSETYPE_PRONE && pose == TPS_POSETYPE_STAND)
	{
		switch(model->currentMove)
		{
			case TPS_MOVETYPE_STILL:
			case TPS_MOVETYPE_IDLE:
			case TPS_MOVETYPE_DEPLOYED:
			{
				transition = fpTPSGetBlend(TPS_POSETYPE_PRONE, TPS_MOVETYPE_STILL, weapon, TPS_POINTTYPE_TOSTAND);
			}
			break;
			case TPS_MOVETYPE_WALK:
			case TPS_MOVETYPE_FORWARD:
			{
				if(move == TPS_MOVETYPE_FORWARD)
					speed = 2.0f;
				else
					speed = 1.5f;

				transition = fpTPSGetBlend(TPS_POSETYPE_PRONE, TPS_MOVETYPE_FORWARD, weapon, TPS_POINTTYPE_TOSTAND);
			}
			break;
			case TPS_MOVETYPE_SPRINT:
			{
				speed = 2.0f;
				transition = fpTPSGetBlend(TPS_POSETYPE_PRONE, TPS_MOVETYPE_SPRINT, weapon, TPS_POINTTYPE_TOSTAND);
			}
            default:
                // otherwise keep default
                break;
			break;
		}
	}
	else if(model->currentPose == TPS_POSETYPE_PRONE && pose == TPS_POSETYPE_CROUCH)
	{
		switch(model->currentMove)
		{
			case TPS_MOVETYPE_STILL:
			case TPS_MOVETYPE_IDLE:
			case TPS_MOVETYPE_DEPLOYED:
			{
				transition = fpTPSGetBlend(TPS_POSETYPE_PRONE, TPS_MOVETYPE_STILL, weapon, TPS_POINTTYPE_TOCROUCH);
			}
			break;
			case TPS_MOVETYPE_FORWARD:
			{
				speed = 1.5f;
				transition = fpTPSGetBlend(TPS_POSETYPE_PRONE, TPS_MOVETYPE_FORWARD, weapon, TPS_POINTTYPE_TOCROUCH);
			}
			break;
            default:
                // otherwise keep default
                break;
		}
	}

	if(transition)
	{
		gr2AnimStartBlendedAnimation(model, transition, easeIn, easeOut);
		gr2BlendControlSetLoopCount(model->skeleton, transition, 1);
		gr2BlendControlRemoveWhenDone(model->skeleton, transition);
		gr2BlendControlSetWeight(model->skeleton, transition, weight);
		gr2BlendControlSetSpeed (model->skeleton, transition, speed);
		gr2BlendControlSetModifier(model->skeleton, transition, weight, 1.0f);
		//gr2BlendControlSetMixin(model->skeleton, transition);

		_blendSpec.easeIn			= specIn;
		//_blendSpec.removeEaseOut	= 1.0f;
	}

	model->transition = transition;
}

fpTPSWeaponType gr2AnimWeaponNumToTPSType (gr2AnimTrooper* model, SINT32 weaponNum)
{
	if (weaponNum == BINOCS_WEAPON_NUMBER)
		return TPS_WEAPONTYPE_BINOCS;
	else if (weaponNum == MAP_WEAPON_NUMBER)
		return TPS_WEAPONTYPE_MAP;
	else if ((weaponNum < 0) || (weaponNum > (MAX_NUM_FPS_WEAPONS-1)))
		weaponNum = 0;

	/*
	switch (model->weaponArray[weaponNum])
	{
		case BREN:
		case FM29:
			return TPS_WEAPONTYPE_LMG;
		case MG34:
			return TPS_WEAPONTYPE_LMG_MG34;
		// german & french rifle grenades just use the regular rifle library
//		case RG_VB:
//		case RG_G30:
//			return TPS_WEAPONTYPE_RG;
//		case RG_MILLS:
//			return TPS_WEAPONTYPE_RG_MILLS;
		default:
			return fpTPSConvertWeaponTypes(fpsWeapons[model->weaponArray[weaponNum]].weaponType);
	}
	*/

	return fpTPSConvertWeaponTypes(fpsWeapons[model->weaponArray[weaponNum]].weaponType);
}

LOGICAL gr2AnimIsTrooperSitting (OBJECT* pObject)
{
	if ((!pObject) || (!pObject->animModel))
	{
		return FALSE;
	}
	else
	{
		return (pObject->animModel->currentPose == TPS_POSETYPE_SIT);
	}
}

LOGICAL gr2AnimIsTrooperProne (OBJECT* pObject)
{
	if ((!pObject) || (!pObject->animModel))
	{
		return FALSE;
	}
	else
	{
		return (pObject->animModel->currentPose == TPS_POSETYPE_PRONE);
	}
}

void gr2AnimUpdateSurfaceID(gr2AnimTrooper* model)
{
	if (!model || !model->InRange(20.0f) || frametime < model->nextSurfaceCheck)
		return;

	FLOAT64VECTOR offset = {0, 0, 0};

	model->skeleton->surfaceID = gr2GetSurfaceIDUnderfoot(model->myVehicle, &offset);

	if(model->skeleton->surfaceID < 0)
		model->skeleton->surfaceID = TPS_SURFACE_DEFAULT;

	model->nextSurfaceCheck = frametime + 250;
}

#ifdef GM_RELEASE
void gr2AnimForceLOD (FLOAT32 lodValue)
{
	LODForceValue = lodValue;
}
#endif

void gr2AnimUpdateLOD(struct gr2AnimTrooper* model, FLOAT32 distance)
{
	static FLOAT32 binocsAdjust = .2f;
	static FLOAT32 aimAdjust = .5f;

	bool binocs = (bool)fpFPSIsBinocs();
	bool aiming = (bool)fpFPSIsAim();

	// bring the distance in if we're looking through binocs
	if(binocs) distance *= (unsigned int)binocsAdjust;
	else if(aiming) distance *= (unsigned int)aimAdjust;

	// save our calculated distance
	UINT32 realDistance = (UINT32)distance;

	// update our next lod check before doing
	// any modification of the distance below
	// closer means check faster
	if(distance <= 5)
		model->nextLODCheck = frametime + 100;
	else if(distance < 15)
		model->nextLODCheck = frametime + 500;
	else if(distance < 30)
		model->nextLODCheck = frametime + 1000;
	else
		model->nextLODCheck = frametime + 2000;

#ifdef LOD_ANIMATIONS
	// calculate our animation lod erro
	model->skeleton->lodError = AllowedErrorAtUnitDistance * distance;

	static FLOAT32 lodErrorMod = .15f;

	FLOAT32 maxError = std::min(distance * lodErrorMod, AllowedErrorMax);

	if(model->skeleton->lodError > maxError)
		model->skeleton->lodError = maxError;

	// scale it down if we're looking thru binocs or aiming
	if(binocs)
		model->skeleton->lodError *= binocsAdjust;
	else if(aiming)
		model->skeleton->lodError *= aimAdjust;
#endif

	// add in any preferential lod modification for the visual models
	distance *= (unsigned int)(gTrooperLOD * gTrooperMOD);

	// update our trooper model
	model->model.UpdateLODS((UINT32)distance, realDistance);

	// update our weapons
	for(int i = 0; i < MAX_NUM_FPS_WEAPONS; i++)
		model->weapons[i].UpdateLODS((UINT32)distance);

	// now update our attachments
	GR2_MODEL_LIST& attachments = model->skeleton->attachments;
	UINT32 count = attachments.size();

	for(size_t i = 0; i < count; i++)
	{
		gr2Model* attached = attachments[i];
		attached->active = (!attached->maxVis || attached->maxVis >= (UINT32)distance) ? true : false;
	}
}

void gr2AnimAddTwitch (OBJECT* pObject)
{
	//return; // no twitches for now

	fpTPSPoseType	usePose;

	gr2AnimTrooper*	model;

	if (!pObject)
		return;

	model = pObject->animModel;

	if (!model || !model->active || !model->InRange(50.0f))
		return;

	// no twitching while paratrooping
	if (model->HasFlag(gr2AnimTrooper::FREE_FALL|gr2AnimTrooper::CHUTE_OPEN))
	{
		return;
	}

	// update this to handle face-up death poses when they show up
	if (model->HasFlag(gr2AnimTrooper::DEAD))
	{
		return; // no twitches while dying now

		// This is a special case for now, but as we add more face-up death animations
		// this will have to be re-designed to something more feasible to the task.
		// Dglenn.

		/*
		if((model->deathState >= 3) && (model->currentPose == TPS_POSETYPE_STAND))
		{
			usePose = TPS_POSETYPE_FACEUP;
		}
		else usePose = TPS_POSETYPE_PRONE;
		*/
	}
	else
	{
		usePose = model->currentPose;
	}

	if (fpTPSNumTwitches[usePose])
	{
		SINT32					twitchNum;
		gr2AnimationBlend*	twitchBlend;

		/*
		if((model->deathState >= 3) && (model->currentPose == TPS_POSETYPE_STAND))
		{
			twitchNum = (model->deathState -3);
		}
		else
		*/
		twitchNum = rand() % fpTPSNumTwitches[usePose];
		twitchBlend = fpTPSTwitchBlends[usePose][twitchNum];

		if (!gr2PlayBlendedAnimation (model->skeleton, twitchBlend))
		{
			Write_Log (LOG_FPS, LOG_INFO, "Failed to play twitch blend %d %d!", usePose, twitchNum);
		}
		gr2BlendControlSetLoopCount (model->skeleton, twitchBlend, 10);
		gr2BlendControlSetEaseInDuration (model->skeleton, twitchBlend, 1.0f/6.0f);
		gr2BlendControlSetEaseOutDuration (model->skeleton, twitchBlend, 1.0f/6.0f);
		gr2BlendControlRemoveWhenDone (model->skeleton, twitchBlend);
	}

}

// FIXME - this should probably be a structure at some point instead of just a string..
void gr2AnimPersonalize (OBJECT* pObject, char* handle)
{
	gr2AnimTrooper* animModel = nullptr;

	if((!pObject) || (!handle) || (!handle[0]) || (!pObject->animModel))
		return;

	animModel = pObject->animModel;

#ifdef USE_NEW_DECALS
    if(!Online)
    {
		SINT32 cid = animModel->myVehicle->VehicleType->WeaponSpec.country();
#ifdef BETA_RELEASE
		SINT32 did = (SINT32)gPersDecalOverride;
#else
		SINT32 did = animModel->myVehicle->GetDecal();
#endif

        gr2ModelPersonalize(animModel->model, cid, (SINT32)BRANCH_LAND, handle, did);

        return;
    }
#else
    if(!Online)
        return;
#endif

	gr2ModelPersonalize(animModel->model, animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);

	/*
	gr2AnimTrooper*	animModel;
	SINT32			altModelID = 0;

	if ((!pObject) || (!handle) || (!handle[0]) || (!pObject->animModel))
		return;

	animModel = pObject->animModel;

#ifdef USE_NEW_DECALS
    if(!Online)
    {
		SINT32 cid = animModel->myVehicle->VehicleType->WeaponSpec.country();
#ifdef BETA_RELEASE
		SINT32 did = (SINT32)gPersDecalOverride;
#else
		SINT32 did = animModel->myVehicle->GetDecal();
#endif

        gr2ModelPersonalize (animModel->model[TPS_LOD_1], cid, (SINT32)BRANCH_LAND, handle, did);
	    gr2ModelPersonalize (animModel->model[TPS_LOD_2], cid, (SINT32)BRANCH_LAND, handle, did);
	    gr2ModelPersonalize (animModel->model[TPS_LOD_3], cid, (SINT32)BRANCH_LAND, handle, did);

        gr2ModelPersonalize (animModel->trooperAltModel[TPS_LOD_1], cid, (SINT32)BRANCH_LAND, handle, did);
	    gr2ModelPersonalize (animModel->trooperAltModel[TPS_LOD_2], cid, (SINT32)BRANCH_LAND, handle, did);
	    gr2ModelPersonalize (animModel->trooperAltModel[TPS_LOD_3], cid, (SINT32)BRANCH_LAND, handle, did);

        return;
    }
#else
    if(!Online)
        return;
#endif

    // HC meshes use default face
	if ((animModel->countryID >= BRITISH_HC) && (animModel->countryID <= FRENCH_HC))
    {
	    // LOD4 has no face
	    gr2ModelPersonalize (animModel->model[TPS_LOD_1], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	    gr2ModelPersonalize (animModel->model[TPS_LOD_2], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	    gr2ModelPersonalize (animModel->model[TPS_LOD_3], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);

        gr2ModelPersonalize(animModel->trooperAltModel[TPS_LOD_1], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	    gr2ModelPersonalize(animModel->trooperAltModel[TPS_LOD_2], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	    gr2ModelPersonalize(animModel->trooperAltModel[TPS_LOD_3], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
		return;
    }

	if (animModel->countryID == BRITISH_PARA_PACK)
		altModelID = BRITISH_PARA_NOPACK;
	else if (animModel->countryID == GERMAN_PARA_PACK)
		altModelID = GERMAN_PARA_NOPACK;
	else if (animModel->countryID == FRENCH_PARA_PACK)
		altModelID = FRENCH_PARA_NOPACK;

	if (altModelID)
	{
		gr2ModelPersonalize (animModel->model[TPS_LOD_1], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
		gr2ModelPersonalize (animModel->model[TPS_LOD_2], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
		gr2ModelPersonalize (animModel->model[TPS_LOD_3], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);

        gr2ModelPersonalize(animModel->trooperAltModel[TPS_LOD_1], altModelID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
		gr2ModelPersonalize(animModel->trooperAltModel[TPS_LOD_2], altModelID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
        gr2ModelPersonalize(animModel->trooperAltModel[TPS_LOD_3], altModelID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	    return;
    }

	// LOD4 has no face
	gr2ModelPersonalize (animModel->model[TPS_LOD_1], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	gr2ModelPersonalize (animModel->model[TPS_LOD_2], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	gr2ModelPersonalize (animModel->model[TPS_LOD_3], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);

    gr2ModelPersonalize (animModel->trooperAltModel[TPS_LOD_1], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	gr2ModelPersonalize (animModel->trooperAltModel[TPS_LOD_2], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	gr2ModelPersonalize (animModel->trooperAltModel[TPS_LOD_3], animModel->countryID, (SINT32)BRANCH_LAND, handle, animModel->myVehicle->persDecal);
	*/
}

void gr2AnimSetWeaponFireTime (OBJECT* pObject, SINT32 time)
{
	if(pObject)
		pObject->animModel->lastWeaponFireTime = time;
}

#ifdef BETA_RELEASE
void gr2ToggleStateTest (void)
{
	stateTestFlag = !stateTestFlag;
}

void gr2AnimMemStats()
{
	int count = 0;
	int memSize = 0;

	gr2MemStats(count, memSize);

	uiChatPrintLine("Anim Model Pool reports: %d objects, %dk memory used", _gr2TrooperPool.Count(), int(_gr2TrooperPool.MemSize()/1000));
	uiChatPrintLine("Anim Ray Pool reports: %d objects, %dk memory used", _gr2RayPool.Count(), int(_gr2RayPool.MemSize()/1000));

	count += _gr2TrooperPool.Count();
	memSize += _gr2TrooperPool.MemSize();

	count += _gr2RayPool.Count();
	memSize += _gr2RayPool.MemSize();

	uiChatPrintLine("Total Pool: %d objects, %dk memory used", count, int(memSize/1000));
}
#endif  // beta

// new sound function(s)
const char* gr2AnimDetermineMovementSound(fpTPSPoseType pose, fpTPSMoveType move, SINT32 surfaceID, FLOAT32* halfDistance, FLOAT32* maxDistance)
{
	const char* retval = nullptr;

	if (pose == TPS_POSETYPE_SWIM)
	{
		retval = (const char*)tpsTroopData.waterSound;
		*halfDistance = tpsTroopData.waterSoundHalfDistance;
		*maxDistance = tpsTroopData.waterSoundMaxDistance;
	}
	else
	{
		if (surfaceID < 0 || surfaceID >= TPS_MAX_NUM_SURFACES)
			return nullptr;

		if(move == TPS_MOVETYPE_STRAFE_RIGHT || move == TPS_MOVETYPE_STRAFE_LEFT)
			move = TPS_MOVETYPE_FORWARD;

		retval = (const char*)fpTPSSurfaces[surfaceID].fpTPSLoopSounds[pose][move];
		*halfDistance = fpTPSSurfaces[surfaceID].fpTPSFootstepHalfDistance[pose][move];
		*maxDistance = fpTPSSurfaces[surfaceID].fpTPSFootstepMaxDistance[pose][move];
	}

	return retval;
}

void gr2AnimSoundStoppedCallback(void* callbackPtr)
{
    gr2AnimTrooper* model = (gr2AnimTrooper*)callbackPtr;

    if(model == nullptr)
        return;

	model->moveSound.Stop();
}

// used to get alternate weapon ammo types if appropriate
// returns -1 to signal GhostAmmo, otherwise an index into the ammo array in the weapon
SINT32 gr2AnimGetAmmoTypeNum (vehVehicle*vehicle)
{
	if (!vehicle)
		return -1;

	if (!(vehicle->VehicleObjectData))
		return -1;

	if (!(vehicle->VehicleObjectData->PlaneObject))
		return -1;

	if (!(vehicle->VehicleObjectData->PlaneObject->animModel))
		return -1;

	gr2AnimTrooper* model = vehicle->VehicleObjectData->PlaneObject->animModel;

	if(model->currentWeaponType == TPS_WEAPONTYPE_RIFLE)
	{
		if (model->HasFlag(gr2AnimTrooper::USE_ALT_WEAPON))
			return 1;
		else
			return 0;
	}
	else
		return -1;
}

LOGICAL gr2AnimIsVehicleHC (vehVehicle*vehicle)
{
	if (!vehicle)
		return FALSE;

	if (!(vehicle->VehicleObjectData))
		return FALSE;

	if (!(vehicle->VehicleObjectData->PlaneObject))
		return FALSE;

	if (!(vehicle->VehicleObjectData->PlaneObject->animModel))
		return FALSE;

	gr2AnimTrooper* model = vehicle->VehicleObjectData->PlaneObject->animModel;

	if ((model->countryID == BRITISH_HC) || (model->countryID == GERMAN_HC) || (model->countryID == FRENCH_HC))
		return TRUE;

	return FALSE;
}

void gr2DetermineFootstepSoundData (OBJECT* pObject, SINT32 eventID, SINT32	surfID, SCHAR** soundName, FLOAT32* fMin, FLOAT32* fMax)
{
	if (!fMin || !fMax || !soundName)
		return;

	if (surfID >= TPS_MAX_NUM_SURFACES)
		surfID = TPS_SURFACE_DEFAULT;

	gr2AnimTrooper* model = pObject->animModel;

	*soundName = (SCHAR *)"";
	*fMin = 0;
	*fMax = 0;

	if (!model)
		return;

	*fMin = fpTPSSurfaces[surfID].fpTPSFootstepHalfDistance[model->currentPose][model->currentMove];
	*fMax = fpTPSSurfaces[surfID].fpTPSFootstepMaxDistance[model->currentPose][model->currentMove];

	if(model->currentMove == TPS_MOVETYPE_STRAFE_LEFT || model->currentMove == TPS_MOVETYPE_STRAFE_RIGHT)
		*soundName = fpTPSSurfaces[surfID].fpTPSFootstepSounds[model->currentPose][TPS_MOVETYPE_FORWARD][eventID];
	else
		*soundName = fpTPSSurfaces[surfID].fpTPSFootstepSounds[model->currentPose][model->currentMove][eventID];
}

void gr2PlayFootstepSound(OBJECT* pObject, gr2Model* model, SINT32 eventID)
{
	if (!pObject || !model)
		return;

	// don't let it play too often; you get shuffling sounds
	if(pObject->animModel)
	{
		if(frametime < pObject->animModel->moveSound.footOKTime)
			return;

		pObject->animModel->moveSound.footOKTime = frametime + 250;

		// update our surface id
		gr2AnimUpdateSurfaceID(pObject->animModel);
	}

	if(model->surfaceID >= 0)
	{
		SCHAR* soundName = nullptr;
		FLOAT32 fMin = 0;
		FLOAT32 fMax = 0;

		gr2DetermineFootstepSoundData(pObject, eventID, model->surfaceID, &soundName, &fMin, &fMax);

		if(soundName)
		{
			if(pObject->flag2 & OBJECT_ISFPS)
            {
				fpFPSfxPlayNormalSound(soundName, IDE_FP_PRIORITY_MANDATORY, TYPE_FINITE_SOUND, nullptr, pObject);
            }
			else
            {
				SINT32 ID = fpFPSfxPlayTPSSingleSound(soundName, pObject, fMin, fMax);
				if(ID > 0) fxSndSetVolume(ID, 1.5f);
            }
		}

		Write_Log (LOG_FPS, LOG_INFO, "gr2PlayFootstepSound: Event %d, Surface %d, Sound >%s<", eventID, model->surfaceID, soundName);
	}
	else
	{
		Write_Log (LOG_FPS, LOG_INFO, "gr2PlayFootstepSound: No surface underfoot, possible fall");
	}
}

FLOAT32 gr2AnimDetermineBaseOffset(gr2AnimTrooper* model, fpTPSPoseType pose, fpTPSMoveType movement)
{
	static FLOAT32 currentOffset = 0;

	//if((movement == TPS_MOVETYPE_TURN) && (pose == TPS_POSETYPE_STAND || pose == TPS_POSETYPE_CROUCH))
	//	currentOffset = .4f;

	/*
	if(pose == TPS_POSETYPE_STAND)
	{
		currentOffset = gr2BlendControlCompletePercent(model->skeleton, fpTPSAnimLib[model->currentPose][model->currentMove][model->currentWeapon][model->currentPoint]);

		if(model->currentMove == movement)
			return currentOffset;

		int curLen			= 0;
		int curAnchor		= 0;
		int curSpeed		= 0;
		int newLen			= 0;
		int newAnchor		= 0;
		int newSpeed		= 0;

		switch(model->currentMove)
		{
			case TPS_MOVETYPE_WALK:
				curLen		= 30;
				curAnchor	= 2;
				curSpeed	= 1;
			break;
			case TPS_MOVETYPE_FORWARD:
				curLen		= 24;
				curAnchor	= 18;
				curSpeed	= 2;
			break;
			case TPS_MOVETYPE_SPRINT:
				curLen		= 17;
				curAnchor	= 13;
				curSpeed	= 4;
			break;
			default:
				return 0;
			break;
		}

		switch(movement)
		{
			case TPS_MOVETYPE_WALK:
				newLen		= 30;
				newAnchor	= 2;
				newSpeed	= 1;
			break;
			case TPS_MOVETYPE_FORWARD:
				newLen		= 30;
				newAnchor	= 2;
				newSpeed	= 2;
			break;
			case TPS_MOVETYPE_SPRINT:
				newLen		= 30;
				newAnchor	= 2;
				newSpeed	= 4;
			break;
			default:
				return 0;
			break;
		}

		int curFrame = (int)(currentOffset * curLen);
		int curOffset = 0;

		if(curFrame > curAnchor)
			curOffset = curAnchor - curFrame;
		else if(curFrame < curAnchor)
			curOffset = curAnchor - curFrame;

		if(curOffset)
		{
			FLOAT32 curDiff = (FLOAT32)curOffset/curLen;
			newAnchor += (int)(newLen * curDiff);

			if(false && newSpeed > curSpeed)
				newAnchor += (newSpeed - curSpeed);
			else if(false && curSpeed > newSpeed)
				newAnchor -= (curSpeed - newSpeed);

			if(newAnchor > newLen)
				newAnchor -= newLen;
			else if(newAnchor < 0)
				newAnchor += newLen;

			currentOffset = (FLOAT32)newAnchor / newLen;
		}
	}
	*/

	return currentOffset;
}

// needs both the object and the model
// object for data about world positions and stuff
// model because each object has multiple models, and we dont' know which model triggered which event
void gr2AnimHandleEvent(OBJECT* pObject, gr2Model* model, SINT32 eventID)
{
	SINT32 type = gr2GetEventType(model, eventID);

	// if this is a footstep type, move our next non-visible footstep out
	if(pObject && pObject->animModel && type < 2)
		pObject->animModel->moveSound.footNextTime = frametime + 700;

	gr2AnimExecuteEvent(pObject, model, type, gr2GetEventData(model, eventID));
}

void gr2AnimExecuteEvent(OBJECT* pObject, gr2Model* model, SINT32 typeID, const char* data)
{
	Write_Log (LOG_FPS, LOG_INFO, "gr2AnimExecuteEvent: %d", typeID);

	switch(typeID)
	{
		case ANIM_EVENT_LFOOT:
			gr2PlayFootstepSound(pObject, model, ANIM_EVENT_LFOOT);
			break;
		case ANIM_EVENT_RFOOT:
			gr2PlayFootstepSound(pObject, model, ANIM_EVENT_RFOOT);
			break;
		case ANIM_EVENT_HIDE_MESH:
			gr2SetMeshActiveByName(model, data, false);
			break;
		case ANIM_EVENT_SHOW_MESH:
			gr2SetMeshActiveByName(model, data, true);
			break;
		case ANIM_EVENT_HIDE_MODEL:
			gr2SetModelActive(model, false);
			break;
		case ANIM_EVENT_SHOW_MODEL:
			gr2SetModelActive(model, true);
			break;
		default:
			Write_Log (LOG_FPS, LOG_INFO, "gr2AnimExecuteEvent: Unknown event type %d", typeID);
			return;
	}
}

SINT32 gr2GetSurfaceIDUnderfoot(vehVehicle*vehicle, FLOAT64VECTOR* offset)
{
	FLOAT64VECTOR	WorldOriginOffset;
	piVECTOR		V1,V2;
	OBJECT*			pObject;
	SINT32			surfID;

	if (!vehicle || !offset)
		return -1;

	trnGetWorldOriginOffset(&WorldOriginOffset);

	if (vehicle->ModelData)
	{
		V1.x = (FLOAT32)(vehicle->ModelData->VehiclePos.x - WorldOriginOffset.x + offset->x);
		V1.y = (FLOAT32)(vehicle->ModelData->VehiclePos.y - WorldOriginOffset.y + offset->y);
		V1.z = -(FLOAT32)(vehicle->ModelData->VehiclePos.z + offset->z);
	}
	else
	{
		V1.x = (FLOAT32)(vehicle->PosData.RenderPos.x - WorldOriginOffset.x + offset->x);
		V1.y = (FLOAT32)(vehicle->PosData.RenderPos.y - WorldOriginOffset.y + offset->y);
		V1.z = -(FLOAT32)(vehicle->PosData.RenderPos.z - offset->z);
	}

	V2 = V1;
	V2.z += 1.0f;
	V1.z -= 0.25f;

#ifdef BETA_RELEASE_DEBUG_FOOTSTEPS
	FLOAT64VECTOR	pt1, pt2;
	SINT32			id;
	static SINT32	counter;
	SCHAR			buf[255];

	if (++counter > 10)
		counter = 0;

	sprintf (buf, "inffoot_%d", counter);

	math32To64Vec(&pt1, &V1); mathAddVectors(&pt1, &pt1, &WorldOriginOffset);
	math32To64Vec(&pt2, &V2); mathAddVectors(&pt2, &pt2, &WorldOriginOffset);

	id = draw3dSetWorldDebugLine(&pt1, &pt2, buf, colorYELLOWALPHA);
	draw3dSetDebugObjectFlipZ(id,FALSE);
	draw3dSetDebugObjectAlwaysOnTop(id, TRUE);
#endif

    surfID = -1;
    trncdIgnoreTreeCollide(TRUE);

	if (trncdCollideWithVehiclesAndTerrain(&V1,&V2,nullptr,&pObject,nullptr,nullptr,nullptr,FALSE,TRUE, FALSE) == TRUE)
	{
		SINT32	retval;

		retval = mlGetObjectBumpinessAndFriction(nullptr,nullptr,nullptr,nullptr,nullptr,&surfID,pObject);

		if (surfID < 0)
		{
			Write_Log (LOG_FPS, LOG_INFO, "WARNING: Failed to correctly identify surface for footstep, using default");
		}

		if (retval != 0)
		{
			surfID = TPS_SURFACE_DEFAULT;
		}

		if (vehicle->ModelData)
		{
			if (strlen(pObject->texname) == 0)
			{
				if (strlen(pObject->matname) != 0)
					Write_Log (LOG_FPS, LOG_INFO, "Troop foot on material <%s>, surface ID <%d>", pObject->matname, surfID);
			}
			else
			{
				if (pObject->texname)
					Write_Log (LOG_FPS, LOG_INFO, "Troop foot on texture <%s>, surface ID <%d>", pObject->texname, surfID);
				else
					Write_Log (LOG_FPS, LOG_INFO, "Troop foot on Null texture, surface ID <%d>", surfID);
			}
		}

		if (surfID < 0)
			surfID = TPS_SURFACE_DEFAULT;
	}

    trncdIgnoreTreeCollide(FALSE);

    return surfID;

	return -1;
}

SINT32 gr2AnimHitMapLocation(SINT32 boneNum)
{
	return fpHitMap[boneNum];
}

FLOAT64 gr2AnimGetLocationThickness(SINT32 boneNum)
{
	static FLOAT64 PERSON_THICKNESS[4] = {0.0004, 0.0012, 0.001, 0.001};

	return (fpHitMap[boneNum] >= 0 && fpHitMap[boneNum] < FP_HIT_LOCATION_NUM)
				? (PERSON_THICKNESS[fpHitMap[boneNum]]) : 0;
}

bool gr2AnimFire(vehVehicle* Vehicle)
{
	piMATRIX FirePoint;
	piMATRIX Attitude;
	FLOAT64VECTOR FirePosition;
	FLOAT64VECTOR FlashPosition;
	FLOAT64VECTOR vehicleVelocity;

	memset(&vehicleVelocity, 0, sizeof(FLOAT64VECTOR));
	memset(&FirePosition, 0, sizeof(FLOAT64VECTOR));
	memset(&FlashPosition, 0, sizeof(FLOAT64VECTOR));

	FirePoint = IdentityMatrix;
	Attitude = IdentityMatrix;

	gr2AnimGetFirePointMatrix(Vehicle->VehicleObjectData->PlaneObject, &FirePoint);

	FlashPosition.x = FirePoint._41;
	FlashPosition.y = FirePoint._42;
	FlashPosition.z = FirePoint._43;

	Attitude = FirePoint;
	Attitude._41 = 0;
	Attitude._42 = 0;
	Attitude._43 = 0;

	math3dMatrixMultiply(&Attitude, &Attitude, &Vehicle->PosData.AttMat);
	mathVectorMultiply64(&FirePosition, &FlashPosition, &Vehicle->PosData.AttMat);
	mathAddVectors(&FirePosition, &FirePosition, &(Vehicle->PosData.RenderPos));

	if(_isnan(FirePosition.x) && _isnan(FirePosition.y) && _isnan(FirePosition.z))
		return false;

	gr2AnimTrooper* model = Vehicle->GetAnim();
	if(!model) return false;

	UINT32 AmmoFlags = 0;

	AmmoFlags |= WEAP_AMMOFLAG_USE_SHORTCOLLIDELIST;
	AmmoFlags |= WEAP_AMMOFLAG_DONT_SEND_DAMAGE;
	// AmmoFlags |= WEAP_AMMOFLAG_CANTHITPLAYER;
	// AmmoFlags |= WEAP_AMMOFLAG_DONTCHECKVEHICLES;
	AmmoFlags |= WEAP_AMMOFLAG_TPS_WEAPON;

	FLOAT64 FireDelta = (frametime - Vehicle->PosData.LastFireTime) / 1000.0;

	if(Vehicle->PosData.LastFireTime == 0)
		FireDelta = 0.0f;

	FLOAT64VECTOR RecoilForce;
	RecoilForce.x = RecoilForce.y = RecoilForce.z = 0.0;

	weapWeapon& weapon = Vehicle->Weapons[model->currentWeapon][0];

	UINT32 fired = weapFireWeapon(&RecoilForce,
									&weapon,
									&FirePosition,
									&vehicleVelocity,
									&Attitude,
									FireDelta,
									frametime / 1000.0,
									0,
									1,
									AmmoFlags,
									nullptr,
									nullptr, // weapFireWeapon
									nullptr,
									Vehicle);

	if(fired)
	{
        OBJECT* o = Vehicle->VehicleObjectData->PlaneObject;

		// eject a shell
		fpFPSEjectShell(o, &Attitude, FALSE);

		// play a sound
		gr2AnimPlayAttackSound(Vehicle, model->currentWeapon, &FlashPosition);

        if(model->currentWeaponType == TPS_WEAPONTYPE_ROCKET)
        {
			TransformEffect* te = nullptr;
            piMATRIX m = IdentityMatrix;
            gr2AnimGetBoneMatrix(o, &m, "alt_firepoint");

            m._43 = -m._43;
            math3dMatrixMultiply(&m, &m, &o->mat);

			if(model->weaponArray[model->currentWeapon] != PIAT)
			{
				// Default direction along Y axis
				piVECTOR dir;
				dir.set(0, 1, 0);
				math3dMultiplyRotation(&dir, &dir, (piMATRIX*)&m);

				math3dNormalize(&dir);

				te = TransformEffect::AddEffect("bazookabackflash", Vec3f(dir), 0, nullptr);

				if(te)
				{
					te->enableemitters(true);
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

					Matrixd mat(m);

					mat(3, 0) += WorldOriginOffset.x;
					mat(3, 1) += WorldOriginOffset.y;

					te->setmatrix(mat);
				}
			}

			if(model->weaponArray[model->currentWeapon] == PIAT)
			    te = TransformEffect::AddEffect("muzzlesmoke", Vec3f(0, 0, 0), 0, nullptr);
            else
			    te = TransformEffect::AddEffect("bazookafrontflash", Vec3f(0, 0, 0), 0, nullptr);

			if(te)
			{
				te->enableemitters(true);;
				te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

                gr2AnimGetBoneMatrix(o, &m, "firepoint");

                m._43 = -m._43;
                math3dMatrixMultiply(&m, &m, &o->mat);

                Matrixd mat(m);

                mat(3, 0) += WorldOriginOffset.x;
                mat(3, 1) += WorldOriginOffset.y;

				te->setmatrix(mat);
			}
        }

		return true;
	}

	return false;
}

bool gr2AnimVisualAttack(gr2AnimTrooper* model, SINT32 attack)
{
	if(!model || model->currentWeapon < 0 || !model->InRange(50.0f))
		return false;

	if(frametime < model->nextVisualAttack)
		return false;

	SINT32 fpsWeapon = (SINT32)model->weaponArray[model->currentWeapon];

	gr2AnimationBlend* blend = fpTPSGetBlend(model->currentPose, model->currentMove, fpsWeapon, attack, 1);

	if(!blend)
		blend = fpTPSGetBlend(model->currentPose, model->currentMove, model->currentWeaponType, attack);

	if(blend)
	{
		static FLOAT32 easeIn = .1f;
		static FLOAT32 easeOut = .1f;
		static FLOAT32 weight = 1.0f;
		static FLOAT32 speed = 1.0f;

		switch(model->currentWeaponType)
		{
			case TPS_WEAPONTYPE_GRENADE:
			case TPS_WEAPONTYPE_KNIFE:
			case TPS_WEAPONTYPE_LTMORTAR:
			case TPS_WEAPONTYPE_SATCHEL:
			case TPS_WEAPONTYPE_BUILD:
			{
				model->nextVisualAttack = frametime + SINT32((gr2BlendGetLength(blend) * .9f) * 1000.0f);
				model->overrideTime		= model->nextVisualAttack;
				model->overrideWeapon	= model->currentWeapon;
				model->overridePoint		= attack;
			}
			break;
			default:
			{
				gr2AnimStartBlendedAnimation(model, blend, easeIn, easeOut);
				gr2BlendControlSetLoopCount(model->skeleton, blend, 1);
				gr2BlendControlRemoveWhenDone(model->skeleton, blend);
				gr2BlendControlSetWeight(model->skeleton, blend, weight);
				gr2BlendControlSetSpeed(model->skeleton, blend, speed);
				gr2BlendControlSetMixin(model->skeleton, blend);
			}
			break;
		}

		model->lastVisualAttack = frametime;
	}

	return true;
}

bool gr2AnimAttack(gr2AnimTrooper* model)
{
	if(!model || model->currentWeapon < 0)
		return false;

	vehVehicle* Vehicle = model->myVehicle;

	if(Vehicle->IsMe() || Vehicle->VehicleType->Type != VEH_TYPE_PERSON)
		return false;

	if(!Vehicle->VehicleObjectData || !Vehicle->VehicleObjectData->PlaneObject)
		return false;

	SINT32 mount = model->currentWeapon;

	if(mount != Vehicle->LastFPSWeaponNum)
		gr2AnimStopAttacking(model, Vehicle->LastFPSWeaponNum);

	Vehicle->LastFPSWeaponNum = mount;

	switch(model->currentWeaponType)
	{
		case TPS_WEAPONTYPE_GRENADE:
		case TPS_WEAPONTYPE_EMPTY:
		case TPS_WEAPONTYPE_BINOCS:
		case TPS_WEAPONTYPE_KNIFE:
		case TPS_WEAPONTYPE_MAP:
		case TPS_WEAPONTYPE_BUILD:
			return false;
		break;
        default:
            // everything else is allowed
            break;
	}

	if(!Vehicle->IsFiring(0))
	{
		if(model->attacking)
			gr2AnimStopAttacking(model);

		return false;
	}

	if(gr2AnimFire(Vehicle))
	{
		if(gr2AnimGetAttack(model) == ATTACK_AUTOMATIC)
			model->attacking = 1;
		else
			gr2AnimStopAttacking(model);
	}

	model->readyUntil = frametime + 3000;

	return false;
}

void gr2AnimPlayAttackSound(vehVehicle* Vehicle, SINT32 mount, FLOAT64VECTOR* pos)
{
	if (Vehicle->Weapons[mount][0].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT)
	{
		if (Vehicle->StateAnimation.MountSoundPlaying[mount] == FALSE &&
			Vehicle->StateAnimation.MountSoundId[mount] == 0)
		{
			SINT32 iID = fxSndGetNewSound();

			if(iID > 0)
			{
				FLOAT32 fMin = 25.0f;
				FLOAT32 fMax = 1609.0f;

				fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->MountSounds[mount]);
				fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
				fxSndSetSoundType(iID, TYPE_INFINITE_EXTERNAL_SOUND);
				fxSndSetPitch(iID, 1.0f);
				fxSndSetVolume(iID, 1.0f);
				fxSndSetPosition(iID, pos->x, pos->y, pos->z);
				fxSndSetEmitterObject(iID, Vehicle->VehicleObjectData->PlaneObject);
				fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
				fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
				fxSndSetMinDistance(iID, fMin);
				fxSndSetMaxDistance(iID, fMax);

				if(fxSndInitSound(iID))
				{
					Vehicle->StateAnimation.MountSoundPlaying[mount] = TRUE;
					Vehicle->StateAnimation.MountSoundId[mount] = iID;
				}
			}
		}
	}
	else
	{
		SINT32 iID = fxSndGetNewSound();

		if(iID > 0)
		{
			FLOAT32 fMin = 25.0f;
			FLOAT32 fMax = 1609.0f;

			fxSndSetSoundName(iID, (SCHAR*)Vehicle->VehicleType->MountSounds[mount]);
			fxSndSetPriority(iID, IDE_TP_PRIORITY_HIGH);
			fxSndSetSoundType(iID, TYPE_FINITE_EXTERNAL_SOUND);
			fxSndSetPitch(iID, 1.0f);
			fxSndSetVolume(iID, 1.0f);
			fxSndSetPosition(iID, pos->x, pos->y, pos->z);
			fxSndSetEmitterObject(iID, Vehicle->VehicleObjectData->PlaneObject);
			fxSndSetPlayType(iID, DEF_PLAYTYPE_OBJECT);
			fxSndPushFlags(iID, DEF_SOUND_STOPS_AMBIENCE);
			fxSndSetMinDistance(iID, fMin);
			fxSndSetMaxDistance(iID, fMax);

			fxSndInitSound(iID);
		}
	}
}

void gr2AnimStopAttackSound(vehVehicle* Vehicle, SINT32 mount)
{
    if(!Vehicle || mount < 0)
        return;

	// check for and shut down weapon sound
	if (mount >= 0 && Vehicle->Weapons[mount][0].WeaponType != nullptr)
	{
		if (Vehicle->Weapons[mount][0].WeaponType->FireType != WEAP_FIRETYPE_SINGLESHOT)
		{
			if (Vehicle->StateAnimation.MountSoundPlaying[mount] == TRUE ||
				Vehicle->StateAnimation.MountSoundId[mount] != 0)
			{
				fxSndStopSoundDelayedFromStart(Vehicle->StateAnimation.MountSoundId[mount]);
				Vehicle->StateAnimation.MountSoundPlaying[mount] = FALSE;
				Vehicle->StateAnimation.MountSoundId[mount] = 0;
			}
		}
	}
}

void gr2AnimStopAttacking(gr2AnimTrooper* model, SINT32 mount)
{
	if(!model)
		return;

	model->attacking		= 0;
	model->attackUntil	= 0;

	if(mount == -1)
		mount = model->currentWeapon;

	// kill our mount flags
	if(!model->myVehicle->IsMe())
	{
		model->myVehicle->MountIsFiring[0] = FALSE;
		model->myVehicle->MountIsFiring[1] = FALSE;
	}

	gr2AnimStopAttackSound(model->myVehicle, mount);
}

fpTPSAttackType gr2AnimGetAttack(gr2AnimTrooper* model)
{
	if(!model || model->HasFlag(gr2AnimTrooper::DEAD))
		return ATTACK_NONE;

	//if(!model->attacking || frametime < model->nextAttack)
	//	return ATTACK_NONE;

	return fpTPSGetAttack(model->currentWeaponType);
}

// for a given gr2AnimTrooper*, determine based on it's state what the footspeed should be
// used to determine how much the animation needs to be sped up/slowed down
// note:  the hardcoded values in here come from art data
FLOAT32 gr2AnimDetermineAnimationFootspeed (gr2AnimTrooper* inmodel, fpTPSWeaponType tpsWeaponType)
{
	FLOAT32 retval = 0.0f;
	gr2AnimTrooper* model = inmodel;

	if (!model)
		return 0.0f;

 	switch (model->currentPose)
	{
		case TPS_POSETYPE_STAND:
			switch (model->currentMove)
			{
				case TPS_MOVETYPE_FORWARD:
					retval = 3.60f;
					break;
				//case TPS_MOVETYPE_FORWARD_AIM:
				case TPS_MOVETYPE_WALK:
					retval = 1.1f;
					break;
				case TPS_MOVETYPE_STRAFE_LEFT:
				case TPS_MOVETYPE_STRAFE_RIGHT:
					{
						if(model->currentPoint == TPS_POINTTYPE_AIM)
							retval = .75f;
						else
							retval = 2.4f;
					}
					break;
				case TPS_MOVETYPE_BACKWARD:
					retval = 1.40f;
					break;
				case TPS_MOVETYPE_SPRINT:
					retval = 6.92f;
					break;
				default:
					break;
			}
			break;
		case TPS_POSETYPE_CROUCH:
			switch (model->currentMove)
			{
				case TPS_MOVETYPE_FORWARD:
					if ((tpsWeaponType == TPS_WEAPONTYPE_LMG) || (tpsWeaponType == TPS_WEAPONTYPE_ATR) || (tpsWeaponType == TPS_WEAPONTYPE_LTMORTAR)) // (tpsWeaponType == TPS_WEAPONTYPE_LMG_MG34) ||
						retval = 1.635f;
					else
						retval = 1.635f * .85f;
					break;
				case TPS_MOVETYPE_STRAFE_LEFT:
				case TPS_MOVETYPE_STRAFE_RIGHT:
					{
						if(model->currentPoint == TPS_POINTTYPE_AIM)
							retval = .7f;
						else
							retval = .9f;
					}
					break;
				case TPS_MOVETYPE_BACKWARD:
					retval = 1.07f;
					break;
				/*
				case TPS_MOVETYPE_FORWARD_AIM:
					retval = 1.635f;
					break;
				*/
				case TPS_MOVETYPE_SPRINT:
					retval = 6.92f * 0.5f;
					break;
				default:
					break;
			}
			break;
		case TPS_POSETYPE_PRONE:
			switch (model->currentMove)
			{
				case TPS_MOVETYPE_FORWARD:
				case TPS_MOVETYPE_BACKWARD:
					retval = 0.55f;
					break;
				default:
					break;
			}
			break;
		case TPS_POSETYPE_SWIM:
			break;
		default:
			break;
	}

	return retval;
}

// based on the current motion type and the angle we're presented with
// determine what the new motion type is
// angle (in degrees) is clamped from -180 to +180 .. with 0 being straight forward
//
// note, all motions are given 5 degrees of "slack"
//
// base angles are 60 and 120
// in game numbers seem to be 59 and 121 for some reason .. so using 55 and 125 for slack
// between 60 and 120 is a right strafe, -60 <-> -120 is a left strafe
// between -60 and +60 forward, > 120 or < -120 is backwards

#define DMT_FORWARD 85.0f // 55.0f
#define DMT_BACKWARD 95.0f // 125.0f

void gr2AnimDetermineMotionType(gr2AnimTrooper* model, FLOAT64 angle)
{
#ifdef DEBUG_MOTION
	fpTPSMotionType startMotion = model->currentMotion;
#endif
    
	if(angle > DMT_FORWARD && angle < DMT_BACKWARD)
		model->strafe = TPS_MOVETYPE_STRAFE_RIGHT;
	else if(angle < -DMT_FORWARD && angle > -DMT_BACKWARD)
		model->strafe = TPS_MOVETYPE_STRAFE_LEFT;
	else
		model->strafe = 0;

	if (model->currentMotion == TPS_MOTION_F) // forward
	{
		if ((angle > DMT_BACKWARD) || (angle < -DMT_BACKWARD)) // backward
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else if (angle > DMT_FORWARD) // strafe right
		{
			model->currentMotion = TPS_MOTION_RFF;
		}
		else if (angle < -DMT_FORWARD) // strafe left
		{
			model->currentMotion = TPS_MOTION_LFF;
		}
		else // still forward
		{
			model->currentMotion = TPS_MOTION_F;
		}
	}
	else if (model->currentMotion == TPS_MOTION_RFF) // strafe right
	{
		if ((angle > DMT_BACKWARD) || (angle < -DMT_BACKWARD)) // backward
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else if (angle > DMT_FORWARD) // strafe right
		{
			model->currentMotion = TPS_MOTION_RFF;
		}
		else if (angle > -DMT_FORWARD) // strafe left
		{
			model->currentMotion = TPS_MOTION_F; // forward
		}
		else
		{
			model->currentMotion = TPS_MOTION_LFB; // strafe left
		}
	}
	else if (model->currentMotion == TPS_MOTION_RFB)
	{
		if ((angle > DMT_BACKWARD) || (angle < -DMT_BACKWARD))
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else if (angle > DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_RFB;
		}
		else if (angle > -DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_F;
		}
		else
		{
			model->currentMotion = TPS_MOTION_LFF;
		}
	}
	else if (model->currentMotion == TPS_MOTION_LFF)
	{
		if ((angle < -DMT_BACKWARD) || (angle > DMT_BACKWARD))
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else if (angle < -DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_LFF;
		}
		else if (angle < DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_F;
		}
		else
		{
			model->currentMotion = TPS_MOTION_RFB;
		}
	}
	else if (model->currentMotion == TPS_MOTION_LFB)
	{
		if ((angle < -DMT_BACKWARD) || (angle > DMT_BACKWARD))
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else if (angle < -DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_LFB;
		}
		else if (angle < DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_F;
		}
		else
		{
			model->currentMotion = TPS_MOTION_RFF;
		}
	}
	else if (model->currentMotion == TPS_MOTION_B)
	{
		if ((angle > DMT_BACKWARD) || (angle < -DMT_BACKWARD))
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else if (angle > DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_RFB;
		}
		else if (angle < -DMT_FORWARD)
		{
			model->currentMotion = TPS_MOTION_LFB;
		}
		else
		{
			model->currentMotion = TPS_MOTION_F;
		}
	}
//#define DEBUG_MOTION
#ifdef DEBUG_MOTION
	if(!model->myVehicle->ModelData && startMotion != model->currentMotion)
	{
		switch (model->currentMotion)
		{
			case TPS_MOTION_F:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_F .. angle = %lf", angle);
				uiChatPrintLine("Motion = TPS_MOTION_F .. angle = %lf", angle);
				break;
			case TPS_MOTION_RFF:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_RFF .. angle = %lf", angle);
				uiChatPrintLine("Motion = TPS_MOTION_RFF .. angle = %lf", angle);
				break;
			case TPS_MOTION_RFB:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_RFB .. angle = %lf", angle);
				uiChatPrintLine("Motion = TPS_MOTION_RFB .. angle = %lf", angle);
				break;
			case TPS_MOTION_LFF:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_LFF .. angle = %lf", angle);
				uiChatPrintLine("Motion = TPS_MOTION_LFF .. angle = %lf", angle);
				break;
			case TPS_MOTION_LFB:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_LFB .. angle = %lf", angle);
				uiChatPrintLine("Motion = TPS_MOTION_LFB .. angle = %lf", angle);
				break;
			case TPS_MOTION_B:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_B .. angle = %lf", angle);
				uiChatPrintLine("Motion = TPS_MOTION_B .. angle = %lf", angle);
				break;
		}
	}
#endif
}

/*
void gr2AnimDetermineMotionType3 (gr2AnimTrooper* model, FLOAT64 angle)
{
	if(angle > 55 && angle < 125)
		model->strafe = 1;
	else if(angle < -55 && angle > -125)
		model->strafe = -1;
	else
		model->strafe = 0;

	if (model->currentMotion == TPS_MOTION_F)
	{
		if ((angle > 115.0) || (angle < -115.0))
			model->currentMotion = TPS_MOTION_B;
		else if (angle > 65.0)
			model->currentMotion = TPS_MOTION_RFF;
		else if (angle < -65.0)
			model->currentMotion = TPS_MOTION_LFF;
		else
			model->currentMotion = TPS_MOTION_F;
	}
	else if (model->currentMotion == TPS_MOTION_RFF)
	{
		if ((angle > 115.0) || (angle < -115.0))
			model->currentMotion = TPS_MOTION_B;
		else if (angle > 65.0)
			model->currentMotion = TPS_MOTION_RFF;
		else if (angle > -65.0)
			model->currentMotion = TPS_MOTION_F;
		else
			model->currentMotion = TPS_MOTION_LFB;
	}
	else if (model->currentMotion == TPS_MOTION_RFB)
	{
		if ((angle > 115.0) || (angle < -115.0))
			model->currentMotion = TPS_MOTION_B;
		else if (angle > 65.0)
		{
			if (!gr2AnimGunPointStraight(model))
				model->currentMotion = TPS_MOTION_RFB;
			else
				model->currentMotion = TPS_MOTION_RFF;
		}
		else if (angle > -65.0)
			model->currentMotion = TPS_MOTION_F;
		else
			model->currentMotion = TPS_MOTION_LFF;
	}
	else if (model->currentMotion == TPS_MOTION_LFF)
	{
		if ((angle < -115.0) || (angle > 115.0))
			model->currentMotion = TPS_MOTION_B;
		else if (angle < -65.0)
		{
			if (!gr2AnimGunPointStraight(model))
				model->currentMotion = TPS_MOTION_LFF;
			else
				model->currentMotion = TPS_MOTION_LFB;
		}
		else if (angle < 65.0)
			model->currentMotion = TPS_MOTION_F;
		else
			model->currentMotion = TPS_MOTION_RFB;
	}
	else if (model->currentMotion == TPS_MOTION_LFB)
	{
		if ((angle < -115.0) || (angle > 115.0))
			model->currentMotion = TPS_MOTION_B;
		else if (angle < -65.0)
			model->currentMotion = TPS_MOTION_LFB;
		else if (angle < 65.0)
			model->currentMotion = TPS_MOTION_F;
		else
			model->currentMotion = TPS_MOTION_RFF;
	}
	else if (model->currentMotion == TPS_MOTION_B)
	{
		if ((angle > 115.0) || (angle < -115.0))
			model->currentMotion = TPS_MOTION_B;
		else if (angle > 65.0)
			model->currentMotion = TPS_MOTION_RFB;
		else if (angle < -65.0)
			model->currentMotion = TPS_MOTION_LFB;
		else
			model->currentMotion = TPS_MOTION_F;
	}

#define DEBUG_MOTION
#ifdef DEBUG_MOTION
		switch (model->currentMotion)
		{
			case TPS_MOTION_F:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_F .. angle = %lf", angle);
				break;
			case TPS_MOTION_RFF:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_RFF .. angle = %lf", angle);
				break;
			case TPS_MOTION_RFB:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_RFB .. angle = %lf", angle);
				break;
			case TPS_MOTION_LFF:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_LFF .. angle = %lf", angle);
				break;
			case TPS_MOTION_LFB:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_LFB .. angle = %lf", angle);
				break;
			case TPS_MOTION_B:
				Write_Log (LOG_FPS, LOG_INFO, "Motion = TPS_MOTION_B .. angle = %lf", angle);
				break;
		}
#endif
}
*/

/*
bool gr2AnimFaceVelocity(struct gr2AnimTrooper* model)
{
	if(model->currentPoint == TPS_POINTTYPE_RELOAD)
		return true;

	return false;
}

bool gr2AnimFaceYaw(struct gr2AnimTrooper* model)
{
	if(model->firing)
	{
		if(model->currentPose == TPS_POSETYPE_STAND || model->currentPose == TPS_POSETYPE_CROUCH)
			return true;
	}

	return false;
}
*/

LOGICAL gr2AnimGunPointStraight(gr2AnimTrooper* model)
{
	if(model->attacking)
	{
		if(model->currentPose == TPS_POSETYPE_STAND || model->currentPose == TPS_POSETYPE_CROUCH)
			return TRUE;
	}

	if(model->currentPose == TPS_POSETYPE_STAND &&
	   model->currentPoint == TPS_POINTTYPE_AIM &&
	   model->currentMove != TPS_MOVETYPE_STILL)
			return TRUE;

	if(model->currentPose == TPS_POSETYPE_CROUCH &&
	   model->currentPoint == TPS_POINTTYPE_AIM &&
	   model->strafe)
			return TRUE;

	return FALSE;

	/*
	if (model->currentPose == TPS_POSETYPE_STAND &&
		model->currentMove == TPS_MOVETYPE_FORWARD &&
		model->currentPoint == TPS_POINTTYPE_AIM)
	{
		return TRUE;
	}
	else if (model->currentPose == TPS_POSETYPE_CROUCH && model->firing)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	*/

	/*
	if ((model->currentPose == TPS_POSETYPE_STAND) &&
		(model->currentPoint != TPS_POINTTYPE_RELAXED) &&
		(model->currentPoint != TPS_POINTTYPE_RELOAD) &&
		(model->currentPoint != TPS_POINTTYPE_RELOAD_CUP))
	{
		return TRUE;
	}
	*/
	/*
	if (model->currentPose == TPS_POSETYPE_STAND &&
		model->currentMove == TPS_MOVETYPE_FORWARD &&
		model->currentPoint == TPS_POINTTYPE_AIM)
	{
		return TRUE;
	}
	else if (model->currentPose == TPS_POSETYPE_CROUCH && model->firing)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
	*/
}

SINT32 gr2AnimGetDeathState()
{
    OBJECT* o = persGetPersonaVehicleObjectDataPtr();

	if(o != nullptr && o->animModel != nullptr && o->animModel->HasFlag(gr2AnimTrooper::DEAD))
		return 1;

    return 0;
}

/*
void gr2AnimTrooperTwists(OBJECT* object,
								  FLOAT64& finalModelTwist,
								  FLOAT64& finalModelTwistSpeed,
								  FLOAT64& finalTorsoTwist,
								  FLOAT64& finalTorsoTwistSpeed,
								  FLOAT64& finalHeadTwist,
								  FLOAT64& finalHeadTwistSpeed,
								  FLOAT64& finalHeadPitch,
								  FLOAT64& finalHeadPitchSpeed)
{
	FLOAT64VECTOR	vel;
	FLOAT64			theta;
	FLOAT64			yaw, diff;
	FLOAT64			speed;

	gr2AnimTrooper* model = object->animModel;

	if (model->myVehicle->ModelData)
	{
		vel = model->myVehicle->ModelData->VehicleVel;
		yaw = model->myVehicle->ModelData->VehicleAtt.Yaw;
		//Write_Log (LOG_FPS, LOG_INFO, "FPS:  %f %f %f .. %f", vel.x, vel.y, vel.z, mlRAD_TO_DEG(yaw));

		yaw = mlRAD_TO_DEG(yaw);
	}
	else
	{
		vel = model->myVehicle->PosData.Vel;
		vel.x = -vel.x;
		vel.z = -vel.z;
		yaw = model->myVehicle->PosData.Att.Yaw;
		yaw = -yaw;
		//Write_Log (LOG_FPS, LOG_INFO, "TPS:  %f %f %f .. %f", vel.x, vel.y, vel.z, mlRAD_TO_DEG(yaw));

		yaw = mlRAD_TO_DEG(yaw);
	}

	FLOAT64 maxDiff = 190.0f;

	speed = mathExactVectorLength2(&vel);

	if (speed > 0)
	{
		model->idleTransitionTime = frametime + 1000;

		if (vel.x == 0)
		{
			if (vel.y > 0)
				theta = 90.0;
			else
				theta = -90.0;
		}
		else
		{
			theta = -1.0 * mlRAD_TO_DEG(atan2 (vel.y, vel.x)) - 90.0;
		}

		//yaw = mlRAD_TO_DEG(yaw);
		model->legsYaw = yaw;
		model->torsoYaw = yaw;
		model->headYaw = yaw;

		diff = yaw + theta;

		//Write_Log (LOG_FPS, LOG_INFO, "\nTheta/Yaw = %f / %f ->>>> %f", theta, yaw, diff);

		FLOAT64 absdiff = fabs(diff);
		UINT32 divdiff = (UINT32)(absdiff / 90.0f);
		FLOAT64 basediff = divdiff * 90.0f;

		if(absdiff - basediff < .5f)
			diff = (diff < 0) ? -basediff: basediff;

		// normalize to -180/+180
		while (diff * diff > 180.0 * 180.0)
		{
			if (diff < 0)
				diff += 360.0f;
			else if (diff >= 0)
				diff -= 360.0f;
		}

		absdiff = fabs(diff);

		//Write_Log (LOG_FPS, LOG_INFO, "Diff/absdiff = %f / %f", diff, absdiff);

		fpTPSMotionType lastMotion = model->currentMotion;

		gr2AnimDetermineMotionType(model, diff);

		switch (model->currentMotion)
		{
			case TPS_MOTION_F:
				if (gr2AnimGunPointStraight(model))
				{
					finalModelTwist = diff; // forward
					finalTorsoTwist = diff - 45.0;
					finalHeadTwist = 0;
				}
				else
				{
					finalModelTwist = diff;
					finalTorsoTwist = 0;
					finalHeadTwist = diff;
				}
				break;
			case TPS_MOTION_RFF:
				if (gr2AnimGunPointStraight(model))
				{
					finalModelTwist = diff *.9f;// / 2.0;
					finalTorsoTwist = diff / 2.0;
					finalHeadTwist = (model->currentMove != TPS_MOVETYPE_FORWARD_AIM ) ? diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						finalModelTwist = 0; //diff/2.0f;
						finalTorsoTwist = 0;
						finalHeadTwist = 0; //diff/2.0f;
					}
					else
					{
						finalModelTwist = diff *.9f;
						finalTorsoTwist = diff / 2.0;
						finalHeadTwist = diff / 2.0;
					}
				}
				break;
			case TPS_MOTION_RFB:
				if (gr2AnimGunPointStraight(model))
				{
					finalModelTwist = - diff *.9f;// / 2.0;
					finalTorsoTwist = - diff / 2.0;
					finalHeadTwist = (model->currentMove != TPS_MOVETYPE_FORWARD_AIM ) ? - diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						finalModelTwist = 0; //-diff/2.0f;
						finalTorsoTwist = 0;
						finalHeadTwist = 0; //-diff/2.0f;
					}
					else
					{
						finalModelTwist = - diff *.9f;
						finalTorsoTwist = - diff / 2.0;
						finalHeadTwist = - diff / 2.0;
					}
				}

				if (!model->movingBackwards)
				{
					gr2AnimSetModelState (object, model->currentLOD, model->currentPose, TPS_MOVETYPE_BACKWARD, model->currentPoint,
						model->currentWeapon, model->deathState, model->emoteID, model->useAltWeaponModel, model->chuteOpen, model->freeFall,
						model->useAltTrooperModel);
				}
				break;
			case TPS_MOTION_LFF:
				if (gr2AnimGunPointStraight(model))
				{
					finalModelTwist = diff *.9f;
					finalTorsoTwist = diff / 2.0;
					finalHeadTwist = (model->currentMove != TPS_MOVETYPE_FORWARD_AIM ) ? diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						finalModelTwist = 0; //diff/2.0f;
						finalTorsoTwist = 0;
						finalHeadTwist = 0; //diff/2.0f;
					}
					else
					{
						finalModelTwist = diff *.9f;
						finalTorsoTwist = diff / 2.0;
						finalHeadTwist = diff / 2.0;
					}
				}
				break;
			case TPS_MOTION_LFB:
			{
				if (gr2AnimGunPointStraight(model))
				{
					finalModelTwist = - diff *.9f;
					finalTorsoTwist = - (diff / 2.0);
					finalHeadTwist = (model->currentMove != TPS_MOVETYPE_FORWARD_AIM ) ? - diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						finalModelTwist = 0; //-diff/2.0f;
						finalTorsoTwist = 0;
						finalHeadTwist = 0; //-diff/2.0f;
					}
					else
					{
						finalModelTwist = -diff *.9f;
						finalTorsoTwist = -diff / 2.0;
						finalHeadTwist = -diff / 2.0;
					}
				}

				if (!model->movingBackwards)
				{
					gr2AnimSetModelState (object, model->currentLOD, model->currentPose, TPS_MOVETYPE_BACKWARD, model->currentPoint,
						model->currentWeapon, model->deathState, model->emoteID, model->useAltWeaponModel, model->chuteOpen, model->freeFall,
						model->useAltTrooperModel);
				}
			}
			break;
			case TPS_MOTION_B:
			{
				FLOAT64 adjust = (lastMotion == TPS_MOTION_LFB || lastMotion == TPS_MOTION_LFB) ? 200.0f: 180.0f;

				if (gr2AnimGunPointStraight(model))
				{
					finalModelTwist = adjust + diff;
					finalTorsoTwist = adjust + diff;
					finalHeadTwist = 0;
				}
				else
				{
					finalModelTwist = adjust + diff;
					finalTorsoTwist = 0;
					finalHeadTwist = adjust + diff;
				}
			}
			break;
		}

		finalModelTwistSpeed = finalTorsoTwistSpeed = finalHeadTwistSpeed = 60.0f;

		if(model->currentPose == TPS_POSETYPE_PRONE)
			finalModelTwistSpeed = 10.0f;

		if (model->headPoint == TPS_HEAD_LEFT)
		{
			finalHeadTwist += 45.0;
		}
		else if (model->headPoint == TPS_HEAD_RIGHT)
		{
			finalHeadTwist -= 45.0;
		}
	}
	else
	{
		model->currentMotion = TPS_MOTION_F;

		// only twist the legs if turned
		finalHeadTwist = 0;
		finalTorsoTwist = 0;
		finalModelTwist = 0;

		if(model->currentPose != TPS_POSETYPE_PRONE)
		{
			finalHeadTwistSpeed = 60.0f;
			finalTorsoTwistSpeed = 60.0f;
			finalModelTwistSpeed = 60.0f;

			FLOAT64 torsoDiff, legsDiff;

			if(model->headYaw != yaw)
			{
				bool left = ((yaw - model->headYaw) < 0) ? false : true;

				FLOAT64 torsoThreshold	= 45.0f;
				FLOAT64 legsThreshold		= 45.0f;

				// adjust thresholds
				if(model->currentPose == TPS_POSETYPE_STAND)
				{
					if(model->currentMove == TPS_MOVETYPE_IDLE_AIM)
					{
						torsoThreshold = 20;
						legsThreshold = (left) ? 55 : 70;
					}
					else
					{
						torsoThreshold = (left) ? 50 : 65;
						legsThreshold = (left) ? 50 : 65;
					}
				}

				torsoThreshold = legsThreshold = 65.0f;

				if(model->currentMove == TPS_MOVETYPE_IDLE_AIM || model->currentMove == TPS_MOVETYPE_TURN_AIM)
				{
					torsoThreshold = 5;
					legsThreshold = 70.0f;
				}

				// direction we're facing
				model->headYaw = yaw;

				torsoDiff = model->headYaw - model->torsoYaw;
				gr2NormalizeAngle(torsoDiff);

				// WORKS TO THE LEFT
				// turn torso towards head?
				if(fabs(torsoDiff) > torsoThreshold)
				{
					if(left)
						model->torsoYaw += (torsoDiff > 0) ? torsoDiff - torsoThreshold : torsoThreshold - torsoDiff;
					else
						model->torsoYaw += (torsoDiff > 0) ? torsoThreshold - torsoDiff : torsoDiff + torsoThreshold;

					gr2NormalizeAngle(model->torsoYaw);
					finalTorsoTwistSpeed = 10.0f;
				}

				legsDiff = model->torsoYaw - model->legsYaw;
				gr2NormalizeAngle(legsDiff);

				// turn torso towards torso?
				if(fabs(legsDiff) > legsThreshold)
				{
					model->turnInPlace = (frametime + 500) * ((legsDiff < 0) ? -1 : 1);
					model->legsYaw = model->headYaw;
					model->torsoYaw = model->headYaw;
				}

				model->nextIdlePlayTime = frametime + 20000 + (SINT32)(mathGetFloat32ChaoticNumber() * 20000.0f);
			}

			if(model->firing || abs(frametime - model->myVehicle->LastFireTime) < 100)
				model->torsoYaw = model->headYaw;

			fpTPSWeaponType tpsWeaponType = gr2AnimWeaponNumToTPSType (model, model->currentWeapon);

			if(model->headPoint != TPS_HEAD_STRAIGHT)
				model->torsoYaw = model->legsYaw = model->headYaw;
			else if(tpsWeaponType == TPS_WEAPONTYPE_BINOCS)
				model->torsoYaw = model->headYaw;

			if(frametime < abs(model->turnInPlace))
			{
				finalModelTwistSpeed = 4.0f;

				if(model->legsYaw != model->headYaw)
				{
					//model->turnInPlace = frametime + 100;
					model->torsoYaw = model->headYaw;
					model->legsYaw = model->headYaw;
				}
			}

			torsoDiff = model->headYaw - model->torsoYaw;
			legsDiff = model->headYaw - model->legsYaw;

			gr2NormalizeAngle(torsoDiff);
			gr2NormalizeAngle(legsDiff);

			finalModelTwist = legsDiff;

			finalTorsoTwist = legsDiff - torsoDiff;
			gr2NormalizeAngle(finalTorsoTwist);

			finalHeadTwist = torsoDiff;

			finalModelTwistSpeed = finalTorsoTwistSpeed = finalHeadTwistSpeed = 60.0f;
		}
		else
		{
			FLOAT64 diff = model->headYaw - yaw;

			if(fabs(diff) >= .5f && !(model->firing || abs(frametime - model->myVehicle->LastFireTime) < 100))
				model->turnInPlace = (frametime + 250) * ((diff < 0) ? -1 : 1);

			model->legsYaw = yaw;
			model->torsoYaw = yaw;
			model->headYaw = yaw;
		}


		if (model->headPoint == TPS_HEAD_LEFT)
		{
			finalHeadTwist += 45.0;
		}
		else if (model->headPoint == TPS_HEAD_RIGHT)
		{
			finalHeadTwist -= 45.0;
		}
	}

	// calculate head pitch
	if(model->currentMove != TPS_MOVETYPE_IDLE_AIM && model->currentMove != TPS_MOVETYPE_FORWARD_AIM)
	{
		const FLOAT32 DOWN_DEFLECTION = 20.0f;
		const FLOAT32 UP_DEFLECTION = 25.0f;

		if(model->upDownWeight >= .5f)
		{
			finalHeadPitch = UP_DEFLECTION * ((model->upDownWeight - .5f) / .5f) * -1.0f;
		}
		else
		{
			finalHeadPitch = DOWN_DEFLECTION * ((.5f - model->upDownWeight) / .5f);
		}

		finalHeadPitchSpeed = 180.0f;
	}
}
*/
#define HEAD_HIT 1<<0
#define BELLY_HIT 1<<1
#define FOOT_HIT 1<<2

bool gr2AnimTerrainizeByCapsule(gr2AnimTrooper* model)
{
	if(model->currentPose == TPS_POSETYPE_PRONE && (!model->myVehicle->ModelData || persIsCameraExternalMode()))
	{
		static colliderQuery collider;
		static dGeomID shape = nullptr;

		if(!shape)
		{
			collider.setBits(COLLIDER_VOID, COLLIDER_TERRAIN_MASK);
			collider.setMaxContactsPerHit(10);
			//collider.setDefaultCollideWithSpace(colliderTerrainSpace());

			shape = dCreateGeomTransform(collider.space());

			// our actual collision capsule
			dGeomID capsule = dCreateCapsule(0, .2f, 1.0f);

			// capsules are oriented up and rotated to the Y below
			// setting the position to the x offset achieves a
			// final the result we want which is a capsule moved
			// along the X axis after its rotated by the shape
			// transform geom below
			dGeomTransformSetCleanup(shape, 1);
			dGeomTransformSetInfo(shape, 1);
			dGeomTransformSetGeom(shape, capsule);

			Vec3d axis = Vec3d(0, 1.0f, 0);

			// rotate it along the x so that it fits our body better
			Matrixf rot = Matrixf::rotate((float)90.0f, axis);
			dMatrix3 dmat;
			MatrixTodMatrix(rot, dmat);
			dGeomSetRotation(capsule, dmat); // rotate our capsule to be oriented along the y

			// add out geom
			collider.addGeom(shape, COLLIDER_VOID, COLLIDER_TERRAIN_MASK);
		}

		// now we need to position and rotate our shape collider to our body

		FLOAT64 zAdjust = 0;
		Vec3d terrainize(0,0,0);
		vehVehicle* v = model->myVehicle;
		unsigned int hits = 0;
		double zPos = 0;

		piMATRIX& t = (v->ModelData) ? v->ModelData->VehicleAttMat: v->PosData.AttMat;
		FLOAT64VECTOR& p = (v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos;

		Vec3d worldPos;
		worldPos.set(p);
		worldPos = physicsGameToPhysical(worldPos);

		Matrixd worldTransform;
		worldTransform.set(t);

		dMatrix3 dmat;
		MatrixTodMatrix(worldTransform, dmat);

		dGeomSetRotation(shape, dmat);
		dGeomSetPosition(shape, worldPos.x(), worldPos.y(), worldPos.z() + .1f);

		if(collider.findCollisions(true))
		{
			const colliderQuery::CollisionList& collisions = collider.getCollisions(false);

			for(unsigned int i = 0; i < collisions.size(); ++i)
			{
				if(collisions[i].normal.z() >= 0)
				{
					terrainize += collisions[i].normal;
					zPos += collisions[i].pos.z();
					++hits;
				}
			}
		}

		if(terrainize.length2())
			terrainize.normalize();
		else
			terrainize.set(0,0,1.0f);

		terrainize.z() *= -1.0f;

		// do pitch smoothing
		static SINT32 rate = 250;
		float delta = float(frametime - model->terrainUpdate)/(float)rate;

		if(delta < 0) delta = 0;
		if(delta > 1.0f) delta = 1.0f;

		terrainize = model->terrainTarget + ((terrainize - model->terrainTarget) * delta);
		terrainize.normalize();

		// do zAdjust smoothing
		zAdjust = model->terrainTargetZ + ((zAdjust - model->terrainTargetZ) * delta);

		model->terrainTargetZ = zAdjust;
		model->terrainTarget = terrainize;
		model->terrainUpdate = frametime;

		piMATRIX tempMat = IdentityMatrix;

		if(v->ModelData)
		{
			tempMat = v->ModelData->VehicleAttInvMat;
		}
		else
		{
			math3dInvertMatrix(&tempMat,&v->PosData.AttMat);
		}

		piVECTOR tp;
		terrainize.get(tp);

		// bring the normal into the vehicle's object space
		math3dVectorMultiply(&tp, &tp, &tempMat);

		// rotate it 180 degrees because granny's y is negated
		generateRotationMatrix (AROUND_Z, 180.0, (piMATRIX*)&tempMat);
		math3dVectorMultiply(&tp, &tp, &tempMat);

		// create the final rotation matrix
		Vec3d from = Vec3d(0,0,-1.0f);
		Vec3d to;
		to.set(tp);

		Matrixd tempRot;
		tempRot.makeRotate(from, tp);

		tempRot.setTrans(0, 0, zAdjust);
		tempRot.get(model->terrainMat);

		return true;
	}

	model->terrainMat = IdentityMatrix;
	model->terrainTarget.set(0,0,-1.0f);
	model->terrainTargetZ = 0;
	model->terrainUpdate = frametime;

	return false;
}

#define UNUSED_Z -100000.0f
#define RAY_ADJUST .25f
#define SLOPE(y1,y2,x1,x2) ((y1-y2)/((x1*RAY_ADJUST)-(x2*RAY_ADJUST)))

inline void _smartZAdjust(FLOAT64& za, FLOAT64 worldZ, FLOAT64* zs, bool fps)
{
	FLOAT64 slopes[4];

	int zCount = 1;
	FLOAT64 zAvg = zs[2]; // belly
	FLOAT64 threshold = .2f; // height variance allowed between slopes

	// head to chest
	slopes[0] = SLOPE(zs[0],zs[1],-2.0f,-1.0f);

	// chest to belly
	slopes[1] = SLOPE(zs[1],zs[2],-1.0f,0);

	// belly to knees
	slopes[2] = SLOPE(zs[2],zs[3],0,1.0f);

	// knees to feet
	slopes[3] = SLOPE(zs[3],zs[4],1.0f,2.0f);

	// use it if a line is sloped down or if the line is within the threshold

	if(zs[0] > UNUSED_Z && (slopes[0] < 0 || fabs(slopes[0] - slopes[1]) <= threshold))
	{
		++zCount;
		zAvg += zs[0];
	}

	if(zs[1] > UNUSED_Z && (slopes[1] < 0 || slopes[1] <= threshold))
	{
		++zCount;
		zAvg += zs[1];
	}

	if(zs[3] > UNUSED_Z && (slopes[2] < 0 || slopes[2] <= threshold))
	{
		++zCount;
		zAvg += zs[3];
	}

	if(zs[4] > UNUSED_Z && (slopes[3] < 0 || fabs(slopes[3] - slopes[2]) <= threshold))
	{
		++zCount;
		zAvg += zs[4];
	}

	// calculate the average height to adjust to
	zAvg = zAvg / zCount;

	if(zAvg > UNUSED_Z)
		za = -(worldZ - zAvg);

	// take the higher of the calculated za and the belly z hit
	za = std::max(za, -(worldZ - zs[2]));

	// cap how much we can zadjust
	if(fabs(za) > 2.0f)
		za = 0;

	// check for the sandbag wall case, ugh
	if(false && fps)
	{
		if((fabs(slopes[0]) + fabs(slopes[3]) < .05f) && // outer lines are pretty flat
		   (slopes[1] > .2f && slopes[2] > .2f)) // inner lines are sloping up to a peak
		{
			// reset our z adjust
			za = 0;

			// force a kneeling position
			fpFPSControlInput(1, IMAP_HOLD_FP_CROUCH, 0);
		}
	}

	/*
	static FLOAT64 lastAdjust = 0;

	if(za != lastAdjust)
	{
		uiChatPrintLine("Z Adjust: %.2f / %.2f,%.2f,%.2f,%.2f,", za, slopes[0],slopes[1],slopes[2],slopes[3]);
		lastAdjust = za;
	}
	*/
}

void gr2AnimUpdate()
{
	static int _nextUpdate = 0;

#ifdef GR2_BLEND_EDITOR
	updateBlendEditor();
#endif

	if(!uiInGame() || frametime < _nextUpdate)
		return;

#ifdef LOD_ANIMATIONS
	AllowedErrorAtUnitDistance = GrannyFindAllowedLODError(AllowedErrorInPixels, (SINT32)FieldOfView, (FLOAT32)WantedWidth, 1.0f);
#endif

	static colliderQuery collider = colliderQuery(0, false);
	static colliderQuery::CollisionList hits;

	static GR2RayMap proneMap; // troopers that are prone
	static GR2RayMap swimMap; // troopers that are swimming
	static GR2RayMap idleMap; // troopers that are standing or kneeling prone and not turning
	static GR2RayMap	castMap; // troopers should cast a shadow or not?

    proneMap.clear();
    swimMap.clear();
    idleMap.clear();
    castMap.clear();

	colliderQuery::dGeomList& rays = collider.geoms();
	const Vec3d dir(0, 0, -1.0f); // down pointer
	Vec3d worldPos;
	Vec3d worldDir = Vec3d(0, 1, 0);
	Vec3d pos;
	Vec3d diff;
	Matrixd transform;
	piMATRIX tempMat;
	piVECTOR finalNormal;

	RayColliders* rc;
	GR2RayMap::iterator it;

	FLOAT64 zAdjust = 0;
	bool collide = false;
	int cursor = 0;

	// do some collider inits here
	if(_nextUpdate == 0)
	{
	    collider.addGeomToCollideWith((dGeomID)colliderTerrainSpace());
		collider.setMaxContactsPerHit(2);
		collider.setNormalFilter(0.0f, 0.0f, 1.0f); // only interested in contacts with normals shooting up
	}

	_nextUpdate = frametime + 25;

	vehVehicle* v = nullptr;
	vehGetVehicleHead(&v);

	for(;v; v = v->getNext())
	{
		if (v->VehicleType->Type != VEH_TYPE_PERSON ||
			v->VehicleObjectData == nullptr ||
			v->VehicleObjectData->PlaneObject == nullptr ||
		    v->VehicleObjectData->PlaneObject->animModel == nullptr ||
			v->VehicleObjectData->PlaneObject->animModel->HasFlag(gr2AnimTrooper::FREE_FALL) ||
		    v->VehicleObjectData->PlaneObject->animModel->active == false ||
			v->isInside)
			continue;

		// is it us and are we in 1st person
		bool isFPS				= (v->ModelData && !persIsCameraExternalMode());

		piMATRIX& t				= (v->ModelData) ? v->ModelData->VehicleAttMat: v->PosData.AttMat;
		FLOAT64VECTOR& p		= (v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos;
		gr2AnimTrooper* model	= v->VehicleObjectData->PlaneObject->animModel;
		FLOAT32	 distToMe		= v->VehicleObjectData->PlaneObject->distsqr;
		bool deployed			= (model->currentMove == TPS_MOVETYPE_DEPLOYED);

#ifdef GR2_BLEND_EDITOR
		if(model == _editModel)
			continue;
#endif

		// reset these every run
		//gr2ModelSetFeetAdjustments(model->skeleton, 0, 0);

		if(model->currentPose == TPS_POSETYPE_PRONE && distToMe <= POLISH_FAR && !deployed)
		{
			if(!collide)
			{
				collide = true;
				collider.disable();
			}

			rc = _gr2RayPool.Pop();

			int rayCount	= rays.size();
			int useRays		= 5;

			// add some new rays if needed
			if(cursor + useRays > rayCount)
			{
				int raysToAdd = cursor + useRays - rayCount;

				for(int c = 0; c < raysToAdd; ++c)
					collider.addGeom(dCreateRay(collider.space(), 1.0f), 0, 0);
			}

			for(int c = 0; c < useRays; ++c)
					rc->rays[c] = rays[cursor++];

			rc->count = useRays;

			worldDir.set(0, 1, 0);
			worldPos.set(p);
			transform.set(t);

			worldDir = worldDir * transform;

			static double adjust = .25f;

			for(int c = 0; c < useRays; ++c)
			{
				FLOAT64 offset = adjust * (c - 2);

				pos = worldPos +  (worldDir * offset);

				pos.z() += .75f;
				pos = physicsGameToPhysical(pos);

				dGeomEnable(rc->rays[c]);
				dGeomRaySetLength(rc->rays[c], 2.0f);
				dGeomRaySet(rc->rays[c], pos.x(), pos.y(), pos.z(), dir.x(), dir.y(), dir.z());
				dGeomSetCollideBits(rc->rays[c], COLLIDER_HARDTERRAIN_MASK);
			}

			proneMap[model] = rc;
		}
		/* terrainize feet; doesn't quite work right
		else if(!isFPS && model->currentMove == TPS_MOVETYPE_IDLE &&
					   (model->currentPose == TPS_POSETYPE_STAND ||
					    model->currentPose == TPS_POSETYPE_CROUCH))
		{
			if(!collide)
			{
				collide = true;
				collider.disable();
			}

			// clean slate
			rc.Reset();

			int rayCount = rays.size();

			// add some new rays if needed
			if(cursor + 2 > rayCount)
			{
				int raysToAdd = cursor + 2 - rayCount;

				for(int c = 0; c < raysToAdd; ++c)
					collider.addGeom(dCreateRay(collider.space(), 1.0f), 0, 0);
			}

			rc.rays[0] = rays[cursor++];
			rc.rays[1] = rays[cursor++];
			rc.rays[2] = nullptr;

			worldPos.set(p);

			for(int p = 0; p < 2; ++p)
			{
				int footBone = (p == 0) ? 12 : 8; // left, right

				if(gr2GetBonePositionByIndex(model->skeleton, footBone, &tempMat))
				{
					pos.set(tempMat._41, tempMat._42, tempMat._43);
					pos += worldPos;
					pos.z() += .5f;

					pos = physicsGameToPhysical(pos);

					dGeomEnable(rc.rays[p]);
					dGeomRaySetLength(rc.rays[p], 1.0f);
					dGeomRaySet(rc.rays[p], pos.x(), pos.y(), pos.z(), dir.x(), dir.y(), dir.z());
					dGeomSetCollideBits(rc.rays[p], COLLIDER_HARDTERRAIN_MASK);
				}
				else
				{
					rc.rays[p] = nullptr;
				}
			}

			idleMap[model] = rc;
		}
		*/
		else if(!isFPS && model->currentPose == TPS_POSETYPE_SWIM && !model->HasFlag(gr2AnimTrooper::SPLASHED) && distToMe <= POLISH_FAR)
		{
			float speed2 = (float)mathExactVectorLength2((v->ModelData) ? &v->ModelData->VehicleVel : &v->PosData.Vel);
			static float minSpeed = 1.0f;

			if(speed2 > minSpeed)
			{
				if(!collide)
				{
					collide = true;
					collider.disable();
				}

				rc = _gr2RayPool.Pop();

				//
				int rayCount = rays.size();

				// add some new rays if needed
				if(cursor + 1 > rayCount)
				{
					int raysToAdd = cursor + 1 - rayCount;

					for(int c = 0; c < raysToAdd; ++c)
						collider.addGeom(dCreateRay(collider.space(), 1.0f), 0, 0);
				}

				rc->rays[0] = rays[cursor++];
				rc->rays[1] = nullptr;
				rc->rays[2] = nullptr;

				pos.set(p);
				//pos.z() += 1.5f;
				pos = physicsGameToPhysical(pos);

				dGeomEnable(rc->rays[0]);
				dGeomRaySetLength(rc->rays[0], 1.0f);
				dGeomRaySet(rc->rays[0], pos.x(), pos.y(), pos.z(), dir.x(), dir.y(), dir.z());
				dGeomSetCollideBits(rc->rays[0], COLLIDER_TRN_WATER);

				swimMap[model] = rc;
			}
			else
			{
				model->SetFlag(gr2AnimTrooper::SPLASHED);
			}

#if(0)  // Needs work
			// add a splash
			TransformEffect* te = nullptr;

			te = TransformEffect::AddEffect("trooperswimsplash", Vec3f(0, 0, -1), 0, nullptr);

			if(te)
			{
				pos.set(p);
				pos.z() = -pos.z();

				te->enableemitters(true);;
				te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				te->setmatrix(Matrixf::translate(pos));
			}
#endif

			// make sure terrainizing get's reset
			model->terrainMat = IdentityMatrix;
			model->terrainTarget.set(0,0,-1.0f);
			model->terrainTargetZ = 0;
			model->terrainUpdate = frametime;
		}
		else if(!isFPS)
		{
			// handle terrainizing rotation back to level
			static SINT32 rate = 500;
			float delta = float(frametime - model->terrainUpdate)/(float)rate;

			if(delta < 0) delta = 0;
			if(delta > 1.0f) delta = 1.0f;

			// calculate it
			zAdjust = (FLOAT64)fpGetZAdjust(model->currentPose, model->currentMove, 1);

			// do zAdjust smoothing
			zAdjust = model->terrainTargetZ + ((zAdjust - model->terrainTargetZ) * delta);

			// save it
			model->terrainTargetZ = zAdjust;

			// set other vars
			model->terrainMat = IdentityMatrix;
			model->terrainTarget.set(0,0,-1.0f);
			model->terrainUpdate = frametime;

			// update our terrain mat
			model->terrainMat._43 = (FLOAT32)model->terrainTargetZ;
		}
		else // fps here, just set this stuff to default
		{
			model->terrainMat = IdentityMatrix;
			model->terrainTarget.set(0,0,-1.0f);
			model->terrainTargetZ = 0;
			model->terrainUpdate = frametime;
		}

		// make sure this flag is on by default because it can get cleared while the block below hits
		model->myVehicle->VehicleObjectData->PlaneObject->flag3 |= OBJECT_CAST_SHADOW;

		// adjust shadow casters as needed
		if(!isFPS && (ShadowMapFlags & SHADOW_VEHICLES) > 0 && distToMe <= 30.0f)
		{
			if(!collide)
			{
				collide = true;
				collider.disable();
			}

			rc = _gr2RayPool.Pop();

			int rayCount	= rays.size();
			int useRays		= 1;

			// add some new rays if needed
			if(cursor + useRays > rayCount)
				collider.addGeom(dCreateRay(collider.space(), 1.0f), 0, 0);

			rc->rays[0] = rays[cursor++];

			worldPos.set(p);

			pos = worldPos;
			pos.z() += .25f;
			pos = physicsGameToPhysical(pos);

			dGeomEnable(rc->rays[0]);
			dGeomRaySetLength(rc->rays[0], 100.0f);
			dGeomRaySet(rc->rays[0], pos.x(), pos.y(), pos.z(), dir.x(), dir.y(), dir.z());
			dGeomSetCollideBits(rc->rays[0], COLLIDER_TRN_BUILDING);

			castMap[model] = rc;
		}
	}

	if(collide)
	{
		collider.findCollisions(true);
		collider.sort();

		Vec3d terrainize;

		it = proneMap.begin();

		for(; it != proneMap.end(); ++it)
		{
			gr2AnimTrooper* model	= it->first;
			vehVehicle* v			= model->myVehicle;
			FLOAT64VECTOR& p		= (v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos;
			RayColliders* rc		= it->second;

			// is it us and are we in 1st person
			bool isFPS = (model->myVehicle->ModelData && !persIsCameraExternalMode());

			int zHits				= 0;
			float zPos			= 0;
			float zLow			= 0;
			int useRays			= rc->count;

			worldPos.set(p);
			terrainize.set(0,0,0);
			zAdjust = 0;

			const FLOAT32 unused = -100000.0f;

			// used for smart z adjustments
			FLOAT64 zs[5] =
			{
				unused,
				unused,
				unused,
				unused,
				unused
			};

			for(int r = 0; r < useRays; ++r)
			{
				hits.clear();

				if(rc->rays[r] && collider.getCollisions(rc->rays[r], hits, false))
				{
					for(unsigned int i = 0; i < hits.size(); ++i)
					{
						if(hits[i].normal.z() >= 0)
						{
							pos = hits[i].pos;
							terrainize += hits[i].normal;
							zPos += (float)hits[i].pos.z();
							zHits += 1;
							zs[r] = (float)hits[i].pos.z();

							if(!zLow || hits[i].pos.z() < zLow)
								zLow = (float)hits[i].pos.z();

							break;
						}
					}
				}
			}

			// if we had some hits
			if(zHits)
			{
				// default handling
				zAdjust = -(worldPos.z() - (zPos / zHits));

				// smarter z adjusment; takes peaks and troughs into account
				_smartZAdjust(zAdjust, worldPos.z(), zs, isFPS);

				// else take the average of all z's done above
			}

			if(terrainize.length2())
				terrainize.normalize();
			else
				terrainize.set(0,0,1.0f);

			terrainize.z() *= -1.0f;

			// do pitch smoothing
			static SINT32 rate = 150;
			float delta = float(frametime - model->terrainUpdate)/(float)rate;

			if(delta < 0) delta = 0;
			if(delta > 1.0f) delta = 1.0f;

			if(model->myVehicle->ModelData)
			{
				/* don't force crouch anymore
				static FLOAT64 maxDIFF = .9f;

				diff = terrainize - dir;

				FLOAT64 dlen2	= diff.length2();

				//uiChatPrintLine("Prone Angle Diff is %.2f", dlen);

				if(dlen2 >= maxDIFF * maxDIFF)
				{
					static SINT32 _nextTry = 0;
					// force a pose change to prone

					if(frametime >= _nextTry)
					{
						fpFPSControlInput(1, IMAP_HOLD_FP_CROUCH, 0);
						_nextTry = frametime + 2000;
					}
				}
				*/

				if(isFPS)
				{
					model->terrainMat = IdentityMatrix;
					model->terrainTarget.set(0,0,-1.0f);
					model->terrainTargetZ = 0;
					model->terrainUpdate = frametime;
					continue;
				}
			}

			diff = terrainize - model->terrainTarget;

			terrainize = model->terrainTarget + (diff * delta);
			terrainize.normalize();

			// do zAdjust smoothing
			zAdjust = model->terrainTargetZ + ((zAdjust - model->terrainTargetZ) * delta);

			model->terrainTargetZ = zAdjust;
			model->terrainTarget = terrainize;
			model->terrainUpdate = frametime;

			// don't allow submarining when crawling off ledges
			/*
             
            FLOAT64 zLowAdjust = -(worldPos.z() - zLow);
             
			if(zLowAdjust && model->terrainTargetZ < zLowAdjust)
			{
				//uiChatPrintLine("ZLow %.2f < %.2f ... clamping!", zLowAdjust, model->terrainTargetZ);
				model->terrainTargetZ = zLowAdjust;
			}
			*/

			tempMat = IdentityMatrix;

			if(v->ModelData)
			{
				tempMat = v->ModelData->VehicleAttInvMat;
			}
			else
			{
				math3dInvertMatrix(&tempMat,&v->PosData.AttMat);
			}

			terrainize.get(finalNormal);

			// bring the normal into the vehicle's object space
			math3dVectorMultiply(&finalNormal, &finalNormal, &tempMat);

			// rotate it 180 degrees because granny's y is negated
			generateRotationMatrix (AROUND_Z, 180.0, (piMATRIX*)&tempMat);
			math3dVectorMultiply(&finalNormal, &finalNormal, &tempMat);

			// create the final rotation matrix and z adjustment
			transform.makeRotate(dir, finalNormal);
			transform.setTrans(0, 0, zAdjust);
			transform.get(model->terrainMat);

			_gr2RayPool.Push(rc);
		}

		/* terrainize feet; doesn't quite work right
		it = idleMap.begin();

		for(; it != idleMap.end(); ++it)
		{
			gr2AnimTrooper* model = it->first;
			vehVehicle* v = model->myVehicle;
			FLOAT64VECTOR& p = (v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos;
			RayColliders& rc = it->second;

			worldPos.set(p);

			FLOAT32 left, right;
			left = right = 0;

			for(int r = 0; r < 2; ++r)
			{
				hits.clear();

				int footBone		= (r == 0) ? 12 : 8; // left, right
				FLOAT32& foot	= (r == 0) ? left : right; // left, right

				if(gr2GetBonePositionByIndex(model->skeleton, footBone, &tempMat))
				{
					pos.set(tempMat._41, tempMat._42, tempMat._43);
					pos += worldPos;

					pos = physicsGameToPhysical(pos);

					if(rc.rays[r] && collider.getCollisions(rc.rays[r], hits, false))
					{
						for(unsigned int i = 0; i < hits.size(); ++i)
						{
							if(hits[i].normal.z() >= 0)
							{
								foot = hits[i].pos.z() - pos.z();
								break;
							}
						}
					}
				}
			}

			gr2ModelSetFeetAdjustments(model->skeleton, left, right);
		}
		*/

		it = swimMap.begin();

		for(; it != swimMap.end(); ++it)
		{
			gr2AnimTrooper* model = it->first;
			RayColliders* rc = it->second;

			hits.clear();

			if(rc->rays[0] && collider.getCollisions(rc->rays[0], hits, false))
			{
				for(unsigned int i = 0; i < hits.size(); ++i)
				{
					if(hits[i].normal.z() >= 0)
					{
						pos = physicsPhysicalToGame(hits[i].pos);

						FLOAT64VECTOR soundPos;
						pos.get(soundPos);

						fxSndAddFiniteWorldSound("sounds/grenadesplash.wav",
															 5,
															 100,
															 IDE_TP_PRIORITY_LOW,
															 soundPos);

						pos.z() = -pos.z();

						Vec3d fuzziness;

						for(int i = 0; i < GetRandom(3, 6); i++)
						{
							TransformEffect* te = TransformEffect::AddEffect("deadtroopersplash", dir, 0, nullptr);

							if(te)
							{
								fuzziness.set(pos);

								fuzziness.x() += GetRandom(.5f, -.5f);
								fuzziness.y() += GetRandom(.5f, -.5f);

								te->enableemitters(true);;
								te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
								te->setmatrix(Matrixf::translate(fuzziness));
							}
						}

						model->SetFlag(gr2AnimTrooper::SPLASHED);
						break;
					}
				}
			}

			_gr2RayPool.Push(rc);
		}

		it = castMap.begin();

		for(; it != castMap.end(); ++it)
		{
			gr2AnimTrooper* model = it->first;
			RayColliders* rc = it->second;

			hits.clear();

			if(rc->rays[0] && collider.getCollisions(rc->rays[0], hits, false))
			{
				for(unsigned int i = 0; i < hits.size(); ++i)
				{
                    OBJECT* o = (OBJECT*)dGeomGetData(hits[i].g2);

                    if(!o)
                        continue;

                    if(!o->modelInstance)
                        continue;

                    if(o->modelInstance->GetModelType() == ModelTypes::BUILDING)
                    {
                        model->myVehicle->VehicleObjectData->PlaneObject->flag3 &= ~OBJECT_CAST_SHADOW;
                    }
				}
			}

			_gr2RayPool.Push(rc);
		}
	}
}

void gr2AnimReset(gr2AnimTrooper* model)
{
	model->active = true;
	model->animState = 0;

	// set up base pose
	model->currentPose			= TPS_POSETYPE_STAND;
	model->currentMove			= TPS_MOVETYPE_IDLE;
	model->currentPoint			= TPS_POINTTYPE_RELAXED;
	model->currentWeapon		= -1;
	model->SetFlag(gr2AnimTrooper::HIDE_CHUTE);
	model->stowedWeapon			= 0; // stow weapon array
	model->terrainMat			= IdentityMatrix;
	model->terrainTargetZ		= 0;
	model->terrainTarget.set(0,0,0);


	model->currentFlags = 0;
	model->currentLOD = TPS_LOD_1;

	model->headPoint = 0;
	model->deathState = 0;
	model->emoteID = 0;

	model->animationOffsetPercent = 0.0f;

	model->upDownWeight = 0;
	model->targetUpDownWeight = 0;
	model->currentUpWeight = 0;
	model->currentDownWeight = 0;
	model->currentMiddleWeight = 0;

	model->modelSpeed = 1.0f;

	// 2d stuff point stuff
	model->rectUpDownWeight = 0;
	model->rectTargetUpDownWeight = 0;
	model->rectLeftRightWeight = 0;
	model->rectTargetLeftRightWeight = 0;

	//model->useAltWeaponModel = 0;
	//model->useAltTrooperModel = FALSE;
	//model->useSelfTrooperModel = FALSE;
	//model->freeFall = FALSE;
	//model->chuteOpen = FALSE;
	//model->splashed = FALSE;
	//model->parade = FALSE;
	//model->movingBackwards = FALSE;

	model->turnInPlace = 0;

	model->currentMotion = TPS_MOTION_F;
	model->strafe = 0;

	model->moveSound.Stop();

	/*
	model->surfaceID = 0;
	model->firing = 0;
	model->readyToFire = 0;
	model->fired = 0;
	model->fireStartTime = 0;
	model->lastFireTime = 0;
	*/
	model->attacking = 0;
	model->lastVisualAttack = 0;
	model->nextVisualAttack = 0;
	model->attackUntil = 0;
	model->nextAttack = 0;
	model->readyUntil = 0;
	//model->readyWeaponAt = 0;

    gr2SetModelActive(model->collisionModel, true);
	gr2ClearBlendedAnimations(model->skeleton);

	if(gr2PlayBlendedAnimation (model->skeleton, fpTPSBasePose))
	{
		gr2BlendControlEaseIn (model->skeleton, fpTPSBasePose, 0.0);
		gr2BlendControlEaseOut (model->skeleton, fpTPSBasePose, 0.1f);
		gr2BlendControlSetWeight (model->skeleton, fpTPSBasePose, 1.0f);
	}

	// make sure the model gets rotated
	gr2ModelSetModelRotation (model->skeleton, .01f);

	// reset our target twists
	model->twist.Update(model, true);
}

bool gr2AnimTerrainize(gr2AnimTrooper* model)
{
	if(!model->myVehicle->ModelData || persIsCameraExternalMode())
	{
		if(model->currentPose == TPS_POSETYPE_PRONE)
		{
			FLOAT64 zAdjust = 0;
			Vec3d terrainize(0,0,0);
			vehVehicle* v = model->myVehicle;

			piMATRIX& t = (v->ModelData) ? v->ModelData->VehicleAttMat: v->PosData.AttMat;
			FLOAT64VECTOR& p = (v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos;

			Matrixd worldTransform;

			Vec3d worldPos;
			Vec3d worldDir = Vec3d(0, 1, 0);

			worldPos.set(p);
			worldTransform.set(t);

			worldDir = worldDir * worldTransform;

			static double adjust = .7f;

			Vec3d dir(0, 0, -1.0f);
			Vec3d headPos = worldPos +  (worldDir * -adjust);
			Vec3d bellyPos = worldPos;
			Vec3d footPos = worldPos +  (worldDir * adjust);

			headPos.z() += 1.5f;
			bellyPos.z() += 1.5f;
			footPos.z() += 1.5f;

			headPos = physicsGameToPhysical(headPos);
			bellyPos = physicsGameToPhysical(bellyPos);
			footPos = physicsGameToPhysical(footPos);

			unsigned int hits = 0;
			float zPos = 0;

			// these collisions need to be optimized; no need to do them seperately
			// actually all tps troopers who are prone could be done at once in another frame call and the model->terrainMat could be updated
			const colliderQuery::CollisionList& headHit = collideRayList(COLLIDER_TERRAIN_MASK, 3.0f, headPos, dir);

			for(unsigned int i = 0; i < headHit.size(); ++i)
			{
				if(headHit[i].normal.z() >= 0)
				{
					headPos = headHit[i].pos;
					terrainize += headHit[i].normal;
					zPos += (float)headHit[i].pos.z();
					hits |= HEAD_HIT;
					break;
				}
			}

			const colliderQuery::CollisionList& bellyHit = collideRayList(COLLIDER_TERRAIN_MASK, 3.0f, bellyPos, dir);

			for(unsigned int i = 0; i < bellyHit.size(); ++i)
			{
				if(bellyHit[i].normal.z() >= 0)
				{
					bellyPos = bellyHit[i].pos;
					terrainize += bellyHit[i].normal;
					zPos += (float)bellyHit[i].pos.z();
					hits |= BELLY_HIT;
					break;
				}
			}

			const colliderQuery::CollisionList& footHit = collideRayList(COLLIDER_TERRAIN_MASK, 3.0f, footPos, dir);

			for(unsigned int i = 0; i < footHit.size(); ++i)
			{
				if(footHit[i].normal.z() >= 0)
				{
					footPos = footHit[i].pos;
					terrainize += footHit[i].normal;
					zPos += (float)footHit[i].pos.z();
					hits |= FOOT_HIT;
					break;
				}
			}

			// special case it when we don't have all 3 hitting
			if(hits & BELLY_HIT && hits != (FOOT_HIT|BELLY_HIT|HEAD_HIT))
			{
				Vec3d dir = Vec3d(0,0,0);
				Vec3d pos = bellyPos;

				if(hits & FOOT_HIT)
				{
					dir = bellyPos - footPos;
					pos = footPos;
					hits |= HEAD_HIT;
				}
				else if(hits & HEAD_HIT)
				{
					dir = bellyPos - headPos;
					pos = headPos;
					hits |= FOOT_HIT;
				}

				if(dir.length2())
				{
					dir.normalize();
					pos += dir * (adjust * 2);
					zPos += (float)pos.z();
				}
			}

			int numHits = 0;

			if(hits & FOOT_HIT) ++numHits;
			if(hits & BELLY_HIT) ++numHits;
			if(hits & HEAD_HIT) ++numHits;

			if(numHits)
			{
				zAdjust = -(worldPos.z() - (zPos / numHits));
			}

			if(terrainize.length2())
				terrainize.normalize();
			else
				terrainize.set(0,0,1.0f);

			terrainize.z() *= -1.0f;

			// do pitch smoothing
			static SINT32 rate = 250;
			float delta = float(frametime - model->terrainUpdate)/(float)rate;

			if(delta < 0) delta = 0;
			if(delta > 1.0f) delta = 1.0f;

			terrainize = model->terrainTarget + ((terrainize - model->terrainTarget) * delta);
			terrainize.normalize();

			// do zAdjust smoothing
			zAdjust = model->terrainTargetZ + ((zAdjust - model->terrainTargetZ) * delta);

			model->terrainTargetZ = zAdjust;
			model->terrainTarget = terrainize;
			model->terrainUpdate = frametime;

			piMATRIX tempMat = IdentityMatrix;

			if(v->ModelData)
			{
				tempMat = v->ModelData->VehicleAttInvMat;
			}
			else
			{
				math3dInvertMatrix(&tempMat,&v->PosData.AttMat);
			}

			piVECTOR tp;
			terrainize.get(tp);

			// bring the normal into the vehicle's object space
			math3dVectorMultiply(&tp, &tp, &tempMat);

			// rotate it 180 degrees because granny's y is negated
			generateRotationMatrix (AROUND_Z, 180.0, (piMATRIX*)&tempMat);
			math3dVectorMultiply(&tp, &tp, &tempMat);

			// create the final rotation matrix
			Vec3d from = Vec3d(0,0,-1.0f);
			Vec3d to;
			to.set(tp);

			Matrixd tempRot;
			tempRot.makeRotate(from, tp);

			tempRot.setTrans(0, 0, zAdjust);
			tempRot.get(model->terrainMat);

			return true;
		}
		else if(model->currentPose == TPS_POSETYPE_SWIM)
		{
			model->terrainMat = IdentityMatrix;
			model->terrainTarget.set(0,0,-1.0f);

			vehVehicle* v = model->myVehicle;
			FLOAT64VECTOR& p = (v->ModelData) ? v->ModelData->VehiclePos: v->PosData.Pos;

			Vec3d worldPos;
			worldPos.set(p);

			static FLOAT64 idleDisplace = 1.4f;
			static FLOAT64 moveDisplace = .6f;

			FLOAT64 adjust = (model->currentMove == TPS_MOVETYPE_IDLE) ? idleDisplace : moveDisplace;
			FLOAT64 zAdjust = -(worldPos.z() - (worldPos.z() - adjust));

			// do pitch smoothing
			static SINT32 rate = 800;
			float delta = float(frametime - model->terrainUpdate)/(float)rate;

			if(delta < 0) delta = 0;
			if(delta > 1.0f) delta = 1.0f;

			// do zAdjust smoothing
			zAdjust = model->terrainTargetZ + ((zAdjust - model->terrainTargetZ) * delta);

			model->terrainTargetZ = zAdjust;
			model->terrainUpdate = frametime;

			Matrixd tempRot;
			tempRot.setTrans(0, 0, zAdjust);
			tempRot.get(model->terrainMat);

			return true;
		}
		else
		{
			static SINT32 rate = 250;
			float delta = float(frametime - model->terrainUpdate)/(float)rate;

			if(delta < 0) delta = 0;
			if(delta > 1.0f) delta = 1.0f;

			// calculate it
			FLOAT64 zAdjust = (FLOAT64)fpGetZAdjust(model->currentPose, model->currentMove);

			// do zAdjust smoothing
			zAdjust = model->terrainTargetZ + ((zAdjust - model->terrainTargetZ) * delta);

			// save it
			model->terrainTargetZ = zAdjust;

			return true;
		}
	}

	model->terrainMat = IdentityMatrix;
	model->terrainTarget.set(0,0,-1.0f);
	model->terrainTargetZ = 0;
	model->terrainUpdate = frametime;

	return false;
}

FLOAT64 gr2AnimTwist::Twist(FLOAT64& target, FLOAT64& twist, FLOAT64& speed, FLOAT64 delta, FLOAT64 offset)
{
	if (target != twist)
	{
		FLOAT64	 diff, move;

		gr2NormalizeAngle(target);

		diff	= target - twist;
		move = (speed < 0) ? fabs(diff) : delta * speed;

		if (diff < 0)
			move *= -1.0;

		if (move * move >= diff * diff)
			twist = target;
		else
			twist += move;

		gr2NormalizeAngle(twist);

		return move;
	}

	return 0;
}

void gr2AnimTwist::UpdateYaw(vehVehicle* v, FLOAT64 delta)
{
	if(!v || v->IsMe())
		return;

	gr2AnimTrooper* anim = v->GetAnim();
	if(!anim) return;

	predictedYaw = v->interpolator.LastUpdateYaw();

	if(vehicleYaw != predictedYaw)
	{
		if(true)
		{
			// change in degrees per second
			static const FLOAT64 MIN_YAW_RATE		= 200.0f;
			static const FLOAT64 MAX_YAW_RATE		= 500.0f;
			static const FLOAT64 LO_YAW_DIFF		= 20.0f;
			static const FLOAT64 HI_YAW_DIFF		= 90.0f;

			// yaw temp vars
			FLOAT64 vYaw	= mlRAD_TO_DEG(vehicleYaw);
			FLOAT64 pYaw	= mlRAD_TO_DEG(predictedYaw);
			FLOAT64 angle	= (FLOAT32)pYaw - (FLOAT32)vYaw;

			// normalize to -180/+180
			gr2NormalizeAngle(angle);

			FLOAT64 absAngle = fabs(angle);

			yawRate	= (absAngle < LO_YAW_DIFF) ? 0:
					   (absAngle > HI_YAW_DIFF) ? 1.0f:
					   (absAngle - LO_YAW_DIFF) / (HI_YAW_DIFF - LO_YAW_DIFF);

			yawRate = MIN_YAW_RATE + ((MAX_YAW_RATE - MIN_YAW_RATE) * yawRate);

			// this updates amount of change
			FLOAT64 move = yawRate * delta; //(v->LastUpdateDelta/1000.0f);

			if(!v->LastUpdateDelta)
				move = yawRate;

			if(angle < 0)
				move *= -1.0f;

			if(move * move >= angle * angle)
				vYaw = pYaw;
			else
				vYaw += move;

			// normalize it
			gr2NormalizeAngle(vYaw);

			//Write_Log (LOG_FPS, LOG_INFO, "TPS Yaw Pred = %f, Cur = %f, Diff = %f, Move = %f", predictedYaw, currentYaw, diff, move);

			// set our posdata to the newly smoothed yaw
			vehicleYaw = mlDEG_TO_RAD(vYaw);
		}
		else
		{
			vehicleYaw = predictedYaw;
		}
	}

	// override for now
	//vehicleYaw = predictedYaw;
}

void gr2AnimTwist::Update(gr2AnimTrooper* model, bool reset)
{
	FLOAT32 delta	= (frametime - lastUpdate)/1000.0f;
	bool isDead		= model->HasFlag(gr2AnimTrooper::DEAD);
	bool inRange	= model->InRange(150.0f);

	if(!straighten)
		straighten = (model->currentMove == TPS_MOVETYPE_IDLE);

	if(!model->myVehicle->IsMe())
	{
		UpdateYaw(model->myVehicle, delta);

		// update our position variables
		model->myVehicle->PosData.RenderAtt.Yaw	= vehicleYaw;
		model->myVehicle->PosData.Att.Yaw		= vehicleYaw;
	}

	model->upDownWeightOverride = 0;
	model->upDownWeightAdjust = 0;

	if(!yawRate)
		yawRate = 500.0f;

	// no twisting while dead or riding or if we were told to reset
	if(!inRange || straighten || reset || isDead || model->myVehicle->RideVehicleHostIndex > 0) // model->deathState ||
	{
		modelTwistTarget = 0.0f;
		torsoTwistTarget = 0.0f;
		headTwistTarget = 0.0f;
		headPitchTarget = 0.0f;
	}

	if(inRange)
	{
		// calculate the new targets
		if(!isDead && !straighten)
			Calculate(delta, model);

		// smooth the current values towards the targets
		Smooth(delta, model);
	}

	if(model->currentPose == TPS_POSETYPE_CROUCH &&
	   model->currentMove == TPS_MOVETYPE_FORWARD &&
	   model->currentMotion == TPS_MOTION_F)
	{
		if(modelTwistTarget > 10)
			model->upDownWeightOverride = .25f;
		else if(modelTwistTarget < 10)
			model->upDownWeightOverride = .5f;
	}
	/*
	else if(model->currentPoint == TPS_POINTTYPE_AIM &&
		fpTPSConvertWeaponClass(model->currentWeaponType) == CLASS_FIREARM &&
		model->targetUpDownWeight >= 0)
	{
		model->upDownWeightAdjust = model->targetUpDownWeight * .2f;
	}
	*/

	// tell our skeleton what rotations to use when drawing
	gr2ModelSetModelRotation(model->skeleton, modelTwist);
	gr2ModelSetTorsoRotation(model->skeleton, torsoTwist, 0);
	gr2ModelSetHeadRotation(model->skeleton, headTwist);
	gr2ModelSetHeadPitch(model->skeleton, headPitch);

	lastUpdate = frametime;
	straighten = false;
}

void gr2AnimTwist::CalculateTurn(gr2AnimTrooper* model, FLOAT64 yaw)
{
	FLOAT64 angle = yaw - legsYaw;
	gr2NormalizeAngle(angle);

	model->currentMotion = TPS_MOTION_F;
	model->strafe = 0;

	// only twist the legs if turning
	headTwistTarget = 0;
	torsoTwistTarget = 0;
	modelTwistTarget = 0;

	if(facing)
	{
		if(modelTwist || headTwist || torsoTwist)
		{
			model->turnInPlace = (frametime + 200) * ((angle < 0) ? -1 : 1);
			return;
		}

		facing = false;
		legsYaw = yaw;
	}

	FLOAT64 absAngle = fabs(angle);

	// crouching, standing, swimming...do torso and head turning
	if(model->currentPose != TPS_POSETYPE_PRONE && model->currentPose != TPS_POSETYPE_SIT)
	{
		if(angle < 0) // turning right
		{
			maxAngle = 90.0f;
			headAngle = 50.0f;
		}
		else // turning left
		{
			maxAngle = 90.0f;
			headAngle = 25.0f;
		}

		if(model->currentPoint == TPS_POINTTYPE_AIM || model->currentWeaponType == TPS_WEAPONTYPE_BINOCS)
		{
			maxAngle = 60.0f;
			headAngle = 0.0f;
		}
		else if((model->currentPoint == TPS_POINTTYPE_READY || model->attacking) &&
				fpTPSConvertWeaponClass(model->currentWeaponType) == CLASS_FIREARM)
		{
			maxAngle = 60.0f;
			headAngle = 0.0f;
		}
		else if(model->currentWeapon > 0)
		{
			maxAngle = 90.0f;
			headAngle = 50.0f;
		}

		modelTwistTarget = angle;

		if(absAngle >= maxAngle)
		{
			headTwistTarget		= headAngle;
			torsoTwistTarget	= maxAngle - headAngle;

			// we need to straighten
			facing = true;
		}
		else
		{
			// we want instante updates of twists
			//modelTwistRate = -1.0f;

			if(absAngle <= headAngle)
			{
				headTwistTarget		= absAngle;
				torsoTwistTarget	= 0.0f;
			}
			else
			{
				headTwistTarget		= headAngle;
				torsoTwistTarget	= absAngle - headAngle;
			}
		}

		if(modelTwistTarget < 0)
		{
			headTwistTarget		*= -1.0f;
			torsoTwistTarget	*= -1.0f;
		}
	}
	else
	{
		// only play turn animation if we haven't fired in a while and we've "turned" enough
		if(absAngle >= .5f && !(model->attacking || abs(frametime - model->myVehicle->LastFireTime) < 100))
			model->turnInPlace = (frametime + 250) * ((angle < 0) ? -1 : 1);

		// always keep everything straight when you get to here
		legsYaw = yaw;
	}
}

/*
void gr2AnimTwist::CalculateTurn(gr2AnimTrooper* model, FLOAT64& yaw, FLOAT64VECTOR& velocity)
{
	model->currentMotion = TPS_MOTION_F;
	model->strafe = 0;

	// only twist the legs if turning
	headTwistTarget = 0;
	torsoTwistTarget = 0;
	modelTwistTarget = 0;

	// crouching, standing, swimming...do torso and head turning
	if(model->currentPose != TPS_POSETYPE_PRONE && model->currentPose != TPS_POSETYPE_SIT)
	{
		FLOAT64 torsoDiff, legsDiff;

		if(headYaw != yaw)
		{
			bool left = ((yaw - headYaw) < 0) ? false : true;

			FLOAT64 torsoThreshold	= 45.0f;
			FLOAT64 legsThreshold	= 45.0f;

			// adjust thresholds
			if(model->currentPose == TPS_POSETYPE_STAND)
			{
				if(model->currentPoint == TPS_POINTTYPE_AIM)
				{
					torsoThreshold = 20;
					legsThreshold = (left) ? 55 : 70;
				}
				else
				{
					torsoThreshold = (left) ? 50 : 65;
					legsThreshold = (left) ? 50 : 65;
				}
			}

			torsoThreshold = legsThreshold = 65.0f;

			if(left)
				torsoThreshold = 55.0f;

			if(model->currentPoint == TPS_POINTTYPE_AIM || model->currentMove == TPS_MOVETYPE_DEPLOYED)
			{
				torsoThreshold = 5;
				legsThreshold = 70.0f;
			}

			// direction we're facing
			headYaw = yaw;

			torsoDiff = headYaw - torsoYaw;
			gr2NormalizeAngle(torsoDiff);

			// WORKS TO THE LEFT
			// turn torso towards head?
			if(fabs(torsoDiff) > torsoThreshold)
			{
				if(left)
					torsoYaw += (torsoDiff > 0) ? torsoDiff - torsoThreshold : torsoThreshold - torsoDiff;
				else
					torsoYaw += (torsoDiff > 0) ? torsoThreshold - torsoDiff : torsoDiff + torsoThreshold;

				gr2NormalizeAngle(torsoYaw);
				//finalTorsoTwistSpeed = 10.0f;
			}

			legsDiff = torsoYaw - legsYaw;
			gr2NormalizeAngle(legsDiff);

			// turn torso towards torso?
			if(fabs(legsDiff) > legsThreshold)
			{
				model->turnInPlace = (frametime + 300) * ((legsDiff < 0) ? -1 : 1);
				legsYaw = headYaw;
				torsoYaw = headYaw;
			}
			else
			{
				modelTwistRate = 500.0f;
			}

			//model->nextIdlePlayTime = frametime + 20000 + (SINT32)(mathGetFloat32ChaoticNumber() * 20000.0f);
		}

		if(model->attacking || abs(frametime - model->myVehicle->LastFireTime) < 25)
		{
			torsoYaw = headYaw;
			torsoTwistRate = 500.0f;
		}
		else if(model->currentPoint == TPS_POINTTYPE_AIM)
		{
			torsoTwistRate = 500.0f;
		}

		fpTPSWeaponType tpsWeaponType = model->currentWeaponType;

		//if(model->headPoint != TPS_HEAD_STRAIGHT)
		//	torsoYaw = legsYaw = headYaw;
		if(tpsWeaponType == TPS_WEAPONTYPE_BINOCS)
			torsoYaw = headYaw;

		if(frametime < abs(model->turnInPlace))
		{
			modelTwistRate = 500.0f;

			if(legsYaw != headYaw)
			{
				//model->turnInPlace = frametime + 100;
				torsoYaw = headYaw;
				legsYaw = headYaw;
			}
		}

		torsoDiff = headYaw - torsoYaw;
		legsDiff = headYaw - legsYaw;

		gr2NormalizeAngle(torsoDiff);
		gr2NormalizeAngle(legsDiff);

		modelTwistTarget = legsDiff;

		torsoTwistTarget = legsDiff - torsoDiff;
		gr2NormalizeAngle(torsoTwistTarget);

		headTwistTarget = torsoDiff;
	}
	else
	{
		FLOAT64 diff = headYaw - yaw;

		// only play turn animation if we haven't fired in a while and we've "turned" enough
		if(fabs(diff) >= .5f && !(model->attacking || abs(frametime - model->myVehicle->LastFireTime) < 100))
			model->turnInPlace = (frametime + 250) * ((diff < 0) ? -1 : 1);

		// always keep everything straight when you get to here
		legsYaw = yaw;
		torsoYaw = yaw;
		headYaw = yaw;
	}

	// add in head turning
	if (model->headPoint == TPS_HEAD_LEFT)
		headTwistTarget += 35.0;
	else if (model->headPoint == TPS_HEAD_RIGHT)
		headTwistTarget -= 35.0;
}
*/

void gr2AnimTwist::CalculateMoveLocked(gr2AnimTrooper* model, FLOAT64 angle)
{
	FLOAT64 diff = angle;

	FLOAT32 aimOffset	= 0;
	bool aiming			= (model->currentPoint == TPS_POINTTYPE_AIM);
	bool ready			= (model->attacking || model->readyUntil > frametime);
	bool reloading		= (model->currentPoint == TPS_POINTTYPE_RELOAD);
	bool straighten		= (model->currentMove == TPS_MOVETYPE_FALL || model->currentPose == TPS_POSETYPE_SIT);

	if(angle > DMT_FORWARD && angle < DMT_BACKWARD)
	{
		model->currentMotion = TPS_MOTION_SR;
		model->strafe = TPS_MOVETYPE_STRAFE_RIGHT;
	}
	else if(angle < -DMT_FORWARD && angle > -DMT_BACKWARD)
	{
		model->currentMotion = TPS_MOTION_SL;
		model->strafe = TPS_MOVETYPE_STRAFE_LEFT;
	}
	else
	{
		model->strafe = 0;

		if((angle > DMT_BACKWARD) || (angle < -DMT_BACKWARD)) // backward
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else
		{
			model->currentMotion = TPS_MOTION_F;
		}
	}

	if(aiming)
	{
		/*
		if(model->currentPose == TPS_POSETYPE_STAND)
		{
			switch(model->currentWeaponType)
			{
				case TPS_WEAPONTYPE_RIFLE:
				case TPS_WEAPONTYPE_SMG:
				case TPS_WEAPONTYPE_LMG:
				{
					aimOffset = -45.0f;
				}
				break;
				case TPS_WEAPONTYPE_PISTOL:
					aimOffset = 45.0f;
				break;
			}
		}
		else if(model->currentPose == TPS_POSETYPE_CROUCH)
		{
			switch(model->currentWeaponType)
			{
				case TPS_WEAPONTYPE_LMG:
				{
					aimOffset = -45.0f;
				}
				break;
			}
		}
		*/

		/*
		if(model->currentPose == TPS_POSETYPE_CROUCH &&
		   model->currentWeaponType == TPS_WEAPONTYPE_LMG &&
		   model->strafe)
		   aimOffset = 45.0f;
		*/
	}

	std::string motion = "NA";

	if(!straighten)
	{
		switch(model->currentMotion)
		{
			case TPS_MOTION_F:
			{
				modelTwistTarget = diff;
				motion = "F";
			}
			break;
			case TPS_MOTION_SR:
			{
				motion = "RFF";
			}
			break;
			case TPS_MOTION_SL:
			{
				motion = "RFF";
			}
			break;
			case TPS_MOTION_B:
			{
				FLOAT64 adjust = 180.0f;

				modelTwistTarget = adjust + diff;

				if (!model->HasFlag(gr2AnimTrooper::BACKWARDS))
				{
					gr2AnimSetModelState(model->myVehicle->VehicleObjectData->PlaneObject,
										 model->currentLOD,
										 model->currentPose,
										 TPS_MOVETYPE_BACKWARD,
										 model->currentPoint,
										 model->currentWeapon,
										 model->animState);
				}

				motion = "B";
			}
			break;
            default:
                // leave default for all others
                break;
		}
	}

	// now lets calculate the torso and head twists
	angle = fabs(modelTwist);

	if(angle >= 60.0f)
	{
		if(aiming)
		{
			headTwist = headTwistTarget		= 0.0f;
			torsoTwist = torsoTwistTarget	= 60.0f;
		}
		else
		{
			headTwist = headTwistTarget		= 30.0f;
			torsoTwist = torsoTwistTarget	= 30.0f;
		}
	}
	else
	{
		if(aiming || ready)
		{
			headTwist = headTwistTarget		= 0.0f;
			torsoTwist = torsoTwistTarget	= angle;
		}
		else
		{
			if(angle <= 30.0f)
			{
				headTwist = headTwistTarget		= angle;
				torsoTwist = torsoTwistTarget	= 0.0f;
			}
			else
			{
				headTwist = headTwistTarget		= 30.0f;
				torsoTwist = torsoTwistTarget	= angle - 30.0f;
			}
		}
	}

	if(modelTwist < 0)
	{
		headTwist			*= -1.0f;
		headTwistTarget		*= -1.0f;
		torsoTwist			*= -1.0f;
		torsoTwistTarget	*= -1.0f;
	}

	//uiChatPrintLine("Current motion is %d, diff = %f", model->currentMotion, diff);
	//aimOffset = 0;

	if(aiming && aimOffset)// && !model->strafe)
	{
		//torsoTwist += aimOffset;
		//torsoTwistTarget = torsoTwist;
		torsoTwistTarget += aimOffset;
	}

	if(reloading)
		headTwist = headTwistTarget = 0;

	/*
	if(model->headPoint == TPS_HEAD_LEFT)
		headTwistTarget += 35.0;
	else if(model->headPoint == TPS_HEAD_RIGHT)
		headTwistTarget -= 35.0;
	*/

	//uiChatPrintLine("Yaw = %f, Diff = %f, Motion = %s", yaw, diff, motion.c_str());
}

void gr2AnimTwist::CalculateMovePrimary(gr2AnimTrooper* model, FLOAT64 angle)
{
	FLOAT64 diff = angle;

	FLOAT32 aimOffset	= 0;
	bool left			= (angle < 0);
	bool prone			= (model->currentPose == TPS_POSETYPE_PRONE);
	bool aiming			= (model->currentPoint == TPS_POINTTYPE_AIM);
	bool ready			= (model->attacking || model->readyUntil > frametime);
	bool reloading		= (model->currentPoint == TPS_POINTTYPE_RELOAD);
	bool straighten		= (model->currentMove == TPS_MOVETYPE_FALL || model->currentPose == TPS_POSETYPE_SIT);

	if(angle > DMT_FORWARD && angle < DMT_BACKWARD)
	{
		model->currentMotion = TPS_MOTION_SR;
		model->strafe = TPS_MOVETYPE_STRAFE_RIGHT;
	}
	else if(angle < -DMT_FORWARD && angle > -DMT_BACKWARD)
	{
		model->currentMotion = TPS_MOTION_SL;
		model->strafe = TPS_MOVETYPE_STRAFE_LEFT;
	}
	else
	{
		model->strafe = 0;

		if((angle > DMT_BACKWARD) || (angle < -DMT_BACKWARD)) // backward
		{
			model->currentMotion = TPS_MOTION_B;
		}
		else
		{
			model->currentMotion = TPS_MOTION_F;
		}
	}

	if(!straighten && model->myVehicle->IsMe() && (fpFPSIsTrooperParachuteOpen() || fpFPSIsTrooperFalling()))
		straighten = true;

	if(aiming && model->currentPose == TPS_POSETYPE_STAND)
	{
		switch(model->currentWeaponType)
		{
			case TPS_WEAPONTYPE_RIFLE:
			case TPS_WEAPONTYPE_SMG:
			{
				aimOffset = -45.0f;
			}
			break;
			case TPS_WEAPONTYPE_PISTOL:
				aimOffset = 45.0f;
			break;
            default:
                // leave default for others
                break;
		}
	}

	std::string motion = "NA";

	if(!straighten)
	{
		switch(model->currentMotion)
		{
			case TPS_MOTION_F:
			{
				modelTwistTarget = diff;
				torsoTwistTarget = 0;
				headTwistTarget = diff;

				if(!prone)
				{
					if(!left && !aiming)
					{
						torsoTwistTarget = diff * .5f;
						headTwistTarget = diff * .5f;
					}
					else if(ready || aiming)
					{
						//modelTwistTarget = diff;
						torsoTwistTarget = diff;
						headTwistTarget = 0;
					}
					else if(reloading)
					{
						//modelTwistTarget = diff;
						torsoTwistTarget = diff * .5f;
						headTwistTarget = 0;
					}
				}

				motion = "F";
			}
			break;
			case TPS_MOTION_SR:
			{
				if(!prone)
				{
					if(aiming && model->currentPose == TPS_POSETYPE_CROUCH)
					{
						switch(model->currentWeaponType)
						{
							case TPS_WEAPONTYPE_RIFLE:
							case TPS_WEAPONTYPE_SMG:
							{
								torsoTwistTarget = 10.0f;
							}
							break;
                            default:
                                // leave default
                                break;
						}
					}
					else if(ready && model->currentPose == TPS_POSETYPE_STAND)
					{
						switch(model->currentWeaponType)
						{
							case TPS_WEAPONTYPE_RIFLE:
							case TPS_WEAPONTYPE_SMG:
							{
								torsoTwistTarget = 10.0f;
							}
							break;
                            default:
                                // leave default
                                break;
						}
					}

					//modelTwistTarget = 0;
					//torsoTwistTarget = diff - DMT_FORWARD;
					//headTwistTarget = 0;

					//if(model->strafe == TPS_MOVETYPE_STRAFE_RIGHT)
					//	headTwistTarget = diff * .1f;
				}

				motion = "RFF";
			}
			break;
			case TPS_MOTION_SL:
			{
				if(!prone)
				{
					if(aiming && model->currentPose == TPS_POSETYPE_CROUCH)
					{
						switch(model->currentWeaponType)
						{
							case TPS_WEAPONTYPE_RIFLE:
							case TPS_WEAPONTYPE_SMG:
							{
								torsoTwistTarget = 10.0f;
							}
							break;
                            default:
                                // leave default
                                break;
						}
					}
					else if(!left && ready && model->currentPose == TPS_POSETYPE_STAND)
					{
						switch(model->currentWeaponType)
						{
							case TPS_WEAPONTYPE_RIFLE:
							case TPS_WEAPONTYPE_SMG:
							{
								torsoTwistTarget = -45.0f;
							}
							break;
                            default:
                                // leave default
                                break;
						}
					}

					//modelTwistTarget = 0;
					//torsoTwistTarget = diff - DMT_FORWARD;
					//headTwistTarget = 0;

					//if(model->strafe == TPS_MOVETYPE_STRAFE_RIGHT)
					//	headTwistTarget = diff * .1f;
				}

				motion = "RFF";
			}
			break;
			case TPS_MOTION_B:
			{
				FLOAT64 adjust = 180.0f;

				modelTwistTarget = adjust + diff;
				torsoTwistTarget = 0;
				headTwistTarget = adjust + diff;

				if (!model->HasFlag(gr2AnimTrooper::BACKWARDS))
				{
					gr2AnimSetModelState(model->myVehicle->VehicleObjectData->PlaneObject,
										 model->currentLOD,
										 model->currentPose,
										 TPS_MOVETYPE_BACKWARD,
										 model->currentPoint,
										 model->currentWeapon,
										 model->animState);
				}

				motion = "B";
			}
			break;
            default:
                // leave defaults
                break;
		}
	}

	//uiChatPrintLine("Current motion is %d, diff = %f", model->currentMotion, diff);

	if(model->currentPose == TPS_POSETYPE_PRONE)
		modelTwistRate = 100.0f;

	if(aiming && !model->strafe)
		torsoTwistTarget += aimOffset;

	if(reloading)
		headTwistTarget = 0;

	if(model->headPoint == TPS_HEAD_LEFT)
		headTwistTarget += 35.0;
	else if(model->headPoint == TPS_HEAD_RIGHT)
		headTwistTarget -= 35.0;

	//uiChatPrintLine("Yaw = %f, Diff = %f, Motion = %s", yaw, diff, motion.c_str());
}

void gr2AnimTwist::CalculateMoveSecondary(gr2AnimTrooper* model, FLOAT64 angle)
{
	FLOAT64 diff = angle;

	bool left			= (angle < 0);
	bool prone			= (model->currentPose == TPS_POSETYPE_PRONE);
	bool ready			= (model->attacking || model->readyUntil > frametime);
	bool reloading		= (model->currentPoint == TPS_POINTTYPE_RELOAD);
	bool straighten		= (model->currentMove == TPS_MOVETYPE_FALL || model->currentPose == TPS_POSETYPE_SIT);

	fpTPSMotionType lastMotion = model->currentMotion;

	gr2AnimDetermineMotionType(model, diff);

	if(!straighten && model->myVehicle->IsMe() && (fpFPSIsTrooperParachuteOpen() || fpFPSIsTrooperFalling()))
		straighten = true;

	model->strafe = 0;

	std::string motion = "NA";

	if(!straighten)
	{
		switch(model->currentMotion)
		{
			case TPS_MOTION_F:
			{
				modelTwistTarget = diff;
				torsoTwistTarget = 0;
				headTwistTarget = diff;

				if(!prone)
				{
					if(ready)
					{
						//modelTwistTarget = diff;
						torsoTwistTarget = diff;
						headTwistTarget = 0;
					}
					else if(reloading)
					{
						//modelTwistTarget = diff;
						torsoTwistTarget = diff * .5f;
						headTwistTarget = 0;
					}
					else if(diff > 0)
					{
						//torsoTwistTarget = diff * .5f;
						headTwistTarget = diff * .5f;
					}
				}

				motion = "F";
			}
			break;
			case TPS_MOTION_RFF:
			{
				if(!prone)
				{
					modelTwistTarget = diff * .9f;
					torsoTwistTarget = diff * .55f;
					//headTwistTarget = diff * .3f;

					//if(model->strafe == TPS_MOVETYPE_STRAFE_RIGHT)
					//	headTwistTarget = diff * .1f;
				}

				motion = "RFF";
			}
			break;
			case TPS_MOTION_RFB:
			{
				if(!prone)
				{
					modelTwistTarget = -diff * .9f;
					torsoTwistTarget = -diff * .45f;
					headTwistTarget = -diff * .45f;
				}

				if (!model->HasFlag(gr2AnimTrooper::BACKWARDS))
				{
					gr2AnimSetModelState(model->myVehicle->VehicleObjectData->PlaneObject,
										 model->currentLOD,
										 model->currentPose,
										 TPS_MOVETYPE_BACKWARD,
										 model->currentPoint,
										 model->currentWeapon,
										 model->animState);
				}

				motion = "RFB";
			}
			break;
			case TPS_MOTION_LFF:
			{
				if(!prone)
				{
					modelTwistTarget = diff * .9f;
					//torsoTwistTarget = diff * .5f;
					headTwistTarget = diff * .9f;
				}

				motion = "LFF";
			}
			break;
			case TPS_MOTION_LFB:
			{
				if(!prone)
				{
					modelTwistTarget = -diff * .9f;
					torsoTwistTarget = -diff * .55f;
					headTwistTarget = -diff * .3f;

					//if(model->strafe == TPS_MOVETYPE_STRAFE_LEFT)
					//	headTwistTarget = -diff * .1f;
				}

				if (!model->HasFlag(gr2AnimTrooper::BACKWARDS))
				{
					gr2AnimSetModelState(model->myVehicle->VehicleObjectData->PlaneObject,
										 model->currentLOD,
										 model->currentPose,
										 TPS_MOVETYPE_BACKWARD,
										 model->currentPoint,
										 model->currentWeapon,
										 model->animState);
				}

				motion = "LFB";
			}
			break;
			case TPS_MOTION_B:
			{
				FLOAT64 adjust = 180.0f; //(lastMotion == TPS_MOTION_LFB || lastMotion == TPS_MOTION_LFB) ? 200.0f: 180.0f;

				if(lastMotion == TPS_MOTION_LFB)
					adjust = 200.0f;
				else if(lastMotion == TPS_MOTION_RFB)
					adjust = 200.0f;

				modelTwistTarget = adjust + diff;

				if(!prone && left)
				{
					torsoTwistTarget = adjust + diff;
					headTwistTarget = 0;
				}
				else
				{
					torsoTwistTarget = 0;
					headTwistTarget = adjust + diff;
				}

				motion = "B";
			}
			break;
            default:
                // leave defaults
                break;
		}
	}

	//uiChatPrintLine("Current motion is %d, diff = %f", model->currentMotion, diff);

	if(model->currentPose == TPS_POSETYPE_PRONE)
		modelTwistRate = 100.0f;

	if(reloading)
		headTwistTarget = 0;

	if(model->headPoint == TPS_HEAD_LEFT)
		headTwistTarget += 35.0;
	else if(model->headPoint == TPS_HEAD_RIGHT)
		headTwistTarget -= 35.0;

	//uiChatPrintLine("Yaw = %f, Diff = %f, Motion = %s", yaw, diff, motion.c_str());
}

void gr2AnimTwist::Calculate(FLOAT64 delta, gr2AnimTrooper* model)
{
	FLOAT64VECTOR vel;
	FLOAT64 yaw = 0;

	modelTwistRate = torsoTwistRate = headTwistRate = yawRate;

	if(model->myVehicle->ModelData)
	{
		vel = model->myVehicle->ModelData->VehicleVel;
		yaw = model->myVehicle->ModelData->VehicleAtt.Yaw;
		//Write_Log (LOG_FPS, LOG_INFO, "FPS:  %f %f %f .. %f", vel.x, vel.y, vel.z, mlRAD_TO_DEG(yaw));

		yaw = mlRAD_TO_DEG(yaw);
	}
	else
	{
		vel = model->myVehicle->PosData.Vel;
		vel.x = -vel.x;
		vel.z = -vel.z;
		yaw = model->myVehicle->PosData.Att.Yaw;
		yaw = -yaw;
		//Write_Log (LOG_FPS, LOG_INFO, "TPS:  %f %f %f .. %f", vel.x, vel.y, vel.z, mlRAD_TO_DEG(yaw));

		yaw = mlRAD_TO_DEG(yaw);
	}

	FLOAT64 speed = mathExactVectorLength2(&vel);

	if(speed > 0) // moving
	{
		FLOAT64 theta = 0;

		if (vel.x == 0)
		{
			if (vel.y > 0)
				theta = 90.0;
			else
				theta = -90.0;
		}
		else
		{
			theta = -1.0 * mlRAD_TO_DEG(atan2 (vel.y, vel.x)) - 90.0;
		}

		legsYaw				= yaw;
		torsoYaw			= yaw;
		headYaw				= yaw;

		headTwistTarget		= 0;
		torsoTwistTarget	= 0;
		modelTwistTarget	= 0;

		// in degrees
		FLOAT64 angle		= yaw + theta;

		//Write_Log (LOG_FPS, LOG_INFO, "\nTheta/Yaw = %f / %f ->>>> %f", theta, yaw, diff);

		FLOAT64 absAngle	= fabs(angle);
		UINT32 divAngle		= (UINT32)(absAngle / 90.0f);
		FLOAT64 baseAngle	= divAngle * 90.0f;

		if(absAngle - baseAngle < .5f)
			angle = (angle < 0) ? -baseAngle: baseAngle;

		// normalize to -180/+180
		gr2NormalizeAngle(angle);

		//absdiff = fabs(diff);

		switch(model->currentWeaponType)
		{
			case TPS_WEAPONTYPE_RIFLE:
			case TPS_WEAPONTYPE_SMG:
			case TPS_WEAPONTYPE_LMG:
			case TPS_WEAPONTYPE_PISTOL:
			case TPS_WEAPONTYPE_ROCKET:
				if(model->currentPose != TPS_POSETYPE_PRONE)
				{
					if(model->currentPoint == TPS_POINTTYPE_READY || model->currentPoint == TPS_POINTTYPE_AIM)
						CalculateMoveLocked(model, angle);
					else
						CalculateMovePrimary(model, angle);
				}
				else
					CalculateMoveSecondary(model, angle);
			break;
			default:
				CalculateMoveSecondary(model, angle);
			break;
		}

		facing = false;
	}
	else // turn in place
	{
		if(straighten)
		{
			legsYaw		= yaw;
			torsoYaw	= yaw;
			headYaw		= yaw;
		}

		CalculateTurn(model, yaw);
	}

	// debug
	if(0)
	{
		modelTwistTarget = 0.0f;
		torsoTwistTarget = 0.0f;
		headTwistTarget = 0.0f;
	}

	// calculate head pitch
	if(model->currentPoint != TPS_POINTTYPE_AIM)
	{
		const FLOAT32 DOWN_DEFLECTION = 20.0f;
		const FLOAT32 UP_DEFLECTION = 25.0f;

		if(model->upDownWeight >= .5f)
		{
			headPitchTarget = UP_DEFLECTION * ((model->upDownWeight - .5f) / .5f) * -1.0f;
		}
		else
		{
			headPitchTarget = DOWN_DEFLECTION * ((.5f - model->upDownWeight) / .5f);
		}
	}

#ifdef GR2_BLEND_EDITOR
	if(model == _editModel)
	{
		modelTwistTarget = 0.0f;
		torsoTwistTarget = 0.0f;
		headTwistTarget = 0.0f;
	}
#endif
}
/*
void gr2AnimTwist::Calculate2(FLOAT64 delta, gr2AnimTrooper* model)
{
	FLOAT64VECTOR	vel;
	FLOAT64			theta;
	FLOAT64			yaw, diff;
	FLOAT64			speed;
	OBJECT*			object = model->myVehicle->VehicleObjectData->PlaneObject;

	modelTwistRate = torsoTwistRate = headTwistRate = 300.0f;

	if (model->myVehicle->ModelData)
	{
		vel = model->myVehicle->ModelData->VehicleVel;
		yaw = model->myVehicle->ModelData->VehicleAtt.Yaw;
		//Write_Log (LOG_FPS, LOG_INFO, "FPS:  %f %f %f .. %f", vel.x, vel.y, vel.z, mlRAD_TO_DEG(yaw));

		yaw = mlRAD_TO_DEG(yaw);
	}
	else
	{
		vel = model->myVehicle->PosData.Vel;
		vel.x = -vel.x;
		vel.z = -vel.z;
		yaw = model->myVehicle->PosData.Att.Yaw;
		yaw = -yaw;
		//Write_Log (LOG_FPS, LOG_INFO, "TPS:  %f %f %f .. %f", vel.x, vel.y, vel.z, mlRAD_TO_DEG(yaw));

		yaw = mlRAD_TO_DEG(yaw);
	}

	FLOAT64 maxDiff = 190.0f;

	speed = mathExactVectorLength2(&vel);

	if (speed > 0) // moving
	{
		model->idleTransitionTime = frametime + 1000;

		if (vel.x == 0)
		{
			if (vel.y > 0)
				theta = 90.0;
			else
				theta = -90.0;
		}
		else
		{
			theta = -1.0 * mlRAD_TO_DEG(atan2 (vel.y, vel.x)) - 90.0;
		}

		//yaw = mlRAD_TO_DEG(yaw);
		legsYaw = yaw;
		torsoYaw = yaw;
		headYaw = yaw;
		headTwistTarget = 0;
		torsoTwistTarget = 0;
		modelTwistTarget = 0;

		diff = yaw + theta;

		//Write_Log (LOG_FPS, LOG_INFO, "\nTheta/Yaw = %f / %f ->>>> %f", theta, yaw, diff);

		FLOAT64 absdiff = fabs(diff);
		UINT32 divdiff = (UINT32)(absdiff / 90.0f);
		FLOAT64 basediff = divdiff * 90.0f;

		if(absdiff - basediff < .5f)
			diff = (diff < 0) ? -basediff: basediff;

		// normalize to -180/+180
		while (diff * diff > 180.0 * 180.0)
		{
			if (diff < 0)
				diff += 360.0f;
			else if (diff >= 0)
				diff -= 360.0f;
		}

		absdiff = fabs(diff);

		fpTPSMotionType lastMotion = model->currentMotion;

		gr2AnimDetermineMotionType(model, diff);

		switch (model->currentMotion)
		{
			case TPS_MOTION_F:
				if (gr2AnimGunPointStraight(model))
				{
					modelTwistTarget = diff; // forward
					torsoTwistTarget = diff;// - 45.0; new stuff doesn't appear to need this
					headTwistTarget = 0;
				}
				else
				{
					modelTwistTarget = diff;
					torsoTwistTarget = 0;
					headTwistTarget = diff;
				}
				break;
			case TPS_MOTION_RFF:
				if (gr2AnimGunPointStraight(model))
				{
					modelTwistTarget = diff *.9f;// / 2.0;
					torsoTwistTarget = diff / 2.0;
					headTwistTarget = (model->currentPoint != TPS_POINTTYPE_AIM ) ? diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						modelTwistTarget = 0; //diff/2.0f;
						torsoTwistTarget = 0;
						headTwistTarget = 0; //diff/2.0f;
					}
					else
					{
						modelTwistTarget = diff *.9f;
						torsoTwistTarget = diff / 2.0;
						headTwistTarget = (diff / 2.0) * .5f;
					}
				}
				break;
			case TPS_MOTION_RFB:
				if (gr2AnimGunPointStraight(model))
				{
					modelTwistTarget = - diff *.9f;// / 2.0;
					torsoTwistTarget = - diff / 2.0;
					headTwistTarget = (model->currentPoint != TPS_POINTTYPE_AIM ) ? - diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						modelTwistTarget = 0; //-diff/2.0f;
						torsoTwistTarget = 0;
						headTwistTarget = 0; //-diff/2.0f;
					}
					else
					{
						modelTwistTarget = - diff *.9f;
						torsoTwistTarget = - diff / 2.0;
						headTwistTarget = - diff / 2.0;
					}
				}

				if (!model->HasFlag(gr2AnimTrooper::BACKWARDS))
				{
					gr2AnimSetModelState(object,
										 model->currentLOD,
										 model->currentPose,
										 TPS_MOVETYPE_BACKWARD,
										 model->currentPoint,
										 model->currentWeapon,
										 model->animState);
				}
				break;
			case TPS_MOTION_LFF:
				if (gr2AnimGunPointStraight(model))
				{
					modelTwistTarget = diff *.9f;
					torsoTwistTarget = diff / 2.0;
					headTwistTarget = (model->currentPoint != TPS_POINTTYPE_AIM ) ? diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						modelTwistTarget = 0; //diff/2.0f;
						torsoTwistTarget = 0;
						headTwistTarget = 0; //diff/2.0f;
					}
					else
					{
						modelTwistTarget = diff * .9f;
						torsoTwistTarget = diff / 2.0;
						headTwistTarget = diff / 2.0;
					}
				}
				break;
			case TPS_MOTION_LFB:
			{
				if (gr2AnimGunPointStraight(model))
				{
					modelTwistTarget = - diff *.9f;
					torsoTwistTarget = - (diff / 2.0);
					headTwistTarget = (model->currentPoint != TPS_POINTTYPE_AIM ) ? - diff / 2.0: 0;
				}
				else
				{
					if(model->currentPose == TPS_POSETYPE_PRONE)
					{
						modelTwistTarget = 0; //-diff/2.0f;
						torsoTwistTarget = 0;
						headTwistTarget = 0; //-diff/2.0f;
					}
					else
					{
						modelTwistTarget = -diff * .9f;
						torsoTwistTarget = -diff / 2.0;
						headTwistTarget = (-diff / 2.0) * .5f;
					}
				}

				if (!model->HasFlag(gr2AnimTrooper::BACKWARDS))
				{
					gr2AnimSetModelState(object,
										 model->currentLOD,
										 model->currentPose,
										 TPS_MOVETYPE_BACKWARD,
										 model->currentPoint,
										 model->currentWeapon,
										 model->animState);
				}
			}
			break;
			case TPS_MOTION_B:
			{
				FLOAT64 adjust = 180.0f; //(lastMotion == TPS_MOTION_LFB || lastMotion == TPS_MOTION_LFB) ? 200.0f: 180.0f;

				if(lastMotion == TPS_MOTION_LFB)
					adjust = 200.0f;
				else if(lastMotion == TPS_MOTION_RFB)
					adjust = 200.0f;

				if (gr2AnimGunPointStraight(model))
				{
					modelTwistTarget = adjust + diff;
					torsoTwistTarget = adjust + diff;
					headTwistTarget = 0;
				}
				else
				{
					modelTwistTarget = adjust + diff;
					torsoTwistTarget = 0;
					headTwistTarget = adjust + diff;
				}
			}
			break;
		}

		//uiChatPrintLine("Current motion is %d, diff = %f", model->currentMotion, diff);

		if(model->currentPose == TPS_POSETYPE_PRONE)
			modelTwistRate = 100.0f;

		if(model->currentPoint == TPS_POINTTYPE_RELOAD)
		{
			headTwistTarget = 0;
			//torsoTwistTarget = 0;
		}

		if (model->headPoint == TPS_HEAD_LEFT)
		{
			headTwistTarget += 45.0;
		}
		else if (model->headPoint == TPS_HEAD_RIGHT)
		{
			headTwistTarget -= 45.0;
		}

		bool straighten = (model->currentMove == TPS_MOVETYPE_FALL || model->currentPose == TPS_POSETYPE_SIT);

		if(!straighten && model->myVehicle->IsMe() &&
		   (fpFPSIsTrooperParachuteOpen() || fpFPSIsTrooperFalling()))
		{
			straighten = true;
		}

		if(straighten)
		{
			modelTwistTarget = 0.0f;
			torsoTwistTarget = 0.0f;
			headTwistTarget = 0.0f;
		}
	}
	else // turn in place?
	{
		model->currentMotion = TPS_MOTION_F;

		// only twist the legs if turned
		headTwistTarget = 0;
		torsoTwistTarget = 0;
		modelTwistTarget = 0;

		bool leaning = (model->currentMove >= TPS_MOVETYPE_LEAN_LEFT && model->currentMove <= TPS_MOVETYPE_LEAN_RIGHT);

		if(model->currentPose != TPS_POSETYPE_PRONE && model->currentPose != TPS_POSETYPE_SIT && !leaning)
		{
			FLOAT64 torsoDiff, legsDiff;

			if(headYaw != yaw)
			{
				bool left = ((yaw - headYaw) < 0) ? false : true;

				FLOAT64 torsoThreshold	= 45.0f;
				FLOAT64 legsThreshold		= 45.0f;

				// adjust thresholds
				if(model->currentPose == TPS_POSETYPE_STAND)
				{
					if(model->currentPoint == TPS_POINTTYPE_AIM)
					{
						torsoThreshold = 20;
						legsThreshold = (left) ? 55 : 70;
					}
					else
					{
						torsoThreshold = (left) ? 50 : 65;
						legsThreshold = (left) ? 50 : 65;
					}
				}

				torsoThreshold = legsThreshold = 65.0f;

				if(model->currentPoint == TPS_POINTTYPE_AIM || model->currentMove == TPS_MOVETYPE_DEPLOYED)
				{
					torsoThreshold = 5;
					legsThreshold = 70.0f;
				}

				// direction we're facing
				headYaw = yaw;

				torsoDiff = headYaw - torsoYaw;
				gr2NormalizeAngle(torsoDiff);

				// WORKS TO THE LEFT
				// turn torso towards head?
				if(fabs(torsoDiff) > torsoThreshold)
				{
					if(left)
						torsoYaw += (torsoDiff > 0) ? torsoDiff - torsoThreshold : torsoThreshold - torsoDiff;
					else
						torsoYaw += (torsoDiff > 0) ? torsoThreshold - torsoDiff : torsoDiff + torsoThreshold;

					gr2NormalizeAngle(torsoYaw);
					//finalTorsoTwistSpeed = 10.0f;
				}

				legsDiff = torsoYaw - legsYaw;
				gr2NormalizeAngle(legsDiff);

				// turn torso towards torso?
				if(fabs(legsDiff) > legsThreshold)
				{
					model->turnInPlace = (frametime + 500) * ((legsDiff < 0) ? -1 : 1);
					legsYaw = headYaw;
					torsoYaw = headYaw;
				}
				else
				{
					modelTwistRate = 600.0f;
				}

				model->nextIdlePlayTime = frametime + 20000 + (SINT32)(mathGetFloat32ChaoticNumber() * 20000.0f);
			}

			if(model->firing || abs(frametime - model->myVehicle->LastFireTime) < 25)
			{
				torsoYaw = headYaw;
				torsoTwistRate = 600.0f;
			}

			fpTPSWeaponType tpsWeaponType = gr2AnimWeaponNumToTPSType (model, model->currentWeapon);

			if(model->headPoint != TPS_HEAD_STRAIGHT)
				torsoYaw = legsYaw = headYaw;
			else if(tpsWeaponType == TPS_WEAPONTYPE_BINOCS)
				torsoYaw = headYaw;

			if(frametime < abs(model->turnInPlace))
			{
				modelTwistRate = 300.0f;

				if(legsYaw != headYaw)
				{
					//model->turnInPlace = frametime + 100;
					torsoYaw = headYaw;
					legsYaw = headYaw;
				}
			}

			torsoDiff = headYaw - torsoYaw;
			legsDiff = headYaw - legsYaw;

			gr2NormalizeAngle(torsoDiff);
			gr2NormalizeAngle(legsDiff);

			modelTwistTarget = legsDiff;

			torsoTwistTarget = legsDiff - torsoDiff;
			gr2NormalizeAngle(torsoTwistTarget);

			headTwistTarget = torsoDiff;
		}
		else
		{
			FLOAT64 diff = headYaw - yaw;

			if(fabs(diff) >= .5f && !(model->firing || abs(frametime - model->myVehicle->LastFireTime) < 100))
				model->turnInPlace = (frametime + 250) * ((diff < 0) ? -1 : 1);

			legsYaw = yaw;
			torsoYaw = yaw;
			headYaw = yaw;
		}

		if (model->headPoint == TPS_HEAD_LEFT)
		{
			headTwistTarget += 45.0;
		}
		else if (model->headPoint == TPS_HEAD_RIGHT)
		{
			headTwistTarget -= 45.0;
		}
	}

	// calculate head pitch
	if(model->currentPoint != TPS_POINTTYPE_AIM)
	{
		const FLOAT32 DOWN_DEFLECTION = 20.0f;
		const FLOAT32 UP_DEFLECTION = 25.0f;

		if(model->upDownWeight >= .5f)
		{
			headPitchTarget = UP_DEFLECTION * ((model->upDownWeight - .5f) / .5f) * -1.0f;
		}
		else
		{
			headPitchTarget = DOWN_DEFLECTION * ((.5f - model->upDownWeight) / .5f);
		}
	}

#ifdef GR2_BLEND_EDITOR
	if(model == _editModel)
	{
		modelTwistTarget = 0.0f;
		torsoTwistTarget = 0.0f;
		headTwistTarget = 0.0f;
	}
#endif
}
*/

void gr2AnimTwist::Smooth(FLOAT64 delta, gr2AnimTrooper* model)
{
	FLOAT64 headPitchRate = 300.0f;

	// update rotations
	FLOAT64 move = Twist(modelTwistTarget, modelTwist, modelTwistRate, delta);
	move = Twist(torsoTwistTarget, torsoTwist, torsoTwistRate, delta, -move);
	Twist(headTwistTarget, headTwist, headTwistRate, delta, move);
	Twist(headPitchTarget, headPitch, headPitchRate, delta);
}

// probably should be a copy constructor
void gr2AnimTwist::Copy(gr2AnimTwist& t)
{
	predictedYaw		= t.predictedYaw;
	vehicleYaw			= t.vehicleYaw;
	headTwist			= t.headTwist;
	torsoTwist			= t.torsoTwist;
	modelTwist			= t.modelTwist;
	headPitch			= t.headPitch;
	headTwistTarget		= t.headTwistTarget;
	torsoTwistTarget	= t.torsoTwistTarget;
	modelTwistTarget	= t.modelTwistTarget;
	headPitchTarget		= t.headPitchTarget;
	headYaw				= t.headYaw;
	torsoYaw			= t.torsoYaw;
	legsYaw				= t.legsYaw;
	lastUpdate			= t.lastUpdate;
	headTwistRate		= t.headTwistRate;
	torsoTwistRate		= t.torsoTwistRate;
	modelTwistRate		= t.modelTwistRate;
	lastUpdate			= t.lastUpdate;
}

bool gr2AnimTwist::InLine() const
{
	return (fabs(legsYaw - headYaw) <= 10.0f && fabs(legsYaw - torsoYaw) <= 10.0f);
}

void gr2AnimTwist::Reset()
{
	predictedYaw = 0;
	vehicleYaw = 0;
	headTwist = 0;
	torsoTwist = 0;
	modelTwist = 0;
	headPitch = 0;
	headTwistTarget = 0;
	torsoTwistTarget = 0;
	modelTwistTarget = 0;
	headPitchTarget = 0;
	headYaw = 0;
	torsoYaw = 0;
	legsYaw = 0;
	lastUpdate = 0;
	lastChange = 0;
	headTwistRate = 0;
	torsoTwistRate = 0;
	modelTwistRate = 0;
	turning = false;
	facing = false;
	maxAngle = 0;
	headAngle = 0;
	yawRate = 0;
}

void gr2AnimTwist::Straighten(gr2AnimTrooper* model)
{
	vehicleYaw = predictedYaw;

	if(model->myVehicle->IsMe())
		model->myVehicle->ModelData->VehicleAtt.Yaw	= vehicleYaw;

	model->myVehicle->PosData.RenderAtt.Yaw	= vehicleYaw;
	model->myVehicle->PosData.Att.Yaw		= vehicleYaw;

	headTwist = 0;
	torsoTwist = 0;
	modelTwist = 0;
	headPitch = 0;
	headTwistTarget = 0;
	torsoTwistTarget = 0;
	modelTwistTarget = 0;
	headPitchTarget = 0;
	headYaw = 0;
	torsoYaw = 0;
	legsYaw = 0;
	maxAngle = 0;
	headAngle = 0;

	straighten = true;
}

void gr2AnimTwist::SetTwists(gr2Model* skeleton)
{
	if(!skeleton || !skeleton->twists.empty())
		return;

	// model rotation
	skeleton->AddBoneTwist(0,0);

	// torso rotation
	skeleton->AddBoneTwist(9,0);

	// head rotation
	skeleton->AddBoneTwist(18,0);

	// head pitch
	skeleton->AddBoneTwist(18,2);
}

void gr2AnimSound::Init(gr2AnimTrooper* anim)
{
	model = anim;
	pose = -1;
	move = -1;
	ID = -1;
	loopID = -1;
	name = nullptr;
	footOKTime = 0;
	footNextTime = 0;
	footEventType = ANIM_EVENT_LFOOT;
}

void gr2AnimSound::Update()
{
	if(!model)
		return;

	OBJECT* ob = model->myVehicle->VehicleObjectData->PlaneObject;
	bool playFootStep	= true;
	bool inRange		= model->InRange(8.0f);

	if(ob->flag2 & OBJECT_ISFPS)
	{
		if(fpsTrooper.wading)
		{
			if(loopID == -1)
			{
				loopID = fpFPSfxPlayNormalSound("sounds/troop/waterwalk.wav", IDE_FP_PRIORITY_MANDATORY, TYPE_INFINITE_SOUND, nullptr, ob);
				fxSndSetVolume(loopID, .75f);
			}

			return;
		}
		else
		{
			if(loopID >= 0)
				Stop();
		}
	}

	if(model->currentPose != TPS_POSETYPE_CROUCH && model->currentPose != TPS_POSETYPE_STAND)
		playFootStep = false;

	if(playFootStep && (!model->InMotion() || frametime < footNextTime))
		playFootStep = false;

	if(playFootStep)
	{
		// update
		if(inRange)
			gr2PlayFootstepSound(ob, model->skeleton, footEventType);

		FLOAT32 speed = fabs(model->modelSpeed);

		footNextTime = 0;
		footEventType = (footEventType == ANIM_EVENT_LFOOT) ? ANIM_EVENT_RFOOT : ANIM_EVENT_LFOOT;

		switch(model->currentMove)
		{
			case TPS_MOVETYPE_WALK:
			case TPS_MOVETYPE_TURN:
			{
				if(model->currentPose == TPS_POSETYPE_CROUCH)
					footNextTime = frametime + SINT32(400 * speed);
				else
					footNextTime = frametime + SINT32(500 * speed);
			}
			break;
			case TPS_MOVETYPE_FORWARD:
			case TPS_MOVETYPE_BACKWARD:
			{
				if(model->currentPose == TPS_POSETYPE_CROUCH)
					footNextTime = frametime + SINT32(400 * speed);
				else
					footNextTime = frametime + SINT32(400 * speed);
			}
			break;
			case TPS_MOVETYPE_STRAFE_LEFT:
			case TPS_MOVETYPE_STRAFE_RIGHT:
			{
				if(model->currentPose == TPS_POSETYPE_CROUCH)
					footNextTime = frametime + SINT32(600 * speed);
				else
					footNextTime = frametime + SINT32(400 * speed);
			}
			break;
			case TPS_MOVETYPE_SPRINT:
			{
				if(model->currentPose == TPS_POSETYPE_CROUCH)
					footNextTime = frametime + SINT32(400 * speed);
				else
					footNextTime = frametime + SINT32(350 * speed);
			}
			break;
            default:
                // leave defaults
                break;
		}
	}

	if(model->currentPose == pose && model->currentMove == move && ID > 0)
		return;

	fpTPSPoseType soundPose = model->currentPose;
	fpTPSMoveType soundMove = model->currentMove;
	FLOAT32	halfDistance = 0;
	FLOAT32 maxDistance = 0;

	const char* newSound = gr2AnimDetermineMovementSound(soundPose, soundMove, model->skeleton->surfaceID, &halfDistance, &maxDistance);

	move = model->currentMove;
	pose = model->currentPose;

	if(newSound != name)
	{
		Stop();

		if(newSound)
		{
			if(ob->flag2 & OBJECT_ISFPS)
			{
				ID = fpFPSfxPlayNormalSound((SCHAR*)newSound, IDE_FP_PRIORITY_MANDATORY, TYPE_INFINITE_SOUND, nullptr, ob);
			}
			else
            {
				ID = fpFPSfxPlayTPSMovementSound((SCHAR*)newSound, ob, halfDistance, maxDistance, model, gr2AnimSoundStoppedCallback);
            }
		}
	}

	name = (ID > 0) ? newSound : nullptr;
}

void gr2AnimSound::Stop()
{
	if(ID > 0)
	{
		SINT32 stopID = ID;
		ID = -1;
		fxSndStopSound(stopID);
	}

	if(loopID > 0)
	{
		SINT32 stopID = loopID;
		loopID = -1;
		fxSndStopSound(stopID);
	}

	name	= nullptr;
	ID		= -1;
	loopID	= -1;
}

#ifdef BETA_RELEASE
#if USE_STATESET
void _drawfpscollider()
{
	FPSColliderData& cd = fpFPSGetCollider();

	if(!cd.collider)
		return;

	colliderQuery::dGeomList& geoms = cd.collider->geoms();
	GLUquadric* obj					= gluNewQuadric();

    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetMode(GL_LIGHTING, 0);
        stateset->SetRenderState(new Color4F(colorYELLOWALPHA));
    }

    const mlVehicleData* player = persGetPersonaModelDataPtr();

	for(size_t i = 0; i < geoms.size(); i++)
	{
		dGeomID tg		= geoms.at(i);
		dGeomID g		= dGeomTransformGetGeom(tg);
		int klass		= dGeomGetClass(g);

		switch(klass)
		{
			case dRayClass:
			{
				UINT32 collideBits	= dGeomGetCollideBits(tg);
				UINT32 color		= colorWHITEALPHA;
				char name[32];

				switch(collideBits)
				{
					case FALL_COLLIDER:
					{
						if(!(gr2DrawFlags & (1<<7)))
							continue;

						color = colorGREENALPHA;
						snprintf(name, 31, "fpsfall%ld", i);
					}
					break;
					case CAGE_COLLIDER:
					{
						if(!(gr2DrawFlags & (1<<6)))
							continue;

						color = colorCYANALPHA;
						snprintf(name, 31, "fpscage%ld", i);
					}
					break;
				}

				dVector3 pos, dir;
				dGeomRayGet(g, pos, dir);

				double length = dGeomRayGetLength(g);

				Vec3d v1 = Vec3d(pos[0], pos[1], pos[2]);
				Vec3d v2 = v1;

				v2.x() += dir[0] * length;
				v2.y() += dir[1] * length;
				v2.z() += dir[2] * length;

				const dReal* tpos = dGeomGetPosition(tg);
				const dReal* trot = dGeomGetRotation(tg);

                Vec3d tp(tpos[0], tpos[1], tpos[2]);

                // move them to game world space
				tp = physicsPhysicalToGame(tp);

                Vec3f pos2((float)tp.x(), (float)tp.y(), (float)tp.z());

                pos2 = pos2 * Matrixf(player->VehicleAttMat);
                pos2 += Vec3f((float)player->VehiclePos.x, (float)player->VehiclePos.y, (float)player->VehiclePos.z);

                Matrixd transform;
				dMatrixToMatrix(trot, transform);
                transform.setTrans(pos2);

				// rotate them into physics world space
				v1 = v1 * transform;
				v2 = v2 * transform;

				// convert this data into what draw3d expects
				FLOAT64VECTOR start, end;

				v1.get(start);
				v2.get(end);

				start.z = -start.z;
				end.z = -end.z;

				// add or update the line
				draw3dSetWorldDebugLine(&start, &end, name, color);
			}
			break;
			case dCapsuleClass:
			case dCylinderClass:
			{
				if(!(gr2DrawFlags & (1<<5)))
					continue;

				gluQuadricOrientation(obj, GLU_OUTSIDE);

                stateset->Apply();

				const dReal* pos = dGeomGetPosition(g);
				const dReal* rot = dGeomGetRotation(g);

				Matrixd transform;
				dMatrixToMatrix(rot, transform);

				piMATRIX mat;
				transform.get(mat);

                oglSetWorldMatrix(&mat);

				double radius = 0.0f;
				double length = 0.0f;

				if(klass == dCapsuleClass)
					dGeomCapsuleGetParams(g, &radius, &length);
				else
					dGeomCylinderGetParams(g, &radius, &length);

				if(length < .01f)
					length = .01f;

				// draw the capsule
                mat = IdentityMatrix;
                mat._41 = (float)pos[0];
                mat._42 = (float)pos[1];
                mat._43 = (float)pos[2];

                oglSetWorldMatrix(&mat);

                gluCylinder(obj, radius, radius, length, 16, 16);

				if(klass == dCapsuleClass)
				{
					// draw the caps
					gluSphere(obj, radius, 16, 16);
					glTranslatef(0,0,(float)length);
					gluSphere(obj, radius, 16, 16);
				}
			}
			break;
			case dSphereClass:
			{
				if(!(gr2DrawFlags & (1<<5)))
					continue;

				gluQuadricOrientation(obj, GLU_OUTSIDE);

                stateset->Apply();

				const dReal* pos = dGeomGetPosition(g);
                Vec3f p((float)pos[0], (float)pos[1], (float)-pos[2]);

				double radius = dGeomSphereGetRadius(g);

                p = p * Matrixf(player->VehicleAttMat);

                Matrixf mat;

                mat(3, 0) = (float)(p.x() + player->VehiclePos.x - WorldOriginOffset.x);
                mat(3, 1) = (float)(p.y() + player->VehiclePos.y - WorldOriginOffset.y);
                mat(3, 2) = (float)(p.z() - player->VehiclePos.z);

                oglSetWorldMatrix(mat);

				// draw the capsule
				gluSphere(obj, radius, 16, 16);
			}
			break;
		}
	}

	gluDeleteQuadric(obj);
}
#else
void _drawfpscollider()
{
	FPSColliderData& cd = fpFPSGetCollider();

	if(!cd.collider)
		return;

	colliderQuery::dGeomList& geoms = cd.collider->geoms();
	GLUquadric* obj					= gluNewQuadric();

	for(size_t i = 0; i < geoms.size(); i++)
	{
		dGeomID tg		= geoms.at(i);
		dGeomID g		= dGeomTransformGetGeom(tg);
		int klass		= dGeomGetClass(g);

		switch(klass)
		{
			case dRayClass:
			{
				UINT32 collideBits	= dGeomGetCollideBits(tg);
				UINT32 color		= colorWHITEALPHA;
				char name[32];

				switch(collideBits)
				{
					case FALL_COLLIDER:
					{
						if(!(gr2DrawFlags & (1<<7)))
							continue;

						color = colorGREENALPHA;
						snprintf(name, 31, "fpsfall%ld", i);
					}
					break;
					case CAGE_COLLIDER:
					{
						if(!(gr2DrawFlags & (1<<6)))
							continue;

						color = colorCYANALPHA;
						snprintf(name, 31, "fpscage%ld", i);
					}
					break;
				}

				dVector3 pos, dir;
				dGeomRayGet(g, pos, dir);

				double length = dGeomRayGetLength(g);

				Vec3d v1 = Vec3d(pos[0], pos[1], pos[2]);
				Vec3d v2 = v1;

				v2.x() += dir[0] * length;
				v2.y() += dir[1] * length;
				v2.z() += dir[2] * length;

				const dReal* tpos = dGeomGetPosition(tg);
				const dReal* trot = dGeomGetRotation(tg);

				Matrixd transform;
				dMatrixToMatrix(trot, transform);
				transform.setTrans(tpos[0], tpos[1], tpos[2]);

				// rotate them into physics world space
				v1 = v1 * transform;
				v2 = v2 * transform;

				// move them to game world space
				v1 = physicsPhysicalToGame(v1);
				v2 = physicsPhysicalToGame(v2);

				// convert this data into what draw3d expects
				FLOAT64VECTOR start, end;

				v1.get(start);
				v2.get(end);

				start.z = -start.z;
				end.z = -end.z;

				// add or update the line
				draw3dSetWorldDebugLine(&start, &end, name, color);
			}
			break;
			case dCapsuleClass:
			case dCylinderClass:
			{
				if(!(gr2DrawFlags & (1<<5)))
					continue;

				gluQuadricOrientation(obj, GLU_OUTSIDE);
				piDisableTexture();
				oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
				oglSetRenderState(oglRENDERSTATE_COLOR4F, colorYELLOWALPHA);

				glPushMatrix();
				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glPushClientAttrib(GL_ALL_ATTRIB_BITS);

				const dReal* pos = dGeomGetPosition(g);
				const dReal* rot = dGeomGetRotation(g);

				Matrixd transform;
				dMatrixToMatrix(rot, transform);

				piMATRIX mat;
				transform.get(mat);

				glMultMatrixf((GLfloat*)&mat);

				double radius = 0.0f;
				double length = 0.0f;

				if(klass == dCapsuleClass)
					dGeomCapsuleGetParams(g, &radius, &length);
				else
					dGeomCylinderGetParams(g, &radius, &length);

				if(length < .01f)
					length = .01f;

				oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
				piDisableTexture();

				// draw the capsule
				glTranslatef((float)pos[0], (float)pos[1], (float)pos[2]);
				gluCylinder(obj, radius, radius, length, 16, 16);

				if(klass == dCapsuleClass)
				{
					// draw the caps
					gluSphere(obj, radius, 16, 16);
					glTranslatef(0,0,(float)length);
					gluSphere(obj, radius, 16, 16);
				}

				glPopClientAttrib();
				glPopAttrib();
				glPopMatrix();
			}
			break;
			case dSphereClass:
			{
				if(!(gr2DrawFlags & (1<<5)))
					continue;

				gluQuadricOrientation(obj, GLU_OUTSIDE);
				piDisableTexture();
				oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
				oglSetRenderState(oglRENDERSTATE_COLOR4F, colorYELLOWALPHA);

				glPushMatrix();
				glPushAttrib(GL_ALL_ATTRIB_BITS);
				glPushClientAttrib(GL_ALL_ATTRIB_BITS);

				const dReal* pos = dGeomGetPosition(g);

				oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
				piDisableTexture();

				double radius = dGeomSphereGetRadius(g);

				// draw the capsule
				glTranslatef((float)pos[0], (float)pos[1], (float)pos[2]);
				gluSphere(obj, radius, 16, 16);

				glPopClientAttrib();
				glPopAttrib();
				glPopMatrix();
			}
			break;
		}
	}

	gluDeleteQuadric(obj);
}
#endif // USE_STATESET
#endif // BETA_RELEASE

#ifdef GR2_BLEND_EDITOR
void initBlendEditor(phui::View* editor, gr2AnimTrooper* model)
{
	_editModel = model;
	_editModel->skeleton->ClearBlends();
	_editAnimSlider = editor->FindView("animTime");
	_editModelControl = editor->FindView("modelControl");
}

void updateBlendEditor()
{
	if(_editModel)
	{
		if(_editAnimSlider)
		{
			GR2EditBlend* selected = getSelectedBlend();

			if(selected)
			{
				FLOAT32 completed = gr2BlendControlCompletePercent(_editModel->skeleton, selected->blend);
				_editAnimSlider->SetValue((completed >= 0) ? completed: 1.0f);
			}
		}

		if(_rotateEditModel)
		{
			// handle rotations
			FLOAT32 pixelsMoved		= _editModelControl->X();
			FLOAT32 fullTurn		= 500;

			double amount			= (double)pixelsMoved / (double)fullTurn;
			double facing			= _editModelFacing + (360.0f * -amount);

			gr2NormalizeAngle(facing);

			_editModel->myVehicle->PosData.Att.Yaw = mlDEG_TO_RAD(facing);
			_editModel->twist.predictedYaw = _editModel->myVehicle->PosData.Att.Yaw;
			_editModel->twist.vehicleYaw = _editModel->myVehicle->PosData.Att.Yaw;

			// handle position
			pixelsMoved				= _editModelControl->Y();
			FLOAT32 pixelsPerMeter	= 100;

			vehVehicle* player	= persGetPersonaVehiclePtr();
			FLOAT64VECTOR vpos	= player->ModelData->VehiclePos;
			FLOAT64ATT vatt		= player->ModelData->VehicleAtt;

			vatt.Pitch = 0;
			vatt.Roll = 0;

			if(player->VehicleType->Type == VEH_TYPE_PERSON)
				vatt.Yaw = -vatt.Yaw;

			piVECTOR AttVec;
			piMATRIX AttMat = IdentityMatrix;

			AttVec.x = 0.0f;
			AttVec.y = 0.0f;
			AttVec.z = vatt.Yaw;

			mathEul_ToMatrix(&AttMat,EulOrdXYZs,&AttVec);

			// move us out in front
			Vec3d forward = Vec3d(0,1.0f,0);
			Matrixd rotate = Matrixd(AttMat);

			forward = forward * rotate;
			forward *= ((_editModelControl->Y()/pixelsPerMeter) * 1.2f);

			vpos.x += _editModelPos.x() + forward.x();
			vpos.y += _editModelPos.y() + forward.y();
			vpos.z = .1f; //+= _editModelPos.z() + forward.z();

			//if(player->VehicleType->Type == VEH_TYPE_FLOATER)
			//	vpos.z -= 1.0f;

			_editModel->myVehicle->PosData.Pos = vpos;
		}

		if(_editModelOscillation)
		{
			if(frametime >= _editModelOscillationExecute)
			{
				_editModelOscillationFirst = _editModelOscillationSecond;
				_editModelOscillationSecond++;

				if(_editModelOscillationSecond >= _editBlends.size())
					_editModelOscillationSecond = 0;

				_editModelOscillationExecute = frametime + _editModelOscillation;
			}
			else
			{
				FLOAT32 delta = (FLOAT32)(_editModelOscillationExecute - frametime) / (FLOAT32)_editModelOscillation;
				std::vector<GR2EditBlend>::iterator it = _editBlends.begin();

				for(int i = 0; it != _editBlends.end(); it++, i++)
				{
					if(i == _editModelOscillationFirst)
					{
						it->weight = delta;
					}
					else if(i == _editModelOscillationSecond)
					{
						it->weight = 1.0f - delta;
					}
					else
					{
						it->weight = 0.0f;
					}

					gr2BlendControlSetWeight(_editModel->skeleton, it->blend, it->weight);
				}
			}
		}

		for(int i = 0; i < _editBlends.size(); i++)
		{
			GR2EditBlend& eb = _editBlends.at(i);

			if(eb.active && (eb.selected || eb.play) && eb.weight && eb.blend->type == gr2AnimationBlend::TRANSITION)
			{
				FLOAT32 weight = _getTransitionWeight(_editModel, eb.blend);
				setEditBlendWeights(i, weight, 0, 0, 0);
			}
		}
	}
}

void killBlendEditor()
{
	if(_editModel)
		_editModel->skeleton->ClearBlends();

	_editAnimation = nullptr;
	_editWeapon = nullptr;
	_editWeaponType = 0;
	_editWeaponNum = 0;
	_editModel = nullptr;
	_editBlends.clear();

	_editAnimSlider = nullptr;
	_editModelControl = nullptr;
}

gr2AnimTrooper* getEditModel()
{
	return _editModel;
}

int addEditBlend(GR2EditBlend::TYPE stateType, GR2EditBlend::BLEND_STATES states)
{
	GR2EditBlend eb;
	SINT32 newIndex = -1;

	switch(stateType)
	{
		case GR2EditBlend::TPS:
		case GR2EditBlend::FPS:
		{
			if(states.size() < 4)
				return -1;

			eb.selected = false;
			eb.modified = false;
			eb.play		= false;
			eb.active	= true;
			eb.type		= stateType;
			eb.states	= states;
			eb.weight	= 1.0f;
			eb.mod1		= 0.0f;
			eb.mod2		= 1.0f;
			eb.blend	= fpTPSGetBlend(states[0], states[1], states[2], states[3], (int)stateType);

			if(!eb.blend)
			{
				const char* key = fpTPSCreateBlendKey(states[0], states[1], states[2], states[3], (int)stateType);
				eb.blend = gr2CreateBlendedAnimation(nullptr, _editModel->skeleton->GetBoneCount());

				const char* finalPtr = key;
				const char* ptr = strstr(key, "\\");

				while(ptr != nullptr)
				{
					finalPtr = ptr + 1;
					ptr = strstr(finalPtr, "\\");
				}

				strncpy(eb.blend->blendFile, finalPtr, 254);
				//fpTPSAnimLib[states[0]][states[1]][states[2]][states[3]] = eb.blend;

				TPS_BLEND_MAP& blends = fpTPSGetBlends();
				blends[fpTPSCreateBlendID(states[0], states[1], states[2], states[3], (int)stateType)] = eb.blend;
			}

			eb.weights.push_back(1.0f);
			eb.weights.push_back(0.0f);
			eb.weights.push_back(0.0f);
			eb.weights.push_back(0.0f);
		}
		break;
	}

	if(eb.blend)
	{
		_editBlends.push_back(eb);
		newIndex = _editBlends.size() - 1;
		updateEditBlends();
	}

	return newIndex;
}

void clearAnimCache()
{
	if(!_editModel)
		return;

	_editModel->skeleton->ClearBlends();

	int boneCount = _editModel->skeleton->GetBoneCount();

	TPS_BLEND_MAP& blends = fpTPSGetBlends();
	TPS_BLEND_MAP::iterator it = blends.begin();

	for(; it != blends.end(); it++)
	{
		/*
		gr2AnimationBlend* blend = it->second;

		for(int it = 0; i < GR2_MAX_ANIMS_PER_BLEND; i++)
		{
			if(blend->animations[i])
			{
				gr2Animation* anim = blend->animations[i];

				std::string path = (const char*)anim->name;

				blend->animations[i] = gr2LoadAnimation(path.c_str(), 0);

				gr2FreeAnimation(anim);
			}
		}
		*/
	}

	/*
	eb->blend->animations[eb->blend->numBlends] = gr2LoadAnimation((SCHAR*)filename, 1);

			if(eb->blend->animations[eb->blend->numBlends])
			{
				eb->blend->animations[eb->blend->numBlends]->mask = GrannyNewTrackMask((granny_real32)weight, boneCount);
				//eb->blend->numBlends++;

				// mark the first animation of the blend as the baseloop
				// NOTE: probably will need some more intelligence here
				if(eb->blend->numBlends == 0)
					eb->blend->animations[0]->animType = GR2_ANIMATION_BASELOOP;

				eb->blend->numBlends++;
			}
*/

	updateEditBlends();
}

bool validEditBlend(GR2EditBlend::TYPE stateType, GR2EditBlend::BLEND_STATES states)
{
	GR2EditBlend eb;

	switch(stateType)
	{
		case GR2EditBlend::TPS:
		case GR2EditBlend::FPS:
		{
			if(states.size() < 4)
				return -1;

			if(fpTPSGetBlend(states[0], states[1], states[2], states[3], (int)stateType))
				return true;
		}
		break;
	}

	return false;
}

const char* getEditBlendName(GR2EditBlend::TYPE stateType, GR2EditBlend::BLEND_STATES states)
{
	switch(stateType)
	{
		case GR2EditBlend::TPS:
		case GR2EditBlend::FPS:
		{
			if(states.size() < 4)
				return "";

			const char* key			= fpTPSCreateBlendKey(states[0], states[1], states[2], states[3], (int)stateType);
			const char* finalPtr	= key;
			const char* ptr = strstr(key, "\\");

			while(ptr != nullptr)
			{
				finalPtr = ptr + 1;
				ptr = strstr(finalPtr, "\\");
			}

			return finalPtr;
		}
		break;
	}

	return "";
}

void removeEditBlend(int index)
{
	std::vector<GR2EditBlend>::iterator it = _editBlends.begin();

	for(int i = 0; it != _editBlends.end(); it++, i++)
	{
		if(index == i)
		{
			// if modified save it

			_editBlends.erase(it);
			break;
		}
	}

	updateEditBlends();
}

void saveEditBlend(int index)
{
	if(index >= 0 && index < _editBlends.size())
	{
		// if modified save it
		GR2EditBlend& eb = _editBlends.at(index);
		eb.modified = false;

		// first save the blend out
		char line[1024];
		snprintf(line, 1023, "gr2Data\\TPS\\blends\\%s", eb.blend->blendFile);

		SINT32 fd = fioOpenWrite(line);

		if(fd >= 0)
		{
			snprintf(line, 1023, "type %d\n", eb.blend->type);
			fioWrite(fd, line, strlen(line));

			snprintf(line, 1023, "version 2\n");
			fioWrite(fd, line, strlen(line));

			for(int a = 0; a < eb.blend->numBlends; a++)
			{
				FLOAT32 defaultWeight = (a == 0) ? 1.0f: 0.0f;

				snprintf(line, 1023, "anim %s.GR2 %.1f\n", eb.blend->animations[a]->name, defaultWeight);
				fioWrite(fd, line, strlen(line));

				for(int b = 0; b < _editModel->skeleton->GetBoneCount(); b++)
				{
					FLOAT32 weight = (FLOAT32)GrannyGetTrackMaskBoneWeight(eb.blend->animations[a]->mask, b);

					if(weight != defaultWeight)
					{
						snprintf(line, 1023, "%d %.1f\n", b, weight);
						fioWrite(fd, line, strlen(line));
					}
				}

				snprintf(line, 1023, "end\n");
				fioWrite(fd, line, strlen(line));
			}

			fioClose(fd);
		}

		// then save any events out by animation
		for(int i = 0; i < eb.blend->numBlends; i++)
		{
			gr2Animation* anim = eb.blend->animations[i];

			if(!anim || !anim->events || anim->events->empty())
				continue;

			snprintf(line, 1023, "gr2Data\\TPS\\animations\\%s.evt", anim->name);

			SINT32 fd = fioOpenWrite(line);

			if(fd >= 0)
			{
				for(int e = 0; e < anim->events->size(); e++)
				{
					gr2Event* evt = anim->events->at(e);

					snprintf(line, 1023, "%d %.2f\n", evt->eventType, evt->eventTime);
					fioWrite(fd, line, strlen(line));
				}

				fioClose(fd);
			}
		}
	}
}

void reorderEditBlend(int index, int animation, int direction)
{
	GR2EditBlend* eb = getEditBlend(index);

	if(eb && eb->blend->numBlends > 1)
	{
		eb->modified = true;

		if(_editModel)
			_editModel->skeleton->ClearBlends();

		if(direction < 0) // up
		{
			if(animation > 0 && animation < eb->blend->numBlends)
			{
				gr2Animation* moved = eb->blend->animations[animation - 1];

				eb->blend->animations[animation - 1] = eb->blend->animations[animation];
				eb->blend->animations[animation] = moved;

				eb->modified = true;
			}
		}
		else // down
		{
			if(animation >= 0 && animation < eb->blend->numBlends - 1)
			{
				gr2Animation* moved = eb->blend->animations[animation + 1];

				eb->blend->animations[animation + 1] = eb->blend->animations[animation];
				eb->blend->animations[animation] = moved;

				eb->modified = true;
			}
		}
	}

	updateEditBlends();
}

void selectEditBlend(int index)
{
	for(int i = 0; i < _editBlends.size(); i++)
	{
		GR2EditBlend& eb = _editBlends.at(i);
		eb.selected = (index == i) ? true: false;
	}

	updateEditBlends();
}

void updateEditBlends()
{
	setEditModelOscillation(0);

	gr2AnimTrooper* et = getEditModel();

	if(et)
	{
		et->skeleton->ClearBlends();

		GR2EditBlend* selected = nullptr;

		for(int i = 0; i < _editBlends.size(); i++)
		{
			GR2EditBlend& eb = _editBlends.at(i);
			if(!eb.blend->numBlends) continue;

			if(eb.active && (eb.selected || eb.play))
			{
				_blendSpec.Set(et, (fpTPSWeaponType)_editWeaponType, eb.states[2]);

				_blendSpec.start	= true;
				_blendSpec.pose		= eb.states[0];
				_blendSpec.move		= eb.states[1];
				_blendSpec.weapon	= eb.states[2];
				_blendSpec.point	= eb.states[3];

				_setBlend(_blendSpec, (fpTPSBlendTypes)eb.type);

				// set our custom weighting
				setEditBlendWeights(i, eb.weights[0], eb.weights[1], eb.weights[2], eb.weights[3]);
				//gr2BlendControlSetWeight(et->skeleton, eb.blend, eb.weights[0], eb.weights[1], eb.weights[2], eb.weights[3]);

				// remember our selected blend
				if(eb.selected)
					selected = &eb;
			}

			//et->skeleton->PlayBlend(eb.blend);
			//et->skeleton->SetWeight(eb.blend, (eb.selected) ? eb.weight : 0.0f);

			//if(eb.selected)
			//	selected = &eb;
		}

		if(selected)
			setEditAnimation(nullptr);
	}
}

GR2EditBlend* getEditBlend(int index)
{
	if(index >= 0 && index < _editBlends.size())
		return &_editBlends.at(index);

	return nullptr;
}

GR2EditBlend* getSelectedBlend()
{
	for(int i = 0; i < _editBlends.size(); i++)
	{
		GR2EditBlend& eb = _editBlends.at(i);

		if(eb.selected)
			return &eb;
	}

	return nullptr;
}

int getEditBlendWeaponType()
{
	return _editWeaponType;
}

int getEditBlendWeaponNum()
{
	return _editWeaponNum;
}

int getEditWeaponTypeToNum(int type)
{
	fpsWeaponTypes weapType = NO_WEAPON;

	if(!_editModel)
		return weapType;

	int country = _editModel->myVehicle->VehicleType->WeaponSpec.country();

	switch(type)
	{
		case TPS_WEAPONTYPE_RIFLE:
		{
			weapType = (country == WWII_ENGLAND) ? ENFIELD : (country == WWII_FRANCE) ? MAS36 : (country == WWII_USA) ? M1GARAND : KAR98K;
		}
		break;
		case TPS_WEAPONTYPE_SMG:
		{
			weapType = (country == WWII_ENGLAND) ? THOMPSON : (country == WWII_FRANCE) ? MAS : (country == WWII_USA) ? M1THOMPSON : MP40;
		}
		break;
		case TPS_WEAPONTYPE_PISTOL:
		{
			weapType = (country == WWII_ENGLAND) ? WEBLEY : (country == WWII_FRANCE) ? M1935A : (country == WWII_USA) ? COLT1911 : LUGER;
		}
		break;
		case TPS_WEAPONTYPE_GRENADE:
		{
			weapType = (country == WWII_ENGLAND) ? MILLS36MK1 : (country == WWII_FRANCE) ? M18 : (country == WWII_USA) ? MARK2 : STIGR;
		}
		break;
		case TPS_WEAPONTYPE_BUILD:
		{
			weapType = BUILD_TOOL;
		}
		break;
		case TPS_WEAPONTYPE_SATCHEL:
		{
			weapType = SATCHEL;
		}
		break;
		case TPS_WEAPONTYPE_BINOCS:
		{
			weapType = BINOCS;
		}
		break;
		case TPS_WEAPONTYPE_KNIFE:
		{
			weapType = (country != WWII_GERMANY) ? KNIFE_ALLIED : KNIFE_AXIS;
		}
		break;
		case TPS_WEAPONTYPE_LMG:
		{
			weapType = (country == WWII_ENGLAND) ? BREN : (country == WWII_FRANCE) ? FM29 : (country == WWII_USA) ? BAR : MG34;
		}
		break;
		case TPS_WEAPONTYPE_ATR:
		{
			weapType = (country == WWII_ENGLAND) ? BOYSMK1 : (country == WWII_FRANCE) ? HGR39 : PZB39;
		}
		break;
		case TPS_WEAPONTYPE_MAP:
		{
			weapType = MAP;
		}
		break;
		case TPS_WEAPONTYPE_LTMORTAR:
		{
			weapType = (country == WWII_ENGLAND) ? LTMORTAR_UK : (country == WWII_FRANCE) ? LTMORTAR_FR : LTMORTAR_DE;
		}
		break;
		case TPS_WEAPONTYPE_ROCKET:
		{
			weapType = (country == WWII_ENGLAND) ? PIAT : (country == WWII_GERMANY) ? PZSCHRECK : BAZOOKA;
		}
		break;
	}

	return (int)weapType;
}

void setEditBlendWeapon(int type)
{
	// handle the current one
	if(_editWeapon)
	{
		// remove it from our model
		_editModel->skeleton->RemoveAttachment(_editWeapon);

		// free it
		gr2FreeModel(_editWeapon);

		// reset it
		_editWeapon = nullptr;
		_editWeaponType = 0;
		_editWeaponNum = 0;

	}

	// now add the new one
	if(_editModel && tpsWeapons[type].attachBone)
	{
		_editWeaponNum = getEditWeaponTypeToNum(type);

		if(_editWeaponNum != NO_WEAPON)
		{
			const char* path = (const char*)fpTPSWeaponMeshFiles[_editWeaponNum][TPS_LOD_1];

			_editWeapon = gr2Model::Load(path, nullptr, nullptr, 1);

			if(_editWeapon)
			{
				_editWeapon->Update(true, true);

				SINT32 from = _editWeapon->FindBone(tpsWeapons[type].attachBone);
				SINT32 to = _editModel->skeleton->FindBone(tpsWeapons[type].attachToBone);
				_editModel->skeleton->AddAttachment(_editWeapon, from, to, true);
				_editModel->skeleton->updateAttachment = true;
				_editWeapon->updateAttachment = true;
				_editWeaponType = type;
			}
			else
			{
				_editWeaponNum = NO_WEAPON;
			}
		}
	}
}

void setEditBlendWeights(int index, FLOAT32 weight1, FLOAT32 weight2, FLOAT32 weight3, FLOAT32 weight4)
{
	GR2EditBlend* eb = getEditBlend(index);

	if(eb)
	{
		/*
		if(_editModel && eb->blend->type == gr2AnimationBlend::TRANSITION)
		{
			weight2 = _getTransitionWeight(_editModel, eb->blend, weight1);
			weight1 = 1.0f - weight2;
		}
		*/

		eb->weights[0] = eb->weight * weight1;
		eb->weights[1] = eb->weight * weight2;
		eb->weights[2] = eb->weight * weight3;
		eb->weights[3] = eb->weight * weight4;

		if(_editModel && (eb->selected || eb->play))
			gr2BlendControlSetWeight(_editModel->skeleton, eb->blend, eb->weights[0], eb->weights[1], eb->weights[2], eb->weights[3]);
	}
}

void setEditAnimation(const char* path)
{
	if(!_editModel)
		return;

	if(_editAnimation)
	{
		gr2FreeControl(_editAnimation);
		_editAnimation = nullptr;
	}

	if(path)
		_editAnimation = _editModel->skeleton->PlayAnimation(gr2LoadAnimation((SCHAR*)path, 1));
}

void setRotateEditModel(bool on)
{
	_rotateEditModel = on;

	if(!on && _editModelControl)
		_editModelControl->MoveTo(0, 0);

	vehVehicle* player	= persGetPersonaVehiclePtr();

	Vec3d myPos			= Vec3d(player->ModelData->VehiclePos);
	Vec3d emPos			= Vec3d(_editModel->myVehicle->PosData.Pos);

	_editModelPos		= emPos - myPos;
	_editModelPos.z()	= 0.1f;
	_editModelFacing	= mlRAD_TO_DEG(_editModel->myVehicle->PosData.Att.Yaw);
}

void setEditModelOscillation(int duration)
{
	/*
	if(duration >= 1000)
	{
		// if we're oscillating, turn off the override animation
		setEditAnimation(nullptr);

		_editModelOscillationFirst		= 0;
		_editModelOscillationSecond		= 1;
		_editModelOscillation			= duration;
		_editModelOscillationExecute	= frametime + duration;

		for(int i = 0; i < _editBlends.size(); i++)
		{
			GR2EditBlend& eb = _editBlends.at(i);
			eb.weight = (i == 0) ? 1.0f : 0.0f;
		}
	}
	else
	{
		_editModelOscillation = 0;

		for(int i = 0; i < _editBlends.size(); i++)
		{
			GR2EditBlend& eb = _editBlends.at(i);
			eb.weight = (eb.selected || eb.play) ? 1.0f : 0.0f;
		}
	}
	*/
}


/************************************************************************************************
	AnimWeightView
*************************************************************************************************/

AnimWeightView::AnimWeightView()
{
	dragdead = dragfull = dragcontrol = false;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void AnimWeightView::DrawSelf()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->DisableTexture();

	GR2EditBlend* eb = getSelectedBlend();

	if(!eb || eb->blend->type != gr2AnimationBlend::TRANSITION)
		return;

	float deadband = 0.0f;
	float fullband = 1.0f;
	float x = 0.0f;
	float y = 0.0f;

	x = eb->blend->mod1;
	y = eb->blend->mod2;

	//float deadband, fullband, x, y;
	//unsigned int iac = iacCode(GetValue().String());
	//ww2KeymapGetJoysickCalibration(iac, &deadband, &fullband, &x, &y);

    float cx = x * Width();
    float cy = y * Height();

	float x1 = deadband * Width();
    float x2 = cx;
    float x3 = x2;
    float x4 = fullband * Width();

    float y1 = 0;
    float y2 = cy;
    float y3 = y2;
    float y4 = Height();

    float delta = 1.0f / 64.0f;
    float t = 0.0f;

	ctx->SetColor(1,1,1);
	glBegin(GL_LINE_STRIP);
    for(int i = 0; i <= 64; i++)
    {
        float x_ = (1 - t) * (1 - t) * (1 - t) * x1 +
			        3 * t * (1 - t) * (1 - t) * x2 +
			        3 * t * t * (1 - t) * x3 +
			        t * t * t * x4;

        float y_ = (1 - t) * (1 - t) * (1 - t) * y1 +
		            3 * t * (1 - t) * (1 - t) * y2 +
		            3 * t * t * (1 - t) * y3 +
		            t * t * t * y4;

		glVertex2f(x_,y_);

        t += delta;
    }
	glEnd();

	phui::BoxVerts box;
    phui::box_verts_with_pos(box,cx-5,cy-5,10,10);

	ctx->SetColor(1,0,0);
    ctx->DrawQuad(box.verts);

	/*
    ctx->SetColor(0,1,0);
    phui::box_verts_with_pos(box,x1-2,0,4,Height());
    ctx->DrawQuad(box.verts);
 	phui::box_verts_with_pos(box,x4-2,0,4,Height());
    ctx->DrawQuad(box.verts);
	*/
}
/************************************************************************************************
	MouseDrag
*************************************************************************************************/

bool AnimWeightView::MouseDrag (float dragx, float dragy, int button, bool start)
{
	GR2EditBlend* eb = getSelectedBlend();

	if(!eb || eb->blend->type != gr2AnimationBlend::TRANSITION)
		return false;

	float deadband = 0.0f;
	float fullband = 1.0f;
	float x = 0.0f;
	float y = 0.0f;

	x = eb->blend->mod1;
	y = eb->blend->mod2;

    float cx = x * Width();
    float cy = y * Height();

	float x1 = deadband * Width();
    float x4 = fullband * Width();

	if(start)
	{
		dragdead = dragfull = dragcontrol = false;

		/*
		if(dragx >= x4-10 && dragx <= x4+10)
		{
			dragfull = true;
			_dragx = dragx - x4;
		}

		if(dragx >= x1-10 && dragx <= x1+10)
		{
			dragdead = true;
			_dragx = dragx - x1;
		}
		*/

		if(dragx >= cx-10 && dragx <= cx+10 && dragy >= cy-10 && dragy <= cy+10)
		{
			dragcontrol = true;
			_dragx = dragx - cx;
			_dragy = dragy - cy;
		}
	}

	if(!dragdead && !dragfull && !dragcontrol)
		return true;

	if(dragcontrol)
	{
		cx = dragx - _dragx;
		cy = dragy - _dragy;

		x = cx / Width();
		y = cy / Height();
	}
	/*
	else if(dragdead)
	{
		x1 = dragx - _dragx;
		deadband = x1 / Width();
	}
	else if(dragfull)
	{
		x4 = dragx - _dragx;
		fullband = x4 / Width();
	}
	*/

	/*
	if(deadband > fullband - .1f) deadband = fullband - .1f;
	if(deadband < 0) deadband = 0;
	if(deadband > 1) deadband = 1;

	if(fullband < deadband + .1f) fullband = deadband + .1f;
	if(fullband < 0) fullband = 0;
	if(fullband > 1) fullband = 1;
	*/

	if(x < deadband + .05f) x = deadband + .05f;
	if(x > fullband - .05f) x = fullband - .05f;
	if(x < 0) x = 0;
	if(x > 1) x = 1;
	if(y < 0) y = 0;
	if(y > 1) y = 1;

	//ww2KeymapSetJoysickCalibration(iac, deadband, fullband, x, y);

	eb->blend->mod1 = x;
	eb->blend->mod2 = y;

	return true;
}

#endif
