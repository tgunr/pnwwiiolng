#pragma once

#include "granny.h"
#include "ogl/oglVertexBuffer.h"

#include <stack>

#define GR2_MEMORY_PAD 1
#define GR2_MEMORY_MULTIPLE 4

// rotation matrix crap
#define	AROUND_X		0
#define	AROUND_Y		1
#define	AROUND_Z		2

#define	GR2_MAX_BLENDS 1500
#define	MAX_GR2_BONE_COUNT			250
#define	MAX_GR2_FILECACHE_SIZE		1024
#define	MAX_GR2_PATH_SIZE			254
#define	GR2_MAX_BLENDS_PER_MODEL	100
#define	GR2_MAX_ANIMS_PER_BLEND		5
#define	GR2_MAX_TEXTURES_PER_MODEL	30
#define	GR2_USE_XML_BLENDS			0

#define GRANNY_EXTENSION			".gr2"
#define TPS_MAIN_DIRECTORY			"TPS\\"
#define TPS_ANIMATIONS_DIRECTORY	"animations\\"

#define GR2_ANIMATION_GENERIC		0
#define	GR2_ANIMATION_BASELOOP		1

#define	MAX_NUM_ANIM_EVENTS			50
#define MAX_EVENT_DATA				32

//#define GR2_TEXTURE_DEBUG
#define LOD_ANIMATIONS

enum ANIM_EVENTS
{
	ANIM_EVENT_LFOOT = 0,
	ANIM_EVENT_RFOOT,
	ANIM_EVENT_HIDE_MESH,
	ANIM_EVENT_SHOW_MESH,
	ANIM_EVENT_HIDE_MODEL,
	ANIM_EVENT_SHOW_MODEL,
	ANIM_EVENT_NUM_EVENTS
};

// forward declares for prototypes
struct vehVehicle;
struct OBJECT;
struct gr2Mesh;
struct gr2Model;
struct gr2Event;
struct gr2QueuedEvent;
struct gr2Control;
struct gr2ControlBlend;
struct gr2Animation;
struct gr2AnimationBlend;
class gr2Object;

using GR2_EVENTHANDLER = void(*)(OBJECT* pObject, gr2Model* model, SINT32 eventID);

// gr2 type definitions
#include "gr2/gr2layerTypes.h"

// load & possibly create an instance of this model
extern gr2Model* gr2LoadModel(const char* modelFile, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex = -1, bool wantNormals = true);
//extern gr2AnimModel* gr2LoadAnimModel(const char* modelFile, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex = -1);

// load animation
extern gr2Animation* gr2LoadAnimation(const SCHAR* animationFile, SINT32 useCache);

extern gr2AnimationBlend* gr2LoadBlendedAnimation(const SCHAR* blendfile, SINT32 boneCount);
extern gr2AnimationBlend* gr2CreateBlendedAnimation(const SCHAR* animFile, SINT32 boneCount);

// this lets me do the cleanup stage (GrannyFreeFileSection) for all of the open model files, getting some memory back
extern void			gr2DoneLoadingModels ();

// attach bone attachBone in model to attachToBone in attachToModel (ex:  attach gunbone in kar98k to handbone in arms model)
extern LOGICAL		gr2AttachModel (gr2Model* model, const SCHAR* attachBone, gr2Model* attachToModel, const SCHAR* attachToBone);
extern LOGICAL		gr2AttachModelByIndex (gr2Model* model, int attachBone, gr2Model* attachToModel, int attachToBone);
extern void			gr2DetachModel (gr2Model* model);
extern void			gr2UpdateAttachment(gr2Model* model);

extern gr2Control*	gr2PlayAnimation (gr2Model* model, gr2Animation* animation);
extern LOGICAL		gr2PlayBlendedAnimation (gr2Model* model, gr2AnimationBlend* animationBlend, bool allowResurrect = true);
extern LOGICAL		gr2PlayBlendedAnimationFull (gr2Model* model, gr2AnimationBlend* blend, FLOAT32 easeInTime, FLOAT32 easeOutTime, bool allowResurrect = true);
extern void			gr2AnimationSyncBaseLoop (gr2Model* model, gr2AnimationBlend* blend, gr2Control* gr2Control);

// blend controls
gr2ControlBlend*	gr2BlendControlFindRunningBlend (gr2Model* model, gr2AnimationBlend* animationBlend);
extern void			gr2BlendControlSetMixin (gr2Model* model, gr2AnimationBlend* animationBlend);
extern void			gr2BlendControlSetModifier(gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 weight, FLOAT32 modifier);
extern void			gr2BlendControlSetWeight (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 weight1, FLOAT32 weight2 = 0.0f, FLOAT32 weight3 = 0.0f, FLOAT32 weight4 = 0.0f);
extern void			gr2BlendControlSetSpeed (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 speed, bool reverseBaseOnly = true);
extern void			gr2BlendControlSetLastFrame (gr2Model* model, gr2AnimationBlend* animationBlend);
extern void			gr2BlendControlSetLoopCount (gr2Model* model, gr2AnimationBlend* animationBlend, SINT32 count);
extern void			gr2BlendControlSetLoopCountForGrenade (gr2Model* model, gr2AnimationBlend* animationBlend, SINT32 count);
extern void			gr2BlendControlSetEaseOutDuration (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time);
extern void			gr2BlendControlSetEaseInDuration (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time);
extern void			gr2BlendControlResurrect (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 maxEaseInTime);
extern FLOAT32		gr2BlendControlGetTime(gr2Model* model, gr2AnimationBlend* animationBlend);

// NOTE: gr2BlendControlRemoveWhenDone assumes you're calling it right as the animation is started
extern void			gr2BlendControlRemoveWhenDone(gr2Model* model, gr2AnimationBlend* animationBlend, CONTROL_CALLBACK c = nullptr, void* data = nullptr);
extern FLOAT32		gr2BlendGetBaseLength(gr2AnimationBlend* animationBlend);
extern FLOAT32		gr2BlendGetLength(gr2AnimationBlend* animationBlend);
extern FLOAT32		gr2BlendGetAnimLength(gr2AnimationBlend* animationBlend, UINT32 index = 0);
extern FLOAT32		gr2BlendGetAnimTime(gr2Model* model, gr2AnimationBlend* animationBlend, UINT32 index = 0);


// will need lots of control specific stuff here!!!
extern void			gr2ControlSetLastFrame (gr2Control* gr2Control);
extern void			gr2ControlSetLoopCount (gr2Control* gr2Control, SINT32 numLoops);
extern void			gr2ControlSetEaseInDuration (gr2Control* gr2Control, FLOAT32 time);
extern void			gr2ControlSetEaseOutDuration (gr2Control* gr2Control, FLOAT32 time);
extern void			gr2ControlSetSpeed (gr2Control* gr2Control, FLOAT32 speed);
extern void			gr2ControlSetWeight (gr2Control* gr2Control, FLOAT32 weight);
extern UINT32		gr2IsControlActive (gr2Control* gr2Control);
extern UINT32		gr2IsControlComplete (gr2Control* gr2Control);

extern LOGICAL		gr2RenderModel(gr2Model* model, bool update = false);
extern void			gr2RenderShadowModel(gr2Model* model, bool update = false);

#ifdef BETA_RELEASE
extern LOGICAL		gr2RenderDamageModel (gr2Model* currentModel);
extern void			gr2ToggleAnimOutput();
#endif

// stops the control from affecting anything
extern void			gr2StopControl (gr2Control* gr2Control, LOGICAL immediate);

// NOTE - these free the memory used by the* s!
extern void			gr2FreeMesh (gr2Mesh* mesh);
extern void			gr2FreeModel (gr2Model* model);
extern void			gr2FreeAnimation (gr2Animation* anim);
extern void			gr2FreeControl (gr2Control* control);
extern void			gr2FreeBlendedAnimation (gr2AnimationBlend* blend);
extern void			gr2FreeBlendedControl (gr2ControlBlend* blendControl);
extern void			gr2FreeQueuedEvent(gr2QueuedEvent* evt);
extern void			gr2UpdateModel(gr2Model* model, bool force = false, bool granular = true);
//extern void			gr2UpdateModel(gr2AnimModel* model, bool force = false, bool granular = true);

extern void			gr2Init ();
extern void			gr2Kill ();

extern void			gr2SetModelRotation (gr2Model* model, union piMATRIX* matrix);
extern void			gr2SetModelPosition (gr2Model* model, struct piVECTOR* position);
extern void			gr2GetModelPosition (gr2Model* model, struct piVECTOR* position);
extern void			gr2GetModelFinalTransform (gr2Model* model, union piMATRIX* matrix);
extern int			gr2GetBoneCount (gr2Model* model);

extern gr2Model*		gr2GetWornOnModel(gr2Model* model);
extern granny_skeleton*	gr2GetSkeleton(gr2Model* model);
extern granny_model_instance*	gr2GetInstance(gr2Model* model);
extern granny_world_pose*		gr2GetWorldPose(gr2Model* model);
extern void			gr2SetWorldPose(gr2Model* from, gr2Model* to);
extern const char*	gr2GetPath(gr2Model* model);

extern void			gr2SetModelActive (gr2Model* model, bool active);
extern LOGICAL		gr2IsModelActive (gr2Model* model);

extern void			gr2SetMeshActive (gr2Model* model, int mesh, bool active);
extern void			gr2SetMeshActiveByName (gr2Model* model, const char* mesh, bool active);
extern LOGICAL		gr2IsMeshActive (gr2Model* model, int mesh);
extern LOGICAL		gr2IsMeshActiveByName (gr2Model* model, const char* mesh);

extern granny_real32*	gr2GetBoneTransform (gr2Model* model, SCHAR* boneName);
extern granny_real32*	gr2GetBoneTransformByIndex (gr2Model* model, int boneIndex);
extern granny_real32*	gr2GetCompositeBoneTransform (gr2Model* model, SCHAR* boneName);
extern granny_real32*	gr2GetCompositeBoneTransformByIndex (gr2Model* model, int boneIndex);
extern LOGICAL		gr2GetBonePosition (gr2Model* model, const SCHAR* boneName, piMATRIX* matrix);
extern LOGICAL		gr2GetBonePositionByIndex (gr2Model* model, int boneIndex, piMATRIX* matrix);
extern LOGICAL		gr2GetRawBonePosition (gr2Model* model, SCHAR* boneName, piMATRIX* matrix);
extern LOGICAL		gr2ModelHasBone (gr2Model* model, SCHAR* boneName);
extern LOGICAL		gr2ModelHasMesh(gr2Model* model, const char* mesh, int* meshIndex, int* boneIndex);
extern int			gr2ModelFindBone (gr2Model* model, const SCHAR* boneName, LOGICAL exact = 1);
extern int			gr2ModelFindBones (std::vector<int>& found, gr2Model* model, SCHAR* boneName, LOGICAL exact = 1);

extern LOGICAL		gr2CollideRayWithModel (gr2Model* collisionModel, piVECTOR* rayStart, piVECTOR* rayNormal, SINT32* location, piVECTOR* collisionPnt);

extern void			gr2SetPositioningBone (gr2Model* model, SCHAR* boneName);

// note .. set easeOutOverrideTime to -1 if you want to use the default ease out time
extern LOGICAL		gr2RemoveAllBlendedAnimations (gr2Model* model, FLOAT32 easeOutOverrideTime, gr2AnimationBlend* blend = nullptr);
extern LOGICAL		gr2ClearBlendedAnimations (gr2Model* model);
extern LOGICAL		gr2BlendIsComplete (gr2Model* model, gr2AnimationBlend* animationBlend);
extern FLOAT32		gr2BlendControlCompletePercent (gr2Model* model, gr2AnimationBlend* animationBlend);
extern void			gr2BlendControlSetStartOffset (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 percent);
extern void			gr2BlendControlSetEndOffset (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 percent);
extern void			gr2BlendControlEaseIn (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time);
extern void			gr2BlendControlEaseOut (gr2Model* model, gr2AnimationBlend* animationBlend, FLOAT32 time);

#ifdef BETA_RELEASE
extern void			gr2ToggleColTest();
extern void			gr2MemStats(int& count, int& memSize);
#endif

extern void			gr2ModelPersonalize (gr2ModelAlt& model, SINT32 countryID, SINT32 branchID, SCHAR* handle, SINT32 decalID);
extern void			gr2ModelPersonalize (gr2Model* model, SINT32 countryID, SINT32 branchID, SCHAR* handle, SINT32 decalID);
extern void			gr2ModelFindAndReplaceTexture(gr2ModelAlt& model, SINT32 origTexID, UINT32 replaceTexID);
extern void			gr2ModelFindAndReplaceTexture (gr2Model* model, SINT32 origTexID, UINT32 replaceTexID);
extern SINT32*		gr2ModelGetLoadedTextures (gr2Model* model, SINT32* numTextures);

extern const char*	gr2GetEventData (gr2Model* model, SINT32 eventID);
extern SINT32		gr2GetEventType (gr2Model* model, SINT32 eventID);
extern void			gr2HandleEvent (gr2Model* model, SINT32 eventID);
extern void			gr2ModelSetEventHandler (gr2Model* model, OBJECT* pObject, GR2_EVENTHANDLER eventHandler);
extern void			gr2UpdateControlEvents(gr2Control* control);
extern void			gr2UpdateBlendEvents(gr2Model* model);
extern void			gr2UpdateEvents(gr2Model* model);

extern void			gr2ModelSetTwists (gr2Model* from, gr2Model* to);
extern void			gr2ModelSetTorsoRotation (gr2Model* model, FLOAT64 angle, LOGICAL useAltAxis);
extern void			gr2ModelSetHeadRotation (gr2Model* model, FLOAT64 angle);
extern void			gr2ModelSetHeadPitch (gr2Model* model, FLOAT64 angle);
extern void			gr2ModelSetModelPitch (gr2Model* model, FLOAT64 angle);
//extern void		gr2ModelSetFeetAdjustments(gr2Model* model, FLOAT64 l, FLOAT64 r);

extern FLOAT64		gr2ModelSetModelRotation (gr2Model* model, FLOAT64 angle);
extern FLOAT64		gr2ModelGetModelRotationDiff (gr2Model* model, FLOAT64 angle);
extern FLOAT64		gr2ModelGetTorsoRotationDiff (gr2Model* model, FLOAT64 angle);

extern void			gr2AddAxisRotationToBoneInLocalPose (granny_local_pose* pose, SINT32 boneNum, double axis[3], double angle);

extern const char*	gr2GetPathFromBlend(gr2AnimationBlend* blend);
extern FLOAT64		gr2NormalizeAngle(FLOAT64& angle);
extern gr2Model*	gr2SetGR2Object(gr2Model* model, gr2Object* object);

extern void			gr2DrawSkeleton(gr2Model* model, UINT32 color);

extern void			gr2ObjectHandleEvent(OBJECT* pObject, gr2Model* model, SINT32 eventID);
extern void gr2DrawTrooperBin();

#ifdef GR2_TEXTURE_DEBUG
extern std::map<std::string,std::string>& gr2GetTextures();
#endif

#ifdef GR2_BLEND_EDITOR
extern gr2Event* gr2GetNewEvent(gr2Animation* anim);
#endif
