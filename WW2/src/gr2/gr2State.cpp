#include <syscfg.h> // Path is project specific.

#include "include/vars.h"
#include "include/systypes.h"
#include "gr2/gr2State.h"
#include "gr2/gr2GameObject.h"
#include "gr2/gr2DamageModel.h"
#include "gr2/gr2Data.h"
#include "math/math3d.h"

char* GR2_STATE_TYPE_NAMES[GST_NUM_TYPES] =
{
	"generic",
	"conditional",	// runs as long as state is true
	"timed",		// runs for x loops; each loop is y ms (can be infinite)
	"chain",		// plays each child state in turn
	"anim",			// plays an animation; built on timeloop
	"sound",		// plays a sound; built on timeloop
	"effect",		// plays an effect; built on timeloop
	"loop",			// starts each children after each loop
	"damage",		// damage state handling
	"trigger",		// fire and forget flag setter
	"transition"
};

char* GR2_FLAG_TYPE_NAMES[GFT_NUM_TYPES] =
{
	"control",
	"action",
	"condition",
	"damage"
};

struct gr2AnimState;

gr2State::gr2State(gr2StateSpec* spec) :
	name(spec->name),
	type((GR2_STATE_TYPE)spec->type),
	flagType((GR2_FLAG_TYPE)spec->flagType),
	value(spec->value),
	valued(spec->valued),
	baseState(spec->baseState),
	key(0),
	life(spec->life),
	started(0)
{
}

gr2State::~gr2State()
{
	for(int s = 0; s < (int)states.size(); s++)
	{
		gr2State* state = states[s];
		delete state;
	}

	states.clear();
}

UINT64 gr2State::GetKey()
{
	if(!key)
	{
		key = (UINT32)type;
		key <<= 32;

		if(valued)
			key |= value;
	}

	//if(!key)
	//	key = (FLOAT32)(value + flagType/10);

	return key;
}

void gr2State::SetHandler(gr2GameObject* o, GSE_HANDLER f)
{
	owner	= o;
	handler = f;

	for(int s = 0; s < (int)states.size(); s++)
		states[s]->SetHandler(o, f);
}

bool gr2State::Start(bool reset)
{
	started = frametime;

	CallHandler((reset) ? GSE_RESET: GSE_START);

	for(int s = 0; s < (int)states.size(); s++)
		states[s]->Start(reset);

	return true;
}

void gr2State::Update(FLOAT64 delta)
{
	if(Active())
	{
		CallHandler(GSE_UPDATE);

		for(int s = 0; s < (int)states.size(); s++)
			states[s]->Update(delta);
	}
	else
	{
		Stop();
	}
}

bool gr2State::Active()
{
	return (started && (!valued || (owner && owner->HasFlag(flagType, value))));
}

bool gr2State::Stop()
{
	started = 0;

	CallHandler(GSE_STOP);

	for(int s = 0; s < (int)states.size(); s++)
		states[s]->Stop();

	return true;
}

void gr2State::CallHandler(GR2_STATE_EVENT e)
{
	if(owner && handler)
		(owner->*handler)(e, this);
}

gr2State* gr2State::Create(gr2StateSpec* spec)
{
	gr2State* newState = NULL;

	if(spec)
	{
		switch(spec->type)
		{
			case GST_GENERIC:
			{
				newState = new gr2State(spec);
			}
			break;
			case GST_CONDITIONAL:
			{
				newState = new gr2ConditionalState(spec);
			}
			break;
			case GST_TIMED:
			{
				newState = new gr2TimedState(spec);
			}
			break;
			case GST_LOOP:
			{
				newState = new gr2LoopState(spec);
			}
			break;
			case GST_CHAIN:
			{
				//newState = new gr2TimeLoopState(spec->name.c_str());
			}
			break;
			case GST_ANIMATION:
			{
				newState = new gr2AnimationState(spec);
			}
			break;
			case GST_SOUND:
			{
				newState = new gr2SoundState(spec);
			}
			break;
			case GST_EFFECT:
			{
				newState = new gr2EffectState(spec);
			}
			break;
			case GST_DAMAGE:
			{
				newState = new gr2DamageState(spec);
			}
			break;
			case GST_TRIGGER:
			{
				newState = new gr2TriggerState(spec);
			}
			break;
			case GST_TRANSITION:
			{
				//newState = new gr2EffectState(spec->name.c_str());
			}
			break;
		}
	}

	if(newState)
	{
		for(int i = 0; i < (int)spec->states.size(); i++)
		{
			gr2StateSpec* childSpec = spec->states[i];
			gr2State* childState	= gr2State::Create(childSpec);

			if(childState)
				newState->states.push_back(childState);
		}
	}

	return newState;
}

UINT64 gr2State::GetKey(GR2_FLAG_TYPE type, UINT32 state)
{
	UINT64 key = (UINT32)type;
	key <<= 32;
	key |= state;

	//return (FLOAT32)(state + type/10);
	return key;
}

gr2TriggerState::gr2TriggerState(gr2StateSpec* spec) : gr2State(spec)
{
}

bool gr2TriggerState::Start(bool reset)
{
	if(owner && valued)
		owner->SetFlag(flagType, value);

	return true;
}

void gr2TriggerState::Update(FLOAT64 delta)
{
	// do nothing
}

bool gr2TriggerState::Active()
{
	return false;
}

bool gr2TriggerState::Stop()
{
	return true;
}

gr2ConditionalState::gr2ConditionalState(gr2StateSpec* spec) : gr2State(spec)
{
}

bool gr2ConditionalState::Start(bool reset)
{
	if(!Active())
		return gr2State::Start();

	return false;
}

bool gr2ConditionalState::Active()
{
	bool active = false;

	if(gr2State::Active())
	{
		if(states.size())
		{
			for(int i = 0; i < (int)states.size(); i++)
			{
				if(states[i]->Active())
				{
					active = true;
					break;
				}
			}
		}
		else
		{
			active = true;
		}
	}

	return active;
}

gr2TimedState::gr2TimedState(gr2StateSpec* spec) : gr2State(spec)
{
}

bool gr2TimedState::Start(bool reset)
{
	if(!Active())
		return gr2State::Start();

	return false;
}

bool gr2TimedState::Active()
{
	if(gr2State::Active())
		return ((started + life) > frametime);

	return false;
}

gr2LoopState::gr2LoopState(gr2StateSpec* spec) : gr2State(spec), lastRun(0), stop(false)
{
}

void gr2LoopState::Update(FLOAT64 delta)
{
	gr2State::Update(delta);

	if(!stop && (frametime >= lastRun + life))
	{
		lastRun = frametime;

		for(int i = 0; i < (int)states.size(); i++)
			states[i]->Start(true);
	}
}

bool gr2LoopState::Active()
{
	if(gr2State::Active() && (!stop || (frametime < lastRun + life)))
		return true;

	return false;
}

bool gr2LoopState::Start(bool reset)
{
	if(!Active())
	{
		// find how long our life is
		for(int i = 0; i < (int)states.size(); i++)
			life = max(life, states[i]->life);

		stop	= false;
		lastRun = frametime;

		return gr2State::Start();
	}

	return false;
}

bool gr2LoopState::Stop()
{
	if(!stop)
	{
		stop = true;
	}
	else if(frametime >= lastRun + life)
	{
		stop	= false;
		lastRun = 0;

		return gr2State::Stop();
	}

	return false;
}

gr2AnimationState::gr2AnimationState(gr2StateSpec* spec, gr2AnimationBlend* b) :
	gr2State(spec), blend(b)
{
	gr2AnimStateSpec* animSpec = (gr2AnimStateSpec*)spec;

	loops		= animSpec->loops;
	speed		= animSpec->speed;
	easeIn		= animSpec->easeIn;
	easeOut		= animSpec->easeOut;
	blend		= gr2CreateBlendedAnimation((SCHAR*)animSpec->animation.c_str(), 31);
	life		= (blend) ? (UINT32)(gr2BlendGetBaseLength(blend) * 1000): 0;
}

gr2AnimationState::~gr2AnimationState()
{
	if(blend)
		gr2FreeBlendedAnimation(blend);
}

bool gr2AnimationState::Start(bool reset)
{
	if(!Active())
	{
		if(gr2PlayBlendedAnimationFull(owner->GetSkeleton(), blend, easeIn, easeOut))
		{
			if(loops > 0)
			{
				gr2BlendControlSetLoopCount(owner->GetSkeleton(), blend, loops);
				gr2BlendControlRemoveWhenDone(owner->GetSkeleton(), blend);
			}

			if(speed != 1.0f)
				gr2BlendControlSetSpeed(owner->GetSkeleton(), blend, speed);

			return gr2State::Start();
		}
	}

	return false;
}

bool gr2AnimationState::Active()
{
	if(owner && blend && gr2State::Active())
		return (!loops || ((started + ((int)loops * life)) > frametime));

	return false;
}

bool gr2AnimationState::Stop()
{
	if(blend)
		gr2RemoveAllBlendedAnimations(owner->GetSkeleton(), -1, blend);

	return gr2State::Stop();
}

gr2SoundState::gr2SoundState(gr2StateSpec* spec) : gr2State(spec)
{
	gr2SoundStateSpec* soundSpec = (gr2SoundStateSpec*)spec;

	id			= -1;
	volume		= soundSpec->volume;
	minVolume	= soundSpec->minVol;
	maxVolume	= soundSpec->maxVol;
	sound		= soundSpec->sound;
}

bool gr2SoundState::Start(bool reset)
{
	if(!Active())
	{
		gr2Object* sounder = owner;

		while(sounder && !sounder->IsObjectType(GOT_VEHICLE))
			sounder = sounder->GetParentObject();

		if(sounder)
		{
			gr2GameObject* gameSounder = dynamic_cast<gr2GameObject*>(sounder);
			if(!gameSounder) gameSounder = owner;

			id = gameSounder->PlaySound(sound.c_str(), volume, minVolume, maxVolume, (life < 0));
		}
		else
		{
			owner->PlaySound(sound.c_str(), volume, minVolume, maxVolume, (life < 0));
		}

		if(life == 0) // instant sound
			id = -1;

		return gr2State::Start();
	}

	return false;
}

bool gr2SoundState::Active()
{
	if(owner && id >= 0 && gr2State::Active())
		return ((life < 0) || ((started + life) > frametime));

	return false;
}

bool gr2SoundState::Stop()
{
	if(id >= 0)
		owner->StopSound(id);

	id = -1;

	return gr2State::Stop();
}

gr2EffectState::gr2EffectState(gr2StateSpec* spec) : gr2State(spec)
{
	gr2EffectStateSpec* effectSpec = (gr2EffectStateSpec*)spec;

	id			= -1;
	bone		= -1;
	update		= effectSpec->update;
	dir[0]		= effectSpec->dir[0];
	dir[1]		= effectSpec->dir[1];
	dir[2]		= effectSpec->dir[2];
	offset[0]	= effectSpec->offset[0];
	offset[1]	= effectSpec->offset[1];
	offset[2]	= effectSpec->offset[2];
	boneName	= effectSpec->bone;
	effect		= effectSpec->effect;

	// effect's direction is straight up if one
	// was not specified
	if(!(dir[0] + dir[1] + dir[2]))
		dir[2] = -1;

	// default it to the origin
	position.x = position.y = position.z = 0;
}

bool gr2EffectState::Start(bool reset)
{
	if(!Active())
	{
		piVECTOR direction;
		piMATRIX attitude;

		direction.x = dir[0];
		direction.y = dir[1];
		direction.z = dir[2];

		if((direction.x + direction.y + direction.z) && owner->GetAttitudeMatrix(&attitude))
			math3dVectorMultiply(&direction, &direction, &attitude);

		if(bone < 0)
			bone = owner->FindBone(boneName.c_str());

		if(bone >= 0)
		{
			piVECTOR offsetVector;

			offsetVector.x = offset[0];
			offsetVector.y = offset[1];
			offsetVector.z = offset[2];

			owner->GetAbsoluteBonePosition(bone, &position, &offsetVector);
		}

		if(position.x + position.y + position.z)
		{
			id = owner->PlayEffect(effect.c_str(), &position, &direction, life);

			if(life == 0)
				id = -1;

			if(id >= 0) return gr2State::Start();
		}
	}

	return false;
}

void gr2EffectState::Update(FLOAT64 delta)
{
    gr2State::Update(delta);
	updateAcc += (UINT32)(delta*1000);

	if(bone >= 0 && update > 0 && updateAcc >= update && Active())
	{
		piVECTOR direction;
		piVECTOR offsetVector;
		piMATRIX attitude;

		offsetVector.x = offset[0];
		offsetVector.y = offset[1];
		offsetVector.z = offset[2];

		direction.x = dir[0];
		direction.y = dir[1];
		direction.z = dir[2];

		if((direction.x + direction.y + direction.z) && owner->GetAttitudeMatrix(&attitude))
			math3dVectorMultiply(&direction, &direction, &attitude);

		if(owner->GetAbsoluteBonePosition(bone, &position, &offsetVector))
			owner->UpdateEffect(id, &position, &direction);

		// reset the accumulator
		updateAcc = 0;
	}
}

bool gr2EffectState::Active()
{
	if(owner && id >= 0 && gr2State::Active())
		return ((life < 0) || ((started + life) > frametime));

	return false;
}

bool gr2EffectState::Stop()
{
	if(id >= 0)
		owner->StopEffect(id);

	id = -1;

	return gr2State::Stop();
}

gr2DamageState::gr2DamageState(gr2StateSpec* spec) :
	gr2State(spec),
	currentState(-1),
	currentLevel(0),
	component(""),
	damageComponent(NULL)
{
	gr2DamageStateSpec* damageSpec = (gr2DamageStateSpec*)spec;
	component = damageSpec->component;
}

bool gr2DamageState::Start(bool reset)
{
	if(!Active())
	{
		started = frametime;
		gr2DamageModel* damageModel = (owner) ? owner->GetDamageModel(): NULL;

		CallHandler(GSE_START);

		if(damageModel)
			damageComponent = damageModel->LookupComponent(component);

		UpdateDamageLevel();

		return true;
	}

	return false;
}

void gr2DamageState::Update(FLOAT64 delta)
{
	if(Active())
	{
		UpdateDamageLevel();

		CallHandler(GSE_UPDATE);

		if(currentState >= 0 && currentState < (int)states.size())
			states[currentState]->Update(delta);
	}
}

void gr2DamageState::UpdateDamageLevel()
{
	if(!damageComponent)
		return;

	if(currentLevel == damageComponent->GetDamageLevel())
		return;

	currentLevel = damageComponent->GetDamageLevel();

	if(currentState >= 0 && currentState < (int)states.size())
		states[currentState]->Stop();

	currentState = -1;

	char levelName[128+1];
	levelName[0] = 0;

	snprintf(levelName, 128, "%s%d", damageComponent->GetDamageModel()->GetDamageTag().c_str(), currentLevel);

	for(int i = 0; i < (int)states.size(); i++)
	{
		if(!strcasecmp(states[i]->name.c_str(), levelName))
		{
			currentState = i;
			break;
		}
	}

	if(currentState >= 0 && currentState < (int)states.size())
		states[currentState]->Start();

	// update the visual model
	damageComponent->UpdateVisualDamage();
}
