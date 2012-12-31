#include <syscfg.h> // Path is project specific.
#include "gr2/gr2GameObject.h"

#include <string>
#include <math.h>
#include "include/vars.h"
#include "ml/mlutility.h"
#include "fx/fxSnd.h"
#include "fx2/effect.h"
#include "math/math3d.h"
#include "math/mathvect.h"
#include "gr2/gr2DamageModel.h"

gr2GameObject::gr2GameObject() :
	gr2Object(),
	_baseState(NULL),
	_damageModel(NULL),
	_object(NULL),
	_type(-1),
	_subtype(-1)
{
	SetObjectType(GOT_GAME);
	memset(_flags, 0, GFT_NUM_TYPES * sizeof(UINT32));
}

gr2GameObject::gr2GameObject(const char* skeletonFile, bool skeletonIsLOD) :
	gr2Object(skeletonFile, skeletonIsLOD),
	_baseState(NULL),
	_damageModel(NULL),
	_object(NULL),
	_type(-1),
	_subtype(-1)
{
	SetObjectType(GOT_GAME);
	memset(_flags, 0, GFT_NUM_TYPES * sizeof(UINT32));
}

gr2GameObject::gr2GameObject(gr2ModelRef skeleton, bool skeletonIsLOD) :
	gr2Object(skeleton, skeletonIsLOD),
	_baseState(NULL),
	_damageModel(NULL),
	_object(NULL),
	_type(-1),
	_subtype(-1)
{
	SetObjectType(GOT_GAME);
	memset(_flags, 0, GFT_NUM_TYPES * sizeof(UINT32));
}

gr2GameObject::~gr2GameObject()
{
	_parent = NULL;
	_baseState = NULL;

	GrannyStateMap::iterator sit = _states.begin();
	for(; sit != _states.end(); sit++)
	{
		gr2State* state = sit->second;
		delete state;
	}

	_states.clear();

	if(_damageModel)
		delete _damageModel;

	_damageModel = NULL;
}

void gr2GameObject::SetFlags(GR2_FLAG_TYPE t, UINT32 maskOn, UINT32 maskOff)
{
	ClearFlag(t, maskOff);
	SetFlag(t, maskOn);
}

bool gr2GameObject::SetFlag(GR2_FLAG_TYPE t, UINT32 mask)
{
	bool wasOff = !(_flags[t] & mask);
	_flags[t] |= mask;
	return wasOff;
}

bool gr2GameObject::HasFlag(GR2_FLAG_TYPE t, UINT32 mask)
{
	return _flags[t] & mask;
}

bool gr2GameObject::ToggleFlag(GR2_FLAG_TYPE t, UINT32 mask)
{
	bool wasOff = !(_flags[t] & mask);
	_flags[t] ^= mask;
	return wasOff;
}

bool gr2GameObject::ClearFlag(GR2_FLAG_TYPE t, UINT32 mask)
{
	bool wasOn = (_flags[t] & mask);
	_flags[t] &= ~mask;
	return wasOn;
}

// update granny models and state
void gr2GameObject::Update(FLOAT64 delta)
{
	UpdateStates(delta);
	gr2Object::Update(delta);
}

int gr2GameObject::PlaySound(const char* sound, FLOAT32 vol, FLOAT32 fMin, FLOAT32 fMax, bool infinite, int priority, int soundType, int playType, OBJECT* emitter)
{
	SINT32 id = fxSndGetNewSound();

	if(id == -1)
		return -1;

	fxSndSetSoundName(id, (SCHAR*)sound);
	fxSndSetPriority(id, (ENUM_PRIORITY)priority);
	fxSndSetSoundType(id, soundType);
	fxSndSetPitch(id, 1.0f);
	fxSndSetVolume(id, 1.0f);
	fxSndPushFlags(id, 0);
	fxSndSetSoundLoop(id, infinite);

    // Set min and max distances
	if(fMin)
		fxSndSetMinDistance(id, fMin);

	if(fMax)
		fxSndSetMaxDistance(id, fMax);

	//if(position != NULL)
	//	fxSndSetPosition(id, position->x, position->y, position->z);

	if(emitter)
		fxSndSetEmitterObject(id, emitter);

	fxSndSetPlayType(id, playType);

    // A failure does not necessarily imply an error. For example, if a sound
    // is beyond its max distance, this function will fail.
    if(!fxSndInitSound(id))
        return -1;

    return id;
}

void gr2GameObject::StopSound(int id)
{
	fxSndStopSound(id);
}

int gr2GameObject::PlayEffect(const char* effect, int bone, int lifetime)
{
	piVECTOR dir = {0,0,-1};
	FLOAT64VECTOR pos;

	if(GetAbsoluteBonePosition(bone, &pos))
		return PlayEffect(effect, &pos, &dir, lifetime);

	return -1;
}

int gr2GameObject::PlayEffect(const char* effect, const FLOAT64VECTOR* p, const piVECTOR* n, int lifetime)
{
	/*
	if(_object)
		return fxaddeffectobject(effect, *p, _object, *n, lifetime);

	return fxaddeffect(effect, *p, *n, lifetime);
	*/
	return -1;
}

void gr2GameObject::UpdateEffect(int effect, const FLOAT64VECTOR* p, const piVECTOR* n)
{
	//fxupdateeffect(effect, *p, *n);
}

void gr2GameObject::StopEffect(int id)
{
	//fxremove(id);
}

gr2State* gr2GameObject::GetState(const char* name)
{
	return _states[name];
}

bool gr2GameObject::GetState(GR2_FLAG_TYPE t, UINT32 s)
{
	return _flagMap[gr2State::GetKey(t, s)];
}

void gr2GameObject::ClearBaseState()
{
	if(_baseState)
		_baseState->Stop();
}

bool gr2GameObject::AddState(gr2State* state)
{
	if(state)
	{
		// set this object as the state handler
		state->SetHandler(this, &gr2GameObject::StateHandler);

		// save the state
		if(state->name.length() && _states.find(state->name) == _states.end())
		{
			_states[state->name] = state;

			if(state->GetKey() && _flagMap.find(state->GetKey()) == _flagMap.end())
				_flagMap[state->GetKey()] = state;

			return true;
		}
	}

	return false;
}

bool gr2GameObject::SetState(const char* name)
{
	return SetState(_states[name]);
}

bool gr2GameObject::SetState(GR2_FLAG_TYPE t, UINT32 s)
{
	gr2State* state = _flagMap[gr2State::GetKey(t, s)];
	if(state) return SetState(state);

	SetFlag(t, s);
	return true;
}

bool gr2GameObject::SetState(gr2State* state)
{
	if(!state || HasFlag(state->flagType, state->value))
		return false;

	return state->Start();
}

bool gr2GameObject::ClearState(const char* name)
{
	return ClearState(_states[name]);
}

bool gr2GameObject::ClearState(GR2_FLAG_TYPE t, UINT32 s)
{
	gr2State* state = _flagMap[gr2State::GetKey(t, s)];
	if(state) ClearState(state);

	ClearFlag(t, s);
	return true;
}

bool gr2GameObject::ClearState(gr2State* state)
{
	if(!state || !HasFlag(state->flagType, state->value))
		return false;

	return state->Stop();
}

bool gr2GameObject::ToggleState(const char* name)
{
	return ToggleState(_states[name]);
}

bool gr2GameObject::ToggleState(GR2_FLAG_TYPE t, UINT32 s)
{
	gr2State* state = _flagMap[gr2State::GetKey(t, s)];
	if(state) ToggleState(state);

	ToggleFlag(t, s);
	return true;
}

bool gr2GameObject::ToggleState(gr2State* state)
{
	if(state)
	{
		if(!HasFlag(state->flagType, state->value))
			return SetState(state);
		else
			return ClearState(state);
	}

	return false;
}

void gr2GameObject::StateHandler(GR2_STATE_EVENT e, gr2State* s)
{
	if(s->valued)
	{
		switch(e)
		{
			case GSE_START:
			case GSE_RESET:
			{
				// stop all current base states
				if(s->baseState)
				{
					_baseState = s;

					GrannyActiveStateMap::iterator it = _activeStates.begin();

					for(; it != _activeStates.end(); it++)
						if(it->first->baseState) it->first->Stop();
				}

				SetFlag(s->flagType, s->value);
				_activeStates[s] = true;
			}
			break;
			case GSE_UPDATE:
				// subclasses might do something with this
			break;
			case GSE_STOP:
			{
				ClearFlag(s->flagType, s->value);
				_activeStates[s]	= false;
				_completedStates[s] = true;

				if(s == _baseState)
					_baseState = NULL;
			}
			break;
		}

		// let subclasses do stuff
		HandleStateEvent(e, s);
	}
}

gr2DamageModel* gr2GameObject::GetDamageModel()
{
	return _damageModel;
}

// are we damaged to a certain level or above?
bool gr2GameObject::IsDamaged(UINT32 state, UINT32 level)
{
	gr2DamageModel* dm = GetDamageModel();

	if(dm && HasFlag(GFT_DAMAGE, state))
	{
		gr2DamageState* ds = (gr2DamageState*)GetState(GFT_DAMAGE, state);

		if(ds && ds->damageComponent && ds->damageComponent->GetDamageLevel() >= level)
			return true;
	}

	return false;
}


gr2ModelRef gr2GameObject::GetHitmap()
{
	return (_damageModel) ? _damageModel->GetHitmap(): gr2Object::GetHitmap();
}

// update any active complex states
void gr2GameObject::UpdateStates(FLOAT64 delta)
{
	GrannyActiveStateMap::iterator it = _activeStates.begin();
	GrannyActiveStateMap::iterator found;

	// update all active states
	for(; it != _activeStates.end(); it++)
	{
		if(it->second)
		{
			gr2State* state = it->first;
			state->Update(delta);
		}
	}

	// we clean up completed states after updating
	// active states because some of them may have
	// popped themselves on the completed queue
	it = _completedStates.begin();

	// clear all completed states
	for(; it != _completedStates.end(); it++)
	{
		found = _activeStates.find(it->first);

		if(found != _activeStates.end())
			_activeStates.erase(found);
	}

	// we clear this map out each frame
	_completedStates.clear();
}

bool gr2GameObject::GetAbsoluteBonePosition(int bone, FLOAT64VECTOR* position, piVECTOR* offset)
{
	piMATRIX transform = identity;

	if(GetBoneTransform(bone, &transform))
	{
		position->x = transform._41;
		position->y = transform._42;
		position->z = transform._43;

		if(offset)
		{
			position->x += offset->x;
			position->y += offset->y;
			position->z += offset->z;
		}

		return GetAbsoluteWorldPosition(position);
	}

	return false;
}

bool gr2GameObject::GetAbsoluteBonePosition(const char* bone, FLOAT64VECTOR* position, piVECTOR* offset)
{
	return GetAbsoluteBonePosition(FindBone(bone), position, offset);
}

bool gr2GameObject::GetAttitudeMatrix(piMATRIX* mat)
{
	if(_object)
	{
		*mat = _object->mat;
		return true;
	}

	return false;
}

void gr2GameObject::ApplyDamage(UINT32 location, FLOAT32 damage)
{
	if(_damageModel)
		_damageModel->ApplyDamage(location, damage);
}
