#include <syscfg.h> // Path is project specific.
#include <set>
#include <map>

#include "physics/physicsTypes.h"
#include "draw/draw3d.h"
#include "pi/pi.h"
#include "math/mathEul.h"
#include "ob/obmain.h"
#include "ob/obMgr.h"
#include "trn/trnwtr.h"
#include "math/Matrixf.h"
#include "math/Matrixd.h"
#include "ml/mlutility.h"
#include "trn/trnvars.h"
#include "ob/obpool.h"
#include "math/math3d.h"
/*
#include "math/mathvect.h"
#include "math/math3d.h"
#include "trn/trnmain.h"
#include "trn/trnvars.h"
#include "physics/physicsMain.h"
#include "gr2/gr2Physics.h"
#include "fp/fpTPSPhysics.h"
#include "sdm/sdm.h"
#include "physics/odeConvert.h"
#include "ui/uiChat.h"
#include "Random.h"

#include "trn/trnwtr.h"
#include "ml/mlutility.h"
*/

static dContact _contact; // utility contact for creating new contact joints
static bool _doDebug = false;

#define DEBUG_ODE_VERTEX_SHEARING
#undef DEBUG_ODE_VERTEX_SHEARING

#ifdef DEBUG_ODE_VERTEX_SHEARING
static unsigned int _totalVsize = 0;
#endif //DEBUG_ODE_VERTEX_SHEARING

/*
static void _debugContact(Vec3d pos, Vec3d normal, UINT32 color = 0, const char* altName = NULL)
{
	// physics object line
	FLOAT64VECTOR v1, v2, temp;
	Vec3d pv1, pv2;

	normal.normalize(); // make sure
	normal *= .3f;

	pv1 = pos;
	pv2 = pos + normal;

	pv1 = physicsPhysicalToGame(pv1);
	pv2 = physicsPhysicalToGame(pv2);

	pv1.get(v1);
	v1.z = -v1.z;

	pv2.get(v2);
	v2.z = -v2.z;

	static int count = 0;

	char name[32];

	if(altName)
	{
		if(!color)
			color = (normal.z() >= 0) ? colorGREENALPHA : colorREDALPHA;

		draw3dSetWorldDebugLine(&v1, &v2, altName, color);
	}
	else
	{
		snprintf(name, 31, "contact%d", count++);
		if(count > 500) count = 0;

		if(!color)
			color = (normal.z() >= 0) ? colorGREENALPHA : colorREDALPHA;

		draw3dSetWorldDebugLine(&v1, &v2, name, color);

		// now we draw the base
		temp = v1;

		v2 = v1;

		v1.x -= .05f;
		v2.x += .05f;

		snprintf(name, 31, "contact%d", count++);
		if(count > 500) count = 0;

		draw3dSetWorldDebugLine(&v1, &v2, name, color);

		v1 = temp;
		v2 = v1;

		v1.y -= .05f;
		v2.y += .05f;

		snprintf(name, 31, "contact%d", count++);
		if(count > 500) count = 0;

		draw3dSetWorldDebugLine(&v1, &v2, name, color);
	}
}

static void _debugContact(const colliderQuery::Collision& contact, UINT32 color = 0)
{
	_debugContact(contact.pos, contact.normal, color);
}

static void _debugContact(dContact& contact, UINT32 color = 0)
{
	Vec3d pos		= Vec3Ode(contact.geom.pos);
	Vec3d normal	= Vec3Ode(contact.geom.normal);

	_debugContact(pos, normal, color);
}
*/

static void _initContact(const colliderQuery::Collision& hit, double scfm = .0008f, double serp = .2f, double bounce = .2f, double bvel = .1f)
{
	memset(&_contact,0x0,sizeof(dContact));
	OdeVec3d(_contact.geom.pos, hit.pos);
	OdeVec3d(_contact.geom.normal, hit.normal);
	_contact.geom.depth = hit.depth;
	_contact.geom.g1 = hit.g1;
	_contact.geom.g2 = hit.g2;
	_contact.surface.mode = dContactSoftERP | dContactSoftCFM | dContactBounce;
	_contact.surface.mu = 10000.0f;
	_contact.surface.soft_cfm = scfm;
	_contact.surface.soft_erp = serp;
	_contact.surface.bounce = bounce;
	_contact.surface.bounce_vel = bvel;
}

/*
static void odeCollisionCallback (void* cookie, dGeomID o1, dGeomID o2)
{
	if(dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		dSpaceCollide2 ((dGeomID)o1, (dGeomID)o2, cookie, odeCollisionCallback);
		return;
	}
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	if ((b1 && b2) || (!b1 && !b2)) return;

	physicsBody* pbody = (physicsBody*)dBodyGetData(b1 ? b1 : b2);
	if(pbody)
		pbody->Collide(b1 ? o1 : o2, b1 ? o2 : o1);
}
*/

void physicsModel::BeginStep()
{
	physicsBodySet::iterator it = bodies.begin();
	for(; it != bodies.end(); ++it) (*it)->BeginStep();
}

void physicsModel::EndStep()
{
	physicsBodySet::iterator it = bodies.begin();
	for(; it != bodies.end(); ++it) (*it)->EndStep();
}

bool physicsModel::IsActive()
{
	return active;
}

void physicsModel::Activate()
{
	active = true;
	killed = false;
	physicsAddObject(this);
}

void physicsModel::Deactivate()
{
	active = false;
	physicsRemoveObject(this);
}


physicsBodyBase::physicsBodyBase() :
	_expired(false),
	_owner(NULL)
{
	_lastFreePos[0] = _lastFreePos[1] = _lastFreePos[2] = 0.0f;
	_prevPos[0] = _prevPos[1] = _prevPos[2] = 0.0f;
	_force.x=_force.y=_force.z=0;
	_torque.x=_torque.y=_torque.z=0;
	_cm[0] = _cm[1] = _cm[2] = 0;

	_body = new dBody(*physicsGetWorld());
	_body->setFiniteRotationMode(1);
	_body->setData(this);

	_body->setAutoDisableFlag(1);
	_body->setAutoDisableLinearThreshold(.2);
	_body->setAutoDisableAngularThreshold(.2);
	//_body->setAutoDisableSteps(1);
	//_body->setAutoDisableTime(1);
}

physicsBodyBase::~physicsBodyBase()
{
	if(_body)
	{
		_body->disable();
		delete _body;
	}

	_body = NULL;
}


void physicsBodyBase::setPosition(FLOAT64VECTOR const& pos)
{
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();
	_body->setPosition(pos.x-wo.x, pos.y-wo.y, pos.z-wo.z);
	dOPE(_prevPos,=,_body->getPosition());
}

void physicsBodyBase::setLinearVel(FLOAT64VECTOR const& vel)
{
	_body->setLinearVel(vel.x, vel.y, vel.z);
}

void physicsBodyBase::setAngularVel(FLOAT64VECTOR const& vel)
{
	_body->setAngularVel(vel.x, vel.y, vel.z);
}

void physicsBodyBase::setRotation(FLOAT64QUAT const &quat)
{
	dQuaternion orient;
	orient[0] = quat.x;
	orient[1] = quat.y;
	orient[2] = quat.z;
	orient[3] = quat.w;

	_body->setQuaternion(orient);
}

FLOAT64VECTOR physicsBodyBase::getAngularVel() const
{
	FLOAT64VECTOR avel;
	const dReal* ravel = _body->getAngularVel();

	avel.x = ravel[0];
	avel.y = ravel[1];
	avel.z = ravel[2];

	return avel;
};

FLOAT64VECTOR physicsBodyBase::getLinearVel() const
{
	FLOAT64VECTOR vel;
	const dReal* rvel = _body->getLinearVel();

	vel.x = rvel[0];
	vel.y = rvel[1];
	vel.z = rvel[2];

	return vel;
}


FLOAT64VECTOR physicsBodyBase::getPosition() const
{
	FLOAT64VECTOR pos;
	const dReal* rpos = _body->getPosition();
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	pos.x = rpos[0]+wo.x;
	pos.y = rpos[1]+wo.y;
	pos.z = rpos[2]+wo.z;

	return pos;
}

void physicsBodyBase::setAttitude(FLOAT64ATT const& Att)
{
	piVECTOR piAtt;
	piAtt.x = (float)Att.Pitch;
	piAtt.y = (float)Att.Roll;
	piAtt.z = (float)-Att.Yaw;

	piMATRIX piMat;
	mathEul_ToMatrix(&piMat,EulOrdXYZr,&piAtt);

	dMatrix3 rot;
	rot[0] = piMat._11;
	rot[1] = piMat._12;
	rot[2] = piMat._13;
	rot[4] = piMat._21;
	rot[5] = piMat._22;
	rot[6] = piMat._23;
	rot[8] = piMat._31;
	rot[9] = piMat._32;
	rot[10]= piMat._33;

	_body->setRotation(rot);
}

FLOAT64ATT physicsBodyBase::getAttitude() const
{
// try to convert to our inexplicable brain dead co-ordinate system
	const dReal* rot = _body->getRotation();

	piMATRIX piMat;
	piMat._11 = (float)rot[0];
	piMat._12 = (float)rot[1];
	piMat._13 = (float)rot[2];
	piMat._21 = (float)rot[4];
	piMat._22 = (float)rot[5];
	piMat._23 = (float)rot[6];
	piMat._31 = (float)rot[8];
	piMat._32 = (float)rot[9];
	piMat._33 = (float)rot[10];

	piVECTOR piAtt;
	mathEul_FromMatrix(&piAtt, &piMat,EulOrdXYZr);

	// usual stupid sign flips
	FLOAT64ATT Att;
	Att.Pitch = piAtt.x;
	Att.Roll =  piAtt.y;
	Att.Yaw = -piAtt.z;

	return Att;
}


FLOAT64VECTOR physicsBodyBase::getModelPosition() const
{
	dVector3 cm;
	_body->vectorToWorld(_cm[0], _cm[1], _cm[2], cm);
	const dReal* rpos = _body->getPosition();
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	FLOAT64VECTOR pos;
	pos.x = rpos[0]-cm[0]+wo.x;
	pos.y = rpos[1]-cm[1]+wo.y;
	pos.z = rpos[2]-cm[2]+wo.z;

	return pos;
}

void physicsBodyBase::addForce(const FLOAT64VECTOR& F)
{
	_force.x += F.x;
	_force.y += F.y;
	_force.z += F.z;
}

void physicsBodyBase::addForce(double x, double y, double z)
{
	_force.x += x;
	_force.y += y;
	_force.z += z;
}

void physicsBodyBase::addTorque(const FLOAT64VECTOR& T)
{
	_torque.x += T.x;
	_torque.y += T.y;
	_torque.z += T.z;
}

void physicsBodyBase::addTorque(double x, double y, double z)
{
	_torque.x += x;
	_torque.y += y;
	_torque.z += z;
}

void physicsBodyBase::applyForces()
{
	if(_force.x || _force.y || _force.z)
		bool stop = true;

	_body->addForce(_force.x,_force.y,_force.z);
	_body->addTorque(_torque.x,_torque.y,_torque.z);

	_force.x=_force.y=_force.z=0;
	_torque.x=_torque.y=_torque.z=0;
}

void physicsBodyBase::setModelPosition(FLOAT64VECTOR const& pos)
{
	dVector3 cm;
	_body->vectorToWorld(_cm[0], _cm[1], _cm[2], cm);
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	_body->setPosition(pos.x+cm[0]-wo.x, pos.y+cm[1]-wo.y, pos.z+cm[2]-wo.z);
	dOPE(_prevPos,=,_body->getPosition());
	dOPE(_lastFreePos,=,_body->getPosition());
}

void physicsBodyBase::SetPosition(Vec3d const& p, bool game)
{
	Vec3d pos = p;
	dVector3 cm;

	_body->vectorToWorld(_cm[0], _cm[1], _cm[2], cm);

	pos.x() += cm[0];
	pos.y() += cm[1];
	pos.z() += cm[2];

	if(game)
	{
		const FLOAT64VECTOR& wo = physicsGetWorldOffset();
		pos.x() -= wo.x;
		pos.y() -= wo.y;
		pos.z() -= wo.z;
	}

	_body->setPosition(pos.x(), pos.y(), pos.z());
	dOPE(_prevPos,=,_body->getPosition());
}

void physicsBodyBase::SetRotation(Matrixd const& mat)
{
	dMatrix3 dmat;
	MatrixTodMatrix(mat, dmat);
	_body->setRotation(dmat);
}

void physicsBodyBase::SetRotation(Quat const& q)
{
	dQuaternion dq;
	OdeQuat(dq, q);
	_body->setQuaternion(dq);
}

void physicsBodyBase::SetLinearVel(Vec3d const& v)
{
	_body->setLinearVel(v.x(),v.y(),v.z());
}

void physicsBodyBase::SetAngularVel(Vec3d const& v)
{
	_body->setAngularVel(v.x(),v.y(),v.z());
}

void physicsBodyBase::SetTransform(Matrixd const& mat)
{
	SetPosition(mat.getTrans());
	SetRotation(mat);
}

Vec3d& physicsBodyBase::GetPosition(bool game) const
{
	static Vec3d position;
	dVector3 cm;

	_body->vectorToWorld(_cm[0], _cm[1], _cm[2],cm);

	position = Vec3Ode(_body->getPosition());

	position.x() -= cm[0];
	position.y() -= cm[1];
	position.z() -= cm[2];

	if(game)
	{
		const FLOAT64VECTOR& wo = physicsGetWorldOffset();
		position.x() += wo.x;
		position.y() +=	 wo.y;
		position.z() += wo.z;
	}

	return position;
}

Matrixf physicsBodyBase::GetRotationMatrix() const
{
	Matrixf mat;
	return dMatrixToMatrix(_body->getRotation(), mat);
}

Quat physicsBodyBase::GetRotation() const
{
	return QuatOde(_body->getQuaternion());
}

Vec3d physicsBodyBase::GetLinearVel() const
{
	return Vec3Ode(_body->getLinearVel());
}

Vec3d physicsBodyBase::GetAngularVel() const
{
	return Vec3Ode(_body->getAngularVel());
}

Matrixd physicsBodyBase::GetTransform(bool game) const
{
	return Matrixd::rotate(GetRotation())*Matrixd::translate(GetPosition(game));
}

dWorldID physicsBodyBase::DefaultWorld()
{
	return physicsGetWorld()->id();
}

bool physicsBody::handleContact(dContact* contact)
{
	/*
	contact->surface.mode = dContactSoftERP | dContactSoftCFM | dContactBounce;// | dContactApprox1;
	contact->surface.mu = dInfinity;
	contact->surface.soft_cfm = _cfm;
	contact->surface.soft_erp = _erp;
	contact->surface.bounce = getRestitution();
	contact->surface.bounce_vel = 0.1;

	dJointID c = dJointCreateContact (*_odeWorld, *_odeContactGroup, contact);
	dJointAttach (c, _body->id(), 0);
	*/

	return true;
}

void physicsBody::Collide(dGeomID bodyGeom, dGeomID geom)
{
	if(!_body->isEnabled()) return;

	const int MAX_CONTACTS = 8;
	dContact contacts[MAX_CONTACTS];

	int numc = dCollide (bodyGeom, geom, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));
	for (int i = 0; i < numc; i++)
	{
		dContact* contact = &contacts[i];
		if(contact->geom.depth < .000001) continue; // depth 0 from various trimesh colliders causes "sticks"

		// throw out any normals that are "on the wrong side" to prevent objects from "sticking" or penetrating
		// NOTE: this code is not generalized for all cases, it only works if center of object is inside geometry
		// so we don't do this for trimesh and assume the center is inside for simple capsule used by bouncing grenades
		if(_enhancedCollisionCheck)
		{
			dVector3 vec;
			dOPC(vec,*,contact->geom.normal,contact->geom.depth);
			dOP(vec,+,contact->geom.pos,vec);
			dOP(vec,-,vec, _lastFreePos);
			if(dDOT(vec, contact->geom.normal) > 0) continue;
		}

/*printf("(%lf %lf %lf) (%lf %lf %lf) %lf %x %x\n"
	,contact->geom.normal[0],contact->geom.normal[1],contact->geom.normal[2]
	,contact->geom.pos[0], contact->geom.pos[1],contact->geom.pos[2]
	,contact->geom.depth, (int)contact->geom.g1, (int)contact->geom.g2);*/
		if(handleContact(contact))
			_contacts.push_back(*contact);
	}

	if(numc > 0)
	{
		_collided += numc;
		double Cf = .01;// arbitrary
		FLOAT64VECTOR T = getAngularVel();
		mathScaleVector(&T, &T, -Cf);
		addTorque(T);
	}
}

// grr damn ODE's first order integrator
// really sucks with rotations
void physicsBody::damp()
{
	FLOAT64VECTOR A = getAngularVel();
	FLOAT64VECTOR L = getLinearVel();

	// cap angular velocity
	double aspd = mathExactVectorLength(&A);

	if(aspd > 10*PI)
	{
		mathScaleVector(&A, &A, (10*PI / aspd));
		setAngularVel(A);
	}

	static FLOAT32 minSpeed = 1.0f;

	// if we're in contact with something, damp angular velocity
	// flatter contacts mean more dampening
	// ranges are 0 to 30 degrees and 0 to .1f angular damp
	if(_contactsNormal.length2() && mathExactVectorLength2(&L) >= (minSpeed * minSpeed))
	{
		Vec3d up = Vec3d(0, 0, 1.0f);

		double cosTheta = _contactsNormal * up;

		// cap it
		cosTheta = (cosTheta < -1.0f) ? -1.0f : (cosTheta > 1.0f) ? 1.0f : cosTheta;

		double angle = mlRAD_TO_DEG(acos(cosTheta));
		double threshold = 25.0f;

		if(angle <= threshold)
		{
			double force = 1.0f - (angle / threshold);

			if(force > 0 && force <= 1.0f)
			{
				double k = .025f * force * _collided;

				A.x = -k * A.x;
				A.y = -k * A.y;
				A.z = -k * A.z;

				addTorque(A);
			}
		}
	}

	// add air resistance (arbitrary) and drag from current collision count
	double k = (_collided) ? _collided * .3f : .1f;

	L.x = -k * L.x;
	L.y = -k * L.y;
	L.z = -k * L.z;

	addForce(L);
}

dMass const* physicsBody::getMass() const
{
	return &_mass;
}

dGeomID physicsBody::preCheck()
{
	if(!_velocity.length2())
		return NULL;

	Vec3d pos = GetPosition(false);
	Vec3d dir = _velocity;

	dir.normalize();

	const colliderQuery::CollisionList& hits = collideRayList(COLLIDER_MOVE_MASK, .3f, pos, dir, true);

	if(hits.empty())
		return NULL;

	const colliderQuery::Collision& hit = hits.at(0);

	if(hits.size() > 1)
		bool stop = true;

	_initContact(hit);

	if(handleContact(&_contact))
	{
		if(_doDebug)
			colliderDebugContact(hit, colorORANGEALPHA, "fhit");

		_contacts.push_back(_contact);

		if(!collideJoints)
			collideJoints = new dJointGroup;

		dJointID c = dJointCreateContact(DefaultWorld(), *collideJoints, &_contact);
		dJointAttach (c, _body->id(), 0);

		return hit.g2;
	}

	return NULL;
}

dGeomID physicsBody::rayCheck()
{
	Vec3d start		= Vec3Ode(_lastFreePos);
	Vec3d dir		= Vec3Ode(_body->getPosition());

	dir = dir - start;

	if(dir.length2() <= 0.0f)
		return NULL;

	double len = dir.length();

	dir.normalize();

	const colliderQuery::CollisionList& hits = collideRayList(COLLIDER_MOVE_MASK, len, start, dir, true);

	if(hits.empty())
		return NULL;

	const colliderQuery::Collision& hit = hits.at(0);

	if(hits.size() > 1)
		bool stop = true;

	_initContact(hit);

	if(handleContact(&_contact))
	{
		_contactsNormal += hit.normal;

		if(true)
		{
			dir = -dir;

			Matrixd rotate;
			rotate.makeRotate(mlDEG_TO_RAD(180.0f), hit.normal);

			dir = dir * rotate;
			dir *= (_speed * .2f);

			SetLinearVel(dir);
		}

		const dReal* pos = _body->getPosition();

		_contact.geom.pos[0] = pos[0];
		_contact.geom.pos[1] = pos[1];
		_contact.geom.pos[2] = pos[2];

		//uiChatPrintLine("Ammo Ray Check Failed: %.2f, %.2f, %.2f -> %.2f, %.2f, %.2f",
		//	pos[0], pos[1], pos[2], 	_lastFreePos[0],_lastFreePos[1],_lastFreePos[2]);

		// we have a ray collision which probably means we just went through something
		// so we'll force it back and attach a joint
		_body->setPosition(_lastFreePos[0],_lastFreePos[1],_lastFreePos[2]);
		//_body->setPosition(_contact.geom.pos[0],_contact.geom.pos[1],_contact.geom.pos[2]);


		if(_doDebug)
			colliderDebugContact(hit, colorBLUEALPHA);

		_contacts.push_back(_contact);

		if(!collideJoints)
			collideJoints = new dJointGroup;

		dJointID c = dJointCreateContact(DefaultWorld(), *collideJoints, &_contact);
		dJointAttach (c, _body->id(), 0);

		++_collided;

		_allHits |= dGeomGetCategoryBits(hit.g2);

		return hit.g2;
	}

	return NULL;
}

/* old method
void physicsBody::rayCheck2()
{
// do a ray collision to make sure we didn't go through
// would be more efficient to do this in one go with all bodies, but we shouldn't have many at once
	dVector3 curPos, dir;
	dOPE(curPos,=,_body->getPosition());
	dOP(dir,-,curPos,_prevPos);

    // No direction results in an assert.
    if(Vec3d(dir[0], dir[1], dir[2]).length2() <= 0.0f)
    {
        return;
    }

	dGeomID ray = dCreateRay(colliderDynamicSpace(),dLENGTH(dir));
	dGeomRaySet (ray,_prevPos[0],_prevPos[1],_prevPos[2],dir[0],dir[1],dir[2]);
	colliderSetBits(ray, 0, COLLIDER_MOVE_MASK);

	const int MAX_CONTACTS = 16;
	dContact contacts[MAX_CONTACTS];

	dContact* hit=0;
	double minlen2=100000;
	int numc = dCollide (ray, (dGeomID)colliderTerrainSpace(), MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));
	numc += dCollide (ray, (dGeomID)colliderVehicleSpace(), MAX_CONTACTS-numc, &contacts[numc].geom, sizeof(dContact));

	for (int i = 0; i < numc; i++)
	{ // find the closest hit
		dContact* contact = &contacts[i];
		dVector3 delta;
		dOP(delta,-,contact->geom.pos,_prevPos);
		double len2 = dLENGTHSQUARED(delta);
		if(len2 < minlen2)
		{
			if(dDOT(delta, contact->geom.normal) > 0)
			{ // weird edge case flip the normal so it's in the "right" direction
				contact->geom.normal[0] = -contact->geom.normal[0];
				contact->geom.normal[1] = -contact->geom.normal[1];
				contact->geom.normal[2] = -contact->geom.normal[2];
			}

			minlen2 = len2;
			hit = contact;
		}
	}

	if(hit && handleContact(hit))
	{
	// we have a ray collision which probably means we just went through something
	// so we'll force it back and attach a joint
		_body->setPosition(_prevPos[0],_prevPos[1],_prevPos[2]);

		hit->surface.mode = dContactSoftERP | dContactSoftCFM | dContactBounce;// | dContactApprox1;
		hit->surface.mu = dInfinity;
		hit->surface.soft_cfm = .0008f;
		hit->surface.soft_erp = .2f;
		hit->surface.bounce = .25f; //getRestitution();
		hit->surface.bounce_vel = 0.1;

		dJointID c = dJointCreateContact (*_odeWorld, *_odeContactGroup, hit);
		dJointAttach (c, _body->id(), 0);
		_collided = true;
	}

	dGeomDestroy(ray);
}
*/

void physicsBody::BeginStep()
{
	_contacts.clear();
	_contactsNormal.set(0, 0, 0);

	if(!IsEnabled())
		return;

	_velocity = GetLinearVel();

	//if(frametime >= _nextUpdate)
	_speed = _velocity.length();

	if(_enhancedCollisionCheck)
	{
		// update our normal collider as needed
		if(collider)
		{
			if(!_thisContact || _thisContact != _lastContact)
			{
				Vec3d dir = -_velocity;
				dir.normalize();

				collider->setNormalFilter(dir.x(), dir.y(), dir.z());

				// allow subclasses the ability to override the filter as needed
				AdjustNormalFilter();
			}
		}

		_lastContact = _thisContact;
	}

	// reset this each frame so we can compare the next frame's _lastContact to it
	_thisContact = NULL;

	// clear out old contact joints
	if(collideJoints)
		collideJoints->empty();

	//if(_enhancedCollisionCheck)
	//	preCheck();

	// now find our collisions
	if(collider && collider->findCollisions())
	{
		colliderQuery::CollisionList& collisions = collider->getCollisions();
		colliderQuery::CollisionList::iterator it = collisions.begin();

		for(; it != collisions.end(); ++it)
		{
			colliderQuery::Collision& contact = *it;

			_initContact(contact);

			if(handleContact(&_contact))
			{
				if(_doDebug)
					colliderDebugContact(contact);

				_contacts.push_back(_contact);
				_contactsNormal += contact.normal;

				if(!collideJoints)
					collideJoints = new dJointGroup;

				dJointID c = dJointCreateContact(DefaultWorld(), *collideJoints, &_contact);
				dJointAttach (c, _body->id(), 0);

				++_collided;

				if(!_thisContact)
					_thisContact = _contact.geom.g2;

				//_allHits |= dGeomGetCategoryBits(_contact.geom.g2);
			}
			else if(_doDebug)
			{
				colliderDebugContact(contact, colorREDALPHA);
			}
		}
	}

	if(_enhancedCollisionCheck && !_collided && !_allHits)
		_thisContact = rayCheck();

	if(!_collided)
	{ // make sure these braces stay here because of the macro
	 	dOPE(_lastFreePos,=,_body->getPosition());
	}

	if(_doDebug)
	{
		Vec3d pos = physicsGameToPhysical(GetPosition());
		colliderDebugContact(pos, _velocity, colorYELLOWALPHA, "bodypos");
	}

	applyForces();

	dOPE(_prevPos,=,_body->getPosition());

	//_collided = 0; // now reset in EndStep()
	_nextUpdate = frametime + 100;

}

void physicsBody::EndStep()
{
	if(!IsEnabled())
		return;

	if(collider)
		collider->setDirty(true);

	damp();

	_collided = 0;
}

/*
void physicsBody::BeginStep()
{
	if(!IsEnabled())
		return;

	if(!_collided && _enhancedCollisionCheck)
		rayCheck();

	if(!_collided)
	{ // make sure these braces stay here because of the macro
	 	dOPE(_lastFreePos,=,_body->getPosition());
	}

	_collided = false;

	applyForces();

	dOPE(_prevPos,=,_body->getPosition());
}

void physicsBody::EndStep()
{
	if(!IsEnabled())
		return;

	damp();
}
*/

int physicsBody::InitCollisions()
{
	if(!_body->isEnabled())
		return 0;

	_contacts.clear();

	return 1;
}

void physicsBody::setCollisionContacts(int c)
{
	collider->setMaxContactsPerHit(c);
}

float physicsBody::getRestitution() const
{
	return _restitution;
}

void physicsBody::setRestitution(float r)
{
	_restitution = r;
}

physicsBody::physicsBody() : physicsBodyBase(),
	_restitution(.5),
	_enhancedCollisionCheck(true),
	_collided(0),
	_lastContact(NULL),
	_thisContact(NULL),
	_nextUpdate(0),
	_speed(0),
	_allHits(0),
	collideJoints(NULL)
{
	// create our model's collider
	collider = new colliderQuery;
	collider->setBits(COLLIDER_ALL, COLLIDER_ALL);

	// let each body manage its geom
	dSpaceSetCleanup(collider->space(), 0);
	dBodySetMaxAngularSpeed (_body->id(), 25.0f);

	_geom = new physicsObjectCollider(collider->space(), COLLIDER_ALL, COLLIDER_ALL);
}

physicsBody::physicsBody(UINT32 cat, UINT32 col) : physicsBodyBase(),
	_restitution(.5),
	_enhancedCollisionCheck(true),
	_collided(0),
	_lastContact(NULL),
	_thisContact(NULL),
	_nextUpdate(0),
	_speed(0),
	_allHits(0),
	collideJoints(NULL)
{
	// create our model's collider
	collider = new colliderQuery;

	// limit the number of interactions
	collider->setMaxContactsPerHit(3);
	collider->setBits(cat, col);

	// let each body manage its geom
	dSpaceSetCleanup(collider->space(), 0);
	dBodySetMaxAngularSpeed (_body->id(), 25.0f);

	_geom = new physicsObjectCollider(collider->space(), cat, col);
}

physicsBody::physicsBody(dSpaceID s, UINT32 cat, UINT32 col) : physicsBodyBase(),
	_restitution(.5),
	_enhancedCollisionCheck(true),
	_collided(0),
	_lastContact(NULL),
	_thisContact(NULL),
	_nextUpdate(0),
	_speed(0),
	_allHits(0),
	collideJoints(NULL)
{
	// create our model's collider
	collider = new colliderQuery(s);

	// limit the number of interactions
	collider->setMaxContactsPerHit(3);
	collider->setBits(cat, col);

	// let each body manage its geom
	dSpaceSetCleanup(collider->space(), 0);
	dBodySetMaxAngularSpeed (_body->id(), 25.0f);

	_geom = new physicsObjectCollider(s, cat, col);
}

physicsBody::~physicsBody()
{
	if(collideJoints)
	{
		collideJoints->empty();
		delete collideJoints;
	}

	if(collider)
		delete collider;

	if(_geom)
		delete _geom;
}


void physicsBody::makeMeshColliderFromObject(OBJECT* Object, FLOAT64VECTOR* offset)
{
	_enhancedCollisionCheck = false; // that code isn't generalized for this

	recurseAndMakeMeshCollider(Object);
	_geom->attachToBody(_body);


	// 180 around the z
	//Matrixf zAxis = Matrixf::rotate(PI, 0, 0, 1);
	//dMatrix3 rotate;

	//MatrixTodMatrix(zAxis, rotate);
	//dMatrix3 invert={1,0,0,0, 0,1,0,0, 0,0,-1,0};
	//_geom->offsetRotation(invert);

	if(offset)
		_geom->offsetPosition(offset->x, offset->y, offset->z);
}

void physicsBody::recurseAndMakeMeshCollider(OBJECT* Object, bool siblings)
{
	if(!(Object->flags & OBJECT_NOTOBJECT))
	{
		int index = _geom->create(Object);

		if(collider && index >= 0)
			collider->addGeom(geom(index), _geom->getCategoryMask(), _geom->getCollideMask());
	}

	if(Object->attached)
		recurseAndMakeMeshCollider(Object->attached, false);

	if(Object->child)
		recurseAndMakeMeshCollider(Object->child, true);

	if(siblings && Object->sibling)
		recurseAndMakeMeshCollider(Object->sibling, true);
}

static void calcObjectBBox(OBJECT* object, piVECTOR& minBox, piVECTOR& maxBox)
{
	minBox.x = minBox.y = minBox.z = 10000;
	maxBox.x = maxBox.y = maxBox.z =-10000;

	for(; object; object=object->attached)
	{
		piVECTOR bbox0,bbox1;
		mathAddVectors(&bbox0,&object->bbox[0],&object->pos);
		mathAddVectors(&bbox1,&object->bbox[1],&object->pos);

		if(bbox0.x < minBox.x) minBox.x = bbox0.x;
		if(bbox0.y < minBox.y) minBox.y = bbox0.y;
		if(bbox0.z < minBox.z) minBox.z = bbox0.z;

		if(bbox1.x > maxBox.x) maxBox.x = bbox1.x;
		if(bbox1.y > maxBox.y) maxBox.y = bbox1.y;
		if(bbox1.z > maxBox.z) maxBox.z = bbox1.z;
	}
}

void physicsBody::makeBoxColliderFromObject(OBJECT* object, FLOAT32 scaler)
{
	piVECTOR minBox, maxBox;
	calcObjectBBox(object, minBox, maxBox);

	if(scaler != 1.0f)
	{
		mathScaleVector(&minBox, &minBox, scaler);
		mathScaleVector(&maxBox, &maxBox, scaler);
	}

	float xlen = (maxBox.x-minBox.x);
	float ylen = (maxBox.y-minBox.y);
	float zlen = (maxBox.z-minBox.z);

	float xc = (maxBox.x+minBox.x)/2.0f;
	float yc = (maxBox.y+minBox.y)/2.0f;
	float zc = -(maxBox.z+minBox.z)/2.0f;

	int index = _geom->createBox(xlen,ylen,zlen);

	_geom->attachToBody(_body);
	_geom->offsetPosition((xc-_cm[0]),(yc-_cm[1]), (zc-_cm[2]));

	if(collider)
		collider->addGeom(geom(index), _geom->getCategoryMask(), _geom->getCollideMask());
}

void physicsBody::makeCapsuleColliderFromObject(OBJECT* object, FLOAT32 scaler)
{
	piVECTOR minBox, maxBox;
	calcObjectBBox(object, minBox, maxBox);

	if(scaler != 1.0f)
	{
		mathScaleVector(&minBox, &minBox, scaler);
		mathScaleVector(&maxBox, &maxBox, scaler);
	}

	float xlen = (maxBox.x-minBox.x);
	float ylen = (maxBox.y-minBox.y);
	float zlen = (maxBox.z-minBox.z);

	float xc = (maxBox.x+minBox.x)/2.0f;
	float yc = (maxBox.y+minBox.y)/2.0f;
	float zc = -(maxBox.z+minBox.z)/2.0f;

	float radius = (xlen + ylen) / 4.0f;
	float length = zlen - 2*radius;

	if(length < 0.1f) length = .01f;

	int index = _geom->createCapsule(radius, length);

	_geom->attachToBody(_body);
	_geom->offsetPosition((xc-_cm[0]),(yc-_cm[1]), (zc-_cm[2]));

	if(collider)
		collider->addGeom(geom(index), _geom->getCategoryMask(), _geom->getCollideMask());
}

// convenience function for single geom bodies
dGeomID physicsBody::geom(int index)
{
	return (_geom) ? _geom->id(index) : NULL;
}

// update OBJECT matrix fromphysics object
void physicsBody::TransformToOBJECT(OBJECT* o)
{
	piMATRIX& out = o->mat;

	const dReal* rotation = dBodyGetRotation(_body->id());

	out._11 = (float)rotation[0];
	out._21 = (float)rotation[1];
	out._31 = (float)rotation[2];
	out._12 = (float)rotation[4];
	out._22 = (float)rotation[5];
	out._32 = (float)rotation[6];
    out._13 = (float)-rotation[8];
	out._23 = (float)-rotation[9];
	out._33 = (float)-rotation[10];

	const dReal* position = dBodyGetPosition(_body->id());
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	out._41 = (float)((position[0]+wo.x) - WorldOriginOffset.x);
	out._42 = (float)((position[1]+wo.y) - WorldOriginOffset.y);
	out._43 = (float)(-((position[2]+wo.z) - WorldOriginOffset.z));
}

// update physics object from OBJECT matrix
void physicsBody::TransformFromOBJECT(OBJECT* o)
{
	piMATRIX& in = o->mat;
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	dBodySetPosition(_body->id()
	,  WorldOriginOffset.x+in._41-wo.x
	,  WorldOriginOffset.y+in._42-wo.y
	,-(WorldOriginOffset.z+in._43-wo.z) );

	dMatrix3 mat;
	mat[0] = in._11;
	mat[1] = in._21;
	mat[2] = in._31;
    mat[3] = 0;
	mat[4] = in._12;
	mat[5] = in._22;
	mat[6] = in._32;
    mat[7] = 0;
	mat[8] = -in._13;
	mat[9] = -in._23;
	mat[10]= -in._33;
    mat[11] = 0;

	dBodySetRotation(_body->id(), mat);
}


void physicsBody::setMassFromMesh(double density, FLOAT64VECTOR const& cm)
{
	dMassSetZero(&_mass);

	dMass triMass;
	int mcount = getCollider()->geoms();

	for(int i = 0; i < mcount; i++)
	{
		dMassSetZero(&triMass);
		dMassSetTrimesh(&triMass, density, geom(i));
		dMassTranslate(&triMass, -triMass.c[0], -triMass.c[1], -triMass.c[2]);
		dMassAdd(&_mass, &triMass);
	}

	_cm[0] = cm.x;
	_cm[1] = cm.y;
	_cm[2] = cm.z;

	if(fabs(_cm[0]) < .001f) _cm[0] = 0;
	if(fabs(_cm[1]) < .001f) _cm[1] = 0;
	if(fabs(_cm[2]) < .001f) _cm[2] = 0;

	_body->setMass(&_mass);
}

void physicsBody::setMass (double m, FLOAT64VECTOR const& cm
					,  double Ixx, double Iyy, double Izz
					,  double Ixy, double Ixz, double Iyz
					, OBJECT* object )
{
	if(Ixx == 0 && Iyy == 0 && Izz == 0)
	{
		if(object)
		{ // simple shapes for now, we get better collider results than if we used actual object geometry

			piVECTOR minBox, maxBox;
			calcObjectBBox(object, minBox, maxBox);

			float xlen = (maxBox.x-minBox.x);
			float ylen = (maxBox.y-minBox.y);
			float zlen = (maxBox.z-minBox.z);

			float xc = (maxBox.x+minBox.x)/2.0f;
			float yc = (maxBox.y+minBox.y)/2.0f;
			float zc = -(maxBox.z+minBox.z)/2.0f;

			dMassSetBoxTotal(&_mass, m, xlen, ylen, zlen);

			_cm[0] = xc;
			_cm[1] = yc;
			_cm[2] = zc;

		}
		else
		{ // some reasonable default
			dMassSetSphereTotal(&_mass, m, 1);
			_cm[0] = _cm[1] = _cm[2] = 0;
		}
	}
	else
	{
		dMassSetParameters (&_mass, 1,0,0,0, Ixx, Iyy, Izz, Ixy, Ixz, Iyz);
		dMassAdjust(&_mass, m);

		_cm[0] = cm.x;
		_cm[1] = cm.y;
		_cm[2] = cm.z;

		if(!dMassCheck(&_mass))
			dMassSetSphereTotal(&_mass, m, 1);
	}

	if(fabs(_cm[0]) < .001f) _cm[0] = 0;
	if(fabs(_cm[1]) < .001f) _cm[1] = 0;
	if(fabs(_cm[2]) < .001f) _cm[2] = 0;

	_body->setMass(&_mass);
}


void physicsCollider::attachToBody(dBody* body)
{
	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
		dGeomSetBody (it->geom, *body);
}

void physicsCollider::offsetPosition(double x,double y, double z)
{
	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
		dGeomSetOffsetPosition(it->geom, x,y,z);
}

void physicsCollider::offsetRotation(const dMatrix3 mat)
{
	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
		dGeomSetOffsetRotation(it->geom, mat);
}

int physicsCollider::createSphere(float radius)
{
	dGeomID geom = dCreateSphere (_space, radius);
	colliderSetBits(geom, _categoryMask, _collideMask);
	_geom.push_back(geomMesh(geom,0));
	return _geom.size() - 1;
}

int physicsCollider::createCapsule(float radius, float length)
{
	dGeomID geom = dCreateCapsule (_space, radius, length);
	colliderSetBits(geom, _categoryMask, _collideMask);
	_geom.push_back(geomMesh(geom,0));
	return _geom.size() - 1;
}

int physicsCollider::createCylinder(float radius, float length)
{
	dGeomID geom = dCreateCylinder(_space,radius,length);
	colliderSetBits(geom, _categoryMask, _collideMask);
	_geom.push_back(geomMesh(geom,0));
	return _geom.size() - 1;
}

int physicsCollider::addGeom(dGeomID geom)
{
	colliderSetBits(geom, _categoryMask, _collideMask);
	_geom.push_back(geomMesh(geom,0));
	return _geom.size() - 1;
}

int physicsCollider::createBox(float lx, float ly, float lz)
{
	dGeomID geom = dCreateBox(_space, lx,ly,lz);
	colliderSetBits(geom, _categoryMask, _collideMask);
	_geom.push_back(geomMesh(geom,0));
	return _geom.size() - 1;
}

void physicsCollider::updateTransform(piMATRIX const& m)
{
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
	{
		dGeomID geom = it->geom;

		dGeomSetPosition(geom
			,  WorldOriginOffset.x+m._41-wo.x
			,  WorldOriginOffset.y+m._42-wo.y
			,-(WorldOriginOffset.z+m._43-wo.z) );

		dMatrix3 mat;
		mat[0] = m._11;
		mat[1] = m._21;
		mat[2] = m._31;
        mat[3] = 0;
		mat[4] = m._12;
		mat[5] = m._22;
		mat[6] = m._32;
        mat[7] = 0;
		mat[8] = -m._13;
		mat[9] = -m._23;
		mat[10]= -m._33;
	    mat[11] = 0;

		dGeomSetRotation(geom, mat);
	}
}

void physicsCollider::updatePosition(float x, float y, float z)
{
	const FLOAT64VECTOR& wo = physicsGetWorldOffset();

	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
	{
		dGeomID geom = it->geom;

		dGeomSetPosition(geom
			,  WorldOriginOffset.x+x-wo.x
			,  WorldOriginOffset.y+y-wo.y
			,-(WorldOriginOffset.z+z-wo.z) );
	}
}

void physicsCollider::setEnabled(bool e)
{
	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
	{
		dGeomID geom = it->geom;

		if(e)
			dGeomEnable(geom);
		else
			dGeomDisable(geom);
	}
}

bool physicsCollider::isCollidable(UINT32 cflags)
{
	return (!cflags || (_categoryMask & cflags));
}

int physicsCollider::createMesh(const void* Vertices, int VStride, int VCount, const void* Indices, int IStride, int ICount, void* userdata, piMATRIX* transform)
{
	if(!ICount)
		return -1;

	// create our trimesh
	dTriMeshDataID mesh = dGeomTriMeshDataCreate();

	// build it using our passed in data
	dGeomTriMeshDataBuildSingle(mesh, Vertices, VStride, VCount, Indices, ICount, IStride);

	dGeomID meshGeom = dCreateTriMesh(transform ? 0 : _space, mesh, 0, 0, 0);
	dGeomID geom = meshGeom;

	dGeomSetData(meshGeom, userdata);

	if(transform)
	{
		geom = dCreateGeomTransform(_space);

		dGeomSetData(geom, userdata);
		dGeomTransformSetGeom(geom, meshGeom);
		dGeomTransformSetCleanup(geom, 1);
		dGeomTransformSetInfo(geom, 1); // collisions return the transform geom

		_geom.push_back(geomMesh(geom, mesh));

		// update our transform
		updateTransform(*transform);
	}
	else
	{
		_geom.push_back(geomMesh(meshGeom,mesh));
	}

	// set up bits
	colliderSetBits(geom, _categoryMask, _collideMask);

	return _geom.size() - 1;
}

void physicsCollider::setCollideMask(unsigned int mask)
{
	_collideMask = mask;

	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
		if(it->geom) dGeomSetCollideBits(it->geom, _collideMask);
}

void physicsCollider::setCategoryMask(unsigned int mask)
{
	_categoryMask = mask;

	for(std::vector<geomMesh>::iterator it = _geom.begin(); it != _geom.end(); ++it)
		if(it->geom) dGeomSetCategoryBits(it->geom, _categoryMask);
}

UINT32 physicsCollider::DetermineCategory(UINT32 cat, const char* path, const char* comments, int texid)
{
	// save it
	UINT32 save = (cat & (COLLIDER_WALK|COLLIDER_TILE|COLLIDER_ENTERABLE|COLLIDER_STO));

	if(cat & COLLIDER_TRN_GROUND)
	{
		if(trnwtrIsWaterTexture(texid))
		{
			cat = COLLIDER_TRN_WATER;
		}
		else if(cat & COLLIDER_ENTERABLE)
		{
			cat = COLLIDER_TRN_BUILDING;
		}
		else if(path)
		{
			if(strstr(path, "\\bld\\"))
			{
				cat = COLLIDER_TRN_BUILDING;
			}
			else if(strstr(path, "\\brdg\\"))
			{
				cat = COLLIDER_TRN_BRIDGE;
			}
			else if(strstr(path, "\\ag\\") ||
					 strstr(path, "\\cap\\") ||
					 strstr(path, "\\cp\\") ||
					 strstr(path, "\\fac\\") ||
					 strstr(path, "\\link\\") ||
					 strstr(path, "\\spwn\\"))
			{
				cat = COLLIDER_TRN_CULTURAL;
			}
		}
	}

	if(comments)
	{
		const char* ptr = strstr(comments, "ode=");

		if(ptr)
		{
			// move past the commentor
			ptr += 4;

			// comments are from -1 to COLLIDER_COUNT - 1; -1 means no collider
			int collider = atoi(ptr);

			// set the mask
			if(collider == 0)
				cat |= (1<<collider); // noragdoll flag; just add it
			else if(collider > 0 && collider < COLLIDER_COUNT)
				cat = (1<<collider); // set it
			else
				cat = 0; // clear it
		}

		if(strstr(comments, "capture"))
			cat |= COLLIDER_CAPTURE|COLLIDER_TRN_BUILDING; // ensure these are buildings

		if(strstr(comments, "nofire"))
			cat |= COLLIDER_NOFIRE|COLLIDER_TRN_BUILDING; // ensure these are buildings
	}

	// re-add saved flags
	cat |= save;

	return cat;
}

UINT32 physicsCollider::DetermineCategory(UINT32 cat, OBJECT* o)
{
	if(!o) return cat;

	if(o->flag3 & OBJECT_STO)
		cat |= COLLIDER_STO;

	ModelInstance* mInstance = o->modelInstance;
	const char* path = NULL;

	if(mInstance)
	{
		if(mInstance->GetModel()->IsReplacementTile())
			cat |= COLLIDER_TILE;

		if(mInstance->GetModel()->IsEnterable())
			cat |= COLLIDER_ENTERABLE;

		path = mInstance->GetModelDirectory();
	}

	return physicsCollider::DetermineCategory(cat, path, o->comment, o->texid);
}

int physicsNodeCollider::create(PoolNode* n, piMATRIX* transform)
{
	// we assume it's a PoolNode*
	if(n && n->objdata->numtris)
	{
		return createMesh(n->vertices,
								 sizeof(piUNIFIED_VERTEX),
								 n->objdata->numverts,
								 n->indices,
								 3*sizeof(short),
								 3*n->objdata->numtris,
								 n);
	}

	return -1;
}

physicsObjectCollider::physicsObjectCollider(dSpaceID space, UINT32 cat, UINT32 col) :
	physicsCollider(cat)
{
	_space = space;
	_collideMask = col;
}

physicsObjectCollider::~physicsObjectCollider()
{
}

int physicsObjectCollider::create(OBJECT* o, piMATRIX* transform)
{
	// create our collider if we have some vertices
	if(o && o->numtris)
	{
		return createMesh(o->RenderPositions().ptr,
								 sizeof(piUNIFIED_VERTEX),
								 o->numverts,
								 o->ptris,
								 3*sizeof(short),
								 3*o->numtris,
								 o,
								 transform);
	}

	return -1;
}

physicsTransformCollider::physicsTransformCollider(dSpaceID space, UINT32 cat, UINT32 col) :
	physicsObjectCollider(space, cat, col)
{
}

physicsTransformCollider::~physicsTransformCollider()
{
}

// override and wrap ou node's collider
int physicsTransformCollider::create(OBJECT* o, piMATRIX* transform)
{
	// we create a transform collider from our pool node
	if(o && o->modelNode)
	{
		PoolNode* node = o->modelNode;

		if(node->collider)
		{
			_nodeCollider = node->collider;

			dGeomID nodeGeom = NULL;

			for(int i = 0; i < _nodeCollider->geoms(); ++i)
			{
				nodeGeom = _nodeCollider->id(i);

				dGeomID geom = dCreateGeomTransform(_space);

				dGeomSetData(geom, o);
				dGeomTransformSetGeom(geom, nodeGeom);
				dGeomTransformSetCleanup(geom, 0);
				dGeomTransformSetInfo(geom, 1); // collisions return the transform geom

				// keep track of our transform geom
				_geom.push_back(geomMesh(geom, NULL));
			}

			// update our transform
			updateTransform((transform) ? *transform : o->mat);

			setCategoryMask(_categoryMask);
			setCollideMask(_collideMask);
		}
	}

	return -1;
}

physicsShearedCollider::physicsShearedCollider(dSpaceID space, UINT32 cat, UINT32 col) :
	physicsObjectCollider(space, cat, col),
	_vertices(NULL)
{
}

physicsShearedCollider::~physicsShearedCollider()
{
	if(_vertices)
		delete [] _vertices;

	_vertices = NULL;
}

void physicsShearedCollider::updateTransform(piMATRIX const& m)
{
	// don't rotate sheared colliders as the vertices that make up the collider have already been rotated (and sheared)
	// so, we just update the position in case the world origin offset has changed
	updatePosition((float)m._41, (float)m._42, (float)m._43);
}

int physicsShearedCollider::create(OBJECT* o, piMATRIX* transform)
{
	// create a sheared collider from copied vertex data
	if(o && o->numtris)
	{
		piMATRIX mat = (transform) ? *transform : o->mat;

		float* verts = _vertices = new float[3*o->numverts];
		piVectorConstPtr pverts = o->RenderPositions();

		// helper vars
		piVECTOR vPos;

		// reset position
		mat._41 = mat._42 = mat._43 = 0;

		for(int i = 0; i < o->numverts; i++, ++pverts)
		{
			math3dVectorMultiply(&vPos, pverts.ptr, &mat);

			*verts++ = vPos.x;
			*verts++ = vPos.y;
			*verts++ = -vPos.z;
		}

		mat = IdentityMatrix;
		mat._41 = o->mat._41;
		mat._42 = o->mat._42;
		mat._43 = o->mat._43;

		// go ahead and shear the vertices so the ode physics world will be correct
		// ode doesn't handling shearing internally, so we have to shear the vertices ourself
		return createMesh(_vertices,
								 3*sizeof(float),
								 o->numverts,
								 o->ptris,
								 3*sizeof(short),
								 3*o->numtris,
								 o,
								 &mat);
	}

	return -1;
}

/*
int physicsCollider::createMeshFromObject(OBJECT* Object, bool addTransform)
{
	//if(Object->numtris < 4) return -1; // <-- this was causing some simple mesh colliders not to be created
	if(!Object->numtris) return -1;

// we use are own hacked up version of ode that uses short and left handed indices
// so we don't have to do all this stuff
	// negative z (sigh)
*/
/*
	dTriMeshDataID mesh = dGeomTriMeshDataCreate();

	float* verts = _vertices = new float[3*Object->numverts];
	piVectorConstPtr pverts = Object->RenderPositions();

	for(int i = 0; i < Object->numverts; i++, ++pverts)
	{
		*verts++ = pverts->x;
		*verts++ = pverts->y;
		*verts++ =-pverts->z;
	}

	// have to change to int indices (sigh)
	int* indices = _indices = new int[3*Object->numtris];
	piINDICES* ptris = Object->ptris;

	for(int i = 0; i < Object->numtris; i++, ptris++)
	{
		*indices++ = ptris->v3;
		*indices++ = ptris->v2;
		*indices++ = ptris->v1;
	}
	dGeomTriMeshDataBuildSingle(mesh, _vertices, 3*sizeof(float), Object->numverts
									, _indices,  3*Object->numtris, 3*sizeof(int));
	dGeomTriMeshDataPreprocess(mesh);
*/

	/*
	if(Object->treeID)
	{
		return -1;
	}

	if(Object->flag2 & OBJECT_SPEEDTREE)
	{
		bool stop = true;
	}

	if(!(Object->flags & OBJECT_COLLISION))
	{
		bool stop = true;
	}
	*/

	/*
	static std::map<std::string, bool> badObjects;

	piVectorPtr p = Object->StaticNormals();

	for(int i = 0; i < Object->numverts; ++i)
	{
		float result = mathVectDotProduct(p, p);

		if(result < .000001f)
		{
			std::string name = Object->modelInstance->GetModelName();

			if(badObjects.find(name) == badObjects.end())
			{
				badObjects[name] = true;
			}
		}

		++p;
	}
	*/
/*
	dTriMeshDataID mesh = dGeomTriMeshDataCreate();
	bool isSheared = (Object->modelInstance && Object->modelInstance->IsShearPlacement());

	if(isSheared)
	{
		piMATRIX mat = Object->mat;
		float* verts = _vertices = new float[3*Object->numverts];
		piVectorConstPtr pverts = Object->RenderPositions();

		// helper vars
		piVECTOR temp;

		// reset position
		mat._41 = mat._42 = mat._43 = 0;

		for(int i = 0; i < Object->numverts; i++, ++pverts)
		{
			//temp.x = pverts->x;
			//temp.y = pverts->y;
			//temp.z = pverts->z;

			math3dVectorMultiply(&temp, pverts.ptr, &mat);

			*verts++ = temp.x;
			*verts++ = temp.y;
			*verts++ = temp.z;
		}

		dGeomTriMeshDataBuildSingle(mesh, _vertices, 3*sizeof(float), Object->numverts
										, Object->ptris,  3*Object->numtris, 3*sizeof(short));

		//dGeomTriMeshDataPreprocess(mesh);

#ifdef DEBUG_ODE_VERTEX_SHEARING
		vsize = (3 * Object->numverts * sizeof(float));

		_totalVsize += vsize;
#endif // DEBUG_ODE_VERTEX_SHEARING
	}
	else
	{
		dGeomTriMeshDataBuildSingle(mesh, Object->RenderPositions().ptr, sizeof(piUNIFIED_VERTEX), Object->numverts
				, Object->ptris,  3*Object->numtris, 3*sizeof(short));

	}

	dGeomID meshGeom = dCreateTriMesh(addTransform ? 0 : _space, mesh,0,0,0);
	dGeomID geom = meshGeom;

	dGeomSetData(meshGeom, Object);

	if(addTransform)
	{
		geom = dCreateGeomTransform(_space);

		dGeomSetData(geom, Object);
		dGeomTransformSetGeom(geom, meshGeom);
		dGeomTransformSetCleanup(geom, 1);
		dGeomTransformSetInfo(geom, 1); // collisions return the transform geom

		_geom.push_back(geomMesh(geom, mesh));

		if(isSheared)
		{
			piMATRIX mat = IdentityMatrix;
			mat._41 = Object->mat._41;
			mat._42 = Object->mat._42;
			mat._43 = Object->mat._43;
			updateTransform(mat);
		}
		else
			updateTransform(Object->mat);
	}
	else
	{
		_geom.push_back(geomMesh(meshGeom,mesh));
	}

	// set up bits
	colliderSetBits(geom, _categoryMask, _collideMask);

	return _geom.size() - 1;
}
*/
