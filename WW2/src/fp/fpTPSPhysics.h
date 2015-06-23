#pragma once

#include "gr2/gr2Physics.h" // for OBJECT's definition

#define TPS_ROPE
#undef TPS_ROPE

struct gr2AnimTrooper;
struct WeaponShape;
class fpTPSWeapon;

class TestRagdoll : public gr2PhysicsModel
{
public:
	TestRagdoll();
	virtual ~TestRagdoll();

	void AnimInit(gr2AnimTrooper* anim);
};

class fpTPSRagdoll : public gr2PhysicsModel
{
public:
	fpTPSRagdoll();
	virtual ~fpTPSRagdoll();

	// initialize this ragdoll to the incoming gr2AnimTrooper
	void AnimInit(gr2AnimTrooper* anim, const Vec3d* force = 0, int timeout = -1, int delay = -1);

	// helpers
	void Personalize(gr2AnimTrooper* anim);
	void AddDrama(const Vec3d* force);

	// update
	virtual void BeginStep();
	virtual void Update();
	virtual void EndStep();
	virtual void Deactivate();
	virtual bool ProcessFlag(unsigned int flag, physicsObject* o = nullptr);
	virtual void OnUnattach(gr2PhysicsModel* m);

private:

	int _delay;
	int _country;
	int _decal;
	int _nextContactSound, _nextImpactSound;
	std::string _handle;
	dJointID _weapJoint;
};

class fpTPSWeapon : public gr2PhysicsModel
{
public:
	fpTPSWeapon(gr2AnimTrooper* anim, const Vec3d* force = 0, const WeaponShape* collider = 0);
	virtual ~fpTPSWeapon();

	virtual void BeginStep();
	virtual void EndStep();
	virtual void InitPose(gr2Model* poseModel);
	virtual void OnExpire(gr2PhysicsModel::DRAW_STATE& state);
	virtual void OnRemove();
	virtual bool ProcessFlag(unsigned int flag, physicsObject* o = nullptr);

	// initialize this ragdoll weapon to the trooper ragdoll's weapon
	void AnimInit(gr2AnimTrooper* anim, fpTPSRagdoll* owner = nullptr, const Vec3d* force = 0, const WeaponShape* collider = 0);

private:

	void damp();
	bool makeCollider(const WeaponShape* collider);
	bool _damp;
};

class fpTPSKitObject : public gr2PhysicsModel
{
public:
	fpTPSKitObject(gr2AnimTrooper* anim, const Vec3d* force = 0, const WeaponShape* collider = 0);
	virtual ~fpTPSKitObject();

	virtual void BeginStep();
	virtual void EndStep();
	virtual void InitPose(gr2Model* poseModel);
	virtual void OnExpire(gr2PhysicsModel::DRAW_STATE& state);
	virtual void OnRemove();
	virtual bool ProcessFlag(unsigned int flag, physicsObject* o = nullptr);

	// initialize this ragdoll weapon to the trooper ragdoll's weapon
	void AnimInit(gr2AnimTrooper* anim, fpTPSRagdoll* owner = nullptr, const Vec3d* force = 0, const WeaponShape* collider = 0);

private:

	void damp();
	bool makeCollider(const WeaponShape* collider);
	bool _damp;
};

#ifdef TPS_ROPE
#include "weap/weapAmmo.h"
#include "draw/draw3d.h"

class fpTPSRope : public physicsBody
{
public:
	// head of rope
	fpTPSRope(dBody* anchor);

	// subsequent rope links
	fpTPSRope(fpTPSRope* rope);

	// destructor
	virtual ~fpTPSRope();

	virtual void EndStep();

	// draw us
	void Draw();

	// accessors
	OBJECT* Node(){ return _node; }

	void SetParent(fpTPSRope* p) { _parent = p; }
	void SetChild(fpTPSRope* c) { _child = c; }
	fpTPSRope* Tail();

	static dJointID Connect(dBody* from, dBody* to, Vec3d pos);

private:

	void _createNode();
	void _updateNode();
	void _initBody(Vec3d position, Matrixd rotation);

	OBJECT* _node; // our game object
	fpTPSRope* _parent; // who we attached too
	fpTPSRope* _child; // who is attached to us

	SINT32 _links; // max number of links
	SINT32 _nextLinkCheck;
};
#endif

extern void fpTPSRagdollInit();
extern void fpTPSRagdollKill();

extern fpTPSRagdoll* fpTPSAddRagdoll(gr2AnimTrooper* anim, int timeout = -1, int delay = -1, bool addForce = true);
extern bool fpTPSAllowRagdoll(FLOAT64VECTOR& pos);
extern void fpTPSCalcCamPos(Vec3d& camPos, Vec3d& lookAt, UINT32 maxLOS);
extern bool fpTPSDeathCamera(piVECTOR* pos, piVECTOR* aim);
extern bool fpTPSDeathPivot(piVECTOR* pos, piMATRIX* att);
extern void fpTPSDeathCameraReset();

#ifdef BETA_RELEASE
extern void fpTPSSetFallSounds(int numSounds);
extern void fpTPSSetFrictionModel(int start, int stop, int duration);
#endif
