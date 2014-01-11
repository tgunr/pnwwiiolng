#include <syscfg.h> // Path is project specific.
#include <string>
#include <vector>
#include "gr2/gr2layer.h"
#include "gr2/gr2layerTypes.h"
#include "gr2/gr2layer.h"

/************ gr2Animation ***********/

gr2Animation* gr2Animation::Load(const char* path, SINT32 useCache)
{
	return nullptr;
}

/************ gr2Control ************/

void gr2Control::SetLastFrame()
{
	gr2ControlSetLastFrame(this);
}

void gr2Control::SetLoopCount(SINT32 numLoops)
{
	gr2ControlSetLoopCount(this, numLoops);
}

void gr2Control::SetEaseIn(FLOAT32 time)
{
	gr2ControlSetEaseInDuration(this, time);
}

void gr2Control::SetEaseOut(FLOAT32 time)
{
	gr2ControlSetEaseOutDuration(this, time);
}

void gr2Control::SetSpeed(FLOAT32 speed)
{
	gr2ControlSetSpeed(this, speed);
}

void gr2Control::SetWeight(FLOAT32 weight)
{
	gr2ControlSetWeight(this, weight);
}

UINT32 gr2Control::IsActive()
{
	return gr2IsControlActive(this);
}

UINT32 gr2Control::IsComplete()
{
	return gr2IsControlComplete(this);
}

void gr2Control::Stop(LOGICAL immediate)
{
	gr2StopControl(this, immediate);
}

void gr2Control::Update()
{
	gr2UpdateControlEvents(this);
}

/*********** gr2AnimationBlend ********/

gr2AnimationBlend* gr2AnimationBlend::Load(const char* path)
{
	return gr2LoadBlendedAnimation((SCHAR*)path, 58);
}

gr2AnimationBlend* gr2AnimationBlend::Create(const char* path)
{
	return gr2CreateBlendedAnimation((SCHAR*)path, 58);
}

FLOAT32 gr2AnimationBlend::GetBaseLength()
{
	return gr2BlendGetBaseLength(this);
}

FLOAT32 gr2AnimationBlend::GetLength(UINT32 index)
{
	return gr2BlendGetAnimLength(this, index);
}

FLOAT32 gr2AnimationBlend::GetTime(gr2Model* model, UINT32 index)
{
	return gr2BlendGetAnimTime(model, this, index);
}

const char*	gr2AnimationBlend::GetPath()
{
	return gr2GetPathFromBlend(this);
}

/************ gr2Model ************/

gr2Model::gr2Model()
	: grannyModel(nullptr)
	, GrannyInstance(nullptr)
	, WorldPose(nullptr)
	, lastUpdateTime(0)
	, lastDeformTime(0)
	, skeleton(nullptr)
	, boneCount(0)
	, usesAnotherSkeleton(0)
	, wornOnModel(nullptr)
	, attachBone(0)
	, attachToBone(0)
	, attachToModel(nullptr)
	, finalModelTransform(nullptr)
	, rootPositioningBone(0)
	, numTextures(0)
	, loadedTextures(nullptr)
	, currentFaceID(-1)
	, patchTexID(-1)
	, pObject(nullptr)
	, active(FALSE)
	, updateAttachment(1)
	, eventHandler()
	, blends(nullptr)
	, maxVis(0)
	, minVis(0)
	, surfaceID(-1)
{
}

gr2Model::~gr2Model()
{
    if(blends)
        delete blends;

    blends = nullptr;
	//reset();
}

void gr2Model::reset()
{
    if(finalModelTransform)
    {
        delete finalModelTransform;
        finalModelTransform = nullptr;
    }

    if(WorldPose)
    {
        GrannyFreeWorldPose(WorldPose);
        WorldPose = nullptr;
    }

    if(GrannyInstance)
    {
        GrannyFreeModelInstance(GrannyInstance);
        GrannyInstance = nullptr;
    }

    if(loadedTextures)
    {
        delete[] loadedTextures;
        loadedTextures = nullptr;
    }

	FreeAttachments();

	if(!meshes.empty())
	{
		for(unsigned int m = 0; m < meshes.size(); ++m)
			gr2FreeMesh(meshes[m]);
	}

	meshes.clear();

	if(blends)
	{
		if(!blends->empty())
		{
			GR2_BLEND_MAP::iterator it = blends->begin();

			for(; it != blends->end(); ++it)
			{
				//gr2FreeBlendedAnimation(it->first);
				gr2FreeBlendedControl(it->second);
			}
		}

		blends->clear();
	}

	if(!events.empty())
	{
		for(unsigned int e = 0; e < events.size(); ++e)
			gr2FreeQueuedEvent(events[e]);
	}

	twists.clear();
	events.clear();
	eventHandler = nullptr;

	piFreeMipTextures(patchTexID);

	//memset(this, 0, (int)((char*)&name - (char*)this));

	name = nullptr;
	currentFaceID = -1;
	patchTexID = -1;
	updateAttachment = 1; // always true initially
	grannyModel = nullptr;
	lastUpdateTime = 0;
#ifdef LOD_ANIMATIONS
	lodError = 0;
#endif

	// pointer to my own skeleton
	skeleton = nullptr;
	boneCount = 0;

	// wearing stuff
	usesAnotherSkeleton = 0;
	wornOnModel = attachToModel = nullptr;
	attachBone = attachToBone = -1;

	memset(&attachOffset, 0, sizeof(granny_matrix_4x4));

	// position stuff
	rootPositioningBone = 0;

	// texture data
	numTextures = 0;

	pObject = nullptr;
	object = nullptr;

	active = 0;

	minVis = 0;
	maxVis = 0;
	surfaceID = -1;

	updateAttachment = 1;
}

gr2Model* gr2Model::Load(const char* path, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex, bool wantNormals)
{
	return gr2LoadModel(path, wornOnModel, boneCount, instantiate, modelIndex, wantNormals);
}

LOGICAL gr2Model::Attach(gr2Model* toModel, SCHAR* toBone, SCHAR* fromBone)
{
	return gr2AttachModel(this, fromBone, toModel, toBone);
}

LOGICAL	gr2Model::Attach(gr2Model* toModel, SINT32 toBone, SINT32 fromBone)
{
	return gr2AttachModelByIndex(this, fromBone, toModel, toBone);
}

void gr2Model::Detach()
{
	gr2DetachModel(this);
}

void gr2Model::UpdateAttachment()
{
	gr2UpdateAttachment(this);
}

void gr2Model::FreeAttachments()
{
	for(size_t i = 0; i < attachments.size(); i++)
	{
		gr2Model* attached = attachments.at(i);
		attached->Detach();
		gr2FreeModel(attached);
	}

	attachments.clear();
}

void gr2Model::TransferAttachments(gr2Model* to)
{
	for(size_t i = 0; i < attachments.size(); i++)
	{
		gr2Model* attached = attachments.at(i);

		int attachFromBone	= attached->attachBone;
		int attachToBone	= attached->attachToBone;

		// detach from us first
		attached->Detach();

		// attach to incoming model
		to->AddAttachment(attached, attachFromBone, attachToBone, true);
	}

	// clear our attachments
	attachments.clear();
}

bool gr2Model::Render(bool update, bool attached)
{
	if(gr2RenderModel(this, update))
	{
		if(attached)
		{
			if(wornOnModel != nullptr)
				wornOnModel->RenderAttachments(update);
			else
				RenderAttachments(update);
		}

		return TRUE;
	}

	return FALSE;
}

void gr2Model::RenderAttachments(bool update)
{
	for(size_t i = 0; i < attachments.size(); i++)
		attachments.at(i)->Render(update);
}

void gr2Model::RenderShadow(bool update)
{
	gr2RenderShadowModel(this, update);
}

void gr2Model::RenderDamage(bool update)
{
#ifdef BETA_RELEASE
	gr2RenderDamageModel(this);
#endif
}

void gr2Model::RenderSkeleton()
{
}

void gr2Model::MakeMutable()
{
	// let our meshes manage a local copy of the vertex data
	for(size_t i = 0; i < meshes.size(); i++)
	{
		gr2Mesh* mesh = meshes.at(i);

		if(!mesh->data->rigidMesh)
			mesh->mutate = TRUE;
	}
}

void gr2Model::Update(bool force, bool granular)
{
	gr2UpdateModel(this, force, granular);

	for(size_t i = 0; i < attachments.size(); i++)
		attachments.at(i)->Update(false, false);
}

void gr2Model::SetRotation(piMATRIX* rotation)
{
	gr2SetModelRotation(this, rotation);
}

void gr2Model::SetPosition(piVECTOR* position)
{
	gr2SetModelPosition(this, position);
}

void gr2Model::GetPosition(piVECTOR* position)
{
	gr2GetModelPosition(this, position);
}

void gr2Model::GetFinalTransform(piMATRIX* transform)
{
	gr2GetModelFinalTransform(this, transform);
}

SINT32 gr2Model::GetBoneCount()
{
	return gr2GetBoneCount(this);
}

gr2Model* gr2Model::GetWornOnModel()
{
	return gr2GetWornOnModel(this);
}

granny_skeleton* gr2Model::GetSkeleton()
{
	return gr2GetSkeleton(this);
}

granny_model_instance* gr2Model::GetInstance()
{
	return gr2GetInstance(this);
}

granny_world_pose* gr2Model::GetWorldPose()
{
	return gr2GetWorldPose(this);
}

void gr2Model::CopyWorldPose(gr2Model* to)
{
	gr2SetWorldPose(this, to);
}

const char*	gr2Model::GetPath()
{
	return gr2GetPath(this);
}

void gr2Model::SetActive(bool active)
{
	gr2SetModelActive(this, active);
}

LOGICAL gr2Model::IsActive()
{
	return gr2IsModelActive(this);
}

void gr2Model::SetMeshActive(int mesh, bool active)
{
	gr2SetMeshActive(this, mesh, active);
}

void gr2Model::SetMeshActive(const char* mesh, bool active)
{
	gr2SetMeshActiveByName(this, mesh, active);
}

LOGICAL gr2Model::IsMeshActive(int mesh)
{
	return gr2IsMeshActive(this, mesh);
}

LOGICAL gr2Model::IsMeshActive(const char* mesh)
{
	return gr2IsMeshActiveByName(this, mesh);
}

granny_real32* gr2Model::GetBoneTransform(SCHAR* boneName)
{
	return gr2GetBoneTransform(this, boneName);
}
granny_real32* gr2Model::GetBoneTransform(int boneIndex)
{
	return gr2GetBoneTransformByIndex(this, boneIndex);
}

granny_real32* gr2Model::GetCompositeBoneTransform(SCHAR* boneName)
{
	return gr2GetCompositeBoneTransform(this, boneName);
}

granny_real32* gr2Model::GetCompositeBoneTransform(int boneIndex)
{
	return gr2GetCompositeBoneTransformByIndex(this, boneIndex);
}

LOGICAL gr2Model::GetBonePosition(SCHAR* boneName, piMATRIX* matrix)
{
	return gr2GetBonePosition(this, boneName, matrix);
}

LOGICAL gr2Model::GetBonePosition(int boneIndex, piMATRIX* matrix)
{
	return gr2GetBonePositionByIndex(this, boneIndex, matrix);
}

LOGICAL gr2Model::GetRawBonePosition(SCHAR* boneName, piMATRIX* matrix)
{
	return gr2GetRawBonePosition(this, boneName, matrix);
}

LOGICAL	gr2Model::HasBone(SCHAR* boneName)
{
	return gr2ModelHasBone(this, boneName);
}

LOGICAL gr2Model::HasMesh(const char* mesh, int* meshIndex, int* boneIndex)
{
	return gr2ModelHasMesh(this, mesh, meshIndex, boneIndex);
}

SINT32 gr2Model::FindBone(SCHAR* boneName, LOGICAL exact)
{
	return gr2ModelFindBone(this, boneName, exact);
}

SINT32 gr2Model::FindBones(std::vector<int>& found, SCHAR* boneName, LOGICAL exact)
{
	return gr2ModelFindBones(found, this, boneName, exact);
}

LOGICAL gr2Model::CollideRay(piVECTOR* rayStart, piVECTOR* rayNormal, SINT32* location, piVECTOR* collisionPnt)
{
	return gr2CollideRayWithModel(this, rayStart, rayNormal, location, collisionPnt);
}

void gr2Model::SetPositioningBone(SCHAR* boneName)
{
	gr2SetPositioningBone(this, boneName);
}

void gr2Model::Personalize(SINT32 countryID, SINT32 branchID, SCHAR* handle, SINT32 decalID)
{
	gr2ModelPersonalize(this, countryID, branchID, handle, decalID);
}

void gr2Model::FindAndReplaceTexture(SINT32 origTexID, UINT32 replaceTexID)
{
	gr2ModelFindAndReplaceTexture(this, origTexID, replaceTexID);
}

SINT32* gr2Model::GetLoadedTextures(SINT32* numTextures)
{
	return gr2ModelGetLoadedTextures(this, numTextures);
}


/************ gr2AnimModel ************/
/*
gr2AnimModel::gr2AnimModel() : gr2Model()
	, eventHandler()
	, torsoTwist(0)
	, headTwist(0)
	, modelTwist(0)
	, headPitch(0)
	, useAltTorsoAxis(FALSE)
	, blends(nullptr)
{
}

gr2AnimModel::~gr2AnimModel()
{
	//reset();

	if(blends)
		delete blends;
}

void gr2AnimModel::reset()
{
	gr2Model::reset();

	if(blends)
	{
		if(!blends->empty())
		{
			GR2_BLEND_MAP::iterator it = blends->begin();

			for(; it != blends->end(); ++it)
			{
				//gr2FreeBlendedAnimation(it->first);
				gr2FreeBlendedControl(it->second);
			}
		}

		blends->clear();
	}

	if(!events.empty())
	{
		for(unsigned int e = 0; e < events.size(); ++e)
			gr2FreeQueuedEvent(events[e]);
	}

	events.clear();
	eventHandler = nullptr;

	// torso twist
	torsoTwist = headTwist = modelTwist = headPitch = 0;
	useAltTorsoAxis = 0;
}

gr2AnimModel* gr2AnimModel::Load(const char* path, gr2Model* wornOnModel, SINT32* boneCount, SINT32 instantiate, int modelIndex)
{
	return gr2LoadAnimModel(path, wornOnModel, boneCount, instantiate, modelIndex);
}
*/

gr2Control*	gr2Model::PlayAnimation(gr2Animation* animation)
{
	return gr2PlayAnimation(this, animation);
}

void gr2Model::StopAnimation(gr2Control* control)
{
	gr2FreeControl(control);
}

LOGICAL	gr2Model::PlayBlend(gr2AnimationBlend* animationBlend, bool allowResurrect)
{
	return gr2PlayBlendedAnimation(this, animationBlend, allowResurrect);
}

LOGICAL	gr2Model::PlayBlend(gr2AnimationBlend* blend, FLOAT32 easeInTime, FLOAT32 easeOutTime, bool allowResurrect)
{
	return gr2PlayBlendedAnimationFull(this, blend, easeInTime, easeOutTime, allowResurrect);
}

void gr2Model::SyncBaseLoop(gr2AnimationBlend* blend, gr2Control* control)
{
	gr2AnimationSyncBaseLoop(this, blend, control);
}

void gr2Model::SetMixin(gr2AnimationBlend* animationBlend)
{
	gr2BlendControlSetMixin(this, animationBlend);
}

void gr2Model::SetWeight(gr2AnimationBlend* animationBlend, FLOAT32 weight)
{
	gr2BlendControlSetWeight(this, animationBlend, weight);
}

void gr2Model::SetSpeed(gr2AnimationBlend* animationBlend, FLOAT32 speed)
{
	gr2BlendControlSetSpeed(this, animationBlend, speed);
}

void gr2Model::SetLastFrame(gr2AnimationBlend* animationBlend)
{
	gr2BlendControlSetLastFrame(this, animationBlend);
}

void gr2Model::SetLoopCount(gr2AnimationBlend* animationBlend, SINT32 count)
{
	gr2BlendControlSetLoopCount(this, animationBlend, count);
}

void gr2Model::SetLoopCountForGrenade(gr2AnimationBlend* animationBlend, SINT32 count)
{
	gr2BlendControlSetLoopCountForGrenade(this, animationBlend, count);
}

void gr2Model::SetEaseOut(gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2BlendControlSetEaseOutDuration(this, animationBlend, time);
}

void gr2Model::SetEaseIn(gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2BlendControlSetEaseInDuration(this, animationBlend, time);
}

void gr2Model::ResurrectBlend(gr2AnimationBlend* animationBlend, FLOAT32 maxEaseInTime)
{
	gr2BlendControlResurrect(this, animationBlend, maxEaseInTime);
}

void gr2Model::RemoveWhenDone(gr2AnimationBlend* animationBlend)
{
	gr2BlendControlRemoveWhenDone(this, animationBlend);
}

/*
void gr2AnimModel::Update(bool force, bool granular)
{
	gr2UpdateModel(this, force, granular);
}
*/

// note .. set easeOutOverrideTime to -1 if you want to use the default ease out time
LOGICAL gr2Model::RemoveAllBlends(FLOAT32 easeOutOverrideTime, gr2AnimationBlend* blend)
{
	return gr2RemoveAllBlendedAnimations(this, easeOutOverrideTime, blend);
}

LOGICAL gr2Model::ClearBlends()
{
	return gr2ClearBlendedAnimations(this);
}

LOGICAL gr2Model::IsBlendComplete(gr2AnimationBlend* animationBlend)
{
	return gr2BlendIsComplete(this, animationBlend);
}

FLOAT32	gr2Model::GetBlendCompletion(gr2AnimationBlend* animationBlend)
{
	return gr2BlendControlCompletePercent(this, animationBlend);
}

void gr2Model::SetStartOffset(gr2AnimationBlend* animationBlend, FLOAT32 percent)
{
	gr2BlendControlSetStartOffset(this, animationBlend, percent);
}

void gr2Model::EaseIn(gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2BlendControlEaseIn(this, animationBlend, time);
}

void gr2Model::EaseOut(gr2AnimationBlend* animationBlend, FLOAT32 time)
{
	gr2BlendControlEaseOut(this, animationBlend, time);
}

const char*	gr2Model::GetEventData(SINT32 eventID)
{
	return gr2GetEventData(this, eventID);
}

SINT32 gr2Model::GetEventType(SINT32 eventID)
{
	return gr2GetEventType(this, eventID);
}

void gr2Model::HandleEvent(SINT32 eventID)
{
	gr2HandleEvent(this, eventID);
}

void gr2Model::SetEventHandler(OBJECT* pObject, GR2_EVENTHANDLER eventHandler)
{
	gr2ModelSetEventHandler(this, pObject, eventHandler);
}

void gr2Model::UpdateBlendEvents()
{
	gr2UpdateBlendEvents(this);
}

void gr2Model::UpdateEvents()
{
	gr2UpdateEvents(this);
}

void gr2Model::CopyTwists(gr2Model* to)
{
	gr2ModelSetTwists(this, to);
}

void gr2Model::SetTorsoRotation(FLOAT64 angle, LOGICAL useAltAxis)
{
	gr2ModelSetTorsoRotation(this, angle, useAltAxis);
}

void gr2Model::SetHeadRotation(FLOAT64 angle)
{
	gr2ModelSetHeadRotation(this, angle);
}

void gr2Model::SetHeadPitch(FLOAT64 angle)
{
	gr2ModelSetHeadPitch(this, angle);
}

void gr2Model::SetModelPitch(FLOAT64 angle)
{
	gr2ModelSetModelPitch(this, angle);
}

FLOAT64 gr2Model::SetModelRotation(FLOAT64 angle)
{
	return gr2ModelSetModelRotation(this, angle);
}

FLOAT64 gr2Model::GetModelRotationDiff(FLOAT64 angle)
{
	return gr2ModelGetModelRotationDiff(this, angle);
}

FLOAT64 gr2Model::GetTorsoRotationDiff(FLOAT64 angle)
{
	return gr2ModelGetTorsoRotationDiff(this, angle);
}

void gr2Model::SetBoneTwist(UINT32 bone, UINT32 axis, FLOAT64 angle)
{
	for(size_t t = 0; t < twists.size(); t++)
	{
		if(twists.at(t).bone == bone && twists.at(t).axis == axis)
		{
			twists.at(t).angle = angle;
			return;
		}
	}

	AddBoneTwist(bone, axis, angle);
}

FLOAT64& gr2Model::AddBoneTwist(UINT32 bone, UINT32 axis, FLOAT64 angle)
{
	twists.push_back(gr2BoneTwist(bone, axis, angle));
	return twists.at(twists.size() - 1).angle;
}

void gr2Model::AddBoneAdjust(UINT32 bone, Vec3f& adjustment, SINT32 duration)
{
	adjustments.push_back(gr2BoneAdjust(bone, adjustment, duration));
}

// returns if we successfully attached it
// store indicates whether this model should manage the attachment
bool gr2Model::AddAttachment(gr2Model* attachModel, int attachBone, int parentBone, bool store)
{
	if(attachModel->Attach(this, parentBone, attachBone))
	{
		if(store)
			attachments.push_back(attachModel);

		// make sure our matrix gets updated
		gr2UpdateAttachment(attachModel);

		return true;
	}

	return false;
}

// returns if it was on the attachments list and removed
bool gr2Model::RemoveAttachment(gr2Model* attachment)
{
	// make sure we're detached
	attachment->Detach();

	// clear us out of the attachments list
	GR2_MODEL_LIST::iterator it = attachments.begin();

	for(; it != attachments.end(); ++it)
	{
		if(attachment == *it)
		{
			attachments.erase(it);
			return true;
		}
	}

	return false;
}

gr2ModelList::gr2ModelList()
{
}

gr2ModelList::~gr2ModelList()
{
	Free();
}

void gr2ModelList::Free()
{
	for(size_t i = 0; i < size(); ++i)
		gr2FreeModel(at(i));

	clear();
}

gr2Model* gr2ModelList::AddModel(gr2Model* model)
{
	push_back(model);
	return model;
}

gr2Model* gr2ModelList::GetActive()
{
	// return the first active found
	for(size_t i = 0; i < size(); ++i)
	{
		GR2_LOD& lod = at(i);

		if(lod->active)
			return lod;
	}

	return nullptr;
}

gr2Model* gr2ModelList::Get(SINT32 index)
{
	if(index >= 0 && index < (int)size())
		return at(index);

	return nullptr;
}

gr2Model* gr2ModelList::Get(FLOAT32 distance)
{
	for(size_t i = 0; i < size(); ++i)
	{
		GR2_LOD& lod = at(i);

		if(!lod->maxVis || lod->maxVis >= distance)
			return lod;
	}

	return nullptr;
}

void gr2ModelList::Set(SINT32 index, bool active)
{
	if(index >= 0 && index < (int)size())
		at(index)->active = active;
}

void gr2ModelList::UpdateLODS(UINT32 distance, UINT32 realDistance)
{
	for(size_t i = 0; i < size(); ++i)
	{
		GR2_LOD& lod = at(i);
		lod->active = (!lod->maxVis || lod->maxVis >= distance);
	}
}

void gr2ModelList::Update(bool force, bool granular)
{
	for(size_t i = 0; i < size(); ++i)
	{
		GR2_LOD& lod = at(i);

		if(lod->active)
			lod->Update(force, granular);
	}
}

bool gr2ModelList::Render(bool update)
{
	bool rendered = false;

	for(size_t i = 0; i < size(); ++i)
	{
		GR2_LOD& lod = at(i);

		if(lod->active && lod->Render(update))
			rendered = true;
	}

	return rendered;
}

gr2ModelLOD::gr2ModelLOD() : gr2ModelList(), _active(0)
{
}

gr2Model* gr2ModelLOD::GetActive()
{
	return Get(_active);
}

void gr2ModelLOD::Set(SINT32 index, bool active)
{
	gr2Model* model = Get(index);

	if(!model)
		return;

	if(active)
	{
		if(_active != index)
		{
			gr2Model* currentActive = Get(_active);

			if(currentActive)
				currentActive->active = false;

			_active = index;
		}
	}
	else
	{
		if(_active == index)
		{
			gr2Model* currentActive = Get(_active);

			if(currentActive)
				currentActive->active = false;

			_active = -1;
		}
	}

	model->active = active;
}

void gr2ModelLOD::UpdateLODS(UINT32 distance, UINT32 realDistance)
{
	_active = -1;

	// find our closest lod within the calculated distance
	for(size_t i = 0; i < size(); ++i)
	{
		GR2_LOD& lod = at(i);

		if(!lod->maxVis || lod->maxVis >= distance)
		{
			_active = i;
			break;
		}
	}

	gr2Model* active = GetActive();

	// if that lod has a min vis, back the lod down until we find an lod that matches our real distance
	if(_active > 0 && active && realDistance < active->minVis)
	{
		for(int i = _active - 1; i >= 0; --i)
		{
			GR2_LOD& lod = at(i);

			if(!lod->minVis || realDistance >= lod->minVis)
			{
				_active = i;
				break;
			}
		}
	}
}

void gr2ModelLOD::Update(bool force, bool granular)
{
	gr2Model* activeModel = GetActive();
	if(activeModel) activeModel->Update(force, granular);
}

bool gr2ModelLOD::Render(bool update)
{
	gr2Model* activeModel = GetActive();
	return (activeModel) ? activeModel->Render(update) : false;
}

gr2ModelAlt::gr2ModelAlt() : _active(0)
{
}

gr2ModelAlt::~gr2ModelAlt()
{
	Free();
}

void gr2ModelAlt::Free()
{
	models[0].Free();
	models[1].Free();
}

gr2Model* gr2ModelAlt::AddModel(UINT32 index, gr2Model* model)
{
	if(index >= 0 && index < 2)
		models[index].AddModel(model);

	return model;
}

gr2Model* gr2ModelAlt::GetActive()
{
	// _active is always either 0 or 1
	return models[_active].GetActive();
}

gr2Model* gr2ModelAlt::Get(SINT32 index)
{
	return models[_active].Get(index);
}

gr2Model* gr2ModelAlt::Get(FLOAT32 distance)
{
	return models[_active].Get(distance);
}

GR2_MODEL_LIST gr2ModelAlt::GetAllModels()
{
	GR2_MODEL_LIST list;

	for(int i = 0; i < 2; i++)
	{
		for(size_t j = 0; j < models[i].size(); j++)
			list.push_back(models[i].at(j));
	}

	return list;
}

SINT32 gr2ModelAlt::GetLOD()
{
	return models[_active].GetLOD();
}

UINT32 gr2ModelAlt::Size(SINT32 index)
{
	if(index == -1)
		return models[0].size() + models[1].size();
	else if(index >= 0 && index < 2)
		return models[index].size();

	return 0;
}

void gr2ModelAlt::Set(SINT32 index)
{
	if(_active != index && Size(index))
		_active = index;
}

void gr2ModelAlt::Set(bool alt)
{
	Set((alt) ? 1 : 0);
}

void gr2ModelAlt::Toggle()
{
	Set((_active == 0) ? 1 : 0);
}

void gr2ModelAlt::UpdateLODS(UINT32 distance, UINT32 realDistance)
{
	models[_active].UpdateLODS(distance, realDistance);
}

void gr2ModelAlt::Update(bool force, bool granular)
{
	gr2Model* model = GetActive();
	if(model) model->Update(force, granular);
}

bool gr2ModelAlt::Render(bool update)
{
	gr2Model* model = GetActive();
	return (model) ? model->Render(update) : false;
}
