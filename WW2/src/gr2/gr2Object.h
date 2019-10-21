#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2Object class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include <map>
#include <vector>
#include "gr2/gr2layer.h"

enum GR2_OBJECT_TYPE
{
	GOT_GENERIC,
	GOT_GAME,
	GOT_VEHICLE,
	GOT_TROOP,
	GOT_WEAPON,
	GOT_TERRAIN,
	GOT_MODULE,
	// add more types here
	GOT_NUM_TYPES
};

using ModelList = std::vector<gr2Model*>;
using GrannyBlendList = std::vector<gr2AnimationBlend*>;

class gr2Object
{
public:

	struct gr2Attachment
	{
		gr2Object* object;
		int fromBone;
		int toBone;
		bool owner;
	};

	using Attachments = std::vector<gr2Attachment>;

	gr2Object();
	gr2Object(const char* skeletonFile, bool skeletonIsLOD = true);
	gr2Object(gr2Model* skeleton, bool skeletonIsLOD = true);
	virtual ~gr2Object();

	// core methods

	// load up models, sounds, etc...
	virtual bool Init();

	// update granny models and state
	virtual void Update(FLOAT64 delta);

	// send granny models to renderer
	virtual void Draw();

	// draw the skeleton
	void DrawSkeleton();

	void SetObjectType(GR2_OBJECT_TYPE t);
	bool IsObjectType(GR2_OBJECT_TYPE t) const;

	// collision detection

	// collide a ray with this object's collision model; if it has one
	virtual bool CollideWithRay(piVECTOR* start,			// where to start
								piVECTOR* end,				// where to end
								SINT32* location,			// hit location in the damage map
								piVECTOR* impact,			// impact point
								piMATRIX* transform = NULL, // transform to apply before checking
								bool recursive = true,		// check children?
								bool update = false);		// update skeleton before checking?


	// bone control

	virtual int FindBone(const char* name, bool exact = true);
	virtual bool GetBoneTransform(const char* name, piMATRIX* transform);
	virtual bool GetBoneTransform(int bone, piMATRIX* transform);
	void GenerateRotation(int axis, FLOAT64 angle, piMATRIX* pMatrix) const;
	void RotateBoneLocally(int bone, int axis, double angle, granny_local_pose* pose = NULL);
	void RotateBoneLocally(int bone, double axis[3], double angle, granny_local_pose* pose = NULL);

	// animation control

	// animation control
	// loader return a unique id
	// play/stop use the id to play or stop the animation or blend
	int LoadAnimation(const char* path);
	int LoadBlend(const char* path);
	int AddBlend(gr2AnimationBlend* blend);
	gr2AnimationBlend* GetBlend(int id);

	virtual bool PlayBlend(int id, float easeIn = 1.0f, float easeOut = 1.0f, bool resurect = true);
	virtual bool StopBlend(int id);
	virtual bool StopAllBlends(float easeOut = -1.0f); // -1 = default easeout

	// attachment control

	// add this object as a child; if this object is already attached, the
	// attachment just gets updated
	virtual void AttachObject(gr2Object* object, const char* from = NULL, const char* to = NULL, bool owner = true);
	virtual void AttachObject(gr2Object* object, int from, int to, bool owner = true);

	// is this object attached to me?
	virtual bool IsAttachedObject(gr2Object* object);

	// get this object's attachment list
	virtual gr2Object::Attachments& GetAttachments();

	// remove this child if found
	virtual void RemoveObject(gr2Object* object, bool freeOwned = true);

	// parent access
	void SetParentObject(gr2Object* newParent = NULL) { _parent = newParent; }
	gr2Object* GetParentObject() { return _parent; }

	// model control

	// are we visually modeled?
	bool IsModeled(){ return (_skeleton) ? true: false; }

	// update our skeleton
	virtual void UpdateModel(FLOAT64 delta);

	// called by the gr2 layer so you
	// have a chance to manipulate the
	// final model outcome before the
	// world pose is built
	virtual void UpdateCallback(gr2Model* model){}

	// called by the gr2 layer so you
	// can perform stuff before and after
	// the model is drawn
	virtual void PreDrawCallback(gr2Model* model){}
	virtual void PostDrawCallback(gr2Model* model){}

	// lod control
	virtual void AddLOD(gr2Model* m, float distance = 0.0f);
	virtual void AddLOD(const char* path, float distance = 0.0f);
	virtual void SetSkeleton(gr2Model* s, bool skeletonIsLOD = true);
	virtual void SetSkeleton(const char* path, bool skeletonIsLOD = true);

	// model accessors
	virtual gr2Model* GetLOD(int index = -1);
	virtual gr2Model* GetHitmap(){ return NULL; }
	virtual gr2Model* GetSkeleton();

	// returns the index of the lod model
	virtual int DetermineLOD(float distance = 0);

	// hide/show this object's model
	virtual void Hide();
	virtual void Show();

	// hide/show a mesh in this object's model
	// must hide it in all lods and damage levels
	// if it has any
	virtual void HideMesh(const char* name){}
	virtual void ShowMesh(const char* name){}

	// handle anim events specified by evt files for animations or blends
	virtual void HandleAnimEvent(gr2Model* model, SINT32 type, SINT32 event);

protected:

	// have we been inited
	bool _inited;

	// gr2Object type
	GR2_OBJECT_TYPE _objectType;

	// skeleton's boneCount
	int _boneCount;

	// current lod; defaults to 0
	// override DetermineLOD() as needed
	int _currentLOD;

	// null if not attached
	gr2Object* _parent;

	// available model lods
	ModelList _lods;

	// first model added or set skeleton
	gr2Model* _skeleton;

	// objects attached to this object
	Attachments _attached;

	// list of added blends; obsolete
	GrannyBlendList _blends;
};

