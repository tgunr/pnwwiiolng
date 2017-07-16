#include "syscfg.h" // Path is project specific.
#include "gr2/gr2Object.h"

#include <string>
#include <math.h>

#include "include/vars.h"
#include "ml/mlutility.h"
#include "math/math3d.h"
#include "math/mathvect.h"

gr2Object::gr2Object() :
	_objectType(GOT_GENERIC),
	_parent(NULL),
	_skeleton(NULL),
	_boneCount(0),
	_currentLOD(-1),
	_inited(false)
{
}

gr2Object::gr2Object(const char* skeletonFile, bool skeletonIsLOD) :
	_objectType(GOT_GENERIC),
	_parent(NULL),
	_skeleton(NULL),
	_boneCount(0),
	_currentLOD(-1),
	_inited(false)
{
	SetSkeleton(skeletonFile, skeletonIsLOD);
}

gr2Object::gr2Object(gr2ModelRef skeleton, bool skeletonIsLOD) :
	_objectType(GOT_GENERIC),
	_parent(NULL),
	_skeleton(skeleton),
	_boneCount(0),
	_currentLOD(-1),
	_inited(false)
{
	SetSkeleton(skeleton, skeletonIsLOD);
}

gr2Object::~gr2Object()
{
	_parent = NULL;

	// are these simply reused LODs or
	// were they sent externally
	if(_lods.size())
	{
		for(int i = 0; i < (int)_lods.size(); i++)
		{
			gr2ModelRef model = _lods[i];

			if(_skeleton == model)
				_skeleton = NULL;

			gr2FreeModel(model);
		}

		_lods.clear();
	}

	if(_skeleton)
		gr2FreeModel(_skeleton);

	// free attached objects if we're the owner
	Attachments::iterator ait = _attached.begin();
	for(; ait != _attached.end(); ait++)
	{
		gr2Attachment& attachment = *ait;

		// detach the object from it's parent
		if(attachment.object->GetSkeleton())
			gr2DetachModel(attachment.object->GetSkeleton());

		// no more parent
		attachment.object->SetParentObject(NULL);

		// we're the owner, so lets delete it
		if(attachment.owner)
			delete attachment.object;
	}

	_attached.clear();

	if(_blends.size())
	{
		for(int i = 0; i < (int)_blends.size(); i++)
		{
			gr2AnimationBlendRef blend = _blends[i];
			gr2FreeBlendedAnimation(blend);
		}

		_blends.clear();
	}
}

// load up models, sounds, etc...
bool gr2Object::Init()
{
	if(_inited) return true;

	if(GetSkeleton())
		gr2ModelSetEventHandler(_skeleton, (OBJECT*)NULL, gr2ObjectHandleEvent);

	for(int i = 0; i < (int)_attached.size(); i++)
		_attached[i].object->Init();

	_inited = true;

	return true;
}

// update granny models and state
void gr2Object::Update(FLOAT64 delta)
{
	DetermineLOD();
	UpdateModel(delta);

	// now lets update my attachments; some
	// depend on me, so their updated after me
	for(int i = 0; i < (int)_attached.size(); i++)
		_attached[i].object->Update(delta);
}

// send granny models to renderer
void gr2Object::Draw()
{
	if(GetSkeleton())
	{
		// by default this returns lod 0 if
		// DetermineLOD() wasn't virtualy defined
		// in a subclass
		gr2RenderModel(GetLOD());

#ifdef BETA_RELEASE
		if(VehicleObjectAnalysis)
		{
			gr2RenderDamageModel(GetHitmap());
			DrawSkeleton();
		}
#endif

		// let's draw our attachments; some may
		// depend on me, so their drawn after me
		for(int i = 0; i < (int)_attached.size(); i++)
			_attached[i].object->Draw();
	}
}

// draw the skeleton
void gr2Object::DrawSkeleton()
{
	gr2DrawSkeleton(GetSkeleton(), 0);
}

void gr2Object::SetObjectType(GR2_OBJECT_TYPE t)
{
	_objectType = t;
}

bool gr2Object::IsObjectType(GR2_OBJECT_TYPE t) const
{
	return (_objectType == t);
}


// given a bone name; can be a full name or a partial name
int gr2Object::FindBone(const char* name, bool exact)
{
	if(GetSkeleton())
		return gr2ModelFindBone(GetSkeleton(), (SCHAR*)name, (LOGICAL)exact);

	return -1;
}

// given a bone name, return its position and rotation
bool gr2Object::GetBoneTransform(const char* name, piMATRIX* transform)
{
	return gr2GetBonePosition(GetSkeleton(), (SCHAR*)name, transform);
}

bool gr2Object::GetBoneTransform(int bone, piMATRIX* transform)
{
	return gr2GetBonePositionByIndex(GetSkeleton(), bone, transform);
}

void gr2Object::GenerateRotation(int axis, FLOAT64 angle, piMATRIX* pMatrix) const
{
    static const piMATRIX identity = {{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}};
    FLOAT32    trigValue;

	angle = angle * PI/180.0;

	trigValue = (FLOAT32)cos(angle);

	if (axis == AROUND_X)
	{
		*pMatrix = identity;
		pMatrix->_22 = trigValue;
		pMatrix->_33 = trigValue;

		trigValue = (FLOAT32)sin(angle);

		pMatrix->_23 = -trigValue;
		pMatrix->_32 = trigValue;
	}
	else if (axis == AROUND_Y)
	{
		*pMatrix = identity;
		pMatrix->_11 = trigValue;
		pMatrix->_33 = trigValue;

		trigValue = (FLOAT32)sin(angle);

		pMatrix->_13 = -trigValue;  // because our coord system has negative z up?
		pMatrix->_31 = trigValue;
	}
	else if (axis == AROUND_Z)
	{
		*pMatrix = identity;
		pMatrix->_11 = trigValue;
		pMatrix->_22 = trigValue;

		trigValue = (FLOAT32)sin(angle);

		pMatrix->_12 = -trigValue;
		pMatrix->_21 = trigValue;
	}
}

void gr2Object::RotateBoneLocally(int bone, int axis, double angle, granny_local_pose* pose)
{
	double axes[3]= {0.0, 0.0, 0.0};
	axes[axis] = 1.0f;
	RotateBoneLocally(bone, axes, angle, pose);
}

void gr2Object::RotateBoneLocally(int bone, double axis[3], double angle, granny_local_pose* pose)
{
	gr2AddAxisRotationToBoneInLocalPose(pose, bone, axis, mlDEG_TO_RAD(angle));
}

bool gr2Object::CollideWithRay(piVECTOR* start,
							   piVECTOR* end,
							   SINT32* location,
							   piVECTOR* impact,
							   piMATRIX* transform,
							   bool recursive,
							   bool update)
{
	if(update)
		Update(0);

	if(recursive)
	{
		for(int i = 0; i < (int)_attached.size(); i++)
		{
			gr2Attachment& attachment = _attached[i];

			// update defaults to false, if it was set at the top level, it and all children were already
			// updated so in any case we just go with the default when checking children, which is false
			if(attachment.object->CollideWithRay(start, end, location, impact, transform, recursive))
				return true;
		}
	}

	if(GetHitmap() && location && start && end)
	{
		piMATRIX inverseMat;
		piVECTOR rayStart, rayEnd, rayNormal;

		math3dInvertMatrix(&inverseMat, transform);

		math3dVectorMultiply(&rayStart, start, &inverseMat);
		rayStart.z = -rayStart.z;

		math3dVectorMultiply(&rayEnd, end, &inverseMat);
		rayEnd.z = -rayEnd.z;

		mathSubVectors(&rayNormal, &rayEnd, &rayStart);

		// no attachments were struct, try me
		return gr2CollideRayWithModel(GetHitmap(), &rayStart, &rayNormal, location, impact);
	}

	return false;
}

int gr2Object::AddBlend(gr2AnimationBlendRef blend)
{
	if(blend)
	{
		_blends.push_back(blend);
		return (int)(_blends.size() - 1);
	}

	return -1;
}

int gr2Object::LoadAnimation(const char* path)
{
	return AddBlend(gr2CreateBlendedAnimation((SCHAR*)path, _boneCount));
}

int gr2Object::LoadBlend(const char* path)
{
	return AddBlend(gr2LoadBlendedAnimation((SCHAR*)path, _boneCount));
}

gr2AnimationBlendRef gr2Object::GetBlend(int id)
{
	if(_blends.size() && id >= 0 && id < (int)_blends.size())
		return _blends[id];

	return NULL;
}

// object animation and effects
bool gr2Object::PlayBlend(int id, float easeIn, float easeOut, bool resurrect)
{
	return gr2PlayBlendedAnimationFull(GetLOD(), GetBlend(id), easeIn, easeOut, resurrect);
}

bool gr2Object::StopBlend(int id)
{
	return true;
}

bool gr2Object::StopAllBlends(float easeOut)
{
	return gr2RemoveAllBlendedAnimations(GetSkeleton(), easeOut);
}

// add this object as a child
void gr2Object::AttachObject(gr2Object* object, const char* from, const char* to, bool owner)
{
	AttachObject(object, object->FindBone(from), FindBone(to), owner);
}

void gr2Object::AttachObject(gr2Object* object, int from, int to, bool owner)
{
	Attachments::iterator it = _attached.begin();
	for(; it != _attached.end(); it++)
	{
		gr2Attachment& attachment = *it;

		if(attachment.object == object)
		{
			// lets detach the visual object
			gr2DetachModel(object->GetSkeleton());

			// now we update the attach info
			attachment.fromBone = from;
			attachment.toBone	= to;
			attachment.owner	= owner;

			// now lets re-attach it
			if(from >= 0 && to >= 0)
				gr2AttachModelByIndex(object->GetSkeleton(), from, GetSkeleton(), to);

			// we're done
			return;
		}
	}

	// if we made it here, the object wasn't already attached

	gr2Attachment attachment;

	attachment.object	= object;
	attachment.fromBone = from;
	attachment.toBone	= to;
	attachment.owner	= owner;

	// save this attachment
	_attached.push_back(attachment);

	if(from >= 0 && to >= 0)
		gr2AttachModelByIndex(object->GetSkeleton(), from, GetSkeleton(), to);

	object->SetParentObject(this);
}

// remove this child if found
void gr2Object::RemoveObject(gr2Object* object, bool freeOwned)
{
	Attachments::iterator it = _attached.begin();
	for(; it != _attached.end(); it++)
	{
		gr2Attachment& attachment = *it;

		if(attachment.object == object)
		{
			gr2DetachModel(attachment.object->GetSkeleton());
			attachment.object->SetParentObject(NULL);

			if(freeOwned && attachment.owner)
				delete attachment.object;

			_attached.erase(it);

			break;
		}
	}
}

bool gr2Object::IsAttachedObject(gr2Object* object)
{
	Attachments::iterator it = _attached.begin();
	for(; it != _attached.end(); it++)
	{
		gr2Attachment& attachment = *it;

		if(attachment.object == object)
			return true;
	}

	return false;
}

gr2Object::Attachments& gr2Object::GetAttachments()
{
	return _attached;
}

void gr2Object::SetSkeleton(gr2ModelRef s, bool skeletonIsLOD)
{
	if(!_skeleton)
	{
		_skeleton	= gr2SetGR2Object(s, this);
		_boneCount	= gr2GetBoneCount(_skeleton);

		if(skeletonIsLOD)
			AddLOD(_skeleton);
	}
}

void gr2Object::SetSkeleton(const char* path, bool skeletonIsLOD)
{
	if(!_skeleton)
		SetSkeleton(gr2LoadModel(path, NULL, NULL, 1), skeletonIsLOD);
}

void gr2Object::AddLOD(gr2ModelRef m, float distance)
{
	_lods.push_back(gr2SetGR2Object(m, this));

	if(!_skeleton)
		SetSkeleton(m, false);
}

void gr2Object::AddLOD(const char* path, float distance)
{
	AddLOD(gr2LoadModel(path, _skeleton, NULL, 1), distance);
}

// model accessors
gr2ModelRef gr2Object::GetLOD(int index)
{
	if(_currentLOD < 0)
		DetermineLOD();

	if(index < 0)
		index = _currentLOD;

	if(index >= 0 && _lods.size())
	{
		if(index >= (int)_lods.size())
			index = _lods.size() - 1;

		return _lods[index];
	}

	return NULL;
}

gr2ModelRef gr2Object::GetSkeleton()
{
	if(!_skeleton)
		_skeleton = GetLOD(0);

	return _skeleton;
}

int gr2Object::DetermineLOD(float distance)
{
	_currentLOD = 0;
	return _currentLOD;
}

void gr2Object::Hide()
{
	gr2SetModelActive(GetLOD(), false);
}

void gr2Object::Show()
{
	gr2SetModelActive(GetLOD(), true);
}

void gr2Object::UpdateModel(FLOAT64 delta)
{
	if(GetSkeleton())
	{
		// update the skeleton
		gr2UpdateModel(_skeleton);

		// now lets update the hitmap
		gr2UpdateModel(GetHitmap());

		// get the current lod
		gr2ModelRef lod	= GetLOD();

		// update the lod if its not the skeleton
		if(lod != _skeleton)
			gr2UpdateModel(lod);
	}
}

void gr2Object::HandleAnimEvent(gr2ModelRef model, SINT32 type, SINT32 event)
{
	switch(type)
	{
		case ANIM_EVENT_HIDE_MESH:
			HideMesh(gr2GetEventData(model, event));
		break;
		case ANIM_EVENT_SHOW_MESH:
			ShowMesh(gr2GetEventData(model, event));
		break;
		case ANIM_EVENT_HIDE_MODEL:
			Hide();
		break;
		case ANIM_EVENT_SHOW_MODEL:
			Show();
		break;
		default:
			// unknown event
		break;
	}
}
