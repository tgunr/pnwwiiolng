#pragma once

#include <map>
#include <string>
#include "fp/fpFPSdata.h"

enum fpTPSLODType
{
	TPS_LOD_1,
	TPS_LOD_2,
	TPS_LOD_3,
	TPS_LOD_4,
	TPS_LOD_5,
	TPS_NUM_LODS
};

enum fpTPSPoseType
{
	TPS_POSETYPE_STAND,
	TPS_POSETYPE_CROUCH,
	TPS_POSETYPE_PRONE,
	TPS_POSETYPE_SIT,
	TPS_POSETYPE_SWIM,
	TPS_POSETYPE_NUM_TYPES,
	TPS_POSETYPE_SHARED = TPS_POSETYPE_NUM_TYPES
};

extern const SCHAR* fpTPSPoseTypeStrings[TPS_POSETYPE_NUM_TYPES];
extern const SCHAR* fpTPSPoseKeyStrings[TPS_POSETYPE_NUM_TYPES];

enum fpTPSMoveType
{
	TPS_MOVETYPE_STILL,
	TPS_MOVETYPE_IDLE,
	TPS_MOVETYPE_WALK,
	TPS_MOVETYPE_FORWARD,
	TPS_MOVETYPE_SPRINT,
	TPS_MOVETYPE_BACKWARD,
	TPS_MOVETYPE_DEPLOYED,
	TPS_MOVETYPE_LEAN_LEFT,
	TPS_MOVETYPE_LEAN_RIGHT,
	TPS_MOVETYPE_EMOTE,
	TPS_MOVETYPE_TURN,
	TPS_MOVETYPE_FALL,
	TPS_MOVETYPE_STRAFE_LEFT,
	TPS_MOVETYPE_STRAFE_RIGHT,
	TPS_MOVETYPE_NUM_TYPES,
	TPS_MOVETYPE_SHARED = TPS_MOVETYPE_EMOTE
};

extern const SCHAR* fpTPSMoveTypeStrings[TPS_MOVETYPE_NUM_TYPES];
extern const SCHAR* fpTPSMoveKeyStrings[TPS_MOVETYPE_NUM_TYPES];

enum fpTPSWeaponType
{
	TPS_WEAPONTYPE_RIFLE,
	TPS_WEAPONTYPE_SMG,
	TPS_WEAPONTYPE_PISTOL,
	TPS_WEAPONTYPE_GRENADE,
	TPS_WEAPONTYPE_EMPTY,
	TPS_WEAPONTYPE_SATCHEL,
	TPS_WEAPONTYPE_BINOCS,
	TPS_WEAPONTYPE_KNIFE,
	TPS_WEAPONTYPE_LMG,
	TPS_WEAPONTYPE_ATR,
	TPS_WEAPONTYPE_MAP,
	TPS_WEAPONTYPE_LTMORTAR,
	TPS_WEAPONTYPE_BUILD,
	TPS_WEAPONTYPE_ROCKET,
	TPS_WEAPONTYPE_ANY,
	TPS_WEAPONTYPE_NUM_TYPES,
	TPS_WEAPONTYPE_SHARED = TPS_WEAPONTYPE_ANY
};

extern const SCHAR* fpTPSWeaponTypeStrings[TPS_WEAPONTYPE_NUM_TYPES];
extern const SCHAR* fpTPSWeaponKeyStrings[TPS_WEAPONTYPE_NUM_TYPES];

enum fpTPSPointType
{
	TPS_POINTTYPE_RELAXED,
	TPS_POINTTYPE_READY,
	TPS_POINTTYPE_AIM,
	TPS_POINTTYPE_RELOAD,
	TPS_POINTTYPE_ACTION1,
	TPS_POINTTYPE_ACTION2,
	TPS_POINTTYPE_ATTACK1,
	TPS_POINTTYPE_ATTACK2,
	TPS_POINTTYPE_STOW,
	TPS_POINTTYPE_TOPRONE,
	TPS_POINTTYPE_TOCROUCH,
	TPS_POINTTYPE_TOSTAND,
	TPS_POINTTYPE_MOTION,
	TPS_POINTTYPE_PARA,
	TPS_POINTTYPE_NUM_TYPES,
	TPS_POINTTYPE_SHARED = TPS_POINTTYPE_STOW
};

extern const SCHAR* fpTPSPointTypeStrings[TPS_POINTTYPE_NUM_TYPES];
extern const SCHAR* fpTPSPointKeyStrings[TPS_POINTTYPE_NUM_TYPES];

enum fpTPSMeshType
{
	TPS_MESH_INVALID = -1,
	TPS_MESH_INFANTRY = 0,
	TPS_MESH_WEAPON,
	TPS_MESH_WEAPON_ALT,
	TPS_MESH_NUM_TYPES
};

enum fpTPSBlendTypes
{
	TPS_BLENDTYPE_TPS,	// tps blend use tps weapon types
	TPS_BLENDTYPE_FPS,	// tps blend use fps weapon number
	TPS_BLENDTYPE_NUM_TYPES
};


// this is the subset required for proper mixin handling (probably will only ever be stand & prone)
enum fpTPSMixinPoseType
{
	TPS_MIXINPOSE_STAND,
	TPS_MIXINPOSE_PRONE,
	TPS_MIXINPOSE_NUM_TYPES
};

enum fpTPSEmoteType
{
	TPS_EMOTETYPE_NONE,
	TPS_EMOTETYPE_COMBAT,
	TPS_EMOTETYPE_FULLBODY,
	TPS_EMOTETYPE_NUM_TYPES
};

enum fpTPSCombatEmoteType
{
	TPS_COMBATEMOTE_DOUBLETIME,
	TPS_COMBATEMOTE_GETDOWN,
	TPS_COMBATEMOTE_HALT,
	TPS_COMBATEMOTE_MOVEOUT,
	TPS_COMBATEMOTE_AMMO,
	TPS_COMBATEMOTE_JUMP,
	TPS_COMBATEMOTE_NUM_TYPES
};

enum fpTPSBodyEmoteType
{
	TPS_BODYEMOTE_SALUTE_GERMAN,
	TPS_BODYEMOTE_SALUTE_FRENCH,
	TPS_BODYEMOTE_SALUTE_BRITISH,
	TPS_BODYEMOTE_SALUTE_DEFAULT,
	TPS_BODYEMOTE_SURRENDER,
	TPS_BODYEMOTE_CHEER,
	TPS_BODYEMOTE_LAUGH,
	TPS_BODYEMOTE_ATTENTION,
	TPS_BODYEMOTE_NUM_TYPES
};

enum fpTPSHandType
{
	TPS_HAND_LEFT,
	TPS_HAND_RIGHT,
	TPS_HAND_NUM_TYPES
};

enum fpTPSMotionType
{
	TPS_MOTION_F,			// forward
	TPS_MOTION_RFF,			// right from forward
	TPS_MOTION_RFB,			// right from backward
	TPS_MOTION_LFF,			// left from forward
	TPS_MOTION_LFB,			// left from backward
	TPS_MOTION_B,			// back
	TPS_MOTION_SR,			// strafe right
	TPS_MOTION_SL,			// strafe right
	TPS_MOTION_NUM_TYPES
};

using TPSType = std::map<std::string, int>;
using TPSTypes = std::map<std::string, TPSType>;

// these are the offsets so we can get the appropriate UI string from the loc file
#define			FPS_BODYEMOTE_OFFSET	10000
#define			FPS_COMBATEMOTE_OFFSET	10100
#define			TPS_BODYEMOTE_OFFSET	11000
#define			TPS_COMBATEMOTE_OFFSET	11100

#define			TPS_HEAD_STRAIGHT	0
#define			TPS_HEAD_LEFT		1
#define			TPS_HEAD_RIGHT		2

enum fpTPSDeathStates
{
	TPS_NOT_DEAD,
	TPS_DEATH1,
	TPS_DEATH2,
	TPS_DEATH3,
	TPS_DEATH4,
	TPS_INF_DEATH_MAX,
	TPS_NUM_INF_DEATHS = TPS_INF_DEATH_MAX-1,
	TPS_DEATH_PARA,
	TPS_NUM_DEATHS
};

enum fpTPSWeaponClass
{
	CLASS_ITEM,
	CLASS_FIREARM,
	CLASS_EXPLOSIVE
};

enum fpTPSAttackType
{
	ATTACK_NONE,
	ATTACK_SINGLESHOT,
	ATTACK_AUTOMATIC,
	ATTACK_THROWN,
	ATTACK_PLACED,
	ATTACK_MELEE,
	ATTACK_MORTAR
};

// used to be enough bits to hold TPS_NUM_INF_DEATHS
// now just signifies a death so we can ragdoll
#define TPS_NUM_DEATH_BITS 1

#define			TPS_NUM_DEATHSOUNDS	10

#define			TPS_EVENT_LFOOT				0
#define			TPS_EVENT_RFOOT				1
#define			TPS_MAX_NUM_EVENTS			2

#define			TPS_FOOTSTEP_LEFT			0
#define			TPS_FOOTSTEP_RIGHT			1
#define			TPS_MAX_NUM_FOOTSTEPS		2

#define			TPS_SURFACE_DEFAULT			0
#define			TPS_MAX_NUM_SURFACES		10

#define			TPS_MAX_NUM_IDLE_ANIMS		8

struct fpTPSWeaponData
{
	const SCHAR*			attachBone;
	const SCHAR*			attachToBone;
	fpTPSHandType	emoteHand;
	LOGICAL			forceWeaponDown;
};

struct fpTPSTroopData
{
	FLOAT32		toProneInTime;
	FLOAT32		toProneOutTime;
	FLOAT32		toProneWeight;

	FLOAT32		toStandInTime;
	FLOAT32		toStandOutTime;
	FLOAT32		toStandWeight;

	FLOAT32		emoteCombatMaxDistance;
	FLOAT32		emoteFullBodyMaxDistance;

	const SCHAR*		waterSound;
	FLOAT32		waterSoundHalfDistance;
	FLOAT32		waterSoundMaxDistance;

	FLOAT32		fireSnapUpTime, fireSnapDownTime, fireHoldTime;

	const SCHAR*		deathSounds[TPS_NUM_DEATHSOUNDS];
	FLOAT32		deathSoundFMin, deathSoundFMax;

};

struct fpTPSSurfaceData
{
	SCHAR*		fpsCrawlSound;
	SCHAR*		fpsCreepSound;
	SCHAR*		fpsCreepAimSound;
	SCHAR*		fpsTrotSound;
	SCHAR*		fpsTrotAimSound;
	SCHAR*		fpsSprintSound;

	SCHAR*		fpTPSLoopSounds[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES];
	FLOAT32		fpTPSLoopHalfDistance[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES];
	FLOAT32		fpTPSLoopMaxDistance[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES];

	SCHAR*		fpTPSFootstepSounds[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES][TPS_MAX_NUM_FOOTSTEPS];
	FLOAT32		fpTPSFootstepHalfDistance[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES];
	FLOAT32		fpTPSFootstepMaxDistance[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES];

};

// prototypes
void			fpTPSDataInit ();
const char*		fpTPSGetTypeNameFromValue(const char* type, int value);
int						fpTPSGetTypeIndexFromValue(const char* type, const char* name);

extern const SCHAR*				fpTPSDeathFiles[TPS_POSETYPE_NUM_TYPES][TPS_NUM_DEATHS];
extern const SCHAR*				fpTPSCombatEmoteFiles[TPS_COMBATEMOTE_NUM_TYPES][TPS_HAND_NUM_TYPES][TPS_MIXINPOSE_NUM_TYPES];
extern const SCHAR*				fpTPSBodyEmoteFiles[TPS_BODYEMOTE_NUM_TYPES];
extern const SCHAR*				fpTPSBasePoseFile;
extern const SCHAR*				fpTPSCombatBlendFile;

extern const SCHAR*				fpTPSSkeletonFile;
extern const SCHAR*				fpTPSDamageFile;
extern const SCHAR*				fpTPSInfantryMeshFiles[NUM_INFANTRY_TYPES][TPS_NUM_LODS];
extern const SCHAR*				fpTPSWeaponMeshFiles[NUM_WEAPONS][TPS_NUM_LODS];
extern const SCHAR*				fpTPSWeaponMeshAltFiles[NUM_WEAPONS][TPS_NUM_LODS];
extern const SCHAR*				fpTPSParachuteFile;

extern fpTPSWeaponData		tpsWeapons[TPS_WEAPONTYPE_NUM_TYPES];
extern fpTPSTroopData		tpsTroopData;

struct gr2AnimationBlend;
extern gr2AnimationBlend*	fpTPSBasePose;
extern gr2AnimationBlend*	fpTPSCombatBlend;
//extern gr2AnimationBlend*	fpTPSAnimLib[TPS_POSETYPE_NUM_TYPES][TPS_MOVETYPE_NUM_TYPES][TPS_WEAPONTYPE_NUM_TYPES][TPS_POINTTYPE_NUM_TYPES];
extern gr2AnimationBlend*	fpTPSDeathBlends[TPS_POSETYPE_NUM_TYPES][TPS_NUM_DEATHS];
extern gr2AnimationBlend*	fpTPSCombatEmotes[TPS_COMBATEMOTE_NUM_TYPES][TPS_HAND_NUM_TYPES][TPS_MIXINPOSE_NUM_TYPES];
extern gr2AnimationBlend*	fpTPSBodyEmotes[TPS_BODYEMOTE_NUM_TYPES];

extern SINT32						fpTPSNumIdleAnims[TPS_POSETYPE_NUM_TYPES][TPS_WEAPONTYPE_NUM_TYPES];

extern FLOAT32				fpTPSBodyEmoteLengths[TPS_BODYEMOTE_NUM_TYPES];

#define	MAX_TWITCHES_PER_POSE	5

extern SINT32				fpTPSNumTwitches[TPS_POSETYPE_NUM_TYPES];
extern const SCHAR*				fpTPSTwitchFiles[TPS_POSETYPE_NUM_TYPES][MAX_TWITCHES_PER_POSE];
extern struct gr2AnimationBlend*	fpTPSTwitchBlends[TPS_POSETYPE_NUM_TYPES][MAX_TWITCHES_PER_POSE];

extern const SCHAR*				fpTPSCombatEmoteCmds[TPS_COMBATEMOTE_NUM_TYPES];
extern const SCHAR*				fpTPSBodyEmoteCmds[TPS_BODYEMOTE_NUM_TYPES];

// convert from fps to tps weapon type
fpTPSWeaponType	fpTPSConvertWeaponTypes(fpFPSWeaponType type);
fpTPSAttackType fpTPSGetAttack(fpTPSWeaponType type);
fpTPSWeaponClass fpTPSConvertWeaponClass(fpTPSWeaponType type);
fpTPSBodyEmoteType fpTPSGetEmoteType(SINT32 emoteID);

#define	MAX_FACES				25
extern const SCHAR*				fpTPSDefaultFaceNames[NUM_INFANTRY_TYPES];

#ifdef USE_NEW_DECALS
extern SINT32				fpTPSPatchTextureIDs[NUM_INFANTRY_TYPES * 3];
extern const SCHAR*				fpTPSPatchTextureNames[NUM_INFANTRY_TYPES * 3];
#else
extern SINT32				fpTPSPatchTextureIDs[NUM_INFANTRY_TYPES];
extern const SCHAR*				fpTPSPatchTextureNames[NUM_INFANTRY_TYPES];
#endif

extern SINT32				fpTPSDefaultFaceTexIDs[NUM_INFANTRY_TYPES];
extern SINT32				fpTPSNumFaces;
extern SINT32				fpTPSFaceTexIDs[MAX_FACES];

// sounds
extern fpTPSSurfaceData		fpTPSSurfaces[TPS_MAX_NUM_SURFACES];

extern TPSTypes				tpsTypesMap;
