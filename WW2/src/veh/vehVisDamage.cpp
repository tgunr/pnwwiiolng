#include "syscfg.h" // Path is project specific.
#include "veh/vehVisDamage.h"
#include "ogrp/ogrptype.h"
#include "include/vars.h"
#include "math/matheul.h"
#include "math/mathnum.h"
#include "ml/mltypes.h"
#include "ml/mlCompon.h"
#include "pers/persmain.h"
#include "trn/trnmain.h"
#include "math/mathvect.h"
#include "trn/trnop.h"
#include "veh/vehanim.h"
#include "physics/physicsMain.h"
#include "math/math3d.h"
#include "ob/obmain.h"
#include "pi/piogl.h"
#include "draw/draw3d.h"
#include "ogrp/ogrptype.h"
#include "fx/TransformEffect.h"
#include "fx/fxSnd.h"
#include "spt/Random.h"
#include "trn/trnvars.h"
#include "scen/ScenarioManager.h"
#include "ui/uiMain.h"
#include "veh/vehAnimEnum.h"

static char _comment[150];

/************************************************************************************************
	vehVisDamage
*************************************************************************************************/

#define CRAP_LIFETIME 30000 // ms

class VisualDamageBody;

static LOGICAL 	_RemoveComponents(vehVehicle*, const char* visdamgroup, LOGICAL CreateCrap);
static LOGICAL 	_AddComponents(vehVehicle*, const char* visdamgroup);
static void 	_HideDestroyedObjects(vehVehicle*, const char* visdamgroup);
static void 	_HideChildStubs(vehVehicle*, const char* visdamgroup);
static LOGICAL 	_HideCallback(OBJECT* lplod, UINT32 user);
static LOGICAL 	_ShowCallback(OBJECT* lplod, UINT32 user);
static LOGICAL _CollideOnCallback(OBJECT* obj, UINT32 user);
static LOGICAL _CreateCrapCallback(OBJECT* obj, UINT32 user);
#if(0)
static LOGICAL _FixCrapCallback(OBJECT* obj, UINT32 user);
#endif
static void _AddEjectSpeed(vehVehicle* veh, double EjectSpeed, FLOAT64VECTOR const* pos, FLOAT64VECTOR* v, FLOAT64VECTOR* w);

static void _CreateCrapAndStubs(vehVehicle*, const char* visdamgroup, LOGICAL LeaveStub, LOGICAL CreateCrap, LOGICAL Explode);
static void _CreateCrap(vehVehicle*, OBJECT* obj, const char* visdamgroup, LOGICAL Explode, SINT32 i);
#if(0)
static void _FixCrapAndStubs(vehVehicle*, const char* visdamgroup);
static void 	_ShowChildStubs(vehVehicle*, const char* visdamgroup);
static void 	_ShowDestroyedObjects(vehVehicle*, const char* visdamgroup);
static void _printCrap(VisualDamageBody* crap);
#endif

#define VD_EXPLODE_EFFECT_GROUND "groundvisdamage"
#define VD_EXPLODE_EFFECT_AIR    "airvisdamage"

#define VD_SMOKE_EFFECT "hulksmoke"

class VisualDamageBody : public physicsBody
{
public:

	VisualDamageBody(vehVehicle* v, OBJECT* o) :
		physicsBody(COLLIDER_DEBRIS, COLLIDER_TERRAIN_MASK),
		next(nullptr),
		prev(nullptr),
        obj(o),
        veh(v),
		expires(frametime + CRAP_LIFETIME),
		_allCollisions(0),
		_lastCollision(0),
		_lastFX(0),
		_nextPush(0),
		_explosion(false),
		_effect(nullptr)
	{
		if(v->ChunksOfCrap)
			v->ChunksOfCrap->prev = this;

		next = v->ChunksOfCrap;
		v->ChunksOfCrap = this;
	}

	virtual ~VisualDamageBody()
	{
		physicsRemoveObject(this, true);
	}

	virtual void Init(const vehanimVehicleStateType* st, const vehInertiaComponent* ic, LOGICAL explode)
	{
		_explosion = (explode == TRUE);

		OBJECT* visualObject = obj->child;

		//_offset.set(visualObject->pos); // loc

		InitObject(visualObject);

		setEnhancedCollisionCheck(false);
		setCollisionContacts(10);

		double m = ic->mass;
		if(m < 100) m = 100;

		FLOAT64VECTOR cm = {0,0,0};

		setMass(m, cm
					, ic->I[0][0], ic->I[1][1], ic->I[2][2]
					, ic->I[0][1], ic->I[0][2], ic->I[1][2]
					, obj);

		Matrixf attitude;
		Vec3d position;

		if(veh->ModelData)
		{
			attitude.set(veh->ModelData->VehicleAttMat);
			position.set(veh->ModelData->RenderPos);
		}
		else
		{
			attitude.set(veh->PosData.AttMat);
			position.set(veh->PosData.RenderPos);
		}

		_offset.z() = -_offset.z();

		AdjustForLag(position);

		_offset = _offset * attitude;
		position = position + _offset;

		// init our body rotation and position
		SetRotation(attitude);
		SetPosition(position);

		// create a collider for us
		makeMeshColliderFromObject(obj);

		// add us to the physics update list
		physicsAddObject(this);

		// set our initial velocity
		InitVelocities(st, explode);

		//SetEnabled(false);
	}

	virtual void InitObject(OBJECT* o)
	{
		if (o == nullptr)
			return;

		OBJECT* current = o;

		while (current)
		{
			if(!_offset.length2() && mathExactVectorLength2(&current->pos))
				_offset.set(current->pos);

			if (current->child)
				InitObject(current->child);

			if (current->attached)
				InitObject(current->attached);

			// reset the pos and loc members for our visual damage object
			current->pos.set(0,0,0);
			current->loc.set(0,0,0);

			// now let's reset the vertex positions to the vehicle's origin
			piVectorPtr pverts = current->MutablePositions();

			for(int i = 0; i < current->numverts; i++)
			{
				// move it back to the origin
				pverts->x -= (float)_offset.x();
				pverts->y -= (float)_offset.y();
				pverts->z -= (float)_offset.z();

				pverts++;
			}

			current = current->sibling;
		}
	}

	virtual FLOAT64VECTOR EjectPosition()
	{
		return getPosition();
	}

	virtual void InitVelocities(const vehanimVehicleStateType* st, LOGICAL explode)
	{
		// set up our initial velocities
		FLOAT64VECTOR lvel		= (veh->ModelData) ? veh->ModelData->VehicleRealVel : veh->PosData.LastUpdateVel;
		FLOAT64VECTOR avel	= {0,0,0};
		FLOAT64VECTOR pos	= EjectPosition();
		double VehSpeed			= mathExactVectorLength(&lvel);

		// add additional velocity from explosive force
		double EjectSpeed = (explode) ? VehSpeed * .7f : 0;

		if(st->EjectSpeed > EjectSpeed)
			EjectSpeed = st->EjectSpeed;

		double variant = 5.0f + GetRandom(5.0f, 10.0f);

		if(EjectSpeed > variant)
			EjectSpeed = variant;

		if(explode && veh->VehicleType->Type != VEH_TYPE_AIR)
			lvel.x = lvel.y = lvel.z = 0;

		_AddEjectSpeed(veh, EjectSpeed, &pos, &lvel, &avel);

		setLinearVel(lvel);
		setAngularVel(avel); // would be nice to set angular velocity for wheels
	}

	virtual void UpdateObject(Vec3d& woo)
	{
		OBJECT* visual = obj->child;
		if(!visual) return;

		Vec3d position = GetPosition();
		Matrixf attitude = GetRotationMatrix();

		position = position - woo;

		piMATRIX mat;
		piVECTOR attVec;
		FLOAT64ATT att;
		FLOAT64VECTOR pos;

		position.get(pos);
		pos.z = -pos.z;

		piVECTOR relPos;
		relPos.x = (float)pos.x;
		relPos.y = (float)pos.y;
		relPos.z = (float)pos.z;

		attitude.get(mat);

		mathEul_FromMatrix(&attVec, &mat, EulOrdXYZr);

		att.Pitch = attVec.x;
		att.Roll =  attVec.y;
		att.Yaw = attVec.z;

		// usual stupid sign flips
		attVec.x = -attVec.x;
		attVec.y = -attVec.y;
		//attVec.z = -attVec.z;

		ogrpMoveAndRotateObjectGroup(obj,&relPos,&attVec);

		//_printCrap(this);

		// vary airplane damage objects in flight to make them flutter
		if(frametime > _nextPush && veh->VehicleType->Type == VEH_TYPE_AIR && !_allCollisions)
		{
			Vec3d lvel = GetLinearVel();

			static float range = 5.0f;

			if(lvel.length2() >= (10.0f * 10.0f))
			{
				Vec3d push = Vec3d(GetRandom(-range, range), GetRandom(-range, range), 0.0f);
				lvel += push;
				SetLinearVel(lvel);
			}

			_nextPush = frametime + 100 + GetRandom(0, 200);
		}
	}

	virtual bool HighSpeedImpact(Vec3d& pos)
	{
		// only play this once
		if(!_allCollisions)
		{
			TransformEffect* te = TransformEffect::AddEffect("shockdust", Vec3d(0,0,1.0f), 0, nullptr);

			if(te)
			{
				te->enableemitters(true);
				te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				te->setmatrix(Matrixf::translate(pos));
			}
		}

		return true;
	}

	virtual bool handleContact(dContact* c)
	{
		if(c->geom.depth < .000001)
			return false;; // depth 0 from various trimesh colliders causes "sticks"

		// throw out any normals that are "on the wrong side" to prevent objects from "sticking" or penetrating
		// NOTE: this code is not generalized for all cases, it only works if center of object is inside geometry
		// so we don't do this for trimesh and assume the center is inside for simple capsule used by bouncing grenades
		dVector3 vec;
		dOPC(vec,*,c->geom.normal,c->geom.depth);
		dOP(vec,+,c->geom.pos,vec);
		dOP(vec,-,vec, _lastFreePos);

		if(dDOT(vec, c->geom.normal) > 0)
			return false;

		bool allowContact = true;
		const char* effect = nullptr;
		const char* sound = nullptr;
		Vec3d fxNormal = Vec3d(0,0,-1.0f);
		Vec3d fxPos = Vec3d(c->geom.pos[0], c->geom.pos[1], c->geom.pos[2]);

		fxPos = physicsPhysicalToGame(fxPos);
		fxPos.z() = -fxPos.z();

		UINT32 category = dGeomGetCategoryBits(c->geom.g2);

		Vec3d lvel = GetLinearVel();
		Vec3d posDiff = _lastFXPos - fxPos;

		double speed = lvel.length();

		bool isWater = colliderIsType(category, COLLIDER_TRN_WATER);

		if(!isWater) // if it's not water, then it's terrain
		{
			if(speed > 3.0f)
			{
				if(frametime > _lastFX + 200)
					sound = "sounds/macair394.wav";

				if(posDiff.length() > .33f)
				{
					effect = "debrishit";
					fxNormal.set(c->geom.normal[0], c->geom.normal[1], -c->geom.normal[2]);
				}

				static double maxSpeed = 10.0f;

				if(speed >= maxSpeed)
				{
					allowContact = HighSpeedImpact(fxPos);
				}
			}
		}
		else if(speed > 1.0f)
		{
			if(frametime > _lastFX + 200)
			{
				sound = "sounds/grenadesplash.wav";
				expires = frametime + 1000;
			}

			if(posDiff.length() > .33f)
				effect = "deadtroopersplash";

			// we don't collide with water
			allowContact = false;
		}

		if(sound || effect)
		{
			FLOAT64VECTOR pos = getPosition();

			if(effect)
			{
				TransformEffect* te = TransformEffect::AddEffect(effect, fxNormal, 0, nullptr);

				if(te)
				{
					te->enableemitters(true);;
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
					te->setmatrix(Matrixf::translate(fxPos));
				}
			}

			if(sound)
				fxSndAddFiniteWorldSound(sound, 10, 100, IDE_TP_PRIORITY_LOW, pos);

			_lastFX = frametime;
			_lastFXPos = fxPos;
		}

		// we hit something
		if(_explosion && _effect)
		{
#if(0)
			_effect->enable(false);
#endif
			_effect = nullptr;

			// start smoking
			TransformEffect* te = TransformEffect::AddEffect(VD_SMOKE_EFFECT, Vec3d(0,0,1.0f), CRAP_LIFETIME, obj);

			if(te)
			{
				te->enableemitters(true);
				te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
			}
		}

		_allCollisions |= category;
		_lastCollision |= category;

		return allowContact;
	}

	virtual void AdjustForLag(Vec3d& position)
	{
		if(veh->IsMe() || veh->VehicleType->Type != VEH_TYPE_AIR)
			return;

		const double travel = 100.0f;

		Vec3d vel = Vec3d(veh->PosData.LastUpdateVel);

		if(!vel.length2())
			vel.set(0, 0, -1.0f);

		Vec3d start = position;
		Vec3d dir = vel * -1.0f;

		dir.normalize();
		dir *= travel;

		start += dir;

		dir.normalize();
		dir *= -1.0;

		start = physicsGameToPhysical(start);

		float hitDist = collideRay(COLLIDER_TRN_GROUND|COLLIDER_TRN_WATER, travel, start, dir);

		if(hitDist)
		{
			position = physicsPhysicalToGame(start);
			position += (dir * (hitDist - 3.0f));
		}
	}

	// our linked list members
	VisualDamageBody* next;
	VisualDamageBody* prev;

	// our root visual damage object
	// object just above the visual object
	OBJECT* obj;

	// vehicle we're attached too
	vehVehicle* veh;

	// when we expire and removed
	SINT32 expires;

protected:

	UINT32 _allCollisions;
	UINT32 _lastCollision;

	SINT32 _lastFX;
	SINT32 _nextPush;

	Vec3d _lastFXPos;
	Vec3d _offset;

	bool _explosion;
	TransformEffect* _effect;
};

// we subclass a few types for damage specific behavior
class WheelDamageBody : public VisualDamageBody
{
public:

	WheelDamageBody(vehVehicle* v, OBJECT* o) :
		VisualDamageBody(v, o)
	{
	}

	// override and add engular velocity perpendicular to the vehicle's velocity
	// so wheel's roll on, etc...
	// we'll also add some sparks here at the end of the call for effect
	virtual void InitVelocities(const vehanimVehicleStateType* st, LOGICAL explode)
	{
		VisualDamageBody::InitVelocities(st, explode);

		Vec3d pos = GetPosition();
		pos.z() = -pos.z();

		Vec3d vel = Vec3d(getLinearVel());
		vel.normalize();

		Vec3d fxNormal;

		if(explode)
		{
			fxNormal.set(0,0,1.0f);
			TransformEffect* te = TransformEffect::AddEffect(VD_SMOKE_EFFECT, fxNormal, CRAP_LIFETIME, obj);

			if(te)
			{
				te->enableemitters(true);;
				te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
			}
		}
		else
		{
			for(int s = 0; s < 3; s++)
			{
				fxNormal = vel;

				fxNormal.x() += GetRandom(-.1f, .1f);
				fxNormal.y() += GetRandom(-.1f, .1f);
				fxNormal.z() += GetRandom(-.1f, .1f);
				fxNormal.normalize();

				TransformEffect* te = TransformEffect::AddEffect("metalbullethit", fxNormal, 0, nullptr);

				if(te)
				{
					te->enableemitters(true);;
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
					te->setmatrix(Matrixf::translate(pos));
				}
			}
		}
	}

	virtual ~WheelDamageBody()
	{
	}
};


class TurretDamageBody : public VisualDamageBody
{
public:

	TurretDamageBody(vehVehicle* v, OBJECT* o) :
		VisualDamageBody(v, o)
	{
			if(v->ModelData)
				v->ModelData->HulkOnDespawn = TRUE;
	}

	virtual FLOAT64VECTOR EjectPosition()
	{
		FLOAT64VECTOR pos = getPosition();

		// vary the eject direction for turrets
		pos.x = veh->PosData.RenderPos.x + GetRandom(-1.0f, 1.0f);
		pos.y = veh->PosData.RenderPos.y + GetRandom(-1.0f, 1.0f);
		// just use current z

		return pos;
	}

	// override and add a transform effect so the turret smokes for a while as it's tossed
	// we'll also add some sparks here at the end of the call for effect
	virtual void InitVelocities(const vehanimVehicleStateType* st, LOGICAL explode)
	{
		VisualDamageBody::InitVelocities(st, explode);

		_effect = TransformEffect::AddEffect(VD_EXPLODE_EFFECT_GROUND, Vec3d(0, 0, 1.0f), CRAP_LIFETIME, obj);

		if(_effect)
		{
			_effect->enableemitters(true);;
			_effect->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
			//te->setmatrix(Matrixf::translate(epos));

			_explosion = true;
		}
	}

	virtual ~TurretDamageBody()
	{
	}
};

class EngineDamageBody : public VisualDamageBody
{
public:

	EngineDamageBody(vehVehicle* v, OBJECT* o) :
		VisualDamageBody(v, o)
	{
		//if(v->ModelData)
		//	v->ModelData->HulkOnDespawn = TRUE;
	}

	// override and add a transform effect so the turret smokes for a while as it's tossed
	// we'll also add some sparks here at the end of the call for effect
	virtual void InitVelocities(const vehanimVehicleStateType* st, LOGICAL explode)
	{
		VisualDamageBody::InitVelocities(st, explode);

		_effect = TransformEffect::AddEffect(VD_EXPLODE_EFFECT_AIR, Vec3d(0, 0, 1.0f), GetRandom(3000, 7000), obj);

		if(_effect)
		{
			_effect->enableemitters(true);;
			_effect->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
			_explosion = true;
		}
	}

	virtual ~EngineDamageBody()
	{
	}
};

class WingDamageBody : public VisualDamageBody
{
public:

	WingDamageBody(vehVehicle* v, OBJECT* o, bool w) :
		VisualDamageBody(v, o),
		isWing(w)
	{
		if(v->ModelData && w)
			v->ModelData->HulkOnDespawn = TRUE;
	}

	// override and add angular velocity around the velocity of the aircraft
	// so the wing appears to lift off it's connect point
	// we'll also add some sparks here at the end of the call for effect
	virtual void InitVelocities(const vehanimVehicleStateType* st, LOGICAL explode)
	{
		// set up our initial velocities
		Vec3d lvel = Vec3d((veh->ModelData) ? veh->ModelData->VehicleRealVel : veh->PosData.LastUpdateVel);
		Vec3d avel = lvel;

		if(avel.length2() == 0)
			avel = Vec3d(0, 30.0f, 0);

		avel.normalize();
		avel *= GetRandom(-2.0f, 2.0f);

		Vec3d oPos	= GetPosition();
		Vec3d vPos = Vec3d(veh->PosData.RenderPos);

		Vec3d push = oPos - vPos;
		push.normalize();
		push *= GetRandom(0.0f, 5.0f);

		if(explode || isWing)
		{
			_effect = TransformEffect::AddEffect(VD_EXPLODE_EFFECT_AIR, Vec3d(0, 0, 1.0f), GetRandom(6000, 14000), obj);

			if(_effect)
			{
				_effect->enableemitters(true);;
				_effect->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				//te->setmatrix(Matrixf::translate(epos));
				_explosion = true;
			}

			push *= 2.5f;
			//avel *= 2.0f;

			lvel *= GetRandom(.2f, .7f);
		}

		lvel += push;

		SetLinearVel(lvel);
		SetAngularVel(avel);
	}

	virtual ~WingDamageBody()
	{
	}

	bool isWing;
};

// for now, will always be a third person vehicle
class HulkDamageBody : public VisualDamageBody
{
public:

	HulkDamageBody(vehVehicle* v, OBJECT* o) :
		VisualDamageBody(v, o)
	{
		// claim it as ours
		v->Flags |= VEH_FLAGS_ODE_CONTROLLED;

		// turn off sounds and some emitters
		KillStates();
	}

	virtual ~HulkDamageBody()
	{
	}

	virtual void Init(const vehanimVehicleStateType* st, const vehInertiaComponent* ic, LOGICAL explode)
	{
		OBJECT* visualObject = obj->child;

		//_offset.set(visualObject->pos); // loc
		_offset.set(0,0,0);

		//InitObject(visualObject);

		setEnhancedCollisionCheck(false);
		setCollisionContacts(10);

		//double density = 1.0f;
		FLOAT64VECTOR cm = {0,0,0};

		// set our mass
		setMass(500.0f, cm, 0, 0, 0, 0, 0, 0, visualObject);

		_cm[0] = 0;
		_cm[1] = 0;
		_cm[2] = 0;

		Matrixf attitude = Matrixf((veh->ModelData) ? veh->ModelData->VehicleAttMat: veh->PosData.AttMat);
		Vec3d position = Vec3d(veh->PosData.RenderPos);

		_offset.z() = -_offset.z();

		AdjustForLag(position);

		_offset = _offset * attitude;
		position = position + _offset;

		// init our body rotation and position
		SetRotation(attitude);
		SetPosition(position);

		// create a collider for us
		makeMeshColliderFromObject(obj);

		// set our mass
		//setMassFromMesh(density, cm);

		// add us to the physics update list
		physicsAddObject(this);

		// set our initial velocity
		InitVelocities(st, explode);

		//SetEnabled(false);
	}

	virtual void InitVelocities(const vehanimVehicleStateType* st, LOGICAL explode)
	{
		// set our linear velocity to that of our vehicle's
		Vec3d lvel = Vec3d((veh->ModelData) ? veh->ModelData->VehicleRealVel : veh->PosData.LastUpdateVel);

		if(veh->VehicleType->Type == VEH_TYPE_AIR)
			lvel *= GetRandom(.2f, .5f);

		if(!Online)
		{
			if(lvel.length2() == 0)
				lvel.set(GetRandom(-5.0f, 5.0f), GetRandom(-5.0f, 5.0f), GetRandom(1.0f, 5.0f));

			Vec3d avel = Vec3d(GetRandom(-2.0f, 2.0f), GetRandom(-2.0f, 2.0f), GetRandom(-2.0f, 2.0f));

			SetAngularVel(avel);
		}

		SetLinearVel(lvel);
	}

	virtual void UpdateObject(Vec3d& woo)
	{
		VisualDamageBody::UpdateObject(woo);

		Vec3d position = GetPosition();
		Matrixf attitude = GetRotationMatrix();

		if(veh->ModelData) // first person
		{
			/*
			mlVehicleData* VehicleData = veh->ModelData;

			piMATRIX mat;
			piVECTOR attVec;
			FLOAT64ATT Att;
			FLOAT64VECTOR Pos;

			position.get(Pos);
			//Pos.z = -Pos.z;

			attitude.get(mat);

			mathEul_FromMatrix(&attVec, &mat, EulOrdXYZr);

			Att.Pitch = -attVec.x;
			Att.Roll =  -attVec.y;
			Att.Yaw = attVec.z;

			// now we update the vehicle
			// this was taken from _DoEyeOperations

			if(VehicleData)
			{
				VehicleData->VehicleRealAtt.Pitch	= Att.Pitch;
				VehicleData->VehicleRealAtt.Yaw	= Att.Yaw;
				VehicleData->VehicleRealAtt.Roll		= Att.Roll;
				VehicleData->VehiclePos				= Pos;
				VehicleData->VehicleLastVel			= VehicleData->VehicleVel;

				VehicleData->VehicleVel.x				= 0;
				VehicleData->VehicleVel.y				= 0;
				VehicleData->VehicleVel.z				= 0;

				// Set all of the attitudes and positions in all of the appoximately one billion places
				// where this info is stored and used by who knows what and who knows how.
				VehicleData->RenderPos		= VehicleData->VehiclePos;
				VehicleData->RenderAtt		= VehicleData->VehicleRealAtt;
			}

			veh->PosData.RenderPos			= Pos;
			veh->PosData.RenderAtt			= Att;

			veh->PosData.Pos					= Pos;
			veh->PosData.Att					= Att;

			if(VehicleData)
			{
				VehicleData->VehicleAtt				= VehicleData->VehicleRealAtt;

				if(veh->VehicleType->Type != VEH_TYPE_FLOATER) // usual random sign flips
				{
					VehicleData->VehicleAtt.Pitch = -VehicleData->VehicleAtt.Pitch;
					VehicleData->VehicleAtt.Yaw = -VehicleData->VehicleAtt.Yaw;
				}

				attVec.x = (FLOAT32)VehicleData->VehicleRealAtt.Pitch;
				attVec.y = (FLOAT32)VehicleData->VehicleRealAtt.Roll;
				attVec.z = (FLOAT32)VehicleData->VehicleRealAtt.Yaw;

				mathEul_ToMatrix(&VehicleData->VehicleAttMat,EulOrdXYZs,&attVec);
				math3dInvert2(&VehicleData->VehicleAttInvMat,&VehicleData->VehicleAttMat);

				FLOAT64VECTOR TempVec = VehicleData->VehicleCenterOfMass;
				mathVectorMultiply64(&TempVec,&TempVec,&VehicleData->VehicleAttMat);

				VehicleData->AdjustedForCGPos = VehicleData->VehiclePos;
				VehicleData->AdjustedForCGPos.x -= TempVec.x;
				VehicleData->AdjustedForCGPos.y -= TempVec.y;
				VehicleData->AdjustedForCGPos.z -= TempVec.z;

				//_UpdateBBoxList(veh->VehicleData);
				VehicleData->VehicleLastPos = VehicleData->VehiclePos;
				VehicleData->LastVehicleUpdateTime = frametime;

				// now we update our posdata
				vehUpdatePositionInfo(veh);
			}
			*/
		}
		else // third person
		{
			piMATRIX mat;
			piVECTOR attVec;
			FLOAT64ATT Att;
			FLOAT64VECTOR Pos;

			position.get(Pos);
			//Pos.z = -Pos.z;

			attitude.get(mat);

			mathEul_FromMatrix(&attVec, &mat, EulOrdXYZr);

			Att.Pitch = -attVec.x;
			Att.Roll =  -attVec.y;
			Att.Yaw = attVec.z;

			// now we update the vehicle
			// this was taken from _DoEyeOperations

			veh->PosData.RenderPos			= Pos;
			veh->PosData.RenderAtt			= Att;
			veh->PosData.Pos					= Pos;
			veh->PosData.Att					= Att;
		}

		if(veh->VehicleType->Type == VEH_TYPE_AIR)
		{
			Vec3d lvel = GetLinearVel();

			if(lvel.length2() < 1.0f)
			{
				// turn off glycol leaks

			}
		}
	}

	void KillStates()
	{
		return;

		for (int i=0; i < VEH_MAX_STATE_BITS; i++)
		{
			vehanimVehicleStateType& type = veh->VehicleType->StateType[i];

			switch(type.StateType)
			{
				case VehAnim::STATE_TYPE_EFFECT:
				case VehAnim::STATE_TYPE_SMOKE:
				case VehAnim::STATE_TYPE_SOUND:
					vehAnimChangeVehicleStateByNumber(veh, i, FALSE);
				break;
			}
		}
	}
};

/************************************************************************************************
	vehVisDamageGroupDestroyed
*************************************************************************************************/

SINT32 vehVisDamageGroupDestroyed(vehVehicle* veh, const char* visdamgroup, LOGICAL LeaveStub, LOGICAL CreateCrap)
{
	LOGICAL HulkVehicle = FALSE;
	veh->VisDamageNoStick = TRUE;

	if(veh->VehicleObjectData == nullptr)
		return -1;

	// remove components if it is a user controlled vehicle we're running physics on and determine if we're hulked.
	// 1st person determines hulk based on component, 3rd person sends explicit hulk visdamage state
	if(veh->ModelData)
	{
		HulkVehicle = _RemoveComponents(veh, visdamgroup, CreateCrap);
	}
	else
	{
		HulkVehicle = !strcmp(visdamgroup, "hulk");
	}

	// hide/disable all appropriate objects associated with damage group
	_HideDestroyedObjects(veh, visdamgroup);

	// show and blow off associated crap
	// causes major physics stutter for now; need to fix
	_CreateCrapAndStubs(veh, visdamgroup, LeaveStub, CreateCrap, HulkVehicle);

	// handle case where child is already stubbed
	_HideChildStubs(veh, visdamgroup);

	// change anim state
	vehAnimChangeVehicleStateByName(veh, (UCHAR*)visdamgroup, 1);

	// Hulk the vehicle if destroying the component would cause it
	if(HulkVehicle)
	{
		vehVisDamageHulkVehicle(veh, CreateCrap);
	}
	else
	{
		// update the collide lines, hulk does this as well
		if(veh->ModelData) // only need to do on your vehicle
		{
			snprintf(_comment, sizeof(_comment)-1, "visdamageon=%s", visdamgroup);
			obmainFindCommentCallback(veh->VehicleObjectData->LineCollideObject, _comment, 0, _CollideOnCallback);
			veh->VehicleObjectData->ProcessContactPointObject = TRUE;
		}
	}

	return 0;
}

/************************************************************************************************
	vehVisDamageGroupRepaired
*************************************************************************************************/

SINT32 vehVisDamageGroupRepaired(vehVehicle* veh, const char* visdamgroup)
{
	veh->VisDamageNoStick = TRUE;

	if(veh->VehicleObjectData == nullptr)
		return -1;

	// add components if it is a user controlled vehicle we're running physics on and determine if we're hulked.
	// 1st person determines hulk based on component, 3rd person sends explicit hulk visdamage state
	if(false && veh->ModelData)
	{
		_AddComponents(veh, visdamgroup);
	}

	// hide all under veh->VehicleObjectData->VisDamageGroup
	// show highest lod
	veh->MinVehicleLOD = 0;

	// update it
	OBJECT* lod = veh->VehicleObjectData->LODObjects[veh->MinVehicleLOD];

	if(lod)
	{
		// setup our comment
		snprintf(_comment, sizeof(_comment)-1, "visdamage=%s", visdamgroup);

		// make our visual model show again
		obmainFindCommentCallback(lod, _comment, (UINT32)veh, _ShowCallback);

		// hide our visual damage model and stubs
		obmainFindCommentCallback(veh->VehicleObjectData->VisDamageGroup, _comment, 0, _HideCallback);
	}

	// show/enable all appropriate objects associated with damage group
	//_ShowDestroyedObjects(veh, visdamgroup);

	// fix stubs
	//_FixCrapAndStubs(veh, visdamgroup);

	// handle case where child is already stubbed
	//_ShowChildStubs(veh, visdamgroup);

	// change anim state
	vehAnimChangeVehicleStateByName(veh, (UCHAR*)visdamgroup, FALSE);

	// update the collide lines, hulk does this as well
	/*
	if(false && veh->ModelData) // only need to do on your vehicle
	{
		snprintf(_comment, sizeof(_comment)-1, "visdamageon=%s", visdamgroup);
		obmainFindCommentCallback(veh->VehicleObjectData->LineCollideObject, _comment, 0, _CollideOnCallback);
		veh->VehicleObjectData->ProcessContactPointObject = TRUE;
	}
	*/

	return 0;
}

/************************************************************************************************
	vehVisDamageHulkVehicle
*************************************************************************************************/

SINT32 vehVisDamageHulkVehicle(vehVehicle* veh, LOGICAL CreateCrap)
{
	if(veh->VehicleObjectData == nullptr || veh->VehicleObjectData->VisDamageHulkLOD == nullptr)
		return -1;

	const vehanimVehicleStateType* stateType = vehFindStateByName(veh->VehicleType, "hulk");
	if(stateType == nullptr) return -1;

	//int index = vehAnimFindVehicleStateNumber(veh, "hulk");

	// have we already hulked?
	//if(index == -1 || veh->PosData.States[index])
	//	return -1;

	vehVehicle* player = persGetPersonaVehiclePtr();
	if(player == nullptr || player->VehicleType == nullptr) return -1;

	bool myVehicle = (veh == player);
	int i = 0;
	OBJECT* Object;

	// nuke all the LOD's
	for(i = 0; i < ML_MAX_LODS; i++)
	{
		Object = veh->VehicleObjectData->LODObjects[i];
		if (Object != nullptr)
		{
			while (Object->flags & OBJECT_NOTOBJECT)
			{
				Object = Object->child;
				if (Object == nullptr)
					break;
			}
		}

		if (Object != nullptr)
		{
			Object->minlod = 0.0f;
			Object->maxlod = 0.0f;
		}
	}

	// most vehicles control their own hulks
	bool createHulkBody = false;

	// exceptions
	// offline, we use a hulk body if it's not us
	// also, online as an airplane, we won't have ode colliders, but ground and sea vehicles will
	// planes move at great speeds and can behave wierd when hitting the ground while host controlled,
	// so for planes, while we're not an airplane, we use a hulk body to simulate the physics of a crashing
	// plane. If I'm a plane tho, all hulks including airplanes will still be host controlled because as
	// stated above, I won't have ode colliders on the ground for things to collide with

	if(!Online && !myVehicle)
		createHulkBody = true;
	else if(player->VehicleType->Type != VEH_TYPE_AIR && veh->VehicleType->Type == VEH_TYPE_AIR)
		createHulkBody = true;

	VisualDamageBody* crap = veh->ChunksOfCrap;

	// make sure we haven't created our hulk damage body already
	if(createHulkBody && crap)
	{
		while(crap)
		{
			// check if a hulk crap
			HulkDamageBody* hulkBody = dynamic_cast<HulkDamageBody*>(crap);

			if(hulkBody)
			{
				createHulkBody = false;
				break;
			}

			crap = crap->next;
		}
	}

	// and lodloadout
	if(veh->VehicleObjectData->LoadoutLODObject)
		ogrpHideObjectAndSubtree(veh->VehicleObjectData->LoadoutLODObject);

	// blow off rest of components
	if(myVehicle && veh->ModelData->ComponentList)
	{
		// go down list and nuke anything with a visible damage component
		mlComponentListIterator it;
		mlComponentListBegin(veh->ModelData->ComponentList, &it);

		for(; it.comp; mlComponentListNext(&it))
		{
			mlComponent* comp = it.comp;

			if(comp->VisDamage[0][0] != '\0')
			{
				comp->ComponentStatus = ML_STATUS_MISSING;
				_HideDestroyedObjects(veh, comp->VisDamage[0]);

				if(veh->VehicleType->Type == VEH_TYPE_AIR && veh->ModelData->FreezeVehicle == 1)
					_CreateCrapAndStubs(veh,comp->VisDamage[0], FALSE, FALSE, TRUE);
				else
					_CreateCrapAndStubs(veh,comp->VisDamage[0], TRUE, CreateCrap, TRUE);

				vehAnimChangeVehicleStateByName(veh, (UCHAR*)comp->VisDamage[0], 1);
			}
		}
	}

	/*
	// if it's not us, shake the came a little for the explosion
	if(!myVehicle && !veh->isHulked())
	{
		FLOAT64VECTOR& vPos = veh->PosData.Pos;

		if(player->InRange(vPos.x, vPos.y, vPos.z, 50.0f))
			persShakeCamera(.005f, 20000);

		// we handle augering planes as a specialty case
		if(Online && veh->VehicleType->Type == VEH_TYPE_AIR)
		{
			// when online, a plane impacting on the ground will actually
			// interpolate below the ground before the hulk message is received,
			// this is due to network lag. As a result, the cool hulking happens underground
			// so, we back up the position a little and shoot a ray in the direction of the velocity
			// to see where we hit the ground at. if our position is below the ground, move
			// the vehicle just above the ground so that the impact and resulting hulk displays
			// at the surface where it should be
			Vec3d vel = Vec3d(veh->PosData.Vel);
			Vec3d pos = Vec3d(vPos);
			Vec3d back = vel * -1.0f;
			Vec3d start = pos + back;



			if(vel.length2() >=
		}
	}
	*/

	// make LOD Hulk visible
	Object = veh->VehicleObjectData->VisDamageHulkLOD;
	if (Object != nullptr)
	{
		veh->VehicleObjectData->VisDamageHulkLOD->flag2 &= ~OBJECT_DONT_UPDATE;
		ogrpShowObjectAndSubtree(veh->VehicleObjectData->VisDamageHulkLOD);

		while (Object->flags & OBJECT_NOTOBJECT)
		{
			Object = Object->child;
			if (Object == nullptr)
				break;
		}

		// update hitmap and walk object
		if(Object)
		{
			veh->VehicleObjectData->CollideWalkObject = Object;
			veh->VehicleObjectData->CollideExternObject = Object;
			veh->VehicleObjectData->CollideInternObject = nullptr;

			// update our walk object's flags so the collider get's created correctly
			//obmainSetFlag3InThisTree(Object, OBJECT_WALK);

			if(!createHulkBody)
				physicsRecurseAndCreateObjectCollider(Object, veh->spaceID, COLLIDER_VEH_HULK, 0, false, true, false);
		}

		// this can result in multiple bodies being created
		if(createHulkBody)
		{
			HulkDamageBody* hulk = new HulkDamageBody(veh, veh->VehicleObjectData->VisDamageHulkLOD);
			if(hulk) hulk->Init(nullptr, nullptr, FALSE);

			if(0)
			{
				// physics object line
				FLOAT64VECTOR v1, v2;
				Vec3d pv1, pv2;

				pv1		= hulk->GetPosition();
				pv2		= pv1;
				pv2.z() += 10.0f;

				pv1 = physicsPhysicalToGame(pv1);
				pv2 = physicsPhysicalToGame(pv2);

				pv1.get(v1);
				v1.z = -v1.z;

				pv2.get(v2);
				v2.z = -v2.z;

				int id = draw3dSetWorldDebugLine(&v1, &v2, "hulkpos", colorREDALPHA);
				draw3dSetDebugObjectAlwaysOnTop(id, TRUE);
			}
		}
	}

	// this probably doesn't need to be an LOD but what the heck
	if (Object != nullptr)
	{
		Object->minlod = 0.0f;
		Object->maxlod = 100000000.0f;
	}

	// set hulk anim state, tells all other clients to hulk the 3rd person vehicle
	vehAnimChangeVehicleStateByName(veh, (UCHAR*)"hulk", 1);

	// update contact points
	if(myVehicle)
		veh->VehicleObjectData->ProcessContactPointObject = TRUE;

	// hide all the stubs
	if(veh->VehicleObjectData->VisDamageStubGroup)
	{
		ogrpHideObjectAndSubtree(veh->VehicleObjectData->VisDamageStubGroup);
		obmainSetFlag2InThisTree(veh->VehicleObjectData->VisDamageStubGroup, OBJECT_DONT_UPDATE | OBJECT_MISSING | OBJECT_HIDDEN);
	}

    // No explosions for STO hulks.
    if(!veh->hulksto)
    {
        vehAnimChangeVehicleStateByName(veh, (UCHAR*)"crashexplosion", TRUE);

		/* taking this out unless I see a problem
		// there is now hulking for hulks sake; ie we do it if we see some significant damage just for pure gameplay fun
		// so don't kill the play because we're spicing the game up; this call was simply an insurance call anyway
		// root problems for the player not dying ought to be fixed instead

        if(veh->ModelData)  // and make sure the player freaking dies if they're not already, sure there's plenty of other hoops I'll have to jump through
        {
	        veh->ModelData->VehicleIsDead = TRUE;
	        persKillCrew();
        }
		*/
	}

	veh->VisDamageNoStick = FALSE;

	if(veh->VehicleObjectData->HitMap)
		physicsRecurseAndToggleObjectCollider(veh->VehicleObjectData->HitMap, false);

	if(myVehicle && !persIsInDeathCam())
		persSetDeathCam(2);

	veh->Flags |= VEH_FLAGS_HULKED;

	return 0;
}

/************************************************************************************************
	vehVisDamageHulkVehicle
*************************************************************************************************/

SINT32 vehVisDamageUnHulkVehicle(vehVehicle* veh)
{
	OBJECT* Object;
	vehanimVehicleStateType const* stateType;

	if(veh->VehicleObjectData == nullptr || veh->VehicleObjectData->VisDamageHulkLOD == nullptr)
		return -1;

	stateType = vehFindStateByName(veh->VehicleType, "hulk");
	if(stateType == nullptr) return -1;

	// nuke all the LOD's
	/*
	for(i = 0; i < ML_MAX_LODS; i++)
	{
		Object = veh->VehicleObjectData->LODObjects[i];
		if (Object != nullptr)
		{
			while (Object->flags & OBJECT_NOTOBJECT)
			{
				Object = Object->child;
				if (Object == nullptr)
					break;
			}
		}

		if (Object != nullptr)
		{
			Object->minlod = 0.0f;
			Object->maxlod = 0.0f;
		}
	}
	*/

	// and lodloadout
	if(veh->VehicleObjectData->LoadoutLODObject)
		ogrpShowObjectAndSubtree(veh->VehicleObjectData->LoadoutLODObject);

	// make LOD Hulk visible
	Object = veh->VehicleObjectData->VisDamageHulkLOD;
	if (Object != nullptr)
	{
		veh->VehicleObjectData->VisDamageHulkLOD->flag2 |= OBJECT_DONT_UPDATE;
		ogrpHideObjectAndSubtree(veh->VehicleObjectData->VisDamageHulkLOD);
		physicsRecurseAndFreeObjectCollider(veh->VehicleObjectData->VisDamageHulkLOD, false);
	}

	//ogrpFindSubObjectByName(&Object,veh->VehicleObjectData->PlaneObject,"hitbox");
	//veh->VehicleObjectData->HitBox = Object;

	ogrpFindSubObjectByName(&Object,veh->VehicleObjectData->PlaneObject,"hitmap");
	veh->VehicleObjectData->HitMap = Object;

	if (Object != nullptr)
	{
		OBJECT* Object1 = nullptr;
		ogrpFindSubObjectByName(&Object1,Object,"inside");
		veh->VehicleObjectData->CollideInternObject = Object1;

		Object1 = nullptr;
		ogrpFindSubObjectByName(&Object1,Object,"outside");
		veh->VehicleObjectData->CollideExternObject = Object1;

		Object1 = nullptr;
		ogrpFindSubObjectByName(&Object1,Object,"walk");
		veh->VehicleObjectData->CollideWalkObject = Object1;

		veh->VehicleObjectData->Radius = 0;

		mlCalculateObjectTreeRadius(&(veh->VehicleObjectData->Radius),Object,0);
		veh->VehicleObjectData->Radius = (FLOAT32)(sqrt(veh->VehicleObjectData->Radius) + 1.0); // margin of error
	}
	else
	{
		veh->VehicleObjectData->CollideInternObject = nullptr;
		veh->VehicleObjectData->CollideExternObject = nullptr;
		veh->VehicleObjectData->Radius = 0.0;
		if (veh->VehicleObjectData->PlaneObject->flag2 & OBJECT_ANIMATION)
			veh->VehicleObjectData->Radius = 3.0;
	}

	UINT32 cat = COLLIDER_VEH_GROUND;

	switch(veh->VehicleType->Type)
	{
		case VEH_TYPE_PERSON:	cat = COLLIDER_VEH_TROOP;		break;
		case VEH_TYPE_AIR:		cat = COLLIDER_VEH_AIR;			break;
		case VEH_TYPE_GROUND:	cat = COLLIDER_VEH_GROUND;	break;
		case VEH_TYPE_SEA:		cat = COLLIDER_VEH_SEA;			break;
	}

	if(veh->VehicleObjectData->CollideInternObject)
		physicsRecurseAndCreateObjectCollider(veh->VehicleObjectData->CollideInternObject, veh->spaceID,cat,0,false,false,false);

	if(veh->VehicleObjectData->CollideExternObject)
		physicsRecurseAndCreateObjectCollider(veh->VehicleObjectData->CollideExternObject, veh->spaceID,cat,0,false,false,false);


	// blow off rest of components
/*
	if(veh->ModelData && veh->ModelData->ComponentList)
	{
		// go down list and nuke anything with a visible damage component
		mlComponentListIterator it;
		mlComponentListBegin(veh->ModelData->ComponentList, &it);
		for(; it.comp; mlComponentListNext(&it))
		{
			mlComponent* comp = it.comp;
			if(comp->VisDamage[0][0] != '\0')
			{
				comp->ComponentStatus = ML_STATUS_MISSING;
				_HideDestroyedObjects(veh, comp->VisDamage[0]);
				_CreateCrapAndStubs(veh,comp->VisDamage[0], TRUE, CreateCrap, TRUE);
				vehAnimChangeVehicleStateByName(veh, (UCHAR*)comp->VisDamage[0], 1);
			}
		}
	}
*/

	// set hulk anim state, tells all other clients to hulk the 3rd person vehicle
	vehAnimChangeVehicleStateByName(veh, (UCHAR*)"hulk", FALSE);

	// update contact points
	/*
	if(veh->ModelData)
		veh->VehicleObjectData->ProcessContactPointObject = TRUE;
	*/

	// hide all the stubs
	/*
	if(veh->VehicleObjectData->VisDamageStubGroup)
	{
		ogrpHideObjectAndSubtree(veh->VehicleObjectData->VisDamageStubGroup);
		obmainSetFlag2InThisTree(veh->VehicleObjectData->VisDamageStubGroup, OBJECT_DONT_UPDATE | OBJECT_MISSING | OBJECT_HIDDEN);
	}
	*/

    // No explosions for STO hulks.
	/*
    if(!veh->hulksto)
    {
        vehAnimChangeVehicleStateByName(veh, (UCHAR*)"crashexplosion", TRUE);

        if(veh->ModelData)  // and make sure the player freaking dies if they're not already, sure there's plenty of other hoops I'll have to jump through
        {
	        veh->ModelData->VehicleIsDead = TRUE;
	        persKillCrew();
        }

    }
	*/
	//veh->VisDamageNoStick = FALSE;

	return 0;
}

/************************************************************************************************
	vehVisDamageUpdateCrap
*************************************************************************************************/
#if(0)
static void _printCrap(VisualDamageBody* crap)
{
	Vec3d position = crap->GetPosition();
	Matrixf attitude = crap->GetRotationMatrix();
	dGeomID geom = crap->geom(0);

	const dReal* gPos = dGeomGetPosition(geom);

	// body position
	FLOAT64VECTOR v1;
	position.get(v1);
	v1.z = -v1.z;

	FLOAT64VECTOR up = { 0, 0, 1.0f };
	dVector3 pt;

	crap->Body()->vectorToWorld(up.x, up.y, up.z, pt);

	FLOAT64VECTOR v2 = v1;
	v2.x += pt[0];
	v2.y += pt[1];
	v2.z += -pt[2];

	std::string oname = crap->obj->name;
	oname += "-body";

	// draw our body position and orientation
	draw3dSetWorldDebugLine(&v1, &v2, oname.c_str(), colorYELLOWALPHA);

	position.get(v1);
	v1.z = -v1.z;
	v2 = v1;
	v1.x += .2f;
	v2.x -= .2f;

	oname = crap->obj->name;
	oname += "-originx";

	draw3dSetWorldDebugLine(&v1, &v2, oname.c_str(), colorGREENALPHA);

	position.get(v1);
	v1.z = -v1.z;
	v2 = v1;
	v1.y += .2f;
	v2.y -= .2f;

	oname = crap->obj->name;
	oname += "-originy";

	draw3dSetWorldDebugLine(&v1, &v2, oname.c_str(), colorGREENALPHA);

	// now let's draw our geom's position
	position.set(gPos[0], gPos[1], gPos[2]);
	position = physicsPhysicalToGame(position);

	v1.z = -v1.z;
	v2 = v1;
	v1.x += .2f;
	v2.x -= .2f;

	oname = crap->obj->name;
	oname += "-goriginx";

	draw3dSetWorldDebugLine(&v1, &v2, oname.c_str(), colorBLUEALPHA);

	position.get(v1);
	v1.z = -v1.z;
	v2 = v1;
	v1.y += .2f;
	v2.y -= .2f;

	oname = crap->obj->name;
	oname += "-goriginy";

	draw3dSetWorldDebugLine(&v1, &v2, oname.c_str(), colorBLUEALPHA);

}
#endif

SINT32 vehVisDamageUpdateCrap()
{
	vehVehicle* veh;
	VisualDamageBody* crap;

	// update render position
	vehGetVehicleHead(&veh);

	FLOAT64VECTOR WorldOffsetOrigin;
	trnGetWorldOriginOffset(&WorldOffsetOrigin);
	WorldOffsetOrigin.z = -WorldOffsetOrigin.z;

	Vec3d woo = Vec3d(WorldOffsetOrigin);

	while(veh)
	{
		crap = veh->ChunksOfCrap;

		while(crap)
		{
			if(frametime >=  crap->expires)
			{
				VisualDamageBody* freeCrap = crap;

				if(crap->prev)
					crap->prev->next = crap->next;

				if(crap->next)
					crap->next->prev = crap->prev;

				if(crap == veh->ChunksOfCrap)
					veh->ChunksOfCrap = crap->next;

				ogrpHideObjectAndSubtree(crap->obj);

				crap = crap->next;
				delete freeCrap;
			}
			else
			{
				crap->UpdateObject(woo);
				crap = crap->next;
			}
		}

		veh = veh->getNext();
	}

	return 0;
}

/************************************************************************************************
	vehVisDamageRemoveAllCrap
*************************************************************************************************/

SINT32 vehVisDamageRemoveAllCrap(vehVehicle* veh)
{
	VisualDamageBody* next, *crap = veh->ChunksOfCrap;

	while(crap)
	{
		next = crap->next;
		delete crap;
		crap = next;
	}

	veh->ChunksOfCrap = nullptr;

	return 0;
}

/************************************************************************************************
	_RemoveComponents
*************************************************************************************************/

static LOGICAL _RemoveComponents(vehVehicle* veh, const char* visdamgroup, LOGICAL CreateCrap)
{
	LOGICAL HulkVehicle = FALSE;
	mlComponentList* compList = veh->ModelData->ComponentList;
	UINT32 i, j;

	for(i = 0; i < compList->NumComponents; i++)
	{
		LOGICAL RemoveComponent = FALSE;
		mlComponent* comp = &compList->Components[i];
		if(comp->ComponentStatus != ML_STATUS_OK && comp->ComponentStatus != ML_STATUS_OK_IF_FAIL) continue;

		for(j = 0; j < MAX_DAMAGE_GROUPS; j++)
		{
			if(!comp->VisDamage[j][0]) break;

			if(!strcmp(comp->VisDamage[j], visdamgroup))
			{
				RemoveComponent = TRUE;
				break;
			}
		}

		if(RemoveComponent)
		{
			if(comp->ComponentStatus == ML_STATUS_OK_IF_FAIL)
			{
				comp->ComponentStatus = ML_STATUS_OK;
			}else
			{
				mlRemoveComponent(veh->ModelData, comp, FALSE);

				if(comp->HulkIfFail)
					HulkVehicle = TRUE;
			}
		}
		else if(comp->VisDamageParent[0] && !strcmp(comp->VisDamageParent, visdamgroup))
		{
			if(comp->ComponentStatus == ML_STATUS_OK_IF_FAIL)
			{
				comp->ComponentStatus = ML_STATUS_OK;
			}else
			{
				mlRemoveComponent(veh->ModelData, comp, TRUE);

				if(comp->HulkIfFail)
					HulkVehicle = TRUE;

				if(comp->VisDamage[0][0])
					vehVisDamageGroupDestroyed(veh, comp->VisDamage[0], FALSE, CreateCrap);
			}
		}
	}

	return HulkVehicle;
}

/************************************************************************************************
	_AddComponents
*************************************************************************************************/

static LOGICAL _AddComponents(vehVehicle* veh, const char* visdamgroup)
{
	mlComponentList* compList = veh->ModelData->ComponentList;
	UINT32 i, j;

	for(i = 0; i < compList->NumComponents; i++)
	{
		LOGICAL AddComponent = FALSE;
		mlComponent* comp = &compList->Components[i];

		if(comp->ComponentStatus == ML_STATUS_OK)
			continue;

		for(j = 0; j < MAX_DAMAGE_GROUPS; j++)
		{
			if(!comp->VisDamage[j][0]) break;

			if(!strcmp(comp->VisDamage[j], visdamgroup))
			{
				AddComponent = TRUE;
				break;
			}
		}

		if(AddComponent)
		{
			if(comp->ComponentStatus == ML_STATUS_OK_IF_FAIL)
			{
				comp->ComponentStatus = ML_STATUS_OK;
			}
			else
			{
				//mlAddComponent(veh->ModelData, comp, FALSE);
			}
		}
		else if(comp->VisDamageParent[0] && !strcmp(comp->VisDamageParent, visdamgroup))
		{
			if(comp->ComponentStatus == ML_STATUS_OK_IF_FAIL)
			{
				comp->ComponentStatus = ML_STATUS_OK;
			}
			else
			{
				//mlAddComponent(veh->ModelData, comp, TRUE);

				if(comp->VisDamage[0][0])
					vehVisDamageGroupRepaired(veh, comp->VisDamage[0]);
			}
		}
	}

	return TRUE;
}

/************************************************************************************************
	_HideDestroyedObjects
*************************************************************************************************/

static void _HideDestroyedObjects(vehVehicle* veh, const char* visdamgroup)
{
	if(veh->VehicleObjectData->PlaneObject)
	{
		snprintf(_comment, sizeof(_comment)-1, "visdamage=%s", visdamgroup);
		obmainFindCommentCallback(veh->VehicleObjectData->PlaneObject, _comment, 0, _HideCallback);
	}
}

/************************************************************************************************
	_ShowDestroyedObjects
*************************************************************************************************/

#if(0)
static void _ShowDestroyedObjects(vehVehicle* veh, const char* visdamgroup)
{
	if(veh->VehicleObjectData->PlaneObject)
	{
		snprintf(_comment, sizeof(_comment)-1, "visdamage=%s", visdamgroup);
		obmainFindCommentCallback(veh->VehicleObjectData->PlaneObject, _comment, 0, _ShowCallback);
	}
}
#endif

/************************************************************************************************
	_HideChildStubs
*************************************************************************************************/

static void _HideChildStubs(vehVehicle* veh, const char* visdamgroup)
{
	// walk down anim list look for children of this damage group
	vehanimVehicleStateType* stateType;
	int i;

	if(veh->VehicleObjectData->VisDamageStubGroup == nullptr) return;

	for(i = 0; i < VEH_MAX_STATE_BITS; i++)
	{
		stateType = &veh->VehicleType->StateType[i];
		if(!stateType->Name[0])
			break;

		if(stateType->ParentVisDamage[0] &&
			!strcmp(stateType->ParentVisDamage, visdamgroup))
		{
			snprintf(_comment, sizeof(_comment)-1, "visdamage=%s", stateType->Name);
			obmainFindCommentCallback(veh->VehicleObjectData->VisDamageStubGroup->child, _comment, 0, _HideCallback);
		}
	}
}

/************************************************************************************************
	_ShowChildStubs
*************************************************************************************************/

#if(0)
static void _ShowChildStubs(vehVehicle* veh, const char* visdamgroup)
{
	// walk down anim list look for children of this damage group
	vehanimVehicleStateType* stateType;
	int i;

	if(veh->VehicleObjectData->VisDamageStubGroup == nullptr) return;

	for(i = 0; i < VEH_MAX_STATE_BITS; i++)
	{
		stateType = &veh->VehicleType->StateType[i];
		if(!stateType->Name[0])
			break;

		if(stateType->ParentVisDamage[0] &&
			!strcmp(stateType->ParentVisDamage, visdamgroup))
		{
			snprintf(_comment, sizeof(_comment)-1, "visdamage=%s", stateType->Name);
			obmainFindCommentCallback(veh->VehicleObjectData->VisDamageStubGroup->child, _comment, 0, _ShowCallback);
		}
	}
}
#endif

/************************************************************************************************
	_HideCallback
*************************************************************************************************/

static LOGICAL _HideCallback(OBJECT* obj, UINT32 user)
{
	ogrpHideObjectAndSubtree(obj);
	obmainSetFlag2InThisTree(obj, OBJECT_DONT_UPDATE | OBJECT_MISSING | OBJECT_HIDDEN);

	//physicsRecurseAndCreateObjectCollider(obj, colliderVehicleSpace(),COLLIDER_DEBRIS,0,false,true,false);

	return TRUE;
}

/************************************************************************************************
	_ShowCallback
*************************************************************************************************/

static LOGICAL _ShowCallback(OBJECT* obj, UINT32 user)
{
	ogrpShowObjectAndSubtree(obj);
	obmainClearFlag2InThisTree(obj, OBJECT_DONT_UPDATE | OBJECT_MISSING | OBJECT_HIDDEN);

	vehVehicle* veh = (vehVehicle*)user;

	UINT32 cat = COLLIDER_VEH_GROUND;

	switch(veh->VehicleType->Type)
	{
		case VEH_TYPE_PERSON:	cat = COLLIDER_VEH_TROOP;		break;
		case VEH_TYPE_AIR:		cat = COLLIDER_VEH_AIR;			break;
		case VEH_TYPE_GROUND:	cat = COLLIDER_VEH_GROUND;	break;
		case VEH_TYPE_SEA:		cat = COLLIDER_VEH_SEA;			break;
	}

	//physicsRecurseAndFreeObjectCollider(obj, false);
	physicsRecurseAndCreateObjectCollider(obj, veh->spaceID,cat,0,false,true,false);

	return TRUE;
}

/************************************************************************************************
	_CollideOnCallback
*************************************************************************************************/

static LOGICAL _CollideOnCallback(OBJECT* obj, UINT32 user)
{
	ogrpHideObjectAndSubtree(obj);
	obmainClearFlag2InThisTree(obj,  OBJECT_MISSING);

	return TRUE;
}

/************************************************************************************************
	_CreateCrapCallback
*************************************************************************************************/

struct crapcallbackdata
{
	vehVehicle* veh;
	LOGICAL Explode;
	const char* visdamgroup;
};

static LOGICAL _CreateCrapCallback(OBJECT* obj, UINT32 user)
{
	SINT32 i = 0;
	char* str = nullptr;

	// update crap object's position
	crapcallbackdata* cb = (crapcallbackdata*)user;

	obmainClearFlag2InThisTree(obj, OBJECT_DONT_UPDATE|OBJECT_HIDDEN);
	ogrpShowObjectAndSubtree(obj);

	// get it into position
	mlMoveAndRotateObject(obj, &cb->veh->PosData.RenderPos, &cb->veh->PosData.RenderAtt);

	// now make sure nothing else but physics updates it
	//obmainSetFlag3InThisTree(obj, OBJECT_PHYSICAL);

	if(obmainFindCommentFirstPartName(&str, obj->comment, "visdamage=") && str)
	{
		str = strchr(str, '_');
		if(str)
			i = atoi(str+1);
	}
	_CreateCrap(cb->veh, obj, cb->visdamgroup, cb->Explode,i);

	return TRUE;
}

/************************************************************************************************
	_FixeCrapCallback
*************************************************************************************************/
#if(0)
static LOGICAL _FixCrapCallback(OBJECT* obj, UINT32 user)
{
	/*
	SINT32 i = 0;
	char* str = nullptr;

	obmainClearFlag2InThisTree(obj, OBJECT_DONT_UPDATE | OBJECT_HIDDEN);
	ogrpShowObjectAndSubtree(obj);
	mlMoveAndRotateObject(obj, &cb->veh->PosData.RenderPos, &cb->veh->PosData.RenderAtt);

	if(obmainFindCommentFirstPartName(&str, obj->comment, "visdamage=") && str)
	{
		str = strchr(str, '_');
		if(str)
			i = atoi(str+1);
	}
	_CreateCrap(cb->veh, obj, cb->visdamgroup, cb->Explode,i);
	*/

	return TRUE;
}
#endif

/************************************************************************************************
	_CreateCrapAndStubs
*************************************************************************************************/

static void _CreateCrapAndStubs(vehVehicle* veh, const char* visdamgroup, LOGICAL LeaveStub, LOGICAL CreateCrap, LOGICAL Explode)
{
	// make crap and stubs visible
	OBJECT* obj;

	snprintf(_comment, sizeof(_comment) - 1, "visdamage=%s", visdamgroup);

	// show stub
	if(LeaveStub)
	{
		obj = nullptr;
		if(veh->VehicleObjectData->VisDamageStubGroup)
			 obmainFindComment(&obj, veh->VehicleObjectData->VisDamageStubGroup->child, _comment);

		if(obj)
		{
			obmainClearFlag2InThisTree(obj, OBJECT_DONT_UPDATE | OBJECT_HIDDEN | OBJECT_MISSING);
			obmainClearFlag3InThisTree(obj, OBJECT_PHYSICAL);
			ogrpShowObjectAndSubtree(obj);
		}
	}

	// create crap
	if(CreateCrap && veh->VehicleObjectData->VisDamageCrapGroup)
	{
		crapcallbackdata cb = {veh, Explode, visdamgroup};
		obmainFindCommentFirstPartCallback(veh->VehicleObjectData->VisDamageCrapGroup->child, _comment, (UINT32)&cb, _CreateCrapCallback);
	}
}

/************************************************************************************************
	_FixCrapAndStubs
*************************************************************************************************/

#if(0)
static void _FixCrapAndStubs(vehVehicle* veh, const char* visdamgroup)
{
	// make crap and stubs visible
	OBJECT* obj;

	snprintf(_comment, sizeof(_comment) - 1, "visdamage=%s", visdamgroup);

	// hid stub
	obj = nullptr;
	if(veh->VehicleObjectData->VisDamageStubGroup)
		 obmainFindComment(&obj, veh->VehicleObjectData->VisDamageStubGroup->child, _comment);

	if(obj)
	{
		obmainSetFlag2InThisTree(obj, OBJECT_DONT_UPDATE | OBJECT_HIDDEN | OBJECT_MISSING);
		ogrpHideObjectAndSubtree(obj);
	}

	// fix crap
	if(false && veh->VehicleObjectData->VisDamageCrapGroup)
		obmainFindCommentFirstPartCallback(veh->VehicleObjectData->VisDamageCrapGroup->child, _comment, nullptr, _FixCrapCallback);
}
#endif

/************************************************************************************************
	_CreateCrap
*************************************************************************************************/

static void _CreateCrap(vehVehicle* veh, OBJECT* obj, const char* visdamgroup, LOGICAL Explode, SINT32 i)
{
	// no visual damage for your vehicle for now
	//if(veh && veh->ModelData)
	//	return;

	const vehanimVehicleStateType* st = vehFindStateByName(veh->VehicleType, visdamgroup);

	if(st == nullptr)
	{
#ifdef BETA_RELEASE
		WriteLog(LOG_VEHICLE, LOG_WARN,  "no visdamage group called %s in state file!", visdamgroup);
#endif
		return;
	}

	char MassName[64];

	if(i==0)
		strsafecpy(MassName, st->MassObject);
	else
		strprintf(MassName, "%s_%d", st->MassObject, i);

	const vehInertiaComponent* ic = vehFindInertiaComponent(veh->VehicleType, MassName);

	if(ic == nullptr)
	{
		if(i == 0)
		{
#ifdef BETA_RELEASE
			char s[128];
			strprintf(s, "no inertia component called %s in mass file!", st->MassObject);
			piExitProgram(_FL, s);
#endif
		}

		return;
	}

	VisualDamageBody* crap = nullptr;

	// create a new physics object representing the crap
	if(strstr(MassName,"wheel"))
		crap = new WheelDamageBody(veh, obj);
	else if(strstr(MassName,"wing") || strstr(MassName,"stab"))
		crap = new WingDamageBody(veh, obj, strstr(MassName,"wing") != nullptr);
	else if(strstr(MassName,"turret"))
		crap = new TurretDamageBody(veh, obj);
	else if(strstr(MassName,"engine") || strstr(MassName,"mass_eng"))
		crap = new EngineDamageBody(veh, obj);
	else
		crap = new VisualDamageBody(veh, obj);

	if(!crap)
		return;

	crap->Init(st, ic, Explode);
}

static void _AddEjectSpeed(vehVehicle* veh, double EjectSpeed, FLOAT64VECTOR const* pos, FLOAT64VECTOR* v, FLOAT64VECTOR* w)
{
	FLOAT64VECTOR delta, randDir, dv;
	double randSpeed;

	if(EjectSpeed < 1)
		EjectSpeed = 1; // arbitrary

	mathSubVectors(&delta, pos, &veh->PosData.RenderPos);

	if(mathExactVectorLength2(&delta) > 0.001)
	{
		if(delta.z < 0)
			delta.z = -delta.z; // always eject up

		mathNormalize(&delta);

		// make lower angles produce slower speeds
		EjectSpeed *= delta.z;
	}
	else
	{
		delta.x = delta.y = 0;
		delta.z = 1;
	}

	dv.x = EjectSpeed * delta.x;
	dv.y = EjectSpeed * delta.y;
	dv.z = EjectSpeed * delta.z;

	// randomize the velocity somewhat
	randDir.x = 2 * mathGetFloat32ChaoticNumber() - 1;
	randDir.y = 2 * mathGetFloat32ChaoticNumber() - 1;
	randDir.z = 2 * mathGetFloat32ChaoticNumber() - 1;

	if(mathExactVectorLength2(&randDir) > 0.001)
	{
		mathNormalize(&randDir);
	}
	else
	{
		randDir.x = randDir.y=0;
		randDir.z = 1;
	}

	FLOAT64VECTOR dir;

	mathVectDoCrossProduct(&dir, &randDir, &delta); // right angle to delta vector

	randSpeed = mathGetFloat32ChaoticNumber() * EjectSpeed * .5; // arbitrary 1/2 eject speed

	dv.x += randSpeed * dir.x;
	dv.y += randSpeed * dir.y;
	dv.z += randSpeed * dir.z;

	// random angular velocity
	mathVectDoCrossProduct(w, &delta, &dv); // right angle to velocity and delta

	if(mathExactVectorLength2(w) > 0.001)
	{
		mathNormalize(w);
	}
	else
	{
		w->x = w->y = 0;
		w->z = 1;
	}

	randSpeed = mathGetFloat32ChaoticNumber() * 2 * 3.14; // arbitrary 1 rot/sec

	w->x *= randSpeed;
	w->y *= randSpeed;
	w->z *= randSpeed;

	mathAddVectors(v, v, &dv);
}

bool vehVisDamageCamera(piVECTOR* pos, piVECTOR* aim)
{
	vehVehicle* v = persGetPersonaVehiclePtr();

	if(!v || !v->ModelData)
		return true;

	static Vec3d _camPos;
	static Vec3d _lookAt;
	static piVECTOR lastAim;
	static SINT32 respawnTime = 0;

	if(!pos)
	{
		_camPos.set(v->ModelData->VehiclePos);

		piVECTOR attVec;
		attVec.x = 0.0f;
		attVec.y = 0.0f;
		attVec.z = (float)v->ModelData->VehicleAtt.Yaw;

		piMATRIX attMat;
		mathEul_ToMatrix(&attMat, EulOrdXYZr, &attVec);

		Matrixd attitude;
		attitude.set(attMat);

		Vec3d forward = Vec3d(0,1.0f,0); // forward
		Vec3d side		= Vec3d(1.0f, 0, 0);

		if(GetRandom(0.0f, 1.0f) > .5f)
			side.x() = -1.0f;

		// rotate it by our attitude matrix
		forward = attitude * forward;
		side = attitude * side;

		forward *= 100.0f;
		side *= 20.0f;

		forward.z() = 0.0f;
		side.z() = 0.0f;

		// move the camera out and up a bit
		//_camPos += Vec3d(GetRandom(-5.0f, 5.0f), 5.0f, 30.0f);
		_camPos += forward;
		_camPos += side;
		_camPos.z() += 20.0f;

		respawnTime = frametime + 1000;
		lastAim.set(0,0,0);
	}

	_lookAt.set(v->ModelData->VehiclePos);

	if(pos)
	{
		pos->x = -float(_camPos.x() - WorldOriginOffset.x);
		pos->y = -float(_camPos.y() - WorldOriginOffset.y);
		pos->z = float(_camPos.z());
	}

	if(aim)
	{
		aim->x = -float(_lookAt.x() - WorldOriginOffset.x);
		aim->y = -float(_lookAt.y() - WorldOriginOffset.y);
		aim->z = float(_lookAt.z());

		if(SCEN_MGR->IsActive() && v && !v->VehicleDeadTime)
		{
			if(lastAim.x != aim->x || lastAim.y != aim->y || lastAim.z != aim->z)
			{
				lastAim.set(aim->x, aim->y, aim->z);
				respawnTime = frametime + 1000;
			}

			if(frametime > respawnTime && !uiFindView("despawn"))
			{
				SCEN_MGR->SetFlag(ScenarioManager::RESPAWN_ME);
				vehExitVehicle();

				// make it exit right away
				v->VehicleDeadTime = frametime - 4000;
			}
		}
	}

	return true;
}
