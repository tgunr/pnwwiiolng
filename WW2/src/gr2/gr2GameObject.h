#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2GameObject class
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include <map>
#include <vector>
#include "gr2/gr2Object.h"
#include "gr2/gr2JSObject.h"
#include "gr2/gr2State.h"
#include "fx/fxSnd.h"

class gr2DamageModel;

using GrannyActiveStateMap = std::map<gr2State*,bool>;
using GrannyStateMap = std::map<std::string, gr2State*>;
using GrannyStateList = std::vector<gr2State*>;
using GrannyFlagMap = std::map<UINT64, gr2State*>;

class gr2GameObject : public gr2Object, public gr2JSObject
{
public:

	gr2GameObject();
	gr2GameObject(const char* skeletonFile, bool skeletonIsLOD = true);
	gr2GameObject(gr2ModelRef skeleton, bool skeletonIsLOD = true);
	virtual ~gr2GameObject();

	// returns true if this flag was switched on
	bool SetFlag(GR2_FLAG_TYPE t, UINT32 mask);

	// turn on and off flags at the same time
	void SetFlags(GR2_FLAG_TYPE t, UINT32 maskOn, UINT32 maskOff);

	// returns true if at least 1 bit in the mask is set
	bool HasFlag(GR2_FLAG_TYPE t, UINT32 mask);

	// returns true if the flag was switched on
	bool ToggleFlag(GR2_FLAG_TYPE t, UINT32 mask);

	// returns true if the flag was switched off
	bool ClearFlag(GR2_FLAG_TYPE t, UINT32 mask = 1);

	// type specific flag accessors; 1 set for each of the 3 main flag types
	// control flags
	bool SetControl(UINT32 mask){ return SetFlag(GFT_CONTROL, mask); }
	void SetControls(UINT32 maskOn, UINT32 maskOff){ SetFlags(GFT_CONTROL, maskOn, maskOff); }
	bool HasControl(UINT32 mask){ return HasFlag(GFT_CONTROL, mask); }
	bool ToggleControl(UINT32 mask){ return ToggleFlag(GFT_CONTROL, mask); }
	bool ClearControl(UINT32 mask = 1){ return ClearFlag(GFT_CONTROL, mask); }

	// action flags
	bool SetAction(UINT32 mask){ return SetFlag(GFT_ACTION, mask); }
	void SetActions(UINT32 maskOn, UINT32 maskOff){ SetFlags(GFT_ACTION, maskOn, maskOff); }
	bool HasAction(UINT32 mask){ return HasFlag(GFT_ACTION, mask); }
	bool ToggleAction(UINT32 mask){ return ToggleFlag(GFT_ACTION, mask); }
	bool ClearAction(UINT32 mask = 1){ return ClearFlag(GFT_ACTION, mask); }

	// condition flags
	bool SetCondition(UINT32 mask){ return SetFlag(GFT_CONDITION, mask); }
	void SetConditions(UINT32 maskOn, UINT32 maskOff){ SetFlags(GFT_CONDITION, maskOn, maskOff); }
	bool HasCondition(UINT32 mask){ return HasFlag(GFT_CONDITION, mask); }
	bool ToggleCondition(UINT32 mask){ return ToggleFlag(GFT_CONDITION, mask); }
	bool ClearCondition(UINT32 mask = 1){ return ClearFlag(GFT_CONDITION, mask); }

	virtual UINT32 GetSharedMask(){ return 0; }

	// object specific behavior
	virtual gr2State* GetState(const char* name);
	virtual bool GetState(GR2_FLAG_TYPE t, UINT32 s);
	virtual void ClearBaseState();
	virtual bool AddState(gr2State* state);
	virtual bool SetState(const char* name);
	virtual bool SetState(GR2_FLAG_TYPE t, UINT32 s);
	virtual bool SetState(gr2State* state);
	virtual bool ClearState(const char* name);
	virtual bool ClearState(GR2_FLAG_TYPE t, UINT32 s);
	virtual bool ClearState(gr2State* state);
	virtual bool ToggleState(const char* name);
	virtual bool ToggleState(GR2_FLAG_TYPE t, UINT32 s);
	virtual bool ToggleState(gr2State* state);
	virtual void StateHandler(GR2_STATE_EVENT e, gr2State* s);
	virtual void HandleStateEvent(GR2_STATE_EVENT e, gr2State* s){}

	// update granny models and state
	virtual void Update(FLOAT64 delta);

	// lets attachments call up to their parent to get absolute
	// world positions for things like bones, etc... returns
	// true if it actually modified the vector in some way
	virtual bool GetAbsoluteWorldPosition(FLOAT64VECTOR* position){ return false; }

	// look up an absolute bone positions
	virtual bool GetAbsoluteBonePosition(int bone, FLOAT64VECTOR* position, piVECTOR* offset = NULL);
	virtual bool GetAbsoluteBonePosition(const char* bone, FLOAT64VECTOR* position, piVECTOR* offset = NULL);

	virtual bool GetAttitudeMatrix(piMATRIX* mat);

	virtual int PlaySound(const char* sound,
							FLOAT32 vol = 1.0f,
							FLOAT32 fMin = 1.0f,
							FLOAT32 fMax = 2.0f,
							bool infinite = true,
							int priority = IDE_FP_PRIORITY_HIGH,
							int soundType = TYPE_FINITE_SOUND,
							int playType = DEF_PLAYTYPE_HEAD,
							OBJECT* emitter = NULL);

	virtual void StopSound(int id);

	virtual int PlayEffect(const char* effect, const FLOAT64VECTOR* p, const piVECTOR* n, int lifetime);
	virtual int PlayEffect(const char* effect, int bone, int lifetime);
	virtual void UpdateEffect(int effect, const FLOAT64VECTOR* p, const piVECTOR* n);
	virtual void StopEffect(int id);

	// handle inputs;
	virtual UINT32 ProcessControlInputs(SINT32 param, UINT32 id, UINT32 reserve){ return 0; };

	virtual gr2ModelRef GetHitmap();

	// damage model
	virtual gr2DamageModel* GetDamageModel();

	// are we damaged to a certain level or above?
	virtual bool IsDamaged(UINT32 state, UINT32 level);

	// update any active complex states
	virtual void UpdateStates(FLOAT64 delta);

	// called by the gr2 layer so you
	// have a chance to manipulate the
	// final model outcome before the
	// world pose is built
	virtual void UpdateCallback(gr2ModelRef model){}

	// apply damage to the model
	virtual void ApplyDamage(UINT32 location, FLOAT32 damage);

	// sets this object's game object
	void SetGameObject(OBJECT* o){ _object = o; }

	// gets this gr2Object's game object
	OBJECT* GetGameObject() const { return _object; }

protected:

	// this object's type (subclass dependent)
	int _type;

	// this object's subtype (subclass dependent)
	int _subtype;

	// our state flags
	UINT32 _flags[GFT_NUM_TYPES];

	// game object
	OBJECT* _object;

	// model used for damage modeling
	// uses 1st model added if not set
	gr2DamageModel* _damageModel;

	// current base state for easy access
	gr2State* _baseState;

	// states available to this object
	GrannyStateMap _states;

	// flag maps; used to lookup states
	// by flag type and value
	// key is <value.(flagtype/10)>
	// value is the associated state
	GrannyFlagMap _flagMap;

	// map of currently active states
	GrannyActiveStateMap _activeStates;

	// list of completed states
	GrannyActiveStateMap _completedStates;
};

