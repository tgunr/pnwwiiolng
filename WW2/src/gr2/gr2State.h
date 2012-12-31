#pragma once
// Date: Tue Sep  6 18:49:28 CDT 2005
//
// Defines the gr2State struct and types
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Troy Lowe

#include <vector>
#include "include/systypes.h"

// defines the state class's functionality
enum GR2_STATE_TYPE
{
	GST_INVALID = -1,
	GST_GENERIC,		//default
	GST_CONDITIONAL,	//1			// valid as long as its flag is still set and/or at least 1 child is still active
	GST_TIMED,
	GST_CHAIN,						// plays each state in turn
	GST_ANIMATION,					// base on timeloop
	GST_SOUND,			//5			// base on timeloop
	GST_EFFECT,						// base on timeloop
	GST_LOOP,						// fire's off any children each loop after "life" ms
	GST_DAMAGE,						// damage state handling
	GST_TRIGGER,					// sets and forgets about flag
	GST_TRANSITION,					// transitions between 2 base states
	GST_NUM_TYPES
};

// used to alert the parent object about state events
enum GR2_STATE_EVENT
{
	GSE_START,		// this state has started
	GSE_RESET,		// this state has been re-started
	GSE_UPDATE,		// loop or other state effect has completed
	GSE_COMPLETE,	// tells a parent state a child has completed
	GSE_STOP		// this state has stopped
};

enum GR2_FLAG_TYPE
{
	GFT_CONTROL,	// control input flags
	GFT_ACTION,		// default; idle, moving,
	GFT_CONDITION,	// engine on, burning, overheating
	GFT_DAMAGE,
	// add more types here
	GFT_NUM_TYPES
};

extern char* GR2_STATE_TYPE_NAMES[GST_NUM_TYPES];
extern char* GR2_FLAG_TYPE_NAMES[GFT_NUM_TYPES];

class gr2GameObject;
class gr2DamageComponent;
struct gr2State;
struct gr2StateSpec;
struct gr2AnimationBlend;

typedef void (gr2GameObject::*GSE_HANDLER)(GR2_STATE_EVENT, gr2State*);

struct gr2State
{
	gr2State(gr2StateSpec* spec);
	virtual ~gr2State();

	UINT64 GetKey();

	// set the parent object and handler function
	void SetHandler(gr2GameObject* o, GSE_HANDLER f);
	void AddState(gr2State* newState);

	virtual bool Start(bool reset = false);
	virtual void Update(FLOAT64 delta);
	virtual bool Active();
	virtual bool Stop();

	bool baseState;
	std::string name;
	int life;						// length of this state
	int started;					// when the state was started
	UINT64 key;
	GR2_STATE_TYPE type;			// type of state this is
	GR2_FLAG_TYPE flagType;			// type flag value is
	UINT32 value;					// class specific state value
	bool valued;					// does our value mean something?
	std::vector<gr2State*> states;

	static gr2State* Create(gr2StateSpec* spec);
	static UINT64 GetKey(GR2_FLAG_TYPE type, UINT32 state);

protected:

	gr2GameObject* owner;
	GSE_HANDLER handler;

	void CallHandler(GR2_STATE_EVENT e);
};

struct gr2TriggerState : public gr2State
{
	gr2TriggerState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual void Update(FLOAT64 delta);
	virtual bool Active();
	virtual bool Stop();
};

struct gr2ConditionalState : public gr2State
{
	gr2ConditionalState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual bool Active();
	//virtual bool Stop();
};

struct gr2TimedState : public gr2State
{
	gr2TimedState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual bool Active();
	//virtual bool Stop();
};

struct gr2AnimationState : public gr2State
{
	gr2AnimationState(gr2StateSpec* spec, gr2AnimationBlend* b = NULL);
	virtual ~gr2AnimationState();

	virtual bool Start(bool reset = false);
	virtual bool Active();
	virtual bool Stop();

	UINT32 loops;		// 0 = infinite;
	float speed;		// 1.0f = full speed
	float easeIn;
	float easeOut;
	gr2AnimationBlend* blend;
};

struct gr2SoundState : public gr2State
{
	gr2SoundState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual bool Active();
	virtual bool Stop();

	int id;					// id of the played sound; -1 if instant sound
	float volume;
	float minVolume;
	float maxVolume;
	std::string sound;
};

struct gr2EffectState : public gr2State
{
	gr2EffectState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual void Update(FLOAT64 delta);
	virtual bool Active();
	virtual bool Stop();

	int id;				// id of effect
	int bone;
	UINT32 updateAcc;	// update accumulator
	UINT32 update;		// how often to update; 0 means don't update
	float dir[3];
	float offset[3];
	FLOAT64VECTOR position;
	std::string effect;
	std::string boneName;
};

struct gr2LoopState : public gr2State
{
	gr2LoopState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual bool Active();
	virtual void Update(FLOAT64 delta);
	virtual bool Stop();

	bool stop;
	int lastRun; // last time a loop was run
};

struct gr2DamageState : public gr2State
{
	gr2DamageState(gr2StateSpec* spec);

	virtual bool Start(bool reset = false);
	virtual void Update(FLOAT64 delta);

	void UpdateDamageLevel();

	int currentLevel;
	int currentState;
	std::string component;
	gr2DamageComponent* damageComponent;
};

