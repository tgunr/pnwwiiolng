#include <syscfg.h> // Path is project specific.
#include <vector>
#include <set>
#include "ob/obmain.h"
#include "ob/obpool.h"
#include "gr2/gr2Physics.h"
#include "gr2/gr2Layer.h"
#include "math/math3d.h"
#include "math/mathEul.h"
#include "draw/draw3d.h"
#include "fp/fpFPS.h"
#include "physics/odeConvert.h"
#include "physics/physicsMain.h"
#include "ui/uiChat.h"
#include "spt/Random.h"
#include <ode/contact.h>
#include "gr2/gr2Anim.h"
#include "trn/trnwtr.h"
#include "fx/TransformEffect.h"
#include "fx/fxSnd.h"
#include "ob/obdraw.h"
#include "ml/mlutility.h"

class gr2PhysicsBone;
class gr2PhysicsModel;

#define DEGREES_90 mlDEG_TO_RAD(90.0f)
#define ITOM(x) ((x)*0.0254f)			// inches to meters
#define DEG(x) mlDEG_TO_RAD((double)x)	// degrees to radians

typedef void (*jointParam)(dJointID, int param, double value);
//typedef std::set<gr2PhysicsModel*> ModelList;
//static ModelList _models;

static OBJECT* gr2PhysicsGetModelNode();

gr2PhysicsBody::gr2PhysicsBody() :
	physicsBodyBase(),
	_allCollisions(0),
	_lastCollision(0),
	_collideJoints(nullptr)
{
}

gr2PhysicsBody::~gr2PhysicsBody()
{
	if(!_geoms.empty())
	{
		for(unsigned int i = 0; i < _geoms.size(); ++i)
			dGeomDestroy(_geoms.at(i));

		_geoms.clear();
	}
}

bool gr2PhysicsBody::HandleContact(colliderQuery::Collision& contact)
{
	UINT32 category = dGeomGetCategoryBits(contact.g2);
	bool allowContact = true;

	/*
	unsigned int cat1 = dGeomGetCategoryBits(contact.g1);
	unsigned int col1 = dGeomGetCollideBits(contact.g1);

	unsigned int cat2 = dGeomGetCategoryBits(contact.g2);
	unsigned int col2 = dGeomGetCollideBits(contact.g2);
	*/

	/*
	OBJECT* object = (OBJECT*)dGeomGetData(contact.g2);

	if(!object)
		object = (OBJECT*)dGeomGetData(contact.g1);
	*/

	gr2PhysicsModel* m = (gr2PhysicsModel*)_owner;

	if(colliderIsType(category, COLLIDER_TRN_WATER))
	{
		bool handled = (m &&  m->ProcessFlag(COLLIDER_TRN_WATER, this));

		if(!handled && !(_allCollisions & COLLIDER_TRN_WATER))
		{
			TransformEffect* te = nullptr;

			te = TransformEffect::AddEffect("deadtroopersplash", Vec3f(0, 0, -1), 0, nullptr);

			if(te)
			{
				Vec3d p(getPosition());
				p.z() = -p.z();

				te->enableemitters(true);;
				te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				te->setmatrix(Matrixf::translate(p));
			}
		}

		// we don't collide with water
		allowContact = false;
	}
	else if(colliderIsType(category, COLLIDER_NORAGDOLL) && contact.normal.z() < 0)
	{
		allowContact = false;
	}
	else if(m)
	{
		m->ProcessFlag(category, this);
	}
	/*
	else if(colliderIsType(category, COLLIDER_VEHICLE_MASK))
	{
		if(m)
			m->ProcessFlag(CONTACT, this);

		if(!(_flags & VEHICLE_HIT))
		{
			dSpaceID space = dGeomGetSpace(contact.g2);

			if(space)
			{
				vehVehicle* v = (vehVehicle*)dGeomGetData((dGeomID)space);

				if(v)
				{
					Vec3d velocity = Vec3d(v->ModelData ? v->ModelData->VehicleVel : v->PosData.Vel);
					static double maxVelocity = 1.0f;

					if(velocity.length2() >= (maxVelocity * maxVelocity))
					{
						dVector3 force;
						_body->addForce(OdeVec3d(force, velocity));
						_flags |= VEHICLE_HIT;
					}
				}
			}
		}

		allowContact = !(_flags & VEHICLE_HIT);
	}
	else if(colliderIsType(category, COLLIDER_TERRAIN_MASK))
	{
		if(m)
			m->ProcessFlag(CONTACT, this);
	}
	*/

	_allCollisions |= category;
	_lastCollision |= category;

	//if(allowContact)
	//	colliderDebugContact(contact);

	return allowContact;
}

void gr2PhysicsBody::AttachCollideJoint(colliderQuery::Collision& contact)
{
	if(!_collideJoints) _collideJoints = new dJointGroup;

	dContact dcontact;
	memset(&dcontact,0x0,sizeof(dcontact));
	OdeVec3d(dcontact.geom.pos, contact.pos);
	OdeVec3d(dcontact.geom.normal, contact.normal);
	dcontact.geom.depth = contact.depth;
	dcontact.geom.g1 = contact.g1;
	dcontact.geom.g2 = contact.g2;
	dcontact.surface.mode = dContactSoftERP | dContactSoftCFM | dContactBounce;
	dcontact.surface.mu = dInfinity;
	dcontact.surface.soft_cfm = physicsGetCFM();
	dcontact.surface.soft_erp = physicsGetERP();
	dcontact.surface.bounce = 0.1; // restitution
	dcontact.surface.bounce_vel = 0.1;

	// let subclasses adjust parameters
	AdjustCollideJoint(contact, &dcontact);

	dJointID c = dJointCreateContact(DefaultWorld(), *_collideJoints, &dcontact);
	dJointAttach (c, _body->id(), 0);
}

void gr2PhysicsBody::ClearCollideJoints()
{
	if(_collideJoints)
		_collideJoints->empty();
}

void gr2PhysicsBody::SetCollideWithSiblings()
{
	for(unsigned int i = 0; i < _geoms.size(); ++i)
		colliderAddBits(_geoms.at(i), 0, COLLIDER_CORPSE);
}

void gr2PhysicsBody::SetExpired(bool e)
{
	physicsBodyBase::SetExpired(e);

	/*
	if(Expired())
	{
		physicsRemoveBody(this);
	}
	*/
	/*
	else
	{
		physicsAddBody(this);
	}
	*/
}

// physics bone
gr2PhysicsBone::gr2PhysicsBone(gr2PhysicsModel* model,	// parent physics model
								const char* name,		// bone name
								int orient,				// bone index
								double mass,			// bone mass
								double len,				// capsule length
								double rad,				// capsule radius
								double ang,				// rotation angle
								int axis) : 			// rotation axis 1 = x, 2 = y, 3 = z
		gr2PhysicsBody(),
		_model(model),
		_parent(nullptr),
		_length((float)len),
		_radius((float)rad),
		_orientation((gr2PhysicsBone::ORIENTATION)orient),
        _offset(0),
		_friction(4000.0f),
		_stiffness(0),
		_autoDisableTime(30000),
        _initTime(0)
{
	// temp skeleton
	gr2Model* skeleton = _model->GetSkeleton();

	// lookup bone index
	_bone = skeleton->FindBone((SCHAR*)name);

	// remember our rest bone transform
	_restTransform.set(skeleton->GetBoneTransform(_bone));

	// grab body id
	_bodyID = _body->id();

	// need to set our autodisable threshold here
	dBodySetAutoDisableFlag(_bodyID, 1);
	dBodySetAutoDisableLinearThreshold(_bodyID, 0);
	dBodySetAutoDisableAngularThreshold(_bodyID, 0);
	dBodySetAutoDisableSteps(_bodyID, 0);
	dBodySetAutoDisableTime(_bodyID, 0);

	// make this body persistent over spawns
	SetOwner(_model);

	// set our name
	strncpy(_name, name, GR2_PHYSICS_MAXNAME);

	// setup our collider shape if we have one
	if(mass) SetCapsuleShape(mass, ang, axis);

	// overridable behavior
	//Init();

	/*
	gr2Model* skeleton = _model->getSkeleton();

	if(skeleton)
	{
		// now we need to position the body
		Matrixf transform(gr2GetBoneTransformByIndex(skeleton, bone));

		// set the body's rotation and position
		SetRotation(transform);
		SetPosition(transform.getTrans());
	}
	*/

	// start us off disabled
	SetEnabled(false);
}

gr2PhysicsBone::~gr2PhysicsBone()
{
}

void gr2PhysicsBone::VisualToPhysical(Matrixd& transform)
{
	Vec3d offset = Vec3d(_offset, 0, 0);
	Adjust(transform, offset, _angle, _axis);
}

void gr2PhysicsBone::PhysicalToVisual(Matrixd& transform)
{
	Vec3d offset = Vec3d(-_offset, 0, 0);
	Adjust(transform, offset, -_angle, _axis);
}

void gr2PhysicsBone::Adjust(Matrixd& transform, Vec3d& offset, double angle, int axis)
{
	if(offset.length2() == 0 && angle == 0)
		return;

	Matrixd invTransform = Matrixd::inverse(transform);

	// get it into object space
	Matrixd objTransform = transform * invTransform;

	if(angle)
	{
		Vec3d boneAxis;

		switch(axis)
		{
			case X: boneAxis.x() = 1.0f; break;
			case Y: boneAxis.y() = 1.0f; break;
			case Z: boneAxis.z() = 1.0f; break;
		}

		Matrixd objRotate = Matrixd::rotate(angle, boneAxis);

		// apply the offset rotation
		objTransform = objTransform * objRotate;
	}

	if(offset.length2())
	{
        /*
          TBD -calculate our adjust position
		Vec3d position = objTransform.getTrans();

		
		position = position + offset;
         */
        
		// re-save it
		objTransform.setTrans(offset);
	}

	// put it back into model space
	transform = objTransform * transform;
}

/*
void gr2PhysicsBone::Adjust(Matrixd& transform, Vec3d offset, double angle, int axis)
{
	if(offset.length2())
	{
		Vec3d position = transform.getTrans();

		// remove the position component
		transform.setTrans(0,0,0);

		// rotate it into model space
		offset = offset * transform;

		// calculate our adjust position
		position = position + offset;

		// re-save it
		transform.setTrans(position);
	}

	if(angle)
	{
		Vec3d boneAxis;

		switch(axis)
		{
			case X: boneAxis.x() = 1.0f; break;
			case Y: boneAxis.y() = 1.0f; break;
			case Z: boneAxis.z() = 1.0f; break;
		}

		Matrixf invTransform = Matrixf::inverse(transform);

		// get it into object space
		Matrixf objTransform = transform * invTransform;
		Matrixf objRotate = Matrixf::rotate(angle, boneAxis);

		// apply the offset rotation
		objTransform = objTransform * objRotate;

		// put it back into model space
		transform = objTransform * transform;
	}
}
*/

// visual bone to physics body
void gr2PhysicsBone::BoneToBody(gr2Model* from)
{
	if(!from)
		from = _model->GetSkeleton();

	// now we need to position the body
	Matrixd boneTransform(from->GetBoneTransform(_bone));

	//Matrixf rotate = Matrixf::rotate(DEGREES_90, 0, 0, 1.0f);

	//boneTransform = boneTransform * rotate;

	// apply adjustments
	VisualToPhysical(boneTransform);
	//Adjust(boneTransform, _offset, _angle, _axis);

	// plus our model's world position
	boneTransform = boneTransform * _model->GetWorldTransform();

	if(boneTransform.valid())
	{
		SetRotation(boneTransform.getRotate());
		SetPosition(boneTransform.getTrans());
	}
}

// body to bone
void gr2PhysicsBone::BodyToBone(gr2Model* to)
{
	if(!to)
		to = _model->GetSkeleton();

	// now we need to position the body
	Matrixd bodyTransform	= Matrixd(GetTransform());
	Matrixd inverseWorldMat = Matrixd::inverse(_model->GetWorldTransform());

	// bring it down to model space
	bodyTransform = bodyTransform * inverseWorldMat;

	//Matrixf rotate = Matrixf::rotate(-DEGREES_90, 0, 0, 1.0f);

	//bodyTransform = bodyTransform * rotate;

	// unapply adjustments
	static bool apply = true;

	if(apply)
	{
		PhysicalToVisual(bodyTransform);
		//Vec3d _tempOff = Vec3d(0, 0, _offset.x());
		//Adjust(bodyTransform, -_tempOff, -_angle, _axis);
	}

	if(bodyTransform.valid())
	{
		granny_world_pose* WorldPose		= gr2GetWorldPose(to);
		granny_matrix_4x4* WorldMatrices	= GrannyGetWorldPose4x4Array(WorldPose);
		granny_real32* boneTransform		= (float*)WorldMatrices[_bone];

		// update the bone position
        Vec3d temp = bodyTransform.getTrans(); // this construct is necessary for gcc to compile the UpdateBoneTransform call
		_model->UpdateBoneTransform(boneTransform, temp, bodyTransform);
	}
}

void gr2PhysicsBone::Init()
{
	// initial setup
	Setup();
}

void gr2PhysicsBone::Setup()
{
	BoneToBody();

	if(IsCollidable())
	{
		// off by default, turned on once the bone is in motion
		dBodySetAutoDisableFlag(_bodyID, 0);
		dBodySetAutoDisableLinearThreshold(_bodyID, 0);
		dBodySetAutoDisableAngularThreshold(_bodyID, 0);

		// start simulating
		SetExpired(false);

		GR2ValueModelMap::iterator it = _valueModels.begin();

		for(; it != _valueModels.end(); ++it)
			it->second.init();

		// reset our friction and stuffness mods
		SetMods(1.0f, 1.0f);
	}

	_initTime = frametime;
	_allCollisions = 0;
	_lastCollision = 0;

	SetLinearVel(_model->GetVelocity());

	if(!_color)
		_color = piRGBA(0, 0, 1, 1);
}

void gr2PhysicsBone::Start()
{

}

void gr2PhysicsBone::BeginStep()
{
	if(Expired())
		return;

	if(!IsEnabled())
		return;

	// reset every frame
	_lastCollision = 0;

	// set this flag so that the body auto disables after meeting the
	// threshold requirements set in the constructor
	if(!dBodyGetAutoDisableFlag(_bodyID))
		dBodySetAutoDisableFlag(_bodyID, 1);

	// do model collisions
	//_model->BeginUpdate();

	// clear out old contact joints
	ClearCollideJoints();

	colliderQuery::CollisionList& collisions = _model->GetCollisions(this);
	colliderQuery::CollisionList::iterator it = collisions.begin();

	for(; it != collisions.end(); ++it)
	{
		colliderQuery::Collision& contact = *it;

		// depth 0 from various trimesh colliders causes "sticks"
		if(!MyShape(contact.g1)) // contact.depth < .000001
			continue;

		dBodyID toBodyID = dGeomGetBody(contact.g2);

		if(toBodyID && dAreConnected(_bodyID, toBodyID))
			continue;

		if(contact.g1 == contact.g2)
		{
			continue;
		}

		// Do I need this anymore?
		//	Vec3d vec = contact.normal * contact.depth + contact.pos- _lastFreePos;
		//	if(vec * contact.normal > 0) continue;

		//GR2PhysicsBoneRef g1Bone = _model->getBone(dGeomGetBody(contact.g1));
		//GR2PhysicsBoneRef g2Bone = _model->getBone(dGeomGetBody(contact.g2));

		//void* g1Data = dGeomGetData(contact.g1);
		//void* g2Data = dGeomGetData(contact.g2);

		/*
		colliderGeom* hit1 = nullptr;
		colliderGeom* hit2 = nullptr;

		if(g1Data) hit1 = static_cast<colliderGeom*>(g1Data);
		if(g2Data) hit2 = static_cast<colliderGeom*>(g2Data);
		*/

		if(HandleContact(contact))
			AttachCollideJoint(contact);
	}

	gr2PhysicsBody::BeginStep();
}

#define MAX_FRICTION_VEL 1.0f

void gr2PhysicsBone::EndStep()
{
	//_model->EndUpdate(0);

	Vec3d lvel = GetLinearVel();

	// update the bone's friction model
	if(lvel.length2() < (MAX_FRICTION_VEL * MAX_FRICTION_VEL))
		RunValueModel(_friction);

	if(_stiffness && _joint && RunValueModel(_stiffness))
		SetJointParams(0, -1.0f, _stiffness * _stiffMod);

	// update the autodisable time
	float threshold = ((float)((frametime - _initTime)/(float)_autoDisableTime)) * 1.0f; //.5f;

	//if(threshold > 1.0f)
	//	threshold = 1.0f;

	// set it
	dBodySetAutoDisableLinearThreshold(_bodyID, threshold);
	dBodySetAutoDisableAngularThreshold(_bodyID, threshold);

	//uiChatPrintLine("Ragdoll Threshold is %.2f", threshold);

	//_lastUpdate = frametime;
}

void gr2PhysicsBone::SetExpired(bool e)
{
	gr2PhysicsBody::SetExpired(e);

	if(IsCollidable())
	{
		for(unsigned int i = 0; i < _geoms.size(); ++i)
		{
			if(Expired())
				dGeomDisable(_geoms.at(i));
			else
				dGeomEnable(_geoms.at(i));
		}
	}
}

void gr2PhysicsBone::SetFrictionModel(float start, float stop, int duration)
{
	AddValueModel(_friction, start, stop, duration);
}

void gr2PhysicsBone::SetStiffnessModel(float start, float stop, int duration)
{
	AddValueModel(_stiffness, start, stop, duration);

	// make sure and update the joint
	SetJointParams(0, -1.0f, _stiffness);
}

void gr2PhysicsBone::SetMods(float smod, float fmod)
{
	_stiffMod = smod;
	_frictMod = fmod;
}

void gr2PhysicsBone::SetDisableModel(int duration)
{
	_autoDisableTime = duration;
}

void gr2PhysicsBone::AdjustCollideJoint(colliderQuery::Collision& hit, dContact* contact)
{
	contact->surface.mode = dContactBounce;
	contact->surface.mu = _friction * _frictMod;
	//contact->surface.soft_cfm = _cfm;
	//contact->surface.soft_erp = .01f;
	//contact->surface.bounce_vel = 0;

	if(colliderIsType(contact->geom.g2, COLLIDER_CORPSE))
	{
		static FLOAT32 scaler = .01f;
		contact->surface.mu *= scaler;
	}
}

void gr2PhysicsBone::AddValueModel(float& value, float start, float stop, int duration)
{
	gr2ValueModel vm;

	vm._value = &value;
	vm._start = start;
	vm._stop = stop;
	vm._duration = duration;

	// set initial state
	vm.init();

	_valueModels[&value] = vm;
}

bool gr2PhysicsBone::RunValueModel(float& value, bool loop)
{
	GR2ValueModelMap::iterator vm = _valueModels.find(&value);

	if(vm != _valueModels.end())
		return vm->second.run(_initTime, loop);

	return false;
}

void gr2PhysicsBone::SetJointID(dJointID joint)
{
	_joint = joint;
}

bool gr2PhysicsBone::MyShape(dGeomID geom)
{
	return (dGeomGetBody(geom) == _body->id());
}

dGeomID gr2PhysicsBone::SetShape(dGeomID geom, int cat, int col)
{
	if(!_geoms.empty())
	{
		for(unsigned int i = 0; i < _geoms.size(); ++i)
			dGeomDestroy(_geoms.at(i));

		_geoms.clear();
	}

	return AddShape(geom, cat, col);
}

dGeomID gr2PhysicsBone::AddShape(dGeomID geom, int cat, int col)
{
	// set our collision bits
	colliderSetBits(geom, cat, col);
	dGeomSetBody(geom, _bodyID);
	//dGeomSetData(geom, this);

	// save it
	_geoms.push_back(geom);

	return geom;
}

/*
dGeomID gr2PhysicsBone::setSphereShape(dReal mass)
{
	dMassSetSphereTotal(&_mass, mass, _length/2);
	dBodySetMass(_body, &_mass);
	return setShape(dCreateSphere(getSpaceID(), _length/2));
}
*/

dGeomID gr2PhysicsBone::SetBoxShape(dReal mass, double lx, double ly, double lz)
{
	/*
	dMassSetBoxTotal(&_mass, mass, _length, _width, _height);
	dBodySetMass(_body, &_mass);
	return setShape(dCreateBox(getSpaceID(), _length, _width, _height));
	*/
	return 0;
}

dGeomID gr2PhysicsBone::SetCapsuleShape(dReal mass, double angle, int axis)
{
	_axis = (AXIS)axis;
	_angle = angle;

	double realLength = _length - (_radius * 2.0f);

	if(realLength < .01f)
		realLength = .01f;

	// direction is always Z; capsule bodies run along the Z axis
	// we rotate them to match the bones they represent
	// length is minus the caps on each end which is radius * 2

	// make length the length of the body, not the total length
	// because length is passed in as the total length of the capsule
	dMassSetCapsuleTotal(&_mass, mass, Z, _radius, realLength);
	dBodySetMass(_bodyID, &_mass);

	// our parent space
	dSpaceID space = _model->GetSpaceID();

	// our actual collision capsule
	dGeomID shape = SetShape(dCreateCapsule(space, _radius, realLength), COLLIDER_CORPSE, COLLIDER_CORPSE_MASK);

	// capsules are oriented up and rotated to the Y below
	// setting the position to the x offset achieves a
	// final the result we want which is a capsule moved
	// along the X axis after its rotated by the shape
	// transform geom below
	/*
	if(angle) // arbitrary rotation passed in?
	{
		Vec3d boneAxis;

		switch(axis)
		{
			case X: boneAxis.x() = 1.0f; break;
			case Y: boneAxis.y() = 1.0f; break;
			case Z: boneAxis.z() = 1.0f; break;
		}

		// rotate it along the x so that it fits our body better
		SetRotation(Matrixf::rotate((float)angle, boneAxis));
	}
	*/

	return shape;
}

dGeomID gr2PhysicsBone::GetGeom(int index) const
{
	return (index >= 0 && index < (int)_geoms.size()) ? _geoms[index]: nullptr;
}

int gr2PhysicsBone::GetShape(int index) const
{
	return (index >= 0 && index < (int)_geoms.size()) ? dGeomGetClass(_geoms[index]): -1;
}

dJointID gr2PhysicsBone::SetJointParams(int axis, double fudge, double stiffness, double cfm, double serp, double scfm, double bounce)
{
	if(!_joint)
		return nullptr;

	int type = dJointGetType(_joint);

	for(int a = 1; a < 4; a++)
	{
		if(axis != 0 && a != axis)
			continue;

		int paramFmax		= (a == 3 ) ? dParamFMax3 :				(a == 2) ? dParamFMax2 :		dParamFMax;
		int paramVel		= (a == 3 ) ? dParamVel3 :				(a == 2) ? dParamVel2 :			dParamVel;
		int paramFudge		= (a == 3 ) ? dParamFudgeFactor3 :		(a == 2) ? dParamFudgeFactor2 :	dParamFudgeFactor;
		int paramCFM		= (a == 3 ) ? dParamCFM3 :				(a == 2) ? dParamCFM2 :			dParamCFM;
		int paramSERP		= (a == 3 ) ? dParamStopERP3 :			(a == 2) ? dParamStopERP2 :		dParamStopERP;
		int paramSCFM		= (a == 3 ) ? dParamStopCFM3 :			(a == 2) ? dParamStopCFM2 :		dParamStopCFM;

		// setter
		jointParam param = nullptr;

		switch(type)
		{
			case dJointTypeHinge:
			{
				if(a == 1)
					param = dJointSetHingeParam;
			}
			break;
			case dJointTypeUniversal:
			{
				if(a < 3)
					param = dJointSetUniversalParam;
			}
			break;
			case dJointTypeAMotor:
			{
				param = dJointSetAMotorParam;
			}
			break;
		}

		if(param)
		{
			// set the joint's params
			if(stiffness >= 0)	param(_joint, paramFmax, stiffness);
			if(stiffness >= 0)	param(_joint, paramVel, 0);
			if(fudge >= 0)		param(_joint, paramFudge, fudge);
			if(cfm >= 0)		param(_joint, paramCFM, cfm);
			if(serp >= 0)		param(_joint, paramSERP, serp);
			if(scfm >= 0)		param(_joint, paramSCFM, scfm);
		}
	}

	return _joint;
}

Vec3d gr2PhysicsBone::PhysicalPosition() const
{
	return Vec3d();
}

Matrixf gr2PhysicsBone::PhysicalRotation() const
{
	return Matrixf();
}

Matrixf gr2PhysicsBone::PhysicalTransform() const
{
	return Matrixf();
}

Vec3d gr2PhysicsBone::VisualPosition() const
{
	/*
	gr2Model* skeleton = _model->GetSkeleton();

	// now we need to position the body
	Matrixd boneTransform(skeleton->GetBoneTransform(_bone));

	// apply adjustments
	VisualToPhysical(boneTransform);

	// plus our model's world position
	boneTransform = boneTransform * _model->GetWorldTransform();

	return boneTransform.getTrans();
	*/
	return Vec3d();
}

Matrixf gr2PhysicsBone::VisualRotation() const
{
	return Matrixf();
}

Matrixf gr2PhysicsBone::VisualTransform() const
{
	return Matrixf();
}

void renderCylinder(float x1, float y1, float z1, float x2,float y2, float z2, float radius,int subdivisions)
{
	float vx = x2-x1;
	float vy = y2-y1;
	float vz = z2-z1;
	//handle the degenerate case with an approximation
	if(vz == 0)
	vz = .00000001f;
	float v = sqrt( vx*vx + vy*vy + vz*vz );
	float ax = 57.2957795f*acos( vz/v );
	if ( vz < 0.0 )
	ax = -ax;
	float rx = -vy*vz;
	float ry = vx*vz;

	GLUquadricObj* quadric=gluNewQuadric();
	gluQuadricNormals(quadric, GLU_SMOOTH);

	glPushMatrix();
	glTranslatef( x1,y1,z1 );
	glRotatef(ax, rx, ry, 0.0);
	//draw the cylinder
	gluCylinder(quadric, radius, radius, v, 32, 1);
	gluQuadricOrientation(quadric,GLU_INSIDE);
	//draw the first cap
	gluDisk( quadric, 0.0, radius, 32, 1);
	glTranslatef( 0,0,v );
	//draw the second cap
	gluQuadricOrientation(quadric,GLU_OUTSIDE);
	gluDisk( quadric, 0.0, radius, 32, 1);
	glPopMatrix();

	gluDeleteQuadric(quadric);
}

Vec3d gr2PhysicsBone::CollidePoint(Vec3d& pos) const
{
	Matrixd worldTransform = _model->GetWorldTransform();
	Matrixd invTransform = Matrixd::inverse(worldTransform);

	// get it into model space
	Vec3d localPos = pos * invTransform;

	// rotate it 90 degrees (infantry is off by that amount) wtf?
	Matrixd rotate = Matrixd::rotate(-DEGREES_90, 0, 0, 1);

	localPos = localPos * rotate;

	return localPos;
}

Matrixd gr2PhysicsBone::CollideTransform() const
{
	// grab our body's rotation
	Matrixd bodyTransform = GetTransform();

	Matrixd worldTransform = _model->GetWorldTransform();
	Matrixd invTransform = Matrixd::inverse(worldTransform);

	bodyTransform = bodyTransform * invTransform;

	/*
	if(_offset)
	{
		Matrixf invTransform = Matrixf::inverse(bodyTransform);

		// get it into object space
		Matrixf objTransform = bodyTransform * invTransform;
		Matrixf objRotate = Matrixf::rotate(PI * .5f, 0, 1, 0);

		dQuaternion dQuat;
		dGeomGetOffsetQuaternion(GetGeom(), dQuat);

		Quat test1 = QuatOde(dQuat);
		Quat test2 = objRotate.getRotate();

		Matrixf offsetRotation = Matrixf::rotate(test1);

		// apply the offset rotation
		objTransform = objTransform * objRotate;

		// put it back into model space
		bodyTransform = objTransform * bodyTransform;
	}
	*/

	// now lets rotate the whole thing around the z
	//Matrixf rotate = Matrixf::rotate(-PI * .5f, 0, 0, 1);

	// rotate it
	//bodyTransform = bodyTransform * rotate;

	// return it
	return bodyTransform;
}

#if USE_STATESET
void gr2PhysicsBone::DrawCollider(GLUquadric* obj)
{
    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetRenderState(new Color4F(_color));
        stateset->SetMode(GL_LIGHTING, 0);
    }

	if(!IsCollidable())
		return;

    stateset->Apply();

	glPushMatrix();
	Matrixd transform = CollideTransform();

	piMATRIX mat;
	transform.get(mat);

	glMultMatrixf((GLfloat*)&mat);

	double radius = GetRadius();
	double length = GetLength() - (radius * 2.0f);

	if(length < .01f)
		length = .01f;

	// draw the capsule
	glTranslatef(0,0,-((float)length * .5f));
	gluCylinder(obj, radius, radius, length, 16, 16);

	// draw the caps
	gluSphere(obj, radius, 16, 16);
	glTranslatef(0,0,(float)length);
	gluSphere(obj, radius, 16, 16);

	glPopMatrix();
}
#else
void gr2PhysicsBone::DrawCollider(GLUquadric* obj)
{
	//if(IsEnabled())
		oglSetRenderState(oglRENDERSTATE_COLOR4F, _color);
	//else
	//	oglSetRenderState(oglRENDERSTATE_COLOR4F, colorBLACKALPHA);

	if(!IsCollidable())
		return;

	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_ALL_ATTRIB_BITS);

	Matrixd transform = CollideTransform();

	piMATRIX mat;
	transform.get(mat);

	glMultMatrixf((GLfloat*)&mat);

	double radius = GetRadius();
	double length = GetLength() - (radius * 2.0f);

	if(length < .01f)
		length = .01f;

	oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
	piDisableTexture();

	// draw the capsule
	glTranslatef(0,0,-((float)length * .5f));
	gluCylinder(obj, radius, radius, length, 16, 16);

	// draw the caps
	gluSphere(obj, radius, 16, 16);
	glTranslatef(0,0,(float)length);
	gluSphere(obj, radius, 16, 16);

	glPopClientAttrib();
	glPopAttrib();
	glPopMatrix();
}
#endif

#if USE_STATESET
void gr2PhysicsBone::DrawBodyPosition(GLUquadric* obj, bool adjusted)
{
    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetRenderState(new Color4F(1, 0, 0, 1));
        stateset->SetRenderState(new PointSize(4.0f));
        stateset->SetMode(GL_LIGHTING, 0);
    }

	if(!IsCollidable())
		return;

    stateset->Apply();

	glPushMatrix();

	Matrixd transform = CollideTransform();

	if(adjusted)
	{
		PhysicalToVisual(transform);
	}

	piMATRIX mat;
	transform.get(mat);

	glMultMatrixf((GLfloat*)&mat);

	gluSphere(obj, .02f, 16, 16);

    Color4F* color = dynamic_cast<Color4F*>(stateset->GetRenderState(RS_COLOR4F));

    color->set(0, 0, 1, 1);
    color->apply();

    glBegin(GL_LINES);
    {
        glVertex3f(0, 0, 0);
	    glVertex3f(0, 0, ITOM(2.0f));
    }
    glEnd();

    color->set(1, 0, 0, 1);
    color->apply();

    glBegin(GL_LINES);
    {
        glVertex3f(0, 0, 0);
	    glVertex3f(ITOM(2.0f), 0, 0);
    }
    glEnd();

    color->set(0, 1, 0, 1);
    color->apply();

    glBegin(GL_LINES);
    {
        glVertex3f(0, 0, 0);
	    glVertex3f(0, ITOM(2.0f), 0);
    }
    glEnd();

	glPopClientAttrib();
}
#else
void gr2PhysicsBone::DrawBodyPosition(GLUquadric* obj, bool adjusted)
{
	if(!IsCollidable())
		return;

	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 0, 0, 1));

	//glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_ALL_ATTRIB_BITS);

	Matrixd transform = CollideTransform();

	if(adjusted)
	{
		PhysicalToVisual(transform);
		//Vec3d _tempOff = Vec3d(0, 0, _offset.x());
		//Adjust(transform, -_tempOff, -_angle, _axis);
	}

	piMATRIX mat;
	transform.get(mat);

	glMultMatrixf((GLfloat*)&mat);

	piDisableTexture();
	oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

	gluSphere(obj, .02f, 16, 16);

	glPointSize(4.0f);
    glDisable(GL_TEXTURE_2D);
	piDisableTexture();
	oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 1, 1));
    glBegin(GL_LINES);
    {
        glVertex3f(0, 0, 0);
	    glVertex3f(0, 0, ITOM(2.0f));
    }
    glEnd();

	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 0, 0, 1));
    glBegin(GL_LINES);
    {
        glVertex3f(0, 0, 0);
	    glVertex3f(ITOM(2.0f), 0, 0);
    }
    glEnd();


	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 1, 0, 1));
    glBegin(GL_LINES);
    {
        glVertex3f(0, 0, 0);
	    glVertex3f(0, ITOM(2.0f), 0);
    }
    glEnd();

    glPointSize(1.0f);
	glPopClientAttrib();
	glPopAttrib();
	glPopMatrix();
	//glEnable(GL_DEPTH_TEST);
}
#endif

#if USE_STATESET
void gr2PhysicsBone::DrawBonePosition(GLUquadric* obj)
{
    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetRenderState(new Color4F(1, 0, 0, 1));
        stateset->SetMode(GL_LIGHTING, 0);
    }

    stateset->Apply();

	glPushMatrix();

	// now we need to position the body
	Matrixd transform		= GetTransform();
	Matrixd inverseWorldMat = Matrixd::inverse(_model->GetWorldTransform());

	// bring it down to model space
	transform = transform * inverseWorldMat;

	// unapply adjustments
	PhysicalToVisual(transform);
	//Adjust(transform, -_offset, -_angle, _axis);

	piMATRIX mat;
	transform.get(mat);

	glMultMatrixf((GLfloat*)&mat);

	gluSphere(obj, .005f, 16, 16);

	glPopMatrix();
}
#else
void gr2PhysicsBone::DrawBonePosition(GLUquadric* obj)
{
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 0, 0, 1));

	glPushMatrix();

	// now we need to position the body
	Matrixd transform		= GetTransform();
	Matrixd inverseWorldMat = Matrixd::inverse(_model->GetWorldTransform());

	// bring it down to model space
	transform = transform * inverseWorldMat;

	// unapply adjustments
	PhysicalToVisual(transform);
	//Adjust(transform, -_offset, -_angle, _axis);

	piMATRIX mat;
	transform.get(mat);

	glMultMatrixf((GLfloat*)&mat);

	gluSphere(obj, .005f, 16, 16);

	glPopMatrix();
}
#endif

gr2CenterBone::gr2CenterBone(gr2PhysicsModel* model, const char* name, double mass, double len, double rad) :
	gr2PhysicsBone(model, name, CENTER, mass, len, rad, 0.0f, 0)
{
}

gr2AxialBone::gr2AxialBone(gr2PhysicsModel* model, const char* name, double mass, double len, double rad) :
	gr2PhysicsBone(model, name, AXIAL, mass, len, rad, DEGREES_90, 2)
{
}

void gr2AxialBone::Init()
{
	if(!_offset)
		_offset = _length * .5f;

	// run the normal path
	gr2PhysicsBone::Init();
}

void gr2AxialBone::PhysicalToVisual(Matrixd& transform)
{
	Vec3d offset = Vec3d(0, 0, -_offset);
	Adjust(transform, offset, -_angle, _axis);
}


gr2AxialFootBone::gr2AxialFootBone(gr2PhysicsModel* model, const char* name, double mass, double len, double rad) :
	gr2PhysicsBone(model, name, AXIAL, mass, len, rad, DEGREES_90, 1)
{
}

void gr2AxialFootBone::Init()
{
	if(!_offset)
		_offset = _length * .5f;

	// run the normal path
	gr2PhysicsBone::Init();
}

void gr2AxialFootBone::PhysicalToVisual(Matrixd& transform)
{
	Vec3d offset = Vec3d(0, -_offset, 0);
	Adjust(transform, offset, -_angle, _axis);
}

void gr2AxialFootBone::VisualToPhysical(Matrixd& transform)
{
	Vec3d offset = Vec3d(0, _offset, 0);
	Adjust(transform, offset, _angle, _axis);
}

gr2StaticBone::gr2StaticBone(gr2PhysicsModel* model, const char* name) :
	_model(model), _parent(-1), _bone(-1), _initTime(0)
{
		// temp skeleton
	gr2Model* skeleton = _model->GetSkeleton();

	// lookup bone index
	_bone = skeleton->FindBone((SCHAR*)name);

	// remember our rest bone transform
	_restTransform.set(skeleton->GetBoneTransform(_bone));

	// set our name
	strncpy(_name, name, GR2_PHYSICS_MAXNAME);
}

void gr2StaticBone::Init()
{
	// get our physics model
	gr2Model* skeleton = _model->GetSkeleton();

	// lookup our parent if we don't have a manually set parent bone
	granny_skeleton* gs = gr2GetSkeleton(skeleton);

	if(_parent == -1)
		_parent = gs->Bones[_bone].ParentIndex;

	if(_parent >= 0)
	{
		// remember our parent bone
		Matrixf parentTransform;

		// create an inverse matrix of our parent's transform
		parentTransform.set(skeleton->GetBoneTransform(_parent));
		parentTransform = Matrixf::inverse(parentTransform);

		// remembre our local transform for later positioning
		_localTransform = _restTransform * parentTransform;
	}

	_initTime = frametime;
}

void gr2StaticBone::BodyToBone(gr2Model* to)
{
	// this bone may be a little behind where it should be, but
	// it probably won't have a visual impact on the simulation
	if(_parent < 0) return;

	if(!to)
		to = _model->GetSkeleton();

	// now we need to position the body
	Matrixf transform = Matrixf(to->GetBoneTransform(_parent));

	if(transform.valid())
	{
		granny_world_pose* WorldPose		= gr2GetWorldPose(to);
		granny_matrix_4x4* WorldMatrices	= GrannyGetWorldPose4x4Array(WorldPose);
		granny_real32* boneTransform		= (float*)WorldMatrices[_bone];

		transform = _localTransform * transform;

		// update the bone position
        Vec3d temp = transform.getTrans();  // this construct is necessary for gcc to compile the UpdateBoneTransform call
		_model->UpdateBoneTransform(boneTransform, temp, transform);
	}

	/*
	granny_world_pose* WorldPose = gr2GetWorldPose(_model->GetSkeleton());
	granny_matrix_4x4* WorldMatrices = GrannyGetWorldPose4x4Array(WorldPose);

	Matrixf transform;

	transform.set((float*)WorldMatrices[parent->GetBone()]);
	transform = _localTransform * transform;

	Vec3d position = transform.getTrans();

	granny_real32* boneTransform = (float*)WorldMatrices[_bone];

	Matrixf temp;
	temp.set((float*)boneTransform);

	// update it
	_model->UpdateBoneTransform(boneTransform, position, transform);
	*/
}

gr2MeshBone::gr2MeshBone(gr2PhysicsModel* model, const char* name) :
	gr2PhysicsBone(model, name, MESH, 0.0f, 0.0f, 0.0f, 0.0f, 0)
{
	gr2Model* skeleton = model->GetSkeleton();
	/*
	gr2PhysicsModel::BoneList& bones = model->GetBones();
	piMATRIX boneMat;
	int count = bones.size();
	Vec3d rootPos;

	for(int i = 0; i < count; i++)
	{
		gr2PhysicsBone* bone = bones[i];

		if(skeleton->GetBonePosition((SCHAR*)name, &boneMat))
		{
			colliderOffset.x() += boneMat._41;
			colliderOffset.y() += boneMat._42;
			colliderOffset.z() += boneMat._43;

			if(i == 0)
				rootPos.set(boneMat._41, boneMat._42, boneMat._43);
		}
	}
	*/

	piMATRIX boneMat;
	Vec3d rootPos;

	if(skeleton->GetBonePosition((SCHAR*)name, &boneMat))
	{
		colliderOffset.x() += boneMat._41;
		colliderOffset.y() += boneMat._42;
		colliderOffset.z() += boneMat._43;

		rootPos.set(boneMat._41, boneMat._42, boneMat._43);
	}

	// average our bone position
	//colliderOffset /= count;

	// now calculate the offset
	colliderOffset -= rootPos;

	// now lets create our collider
	meshCollider.setSpace(model->GetSpaceID());
	meshCollider.setCategoryMask(COLLIDER_WEAPON);
	meshCollider.setCollideMask(COLLIDER_WEAPON_MASK);

	gr2Model* visual = model->GetModel();

	dMassSetZero(&_mass);
	//dMass triMass;

	for(size_t i = 0; i < visual->meshes.size(); i++)
	{
		gr2Mesh* mesh = visual->meshes[i];

		const void* Vertices = mesh->data->vertexArray;
		int VStride = sizeof(piGVERTEX);
		int VCount = GrannyGetMeshVertexCount(mesh->data->GrannyMesh);

		const void* Indices = mesh->data->indexArray;
		int IStride = 3*sizeof(SINT16);
		int ICount = GrannyGetMeshIndexCount(mesh->data->GrannyMesh);

		for(int i = 0; i < ICount; i += 3)
		{
			piGVERTEX* v1 = (piGVERTEX*)Vertices + *(((SINT16*)Indices) + i);
			piGVERTEX* v2 = (piGVERTEX*)Vertices + *(((SINT16*)Indices) + i + 1);
			piGVERTEX* v3 = (piGVERTEX*)Vertices + *(((SINT16*)Indices) + i + 2);

			uiChatPrintLine("V1(%.2f,%.2f,%.2f) -> V2(%.2f,%.2f,%.2f) -> V3(%.2f,%.2f,%.2f)",
					v1->x, v1->y, v1->z,
					v2->x, v2->y, v2->z,
					v3->x, v3->y, v3->z);
		}

		int index = meshCollider.createMesh(Vertices, VStride, VCount, Indices, IStride, ICount, this);

		_geoms.push_back(meshCollider.id(index));

		//dMassSetZero(&triMass);
		//dMassSetTrimesh(&triMass, 1.0f, meshCollider.id(index));
		//dMassTranslate(&triMass, -triMass.c[0], -triMass.c[1], -triMass.c[2]);
		//dMassAdd(&_mass, &triMass);
	}

	meshCollider.setEnabled(true);
	meshCollider.attachToBody(_body);

	//meshCollider.offsetPosition(colliderOffset.x(), colliderOffset.y(), colliderOffset.z());

	dMassSetSphereTotal(&_mass, 5.0f * .45359237f, 1.0f);
	_body->setMass(&_mass);
}

void gr2MeshBone::BeginStep()
{
	gr2PhysicsBone::BeginStep();
	lastFreePos = GetPosition(false);
}

bool gr2MeshBone::HandleContact(colliderQuery::Collision& contact)
{
	if(lastFreePos.length2())
	{
		Vec3d vec = contact.normal * contact.depth + contact.pos - lastFreePos;

		if(vec * contact.normal > 0)
			return false;
	}

	return gr2PhysicsBone::HandleContact(contact);
}

// physics model

gr2PhysicsModel::gr2PhysicsModel(int timeout) :
	_skeleton(0),
	_model(0),
    _material(-1),
	_initTime(0),
	_collideTime(0),
	_lastUpdate(0),
    _expires(0),
    _expired(false),
	_enabledBones(-1),
	_lastEnabledBones(0),
    _allCollisions(0),
    _lastCollision(0),
	_collider(nullptr),
	_modelNode(nullptr),
	_rootBone(nullptr),
	_attached(nullptr)
{
	Init(timeout);
}

gr2PhysicsModel::gr2PhysicsModel(gr2Model* model, OBJECT* ob, int timeout) :
    _skeleton(0),
    _model(0),
    _material(-1),
    _initTime(0),
    _collideTime(0),
    _lastUpdate(0),
    _expires(0),
    _expired(false),
    _enabledBones(-1),
    _lastEnabledBones(0),
    _allCollisions(0),
    _lastCollision(0),
    _collider(nullptr),
    _modelNode(nullptr),
    _rootBone(nullptr),
    _attached(nullptr)
{
	Init(timeout);
	InitModel(model);
	InitNode(ob);

	if(timeout > 0)
		_expires = frametime + timeout;
}

gr2PhysicsModel::~gr2PhysicsModel()
{
	for(int i = 0; i < (int)_joints.size(); i++)
	{
		dJointID joint = _joints[i];
		dJointDestroy(joint);
	}

	BoneList::iterator it = _bones.begin();
	for(; it != _bones.end(); it++)
	{
		gr2PhysicsBone* bone = *it;
		bone->SetOwner(nullptr);
		bone->Expire();
		delete bone;
	}

	StaticBoneList::iterator sit = _staticBones.begin();
	for(; sit != _staticBones.end(); sit++)
	{
		gr2StaticBone* bone = *sit;
		delete bone;
	}

	bodies.clear();
	_bones.clear();
	_joints.clear();
	_staticBones.clear();

	if(_collider)
		delete _collider;

	if(_model)
		gr2FreeModel(_model);

	if(_skeleton && _skeleton != _model)
		gr2FreeModel(_skeleton);

	// need to remove our model node from the scenegraph
	//if(_gr2group.valid() && _modelNode.valid())
	//	_gr2group->removeChild(_modelNode.get());

	if(_modelNode)
	{
		_modelNode->physicalModel = nullptr;
		_modelNode->drawcallback = nullptr;
		obpoolFreeObjectCallback(_modelNode);
	}
}

void gr2PhysicsModel::Init(int timeout)
{
	if(!_collider)
	{
		//setThreadSafeReferenceCounting(false);
		//setThreadSafeRefUnref(false);

		// create our model's collider
		_collider = new colliderQuery;

		// limit the number of interactions
		_collider->setMaxContactsPerHit(5);

		// let each bone manage its geom
		dSpaceSetCleanup(_collider->space(), 0);

		// allow bones underneath us to collide with each other?
		_collider->addGeomToCollideWith((dGeomID)_collider->space());
	}

	SetExpires(timeout);

	_allCollisions = 0;
	_lastCollision = 0;
	_initTime = frametime;
	_collideTime = frametime;
}

void gr2PhysicsModel::InitModel(gr2Model* m)
{
	// free the old model
	if(_model) gr2FreeModel(_model);

	// set our new model
	_model = m;

	// copy the vertices
	_model->MakeMutable();

	// update our skeleton and model to
	// the default positions
	gr2UpdateModel(_skeleton, true, true);

	//if(_model != _skeleton)
		gr2UpdateModel(_model, true, true);

	_material = -1;
}

void gr2PhysicsModel::InitNode(OBJECT* ob)
{
	if(!_modelNode)
	{
		_modelNode = gr2PhysicsGetModelNode();
		_modelNode->physicalModel = this;
	}

	if(ob)
	{
		_modelNode->mat = ob->mat;
		_modelNode->pos = ob->pos;

        if(ob->flag3 & OBJECT_IS_INTERIOR)
            _modelNode->flag3 |= OBJECT_IS_INTERIOR;

		if(!(ob->flag3 & OBJECT_CAST_SHADOW))
            _modelNode->flag3 &= ~OBJECT_CAST_SHADOW;
    }
	else
	{
		_modelNode->mat = IdentityMatrix;
		_modelNode->pos.x = 0;
		_modelNode->pos.y = 0;
		_modelNode->pos.z = 0;
	}

	_rootMatrix = _modelNode->mat;
	_lastRootPos = Vec3d(0,0,0);
}

int gr2PhysicsModel::GetBoneIndex(dBodyID body)
{
	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); it++)
	{
		if(body == (*it)->GetBody())
			return (*it)->GetBone();
	}

	return -1;
}

gr2PhysicsBone* gr2PhysicsModel::GetBone(int boneIndex)
{
	BoneList::iterator it = _bones.begin();
	gr2PhysicsBone* bone = nullptr;

	for(; it != _bones.end(); it++)
	{
		if(boneIndex == (*it)->GetBone())
		{
			bone = *it;
			break;
		}
	}

	return bone;
}

gr2PhysicsBone* gr2PhysicsModel::GetBone(dBodyID body)
{
	BoneList::iterator it = _bones.begin();
	gr2PhysicsBone* bone = nullptr;

	for(; it != _bones.end(); it++)
	{
		if(body == (*it)->GetBody())
		{
			bone = *it;
			break;
		}
	}

	return bone;
}

gr2PhysicsBone* gr2PhysicsModel::GetBone(const char* name)
{
	BoneList::iterator it = _bones.begin();
	gr2PhysicsBone* bone = nullptr;

	for(; it != _bones.end(); it++)
	{
		if(!strcasecmp(name, (*it)->GetName()))
		{
			bone = *it;
			break;
		}
	}

	return bone;
}


gr2StaticBone* gr2PhysicsModel::GetStaticBone(int boneIndex)
{
	StaticBoneList::iterator it = _staticBones.begin();
	gr2StaticBone* bone = nullptr;

	for(; it != _staticBones.end(); it++)
	{
		if(boneIndex == (*it)->GetBone())
		{
			bone = *it;
			break;
		}
	}

	return bone;
}

gr2StaticBone* gr2PhysicsModel::GetStaticBone(const char* name)
{
	StaticBoneList::iterator it = _staticBones.begin();
	gr2StaticBone* bone = nullptr;

	for(; it != _staticBones.end(); it++)
	{
		if(!strcasecmp(name, (*it)->GetName()))
		{
			bone = *it;
			break;
		}
	}

	return bone;
}

bool gr2PhysicsModel::InitSkeleton(const char* path)
{
	// temp var
	int boneCount = 0;

	// load it
	_skeleton = gr2Model::Load(path, nullptr, &boneCount, 1);

	// update the model so we have good bone positions
	if(_skeleton)
	{
		gr2UpdateModel(_skeleton);
		return true;
	}

	return false;
}

void gr2PhysicsModel::InitBones()
{
	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); it++)
		(*it)->Init();

	StaticBoneList::iterator sit = _staticBones.begin();

	for(; sit != _staticBones.end(); sit++)
		(*sit)->Init();
}

gr2Model* gr2PhysicsModel::GetSkeleton()
{
	return (_skeleton) ? _skeleton: _model;
}

gr2PhysicsBone* gr2PhysicsModel::addBone(gr2PhysicsBone* bone)
{
	_bones.push_back(bone);
	bodies.insert(bone);

	if(bone->GetMass())
		bone->SetFrictionModel(30000.0f, 10000.0f, 1000);

	return bone;
}

gr2PhysicsBone* gr2PhysicsModel::addCenterBone(const char* name, double mass, double len, double rad)
{
	if(_skeleton && _skeleton->FindBone((SCHAR*)name) >= 0)
		return addBone(new gr2CenterBone(this, name, mass, len, rad));

	return nullptr;
}

gr2PhysicsBone* gr2PhysicsModel::addAxialBone(const char* name, double mass, double len, double rad)
{
	if(_skeleton && _skeleton->FindBone((SCHAR*)name) >= 0)
		return addBone(new gr2AxialBone(this, name, mass, len, rad));

	return nullptr;
}

gr2PhysicsBone* gr2PhysicsModel::addAxialFootBone(const char* name, double mass, double len, double rad)
{
	if(_skeleton && _skeleton->FindBone((SCHAR*)name) >= 0)
		return addBone(new gr2AxialFootBone(this, name, mass, len, rad));

	return nullptr;
}

gr2StaticBone* gr2PhysicsModel::addStaticBone(const char* name)
{
	gr2StaticBone* newBone = nullptr;

	if(_skeleton && _skeleton->FindBone((SCHAR*)name) >= 0)
	{
		newBone = new gr2StaticBone(this, name);
		_staticBones.push_back(newBone);
	}

	return newBone;
}

gr2PhysicsBone* gr2PhysicsModel::addMeshBone(const char* name)
{
	if(_skeleton && _skeleton->FindBone((SCHAR*)name) >= 0)
		return addBone(new gr2MeshBone(this, name));

	return nullptr;
}

gr2PhysicsBone* gr2PhysicsModel::addBone(const char* name,	// bone name
											double mass,		// bone mass
											double len,			// capsule length
											double rad,			// capsule radius
											double ang,			// initial rotation angle
											int axis,			// initial rotation axis 1 = x, 2 = y, 3 = z
											double xoff,		// initial x offset
											int orient)			// bone position orientation
{
	if(_skeleton && _skeleton->FindBone((SCHAR*)name) >= 0)
		return addBone(new gr2PhysicsBone(this, name, orient, mass, len, rad, ang, axis));

	return nullptr;
}

dJointID gr2PhysicsModel::addJoint(gr2PhysicsBone* from,
									gr2PhysicsBone* to,
									int type,
									Vec3d* ax1,
									SINT32 ax1Lo,
									SINT32 ax1Hi,
									Vec3d* ax2,
									SINT32 ax2Lo,
									SINT32 ax2Hi,
									Vec3d* ax3,
									SINT32 ax3Lo,
									SINT32 ax3Hi)
{
	dWorldID world		= gr2PhysicsBody::DefaultWorld();
	dJointID jointID	= nullptr;

	switch(type)
	{
		case dJointTypeBall:
			jointID = dJointCreateBall(world, 0);
		break;
		case dJointTypeHinge:
			jointID = dJointCreateHinge(world, 0);
		break;
		case dJointTypeSlider:
			jointID = dJointCreateSlider(world, 0);
		break;
		case dJointTypeUniversal:
			jointID = dJointCreateUniversal(world, 0);
		break;
		case dJointTypeHinge2:
			jointID = dJointCreateHinge2(world, 0);
		break;
		case dJointTypeFixed:
			jointID = dJointCreateFixed(world, 0);
		break;
		case dJointTypeAMotor:
			jointID = dJointCreateAMotor(world, 0);
		break;
		default:
			// do nothing
		break;
	}

	if(jointID)
	{
		// save the join ID in the from bone for later
		from->SetJointID(jointID);

		// we own the bone, so save it so we can kill it later
		_joints.push_back(jointID);

		// attach the two bodies to the joint
		dJointAttach(jointID, (from) ? from->GetBody(): 0, (to) ? to->GetBody(): 0);

		// get our body's transform
		Matrixd transform = from->GetTransform(false);

		// adjust it into bone position
		from->PhysicalToVisual(transform);

		// save the adjusted position of the bone
		Vec3d pos = transform.getTrans();

		// remove the position so we just have the rotational info left
		transform.setTrans(0,0,0);

		Vec3d axis1;
		Vec3d axis2;
		Vec3d axis3;

		// rotate our joint axis into a world orientation
		if(ax1) axis1 = *ax1 * transform;
		if(ax2) axis2 = *ax2 * transform;
		if(ax3) axis3 = *ax3 * transform;

		// init the joint by type
		switch(dJointGetType(jointID))
		{
			case dJointTypeBall:
			{
				dJointSetBallAnchor(jointID, pos.x(), pos.y(), pos.z());
			}
			break;
			case dJointTypeAMotor:
			{
				dJointSetAMotorMode(jointID, dAMotorEuler);

				if(ax1)
				{
					dJointSetAMotorAxis (jointID, 0, 1, axis1.x(), axis1.y(), axis1.z());
					//dJointSetAMotorAxis (jointID, 1, 1, axis2.x(), axis2.y(), axis2.z());
					dJointSetAMotorAxis (jointID, 2, 2, axis3.x(), axis3.y(), axis3.z());
					dJointSetAMotorParam(jointID, dParamLoStop, DEG(ax1Lo));
					dJointSetAMotorParam(jointID, dParamHiStop, DEG(ax1Hi));
					dJointSetAMotorParam(jointID, dParamLoStop2, DEG(ax2Lo));
					dJointSetAMotorParam(jointID, dParamHiStop2, DEG(ax2Hi));
					dJointSetAMotorParam(jointID, dParamLoStop3, DEG(ax3Lo));
					dJointSetAMotorParam(jointID, dParamHiStop3, DEG(ax3Hi));
					from->SetJointParams();
                    
                    /*
                      TBD - something related to stiffness we need to do here?
                    
                    double stiffness = 50000.0f;
                    
					from->SetStiffnessModel(10000.0f, 500.0f, 500);
                     */
				}
			}
			break;
			case dJointTypeUniversal:
			{
				dJointSetUniversalAnchor(jointID, pos.x(), pos.y(), pos.z());
				dJointSetUniversalAxis1(jointID, axis1.x(), axis1.y(), axis1.z());
				dJointSetUniversalAxis2(jointID, axis2.x(), axis2.y(), axis2.z());
				dJointSetUniversalParam(jointID, dParamLoStop, DEG(ax1Lo));
				dJointSetUniversalParam(jointID, dParamHiStop, DEG(ax1Hi));
				dJointSetUniversalParam(jointID, dParamLoStop2, DEG(ax2Lo));
				dJointSetUniversalParam(jointID, dParamHiStop2, DEG(ax2Hi));

				from->SetJointParams();
				from->SetStiffnessModel(5000.0f, 100.0f, 500);
			}
			break;
			case dJointTypeFixed:
			{
				dJointSetFixed(jointID);
			}
			break;
			case dJointTypeHinge:
			{
				dJointSetHingeAnchor(jointID, pos.x(), pos.y(), pos.z());
				dJointSetHingeAxis (jointID, axis1.x(), axis1.y(), axis1.z());
				dJointSetHingeParam(jointID, dParamLoStop, DEG(ax1Lo));
				dJointSetHingeParam(jointID, dParamHiStop, DEG(ax1Hi));

				from->SetJointParams();
				from->SetStiffnessModel(5000.0f, 100.0f, 500);
			}
			break;
			case dJointTypeHinge2:
			{
				dJointSetHinge2Anchor(jointID, pos.x(), pos.y(), pos.z());
				dJointSetHinge2Axis1(jointID, axis1.x(), axis1.y(), axis1.z());
				dJointSetHinge2Axis2(jointID, axis2.x(), axis2.y(), axis2.z());
				dJointSetHinge2Param(jointID, dParamLoStop, DEG(ax1Lo));
				dJointSetHinge2Param(jointID, dParamHiStop, DEG(ax1Hi));
				dJointSetHinge2Param(jointID, dParamLoStop2, DEG(ax2Lo));
				dJointSetHinge2Param(jointID, dParamHiStop2, DEG(ax2Hi));

				from->SetJointParams();
				from->SetStiffnessModel(5000.0f, 100.0f, 500);
			}
			break;
			default:
				// do nothing
			break;
		}
	}

	return jointID;
}

void gr2PhysicsModel::UpdateBoneTransform(float* boneMat, Vec3d& pos, Matrixd& rot)
{
	if(boneMat)
	{
		boneMat[0] = (float)rot(0,0);
		boneMat[1] = (float)rot(0,1);
		boneMat[2] = (float)rot(0,2);
		boneMat[12] = (float)pos.x(); // x

		boneMat[4] = (float)rot(1,0);
		boneMat[5] = (float)rot(1,1);
		boneMat[6] = (float)rot(1,2);
		boneMat[13] = (float)pos.y(); // y

		boneMat[8] = (float)rot(2,0);
		boneMat[9] = (float)rot(2,1);
		boneMat[10] = (float)rot(2,2);
		boneMat[14] = (float)pos.z(); // z
	}
}

void gr2PhysicsModel::UpdateBoneTransform(float* boneMat, Vec3d& pos, Matrixf& rot)
{
	if(boneMat)
	{
		boneMat[0] = rot(0,0);
		boneMat[1] = rot(0,1);
		boneMat[2] = rot(0,2);
		boneMat[12] = (float)pos.x(); // x

		boneMat[4] = rot(1,0);
		boneMat[5] = rot(1,1);
		boneMat[6] = rot(1,2);
		boneMat[13] = (float)pos.y(); // y

		boneMat[8] = rot(2,0);
		boneMat[9] = rot(2,1);
		boneMat[10] = rot(2,2);
		boneMat[14] = (float)pos.z(); // z
	}
}

/*
dJointID gr2PhysicsBone::setJointStiffness(dJointID joint, double stiffness)
{
	int fmax	= (axis == 3 ) ? dParamFMax3 : (axis == 2) ? dParamFMax2 : dParamFMax;
	int vel	= (axis == 3 ) ? dParamVel3 : (axis == 2) ? dParamVel2 : dParamVel;

	switch(dJointGetType(joint))
	{
		case dJointTypeBall:
			//dJointSetHingeParam(joint, fmax, speed);
			//dJointSetHingeParam(joint, vel, velocity);
		break;
		case dJointTypeHinge:
			dJointSetHingeParam(joint, fmax, speed);
			dJointSetHingeParam(joint, vel, velocity);
		break;
		case dJointTypeSlider:
			dJointSetSliderParam(joint, fmax, speed);
			dJointSetSliderParam(joint, vel, velocity);
		break;
		case dJointTypeUniversal:
			dJointSetUniversalParam(joint, fmax, speed);
			dJointSetUniversalParam(joint, vel, velocity);
		break;
		case dJointTypeHinge2:
			dJointSetHinge2Param(joint, fmax, speed);
			dJointSetHinge2Param(joint, vel, velocity);
		break;
		case dJointTypeAMotor:
			dJointSetAMotorParam(joint, fmax, speed);
			dJointSetAMotorParam(joint, vel, velocity);
		break;
		default:
			// do nothing
		break;
	}

	return joint;
}
*/

// bone to bodies
void gr2PhysicsModel::InitPose(gr2Model* poseModel)
{
	BoneList::iterator it = _bones.begin();

	Matrixf rotate = Matrixf::rotate(PI, 0, 0, 1);
	Matrixf compositeTransform;
	Matrixf currentTransform;
	Matrixf finalTransform;
	Matrixf parentTransform;
	Matrixf localTransform;
	//Matrixf attachTransform;
	Vec3d position;
	//piMATRIX attach;

	granny_skeleton* skeleton = gr2GetSkeleton(poseModel);

	for(; it != _bones.end(); it++)
	{
		gr2PhysicsBone* bone = *it;

		compositeTransform.set(gr2GetCompositeBoneTransformByIndex(poseModel, bone->GetBone()));
		currentTransform.set(bone->GetTransform(false)); // POSSIBLE MATH ISSUE

		// do any attachment transforms
		/*
		gr2GetModelFinalTransform(poseModel, &attach);
		attachTransform.set(attach);
		currentTransform = currentTransform * attachTransform;
		*/

		finalTransform = currentTransform * compositeTransform;

		// if this is not a collidable bone, we simply save the relative the position
		if(!bone->IsCollidable())
		{
			GR2PhysicsBoneRef parent = bone->GetParent();

			if(!parent) // lookup, if we don't have a manually set parent bone
				parent = GetBone(skeleton->Bones[bone->GetBone()].ParentIndex);

			if(parent)
			{
				bone->SetParent(parent);
				parentTransform.set(gr2GetBoneTransformByIndex(poseModel, parent->GetBone()));
				parentTransform = Matrixf::inverse(parentTransform);
				localTransform = finalTransform * parentTransform;
				bone->SetLocalTransform(localTransform);
			}
		}

		// rotate it 180 around z because granny is backwards
		//finalTransform = finalTransform * rotate;

		// rotate the transform into world position
		finalTransform = finalTransform * _worldTransform;

		if(finalTransform.valid())
		{
			bone->SetRotation(finalTransform.getRotate());
			bone->SetPosition(finalTransform.getTrans(), false);
		}

		bone->SetLinearVel(_velocity);
	}
}

// bodies to bones
void gr2PhysicsModel::UpdateBones()
{
	// reset each update so we always have the current count
	_enabledBones = 0;

	granny_skeleton* skeleton = gr2GetSkeleton(_skeleton);
	granny_world_pose* WorldPose = gr2GetWorldPose(_skeleton);

	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); it++)
	{
		gr2PhysicsBone* bone = *it;

		if(bone->IsCollidable() && bone->IsEnabled())
			++_enabledBones;

		bone->BodyToBone();
	}

	if(_enabledBones || _lastEnabledBones != _enabledBones)
	{
		StaticBoneList::iterator sit = _staticBones.begin();

		for(; sit != _staticBones.end(); sit++)
			(*sit)->BodyToBone();
	}

	// build the model's world pose
	if(GrannyGetWorldPoseComposite4x4Array(WorldPose) != nullptr)
		GrannyBuildWorldPoseComposites(skeleton, 0, skeleton->BoneCount, WorldPose);

	// debug enabled bone count
	if(_lastEnabledBones != _enabledBones)
	{
		// do something?
		//uiChatPrintLine("Enabled Bodies: %d at %d seconds", _enabledBones, (int)((frametime - _initTime)/1000));

		if(_enabledBones)
			_lastEnabledBones = _enabledBones;
	}

	//for(it = _bones.begin(); it != _bones.end(); it++)
	//	dBodyDisable((*it)->GetBody());
}

/*
void gr2PhysicsModel::UpdateBones2()
{
	// update the bone positions and rotations
	// based on each body

	granny_skeleton* skeleton = gr2GetSkeleton(_skeleton);
	granny_world_pose* WorldPose = gr2GetWorldPose(_skeleton);
	granny_matrix_4x4* WorldMatrices = GrannyGetWorldPose4x4Array(WorldPose);

	BoneMap::iterator it = _bones.begin();
	Matrixf inverseWorldMat = Matrixf::inverse(_worldTransform);
    Matrixf rotate = Matrixf::rotate(PI, 0, 0, 1);
	Matrixf transform;
	Vec3d position;

	// reset each update so we always have the current count
	_enabledBones = 0;

	for(; it != _bones.end(); it++)
	{
		gr2PhysicsBone* bone = it->second;

		if(bone->IsCollidable() && bone->IsEnabled())
		{
			++_enabledBones;

			transform.set(bone->GetTransform(false)); // POSSIBLE MATH ISSUE; probably correct
			position.set(transform.getTrans());

			switch(bone->GetOrientation())
			{
				case gr2PhysicsBone::AXIAL:
				{
					switch(dJointGetType(bone->GetJoint()))
					{
						case dJointTypeHinge:
						{
							dVector3 anchor;
							dJointGetHingeAnchor(bone->GetJoint(), anchor);
							position.set(Vec3Ode(anchor));
						}
						break;
						case dJointTypeUniversal:
						{
							dVector3 anchor;
							dJointGetUniversalAnchor(bone->GetJoint(), anchor);
							position.set(Vec3Ode(anchor));
						}
						break;
						default:
						{
							//position.set(transform.getTrans());
							// do nothing
						}
						break;
					}

				}
				break;
				default:
				{
					//position.set(transform.getTrans());
					// do nothing
				}
				break;
			}

			// transform it into object space
			transform = transform * inverseWorldMat;
			position = position * inverseWorldMat;

			// rotate 180 around z because of backwards granny
			//transform = transform * rotate;
			//position = position * rotate;

			granny_real32* boneTransform = (float*)WorldMatrices[bone->GetBone()];

			// update it
			UpdateBoneTransform(boneTransform, position, transform);
		}
	}

	if(_enabledBones)
	{
		it = _bones.begin();

		for(; it != _bones.end(); it++)
		{
			gr2PhysicsBone* bone = it->second;

			if(!bone->IsCollidable())
			{
				// this bone may be a little behind where it should be, but
				// it probably won't have a visual impact on the simulation
				gr2PhysicsBone* parent = bone->GetParent();
				if(!parent)
					continue;

				transform.set((float*)WorldMatrices[parent->GetBone()]);
				transform = bone->GetLocalTransform() * transform;
				position.set(transform.getTrans());

				granny_real32* boneTransform = (float*)WorldMatrices[bone->GetBone()];

				Matrixf temp;
				temp.set((float*)boneTransform);

				// update it
				UpdateBoneTransform(boneTransform, position, transform);
			}
		}
	}

	// build the model's world pose
	if(GrannyGetWorldPoseComposite4x4Array(WorldPose) != nullptr)
		GrannyBuildWorldPoseComposites(skeleton, 0, skeleton->BoneCount, WorldPose);

	// debug enabled bone count
	if(_lastEnabledBones != _enabledBones)
	{
		// do something?
		//uiChatPrintLine("Enabled Bodies: %d at %d seconds", _enabledBones, (int)((frametime - _initTime)/1000));

		if(_enabledBones)
			_lastEnabledBones = _enabledBones;
	}

	//for(it = _bones.begin(); it != _bones.end(); it++)
	//	dBodyDisable(it->second->getBody());
}
*/

// bodies to bones
void gr2PhysicsModel::BodiesToBones(gr2Model* to)
{
	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); it++)
		(*it)->BodyToBone(to);

	StaticBoneList::iterator sit = _staticBones.begin();

	for(; sit != _staticBones.end(); sit++)
		(*sit)->BodyToBone(to);
}

// bones to bodies
void gr2PhysicsModel::BonesToBodies(gr2Model* from)
{
	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); it++)
		(*it)->BoneToBody(from);
}

void gr2PhysicsModel::UpdateVelocity()
{
	static int nextUpdate = 0;

	if(nextUpdate && frametime < nextUpdate)
		return;

	// update velocity every 20ms
	nextUpdate = frametime + 20;

	_velocity *= .9f;

	if(_velocity.length2() < 1.0f)
		_velocity.set(0,0,0);
}

double gr2PhysicsModel::GetBoneDist(const char* bone1, const char* bone2)
{
	Vec3d bone1Pos;
	Vec3d bone2Pos;

	GetWorldBonePosition(bone1, bone1Pos);
	GetWorldBonePosition(bone2, bone2Pos);

	Vec3d dist = bone2Pos - bone1Pos;

	return dist.length();
}

// given a bone name, return its position and rotation
bool gr2PhysicsModel::GetBoneTransform(const char* name, Matrixf* transform)
{
	if(transform)
	{
		transform->set(gr2GetBoneTransform(_skeleton, (SCHAR*)name));
		return true;
	}

	return false;
}

bool gr2PhysicsModel::GetBoneTransform(int bone, Matrixf* transform)
{
	if(transform)
	{
		transform->set(gr2GetBoneTransformByIndex(_skeleton, bone));
		return true;
	}

	return false;
}

bool gr2PhysicsModel::GetWorldBonePosition(const char* bone, Vec3d& position, Vec3d* offset)
{
	Matrixd transform;

	if(GetWorldBoneTransform(bone, &transform))
	{
		// add the translation to the passed in position
		position += transform.getTrans();

		// if an offset was passed, add that
		if(offset)
			position += *offset;

		return true;
	}

	return false;
}

Vec3d& gr2PhysicsModel::GetWorldBonePosition(const char* bone)
{
	static Vec3d position;
	position.set(0,0,0);

	if(!GetWorldBonePosition(bone, position))
		position.set(0,0,0);

	return position;
}

bool gr2PhysicsModel::GetWorldBoneTransform(const char* name, Matrixd* transform)
{
	Matrixf boneTransform;

	if(GetBoneTransform(name, &boneTransform))
	{
		transform->set(boneTransform);
		*transform = *transform * _worldTransform;
		return true;
	}

	return false;
}

void gr2PhysicsModel::BeginStep()
{
	// find new collisions if needed
	if(_collider)
		_collider->findCollisions();

	physicsModel::BeginStep();
}

void gr2PhysicsModel::Update()
{
	if(_enabledBones)
	{
		UpdateBones();

		/* instead save off the model node's mat and update that one
			create a new one for drawing and return it to the draw function
		*/

		// update our game object's position for culling purposes
		if(_rootBone)
		{
			Vec3d rootPos = _rootBone->GetPosition();

			if(_lastRootPos.length2())
			{
				Matrixf obMat;

				// get the difference moved from the last frame
				Vec3d diff = rootPos - _lastRootPos;

				diff.x() = diff.x() * -1.0f;
				diff.y() = diff.y() * -1.0f;

				// init out game object's matrix
				obMat.set(_modelNode->mat);

				// get our game object's position
				Vec3d pos = obMat.getTrans();

				// move the game object the difference
				pos -= diff;

				// reset the game object's position
				obMat.setTrans(pos);

				// update the game object's game matrix
				obMat.get(_modelNode->mat);

				//uiChatPrintLine("Root position is %.2f,%.2f,%.2f", pos.x(), pos.y(), pos.z());
			}

			_lastRootPos.set(rootPos);
		}

		// update any attachments we have
		for(size_t i = 0; i < _skeleton->attachments.size(); i++)
			_skeleton->attachments.at(i)->Update(false, false);
	}
	else
	{
		if(_lastEnabledBones)
		{
			// should be safe to remove all bodies
			// from the physics module once we get to here
			BoneList::iterator it = _bones.begin();

			// physics system will auto-remove expired bodies
			for(; it != _bones.end(); it++)
				(*it)->Expire();

			_lastEnabledBones = 0;
		}
	}
}

void gr2PhysicsModel::EndStep()
{
	physicsModel::EndStep();

	// ensure the next begin update call finds new collisions
	_collider->setDirty(true);

	// save when we last collided with something
	if(_lastCollision)
		_collideTime = frametime;

	// as long as we're active with enabled bones and
	// we haven't hit anything or we have hit something at least GR2_COLLIDE_THRESHOLD ago
	// then update, otherwise we bail out
	if(IsActive() && _enabledBones && (!_allCollisions || ((frametime - _collideTime) < GR2_COLLIDE_THRESHOLD)))
	{
		Update();
	}
	else
	{
		SetActive(false);
	}
}

void gr2PhysicsModel::Deactivate()
{
	if(!_skeleton->attachments.empty())
		_skeleton->FreeAttachments();

	physicsModel::Deactivate();
}

void gr2PhysicsModel::Attach(gr2PhysicsModel* m)
{
	Unattach();

	if(m)
	{
		_attached = m;
		_attached->OnAttach(this);
		OnAttach(_attached);
	}
}

void gr2PhysicsModel::OnAttach(gr2PhysicsModel* m)
{
	if(_attached == nullptr)
		_attached = m;
}

void gr2PhysicsModel::Unattach()
{
	if(_attached)
		_attached->OnUnattach(this);

	OnUnattach(_attached);
}

void gr2PhysicsModel::OnUnattach(gr2PhysicsModel* m)
{
	if(_attached == m)
		_attached = nullptr;
}

gr2PhysicsModel::DRAW_STATE gr2PhysicsModel::OnDraw()
{
	DRAW_STATE state = DS_DRAW;

	if(IsExpired())
	{
		if(IsActive())
			SetActive(false);

		state = DS_NODRAW;

		OnExpire(state);
	}

	return state;
}

#if USE_STATESET
void gr2PhysicsModel::Draw()
{
	gr2PhysicsModel::DRAW_STATE state = OnDraw();// : gr2PhysicsModel::DS_NODRAW;

	if(state == gr2PhysicsModel::DS_DELETE)
	{
		if(IsActive())
			SetActive(false);

		// make sure the physics loop no longer tries to update it
		physicsRemoveObject(this);
	}
	else if(state == gr2PhysicsModel::DS_DRAW)
	{
		// FIXME
		// Since we are using left handed matrices and granny doesn't support them
		// we must kludge our way around that fact
		//piMATRIX world = model->getModelNode()->mat;
		piMATRIX world = GetRootMatrix();
		world._31 = -world._31;
		world._32 = -world._32;
		world._33 = -world._33;

		piSetWorldMatrix(&world);

#ifdef BETA_RELEASE
		if((gr2DrawFlags & (1<<0)) && _model)
		{
			_model->Render(false, true);
		}

		if((gr2DrawFlags ^ 1) > 0)
		{
			GLUquadric* obj = gluNewQuadric();

			gluQuadricOrientation(obj, GLU_OUTSIDE);

			BoneList::iterator it = _bones.begin();

			for(; it != _bones.end(); it++)
			{
				if(gr2DrawFlags & (1<<1))
					(*it)->DrawCollider(obj);

				if(gr2DrawFlags & (1<<2))
				{
					(*it)->DrawBodyPosition(obj, (gr2DrawFlags & (1<<3)) > 0);
				}
			}

			gluDeleteQuadric(obj);

			if(gr2DrawFlags & (1<<4))
				gr2DrawSkeleton(_skeleton, piRGBA(1, 1, 1, 1));

            static ref_ptr<RenderStateSet> stateset;

            if(!stateset.valid())
            {
                stateset = new RenderStateSet;
                stateset->SetRenderState(new PointSize(4.0f));
                stateset->SetRenderState(new Color4F(1, 0, 0, 1));
                stateset->SetMode(GL_LIGHTING, 0);
            }

            stateset->Apply();

			glTranslatef(0,0,world._43 * .95f);
			glBegin(GL_LINES);
			{
				glVertex3f(0, 0, 0);
				glVertex3f(3.0f, 0, 0);
			}
			glEnd();

            Color4F* color = dynamic_cast<Color4F*>(stateset->GetRenderState(RS_COLOR4F));

            color->set(0, 1, 0, 1);
            color->apply();

			glBegin(GL_LINES);
			{
				glVertex3f(0, 0, 0);
				glVertex3f(0, 3.0f, 0);
			}
			glEnd();

            color->set(0, 0, 1, 1);
            color->apply();

            glBegin(GL_LINES);
			{
				glVertex3f(0, 0, 0);
				glVertex3f(0, 0, 3.0f);
			}
			glEnd();
			glTranslatef(0,0,-world._43 * .95f);
		}
#else
		if(_model)
			_model->Render(false, true);
#endif

		oglSetWorldMatrix(&IdentityMatrix);
	}
}
#else
void gr2PhysicsModel::Draw()
{
	gr2PhysicsModel::DRAW_STATE state = OnDraw();// : gr2PhysicsModel::DS_NODRAW;

	if(state == gr2PhysicsModel::DS_DELETE)
	{
		if(IsActive())
			SetActive(false);

		// make sure the physics loop no longer tries to update it
		physicsRemoveObject(this);
	}
	else if(state == gr2PhysicsModel::DS_DRAW)
	{
		static SINT32 defaultInfantryMaterialID = -1;

		int matID = GetMaterial();

		if(defaultInfantryMaterialID == -1)
			defaultInfantryMaterialID = piFindMaterialName("forest1");

		if(matID == -1)
			matID = defaultInfantryMaterialID;

		// set out material
		piSetActiveMaterial(matID);

		// FIXME
		// Since we are using left handed matrices and granny doesn't support them
		// we must kludge our way around that fact
		//piMATRIX world = model->getModelNode()->mat;
		piMATRIX world = GetRootMatrix();
		world._31 = -world._31;
		world._32 = -world._32;
		world._33 = -world._33;

		piSetWorldMatrix(&world);
		piSetRenderState(piRENDERSTATE_CULLMODE, piCULL_CW);

#ifdef BETA_RELEASE
		if((gr2DrawFlags & (1<<0)) && _model)
		{
			_model->Render(false, true);
		}

		if((gr2DrawFlags ^ 1) > 0)
		{
			GLUquadric* obj = gluNewQuadric();

			//oglSetWorldMatrix(&IdentityMatrix);
			//oglSetWorldMatrix(&_transform);

			gluQuadricOrientation(obj, GLU_OUTSIDE);
			//oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

			piDisableTexture();
			oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);
			//oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 1, 1));

			BoneList::iterator it = _bones.begin();

			for(; it != _bones.end(); it++)
			{
				if(gr2DrawFlags & (1<<1))
					(*it)->DrawCollider(obj);

				if(gr2DrawFlags & (1<<2))
				{
					//glTranslatef(0,ITOM(2.0f),0);
					(*it)->DrawBodyPosition(obj, (gr2DrawFlags & (1<<3)) > 0);
					//glTranslatef(0,-ITOM(2.0f),0);
				}
			}

			gluDeleteQuadric(obj);

			if(gr2DrawFlags & (1<<4))
				gr2DrawSkeleton(_skeleton, piRGBA(1, 1, 1, 1));

			//oglSetWorldMatrix(&IdentityMatrix);

			//glPopClientAttrib();
			//glPopAttrib();

			glPushAttrib(GL_ALL_ATTRIB_BITS);
			glPushClientAttrib(GL_ALL_ATTRIB_BITS);
			glPointSize(4.0f);
			glDisable(GL_TEXTURE_2D);
			piDisableTexture();
			oglSetRenderState(oglRENDERSTATE_LIGHTING, 0);

			glTranslatef(0,0,world._43 * .95f);
			oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 0, 0, 1));
			glBegin(GL_LINES);
			{
				glVertex3f(0, 0, 0);
				glVertex3f(3.0f, 0, 0);
			}
			glEnd();


			oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 1, 0, 1));
			glBegin(GL_LINES);
			{
				glVertex3f(0, 0, 0);
				glVertex3f(0, 3.0f, 0);
			}
			glEnd();

			oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(0, 0, 1, 1));
			glBegin(GL_LINES);
			{
				glVertex3f(0, 0, 0);
				glVertex3f(0, 0, 3.0f);
			}
			glEnd();
			glTranslatef(0,0,-world._43 * .95f);

			glPointSize(1.0f);
			glPopClientAttrib();
			glPopAttrib();
		}
#else
		if(_model)
			_model->Render(false, true);
#endif

		piSetRenderState(piRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
		oglSetWorldMatrix(&IdentityMatrix);
	}
}
#endif

/*
bool gr2PhysicsModel::IsActive()
{
	return (physicsModel::IsActive() && _enabledBones) ? true: false;
}
*/

bool gr2PhysicsModel::IsExpired()
{
	return (_expires && frametime >= _expires) ? true: false;
}

void gr2PhysicsModel::Expire()
{
	_expires = frametime;
}

void gr2PhysicsModel::SetExpires(int timeout)
{
	_expires = (timeout > 0) ? frametime + timeout: 0;
}

void gr2PhysicsModel::SetActive(bool a, int timeout)
{
	if(a)
	{
		_enabledBones = -1;
		_lastEnabledBones = 0;

		// reset all bodies
		BoneList::iterator it = _bones.begin();

		for(; it != _bones.end(); it++)
			(*it)->Setup();

		// set our expire time
		SetExpires(timeout);

		// so we get updated
		Activate();
	}
	else
	{
		// remove all bodies from the physics module
		// this will expire them as well
		BoneList::iterator it = _bones.begin();

		for(; it != _bones.end(); it++)
			(*it)->Expire();

		// set this model to be expired
		//_expires = frametime;

		// so we're not updated anymore
		Deactivate();
	}
}

bool gr2PhysicsModel::ProcessFlag(unsigned int f, physicsObject* o)
{
	bool handled = false;

	GR2PhysicsBoneRef bone = dynamic_cast<GR2PhysicsBoneRef>(o);

	if(bone)
	{
		if((f & COLLIDER_TRN_WATER) && !(_allCollisions & COLLIDER_TRN_WATER))
		{
			fxSndAddFiniteWorldSound("sounds/grenadesplash.wav",
											 5,
											 100,
											 IDE_TP_PRIORITY_LOW,
											 bone->getPosition());
		}

		// first contact
		UINT32 killFlags = COLLIDER_CONTACT_MASK|COLLIDER_TRN_WATER;

		if(colliderIsType(f, killFlags) && !colliderIsType(_allCollisions, killFlags))
		{
			Vec3d velocity = bone->GetLinearVel();
			static const float maxVel = 30.0f;

			if(velocity.length2() >= maxVel * maxVel)
			{
				//uiChatPrintLine("High velocity physics impact!");
				/*
				float speed					= (float)velocity.length();
				float mod					= (maxVel / speed);
				GR2PhysicsBoneRef b	= nullptr;

				uiChatPrintLine("Damping model velocity = %.2f -> %.2f", speed, maxVel);

				gr2PhysicsModel::BoneMap::iterator it = _bones.begin();

				for(; it != _bones.end(); ++it)
				{
					b = it->second;

					if(b->isCollidable())
					{
						velocity.set(b->getLinearVel());
						velocity *= mod;
						b->SetLinearVel(velocity);
					}
				}
				*/

				bool isWater = colliderIsType(f, COLLIDER_TRN_WATER);

				const char* effect = (isWater) ? "waterhit" : "shockdust";
				const char* sound = (isWater) ? "sounds/grenadesplash.wav" : "sounds/troop/body/impact.wav";

				fxSndAddFiniteWorldSound(sound, 10, 100, IDE_TP_PRIORITY_LOW, bone->getPosition());

				TransformEffect* te = TransformEffect::AddEffect(effect, Vec3f(0, 0, -1), 0, nullptr);

				if(te)
				{
					Vec3d p(bone->getPosition());
					p.z() = -p.z();

					te->enableemitters(true);;
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
					te->setmatrix(Matrixf::translate(p));
				}

				Kill();
				Expire();

				handled = true;
			}
		}
	}

	_allCollisions |= f;
	_lastCollision |= f;

	return handled;
}

unsigned int gr2PhysicsModel::GetColliderFlags()
{
	unsigned int flags = 0;

	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); ++it)
	{
		const GR2GeomList& geoms = (*it)->GetGeoms();

		for(unsigned int i = 0; i < geoms.size(); ++i)
			flags |= dGeomGetCategoryBits(geoms.at(i));
	}

	return flags;
}

colliderQuery::CollisionList& gr2PhysicsModel::GetCollisions(gr2PhysicsBone* bone)
{
	_collisions.clear();

	const GR2GeomList& geoms = bone->GetGeoms();

	for(unsigned int i = 0; i < geoms.size(); ++i)
		_collider->getCollisions(geoms.at(i), _collisions, false);

	// sort it
	_collisions.sortByDepthAndGeom();

	return _collisions;
}

gr2RigidPhysicsModel::gr2RigidPhysicsModel(const char* path, vehVehicle* veh, Matrixf transform, int timeout) :
	gr2PhysicsModel(timeout),
	_damp(false)
{
	InitSkeleton(path);
	_model = _skeleton;

	// add our model's bones
	granny_skeleton* gs = _skeleton->GetSkeleton();

	for(int i = 0; i < gs->BoneCount; i++)
	{
		if(i == 0)
			_rootBone = addMeshBone(gs->Bones[i].Name);
		else
			addStaticBone(gs->Bones[i].Name);
	}

	// tell our bones to init themselves
	InitBones();

	if(_rootBone)
	{
		_rootBone->SetFrictionModel(1000.0f, 500.0f, 500);

		// keep up us from rotating to fast
		//dBodySetFiniteRotationMode(bone->getBody(), 1);

		dBodySetMaxAngularSpeed(_rootBone->GetBody(), 100000.0);
	}

	piMATRIX& t			= (veh->ModelData) ? veh->ModelData->VehicleAttMat: veh->PosData.AttMat;
	FLOAT64VECTOR& p	= (veh->ModelData) ? veh->ModelData->VehiclePos: veh->PosData.Pos;

	Vec3d worldPos = Vec3d(p);

	_worldTransform.set(t);
	_worldTransform.setTrans(worldPos);

	// init our scenegraph node for drawing
	// need non-osg equivalent
	InitNode(veh->VehicleObjectData->PlaneObject);

	//setEnhancedCollisionCheck(false);
	_collider->setMaxContactsPerHit(10);

	// update to the current pose
	//BonesToBodies(model);

	// double precision
	Matrixd boneTransform = Matrixd(transform);

	{
		// plus our model's world position
		boneTransform = boneTransform * _worldTransform;

		if(boneTransform.valid())
		{
			_rootBone->SetRotation(boneTransform.getRotate());
			_rootBone->SetPosition(boneTransform.getTrans());
		}
	}

	/*
     
     TBD - Is this code that was broken and needs to be fixed?
     
	boneTransform = boneTransform * _worldTransform;

	// get our body world transform
	Matrixd bodyTransform		= Matrixd(_rootBone->GetTransform());
	Matrixd localBodyTransform	= Matrixd::inverse(bodyTransform);

	// get it into object space
	localBodyTransform = bodyTransform * localBodyTransform;

	// put it into double precision
	Matrixd localComposite = Matrixd(transform);

	// rotate it into final rotation and positon
	localBodyTransform = localBodyTransform * localComposite;

	// move it back into world position and rotation
	bodyTransform = localBodyTransform * bodyTransform;

	// reset our body's position and rotation
	_rootBone->SetTransform(bodyTransform);
	*/

	// make sure we're active
	SetActive(true, timeout);

	// now re-orient the granny model's position and rotation
	UpdateBones();

	_material = piFindMaterialName("forest1");
}

void gr2RigidPhysicsModel::BeginStep()
{
	if(IsActive())
		_lastCollision = 0;

	gr2PhysicsModel::BeginStep();
}

void gr2RigidPhysicsModel::EndStep()
{
	gr2PhysicsModel::EndStep();
	damp();

	if(!_joints.empty())
	{
		bool breakIt = false;

		if(frametime < _initTime + 1500)
		{
			if(GetRandom(0.0f, 1.0f) < .001f)
				breakIt = true;
		}
		else
		{
			breakIt = true;
		}

		if(breakIt)
		{
			for(unsigned int j = 0; j < _joints.size(); ++j)
				dJointDestroy(_joints[j]);

			_rootBone->SetJointID(nullptr);
			_joints.clear();
		}
	}

	if(_lastEnabledBones != _enabledBones)
	{
		//uiChatPrintLine("Enabled weapon bones: %d", _enabledBones);

		if(!_enabledBones)
		{
			// we're disabled, expire ourselves?
		}
	}

	if(IsExpired())
	{
		Expire();
		SetActive(false);
	}
}


gr2RigidPhysicsModel::~gr2RigidPhysicsModel()
{
	Unattach();

	_lastCollision = 0;

	// calculate last collision
	for(BoneList::iterator it = _bones.begin(); it != _bones.end(); ++it)
		_lastCollision |= (*it)->LastCollisions();

	Deactivate();

	// we may need to delete this model's geom and body?

#ifdef USE_CORPSES
	/*
	if(colliderIsType(_lastCollision, COLLIDER_CONTACT_MASK) && !killed)
	{
		if(!colliderIsType(_lastCollision, COLLIDER_DRIVABLE_MASK) && cfgPrefsGetBool("app.corpses", true))
		{
			Corpse* c = gr2AnimAddCorpse(this, nullptr, false, 30000); // , bone->getName(), false
			c->shadow = false;
		}
	}
	*/
#endif
}

// bone to bodies
void gr2RigidPhysicsModel::InitPose(gr2Model* poseModel)
{
	BoneList::iterator it = _bones.begin();

	Matrixf rotate = Matrixf::rotate(PI, 0, 0, 1);
	Matrixf compositeTransform;
	Matrixf currentTransform;
	Matrixf finalTransform;
	Matrixf parentTransform;
	Matrixf localTransform;
	Vec3d position;
	piMATRIX rawTransform;

	granny_skeleton* skeleton = gr2GetSkeleton(poseModel);

	for(; it != _bones.end(); it++)
	{
		gr2PhysicsBone* bone = *it;

		if(gr2GetRawBonePosition(poseModel, (SCHAR*)bone->GetName(), &rawTransform))
			compositeTransform.set(rawTransform);

		//currentTransform.set(bone->GetTransform(false)); // POSSIBLE MATH ISSUE

		//finalTransform = currentTransform * compositeTransform;
		finalTransform = compositeTransform;

		// if this is not a collidable bone, we simply save the relative the position
		if(!bone->IsCollidable())
		{
			GR2PhysicsBoneRef parent = bone->GetParent();

			if(!parent) // lookup, if we don't have a manually set parent bone
				parent = GetBone(skeleton->Bones[bone->GetBone()].ParentIndex);

			if(parent)
			{
				bone->SetParent(parent);

				if(gr2GetRawBonePosition(poseModel, (SCHAR*)parent->GetName(), &rawTransform))
					parentTransform.set(rawTransform);

					//parentTransform.set(gr2GetBoneTransformByIndex(poseModel, parent->getBone()));
				parentTransform = Matrixf::inverse(parentTransform);
				localTransform = finalTransform * parentTransform;
				bone->SetLocalTransform(localTransform);
			}
		}

		// rotate it 180 around z because granny is backwards
		//finalTransform = finalTransform * rotate;

		// rotate the transform into world position
		finalTransform = finalTransform * _worldTransform;

		bone->SetRotation(finalTransform.getRotate());
		bone->SetPosition(finalTransform.getTrans(), false);
		bone->SetLinearVel(_velocity);

		//dBodyDisable(bone->getBody());
	}
}

//#define MAX_AVEL 10*PI
#define MAX_AVEL 1.0f

void gr2RigidPhysicsModel::damp()
{
	if(!_rootBone || !_damp)
		return;

	// cap angular velocity
	FLOAT64VECTOR avel = _rootBone->getAngularVel();
	double aspd = mathExactVectorLength(&avel);
	if(aspd > MAX_AVEL)
	{
		mathScaleVector(&avel, &avel, (MAX_AVEL / aspd));
		_rootBone->setAngularVel(avel);
	}

	/*
	double k = .1;// air resistance (arbitrary)

	FLOAT64VECTOR F;
	FLOAT64VECTOR v = _rootBone->getLinearVel();

	F.x =-k*v.x;
	F.y =-k*v.y;
	F.z =-k*v.z;

	dBodyAddForce(_rootBone->getBody(), F.x, F.y, F.z);
	*/
}

bool gr2RigidPhysicsModel::makeCollider()
{
	if(!_rootBone)// || !collider)
		return false;

	/*
	dMass total;
	dMassSetZero(&total);

	const std::vector<WeaponShape::Shape>& shapes = collider->shapes;

	for(unsigned int i = 0; i < shapes.size(); ++i)
	{
		const WeaponShape::Shape& shape = shapes.at(i);
		dMass shapeMass;

		double radius = shape.diameter/2.0f;
		double length = shape.length - shape.diameter;

		if(length < 0)
			length = 0;

		// direction is always Z; capsule bodies run along the Z axis
		// we rotate them to match the bones they represent
		// length is minus the caps on each end which is radius * 2

		// make length the length of the body, not the total length
		// because length is passed in as the total length of the capsule
		dMassSetCapsuleTotal(&shapeMass, LTOK(shape.mass), gr2PhysicsBone::Z, radius, length);
		dMassAdd(&total, &shapeMass);

		// our actual collision capsule
		dGeomID capsule = dCreateCapsule(0, radius, length);

		if(shape.rotation.w())
		{
			// rotate it along the x so that it fits our body better
			Vec3d axis(shape.rotation.x(), shape.rotation.y(), shape.rotation.z());
			Matrixf rot = Matrixf::rotate(PI2, axis); // mlDEG_TO_RAD(shape.rotation.w())
			dMatrix3 dmat;
			MatrixTodMatrix(rot, dmat);
			dGeomSetRotation(capsule, dmat); // should this be capsule and not shape?
		}

		Matrixf boneTransform;
		boneTransform.set(gr2GetBoneTransformByIndex(_skeleton, _rootBone->GetBone()));

		Vec3d spos = shape.offset;
		Vec3d tpos(boneTransform.getTrans());

		//tpos.set(0, .83f, .15f);

		spos = spos - tpos;

		// capsules are oriented up and rotated to the Y below
		// setting the position to the x offset achieves a
		// final the result we want which is a capsule moved
		// along the X axis after its rotated by the shape
		// transform geom below
		dGeomSetPosition(capsule, spos.x(), spos.y(), spos.z());

		// our shape transform
		dSpaceID space = GetSpaceID();
		dGeomID transform = dCreateGeomTransform(space);

		dGeomTransformSetCleanup(transform, 1);
		dGeomTransformSetInfo(transform, 1);
		dGeomTransformSetGeom(transform, capsule);

		_rootBone->AddShape(transform, COLLIDER_WEAPON, COLLIDER_WEAPON_MASK);
	}

	dBodySetMass(_rootBone->GetBody(), &total);

	_damp = collider->damp;
	*/

	/*
	const std::vector<WeaponShape::Link>& links = collider->links;
	for(int i = 0; i < links.size(); ++i)
	{
		const WeaponShape::Link& link = links.at(i);

		GR2PhysicsBoneRef attachTo = _owner->getBone(link.bone.c_str());

		if(attachTo)
			addJoint(_rootBone, attachTo, dJointTypeFixed);
	}
	*/

	return true;
}

void gr2RigidPhysicsModel::OnExpire(DRAW_STATE& state)
{
	state = gr2PhysicsModel::DS_DELETE;
}

void gr2RigidPhysicsModel::OnRemove()
{
	delete this;
}

bool gr2RigidPhysicsModel::ProcessFlag(unsigned int f, physicsObject* o)
{
	/*
	GR2PhysicsBoneRef bone = dynamic_cast<GR2PhysicsBoneRef>(o);

	if(bone)
	{
		if((f & COLLIDER_CONTACT_MASK) && !(_allCollisions & COLLIDER_CONTACT_MASK))
		{
			static double minVel = 1.5f;
			Vec3d bv = bone->GetLinearVel();

			if(bv.length2() >= (minVel * minVel))
			{
				int id = fxSndAddFiniteWorldSound("sounds/troop/body/fall6.wav",
												 0,
												 15,
												 IDE_TP_PRIORITY_LOW,
												 bone->getPosition());
			}

			Unattach();
		}
	}
	*/

	return gr2PhysicsModel::ProcessFlag(f, o);
}














/*
void gr2PhysicsInit()
{
	// do something?
}

void gr2PhysicsUpdate()
{
	ModelList::iterator it = _models.begin();
	ModelList::iterator end = _models.end();

	for(; it != _models.end(); ++it)
	{
		gr2PhysicsModel* model = *it;

		if(model->IsExpired())
		{
			it = _models.erase(it);
			model->SetActive(false);
			if(it == end) break;
		}
		else
		{
			model->Update();
		}
	}
}
*/

#if(0)
static physicsModel* _findModelByObject(physicsModel* m, void* const data)
{
	gr2PhysicsModel* model = dynamic_cast<gr2PhysicsModel*>(m);

	if(model && model->GetModelNode() == data)
		return model;

	return nullptr;
}
#endif

static physicsObject* _findActiveModels(physicsObject* ob, void* const data)
{
	gr2PhysicsModel* model = dynamic_cast<gr2PhysicsModel*>(ob);

	if(!model)
		return nullptr;

	if(model->GetColliderFlags() & (COLLIDER_WEAPON|COLLIDER_CORPSE))
		return model;

	return nullptr;
}

void gr2PhysicsReset()
{
	physicsObjectSet list;

	if(physicsFindObject(list, _findActiveModels))
	{
		physicsObjectSet::iterator it = list.begin();
		gr2PhysicsModel* model = nullptr;

		for(; it != list.end(); ++it)
		{
			model = dynamic_cast<gr2PhysicsModel*>(*it);
			model->Kill();
			model->Expire();
		}
	}
}

bool gr2PhysicsCullCallback(OBJECT* pObject)
{
	if(pObject->distsqr >= GR2ANIM_CULL_DISTANCE)
		return true;

    if(obdrawIsRenderPass(RENDER_SHADOW_MAP) && pObject->distsqr > GR2ANIM_SHADOW_DISTANCE)
        return true;

	return false;
}

bool gr2PhysicsDrawCallback(OBJECT* o)
{
	/*
	physicsModel* pmodel = physicsFindModel(_findModelByObject, ob);
	if(!pmodel) return 0;

	gr2PhysicsModel* model = dynamic_cast<gr2PhysicsModel*>(pmodel);
	if(!model) return 0;
	*/

	if(!obmainValidateObject(o))
		return false;

	gr2PhysicsModel* model = dynamic_cast<gr2PhysicsModel*>(o->physicalModel);

	if(model)
		model->Draw();

	return true;
}

static physicsObject* _findWeaponModels(physicsObject* ob, void* const data)
{
	gr2PhysicsModel* model = dynamic_cast<gr2PhysicsModel*>(ob);

	if(!model)
		return nullptr;

	if((model->GetColliderFlags() & COLLIDER_WEAPON) > 0)
		return model;

	return nullptr;
}

void gr2PhysicsKill()
{
	physicsObjectSet list;

	// other model types are tracked by other systems, but weapons delete themselves
	// so since we're exiting, go ahead and kill these guys
	if(physicsFindObject(list, _findWeaponModels))
	{
		physicsObjectSet::iterator it = list.begin();
		gr2PhysicsModel* model = nullptr;

		for(; it != list.end(); ++it)
		{
			model = dynamic_cast<gr2PhysicsModel*>(*it);
			physicsRemoveObject(model, true);
			model->Kill();
			model->Expire();
			delete model;
		}
	}
}

/*
void gr2PhysicsKill()
{
	// kill the remaining models
	_models.clear();
}

void gr2PhysicsAddModel(gr2PhysicsModel* model)
{
	_models.insert(model);
}

void gr2PhysicsRemoveModel(gr2PhysicsModel* model)
{
	_models.erase(model);
}
*/

OBJECT* gr2PhysicsGetModelNode()
{
	OBJECT* newObject, *childObject;

	obmainCreateObjectFull (&newObject, "ragdollObject", 0, 0);

	newObject->flags = 0;
	newObject->flags |= OBJECT_VISIBLE;
	newObject->flags |= OBJECT_LITE;
	newObject->flags |= OBJECT_DYNALITE;
	newObject->flags |= OBJECT_INDEXED;
	newObject->flags |= OBJECT_TRIANGLELIST;
	newObject->flags |= OBJECT_USERDRAW;
	// copied from anim.c?
	newObject->flags |= OBJECT_COLLISION;
	newObject->flags |= OBJECT_USEMATRIX;

	newObject->flag2 |= OBJECT_ANIMATION;
	newObject->flag2 |= OBJECT_TOPMOST;
    newObject->flag3 |= OBJECT_CAST_SHADOW;

	newObject->drawcallback = gr2PhysicsDrawCallback;
	newObject->cullcallback = gr2PhysicsCullCallback;

	newObject->bbox[0].x = -3.0f;
	newObject->bbox[0].y = -3.0f;
	newObject->bbox[0].z = -3.0f;

	newObject->bbox[1].x = 3.0f;
	newObject->bbox[1].y = 3.0f;
	newObject->bbox[1].z = 3.0f;

	newObject->maxrad = 4.0f;

	// newObject->child make "lod1"

	obmainCreateObjectFull (&childObject, "lod1", 0, 0);

	childObject->flags |= OBJECT_VISIBLE;
	childObject->flags |= OBJECT_LITE;
	childObject->flags |= OBJECT_DYNALITE;
	childObject->flags |= OBJECT_INDEXED;
	childObject->flags |= OBJECT_TRIANGLELIST;
	childObject->flags |= OBJECT_USERDRAW;
	childObject->flags |= OBJECT_LOD;

	//childObject->flag2 |= OBJECT_ANIMATION;

	newObject->child = childObject;

	return newObject;
}
