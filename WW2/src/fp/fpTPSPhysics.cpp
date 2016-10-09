#include <syscfg.h> // Path is project specific.
#include "fp/fpTPSPhysics.h"
#include "gr2/gr2Layer.h"
#include "gr2/gr2Anim.h"
#include "fp/fpTPSData.h"
#include "spt/Random.h"
#include "ode/ode.h"
#include "xml/XMLLoader2.h"
#include "ui/uiChat.h"
#include "veh/vehmain.h"
#include "math/mathnum.h"
#include "pers/persmain.h"
#include "fx/fxSnd.h"
#include "math/math3d.h"
#include "math/matheul.h"
#include "math/Matrixd.h"
#include "fp/fpFPS.h"
#include "ml/mlutility.h"
#include "fp/fptypes.h"
#include "trn/trnMain.h"
#include "mm/mmEnums.h"
#include "cfg/cfgPrefs.h"
#include "trn/trnvars.h"
#include "scen/ScenarioManager.h"
#include "chat/chatLocalizedMessage.h"
#include "client/GameClient.h"
#include "ui/uiMain.h"

#define AHW 180							// average human weight in pounts
#define LTOK(x) ((x)*.45359237)			// pounds to kilograms
#define POB(x) LTOK(AHW*(x))			// percent of body
#define ITOM(x) ((x)*0.0254f)			// inches to meters

#define SHAPE_DEFINITIONS "gr2Data/TPS/meshes/colliders.xml"
#define NUM_RAGDOLLS 25 + 1
#define INFINITE_RAGDOLLS 0

// make limb joints straighten out if possible
static float tforce = 50.0f;
static fpTPSRagdoll* _playerRagdoll = nullptr;
// external ragdoll camera variables

static Vec3d _lookAt;
static Vec3d _camPos;
static Vec3d _focusPos;// player ragdoll position

#ifdef BETA_RELEASE
static int _numFallSounds = 4;
#endif

using RagdollList = std::vector<fpTPSRagdoll*>;

struct WeaponShape
{
	struct Shape
	{
		double mass;
		double length;
		double diameter;
		Vec4d rotation;
		Vec3d offset;
	};

	struct Link
	{
		std::string bone;
		float chance;
		int duration;
	};

	int type;
	int weapon;
	bool damp;
	std::vector<Shape> shapes;
	std::vector<Link> links;
};

using WeaponShapeMap = std::map<std::pair<int, int>, WeaponShape>;

static RagdollList _ragdolls;
static WeaponShapeMap _weapShapes;

// load physics colliders
class WeaponShapeLoader : public XMLLoader2<WeaponShapeLoader>
{
public:

	WeaponShapeLoader(const char* path) : XMLLoader2<WeaponShapeLoader>(path)
	{
		AddHandler("model", &WeaponShapeLoader::DoModel, this);
		AddHandler("capsule", &WeaponShapeLoader::DoCapsule, this);
		AddHandler("rotate", &WeaponShapeLoader::DoRotate, this);
		AddHandler("offset", &WeaponShapeLoader::DoOffset, this);
		AddHandler("link", &WeaponShapeLoader::DoLink, this);
	}

	virtual ~WeaponShapeLoader()
	{
	}

	// tag handlers
	XML_TAG_HANDLER(DoModel)
	{
		if(first)
		{
			WeaponShape model;

			model.weapon = 0;

			xmlGetNodeValue(node, XML_DATA_SINT, &model.type, "type");
			xmlGetNodeValue(node, XML_DATA_SINT, &model.weapon, "weapon");
			xmlGetNodeValue(node, XML_DATA_BOOL, &model.damp, "damp");

			if(ProcessChildren(node, &model))
				_weapShapes[std::make_pair(model.type, model.weapon)] = model;
		}

		return false;
	}

	XML_TAG_HANDLER(DoCapsule)
	{
		if(first && userData)
		{
			WeaponShape* model	= (WeaponShape*)userData;
			WeaponShape::Shape newShape;

			xmlGetNodeValue(node, XML_DATA_DOUBLE, &newShape.mass, "mass");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &newShape.length, "length");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &newShape.diameter, "diameter");

			if(ProcessChildren(node, &newShape))
				model->shapes.push_back(newShape);
		}

		return false;
	}

	XML_TAG_HANDLER(DoRotate)
	{
		if(first && userData)
		{
			WeaponShape::Shape* shape = (WeaponShape::Shape*)userData;

			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->rotation.x(), "x");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->rotation.y(), "y");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->rotation.z(), "z");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->rotation.w(), "w");
		}

		return true;
	}

	XML_TAG_HANDLER(DoOffset)
	{
		if(first && userData)
		{
			WeaponShape::Shape* shape = (WeaponShape::Shape*)userData;

			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->offset.x(), "x");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->offset.y(), "y");
			xmlGetNodeValue(node, XML_DATA_DOUBLE, &shape->offset.z(), "z");
		}

		return true;
	}

	XML_TAG_HANDLER(DoLink)
	{
		if(first && userData)
		{
			WeaponShape* model	= (WeaponShape*)userData;
			WeaponShape::Link newLink;

			newLink.bone = "";
			newLink.chance = 1.0f;
			newLink.duration = 0;

			newLink.bone = xmlGetStringFromNode(node, "bone");
			xmlGetNodeValue(node, XML_DATA_FLOAT, &newLink.chance, "chance");
			xmlGetNodeValue(node, XML_DATA_SINT, &newLink.duration, "duration");

			model->links.push_back(newLink);
		}

		return true;
	}
};

static void _loadWeapShapes()
{
	_weapShapes.clear();
	WeaponShapeLoader loader(SHAPE_DEFINITIONS);
	loader.Load();
}

void fpTPSRagdollInit()
{
	//if(!cfgPrefsGetBool("app.ragdolls", true))
	//	return;

#if !INFINITE_RAGDOLLS
	if(fpTPSSkeletonFile)
	{
		for(int i = 0; i < NUM_RAGDOLLS; i++)
			_ragdolls.push_back(new fpTPSRagdoll);
	}
#endif

	_loadWeapShapes();
}

void fpTPSRagdollKill()
{
	_playerRagdoll = nullptr;

	for(unsigned int i = 0; i < _ragdolls.size(); i++)
	{
		fpTPSRagdoll* ragdoll = _ragdolls[i];
		delete ragdoll;
	}

	_ragdolls.clear();
	_weapShapes.clear();
}

static const WeaponShape* _getWeaponShape(int type, int weapon = 0)
{
	WeaponShapeMap::iterator found = _weapShapes.find(std::make_pair(type, weapon));

	if(found != _weapShapes.end())
		return &found->second;

	if(weapon)
	{
		found = _weapShapes.find(std::make_pair(type, 0));

		if(found != _weapShapes.end())
			return &found->second;
	}

	return nullptr;
}

bool fpTPSAllowRagdoll(FLOAT64VECTOR& pos)
{
	if(true || cfgPrefsGetBool("app.ragdolls", true))
	{
		Vec3d start(pos.x, pos.y, pos.z + .25f);
		Vec3d dir(0,0,-1.0f);

		// put it in the physics world
		start = physicsGameToPhysical(start);

		return (collideRay(COLLIDER_NORAGDOLL, .5f, start, dir) == 0);
	}

	return false;
}

fpTPSRagdoll* fpTPSAddRagdoll(gr2AnimTrooper* anim, int timeout, int delay, bool addForce)
{
	// Offline kill message testing...
	/*
	vehVehicle* me = persGetPersonaVehiclePtr();
	vehVehicle* rd = anim->myVehicle;
	SINT32 rd_side = rd->Side;
	char data[602];
	snprintf(data, 602-1, "[DEV] %s was killed by %s", chatGetCallsign(rd->Chat_Handle), "OFFLINE_PLAYER");

	Chat::LocalizedMessage* msg = new Chat::LocalizedMessage();
	msg->m_prefix = LSTR_SYSTEM;
	msg->m_message = LSTR_GENERIC;
	strcpy(msg->m_data, data);

	GameClient* client = PlayerClient();
	client->ProcessChatLocalizedMessage(*msg);
	*/

	/*
	TestRagdoll* newTest = new TestRagdoll();
	newTest->AnimInit(anim);
	return nullptr;
	*/

	static int instance = 1; // 0 is reserved for me

	fpTPSRagdoll* ragdoll = nullptr;

#if INFINITE_RAGDOLLS
	{
		ragdoll = new fpTPSRagdoll;
		_ragdolls.push_back(ragdoll);
#else

	if(!_ragdolls.size())
		fpTPSRagdollInit();

	if(_ragdolls.size())
	{
		if(anim->myVehicle->ModelData)
		{
			ragdoll = _ragdolls[0]; // 0 is reserved for me
		}
		else
		{
			ragdoll = _ragdolls[instance];

			if(++instance >= (int)_ragdolls.size())
				instance = 1;
		}

#endif
		if(ragdoll && !ragdoll->IsActive())
		{
			vehVehicle* vehicle = anim->myVehicle;
			bool myTarget = (!Online || vehicle->Chat_Handle == vehLastInfantryDamaged());
			OBJECT* ob = vehicle->VehicleObjectData->PlaneObject;
			Vec3d& push = physicsGetWorldForce((vehicle->ModelData) ? vehicle->ModelData->VehiclePos: vehicle->PosData.Pos, false);

			if(myTarget && !push.length2())
			{
				Vec3d targetPos;
				Vec3d myPos;

				targetPos.set((vehicle->ModelData) ? vehicle->ModelData->VehiclePos: vehicle->PosData.Pos);
				myPos.set(persGetPersonaVehiclePtr()->ModelData->VehiclePos);

				Vec3d dir = targetPos - myPos;

				// normalize
				dir.normalize();

				// scale it up to some push force
				dir *= (3.0f + GetRandom(1.0f, 10.0f));

				// add some z for effect
				dir.z() = -(1.0f + GetRandom(1.0f, 3.0f));

				if(SCEN_MGR->IsActive())
				{
					static float dmod = 5.0f;
					dir *= dmod;
				}

				// set push
				push.set(dir);
			}

			if(push.length2())
			{
				switch(anim->currentPose)
				{
					case TPS_POSETYPE_PRONE:
					{
						push *= .25f;
					}
					break;
					case TPS_POSETYPE_CROUCH:
					{
						push *= .75f;
					}
					break;
					default: // standing
						// no change
					break;
				}
			}

			if(!addForce)
				push.set(0, 0, 0);
			else if(anim->currentPose == TPS_POSETYPE_SWIM)
				push.set(0, 0, -8.0f);

			ragdoll->AnimInit(anim, &push, timeout, delay);
			//ragdoll->AnimInit(anim, nullptr, timeout, delay);

			gr2RemoveAllBlendedAnimations (anim->skeleton, -1);

			SINT32 deathSoundID = (SINT32)((mathGetFloat32ChaoticNumber() * (FLOAT32)TPS_NUM_DEATHSOUNDS) - 0.5f);

			if (deathSoundID < 0)
				deathSoundID = 0;
			else if (deathSoundID > TPS_NUM_DEATHSOUNDS - 1)
				deathSoundID = TPS_NUM_DEATHSOUNDS - 1;

			fpFPSfxPlayTPSSingleSound (tpsTroopData.deathSounds[deathSoundID], ob, tpsTroopData.deathSoundFMin, tpsTroopData.deathSoundFMax);

			anim->deathState = TPS_DEATH1;
			anim->lastUpdateTime = frametime - 1; // force an update

			gr2AnimUpdateModel(ob, ob->flag2 & OBJECT_ISFPS);

			anim->active = false;
			gr2SetModelActive(anim->collisionModel, false);

			if(anim->myVehicle->ModelData)
				_playerRagdoll = ragdoll;
		}
		else
		{
			ragdoll = nullptr;
		}
	}

	return ragdoll;
}

#define RAGDOLL_CAM_COLLIDER COLLIDER_TRN_BUILDING|COLLIDER_TRN_BRIDGE|COLLIDER_TRN_GROUND

static void _calculateCamPos(Vec3d& camPos, Vec3d& lookAt, UINT32 maxLOS)
{
	// from my position to the cardinal views out to a set number, grab the farthest position
	static colliderQuery collider = colliderQuery(0, false);

	const int losRay				= 16;
	const int cardinalRays		= 16;
	FLOAT64 rayLen				= 5.0f;
	FLOAT64 vertAngle			= 30.0f;
	FLOAT64 rayAngle			= vertAngle/90.0f;
	dGeomID ray					= nullptr;

	Vec3d start						= physicsGameToPhysical(lookAt);
	Vec3d dir						= Vec3d(0, 1.0f, rayAngle);

	// used to query a ray from ode
	dVector3 rayStart;
	dVector3 rayDir;

	/*
	static char lastDebug[255+1];
	char debug[255+1];

	lastDebug[0] = 0;
	debug[0] = 0;
	*/

	colliderQuery::dGeomList& rays = collider.geoms();

	// init
	if(rays.empty())
	{
		collider.addGeomToCollideWith((dGeomID)colliderTerrainSpace());
		collider.setMaxContactsPerHit(1);

		Matrixd rotator;
		rotator.makeRotate(mlDEG_TO_RAD(45.0f), 0, 0, 1.0f);

		dir.normalize();

		// look down rays
		for(int r = 0; r < 8; ++r) // one ray for each cardinal direction
		{
			// create a top ray
			ray = dCreateRay(collider.space(), rayLen);

			// set the ray's params
			dGeomEnable(ray);
			dGeomRaySet(ray, start.x(), start.y(), start.z(), dir.x(), dir.y(), dir.z());

			// rotate it around the z
			dir = rotator * dir;

			// add the ray to our collider
			collider.addGeom(ray, 0, RAGDOLL_CAM_COLLIDER);
		}

		// same plane rays; same as top rays but with a negated z
		for(int r = 0; r < 8; ++r) // one ray for each cardinal direction
		{
			ray = rays[r];

			// query the top ray
			dGeomRayGet(ray, rayStart, rayDir);

			// create a bottom ray
			ray = dCreateRay(collider.space(), rayLen);

			// shallow version of the look down version
			dir.set(rayDir[0], rayDir[1], rayDir[2] * .1f);
			dir.normalize();

			// set the ray's params
			dGeomEnable(ray);
			dGeomRaySet(ray, start.x(), start.y(), start.z(), dir.x(), dir.y(), dir.z());

			// add the ray to our collider
			collider.addGeom(ray, 0, RAGDOLL_CAM_COLLIDER);
		}

		// one more for our line of sight check ray
		ray = dCreateRay(collider.space(), 1.0f);

		// add the ray to our collider
		collider.addGeom(ray, 0, RAGDOLL_CAM_COLLIDER);
	}
	else
	{
		// update the cardinal rays
		for(int r = 0; r < cardinalRays; ++r) // one ray for each cardinal direction
		{
			ray = rays[r];
			dGeomRayGet(ray, rayStart, rayDir);
			dGeomRaySet(ray, start.x(), start.y(), start.z(), rayDir[0], rayDir[1], rayDir[2]);
		}
	}

	// get the line of sight ray to test first
	ray = rays[losRay];

	// now we update the line of sight ray
	dir = camPos - lookAt; // ray from lookat to cam pos

	FLOAT64 losLength = dir.length();

	dGeomRaySetLength(ray, losLength);

	dir.normalize();

	dGeomRaySet(ray, start.x(), start.y(), start.z(), dir.x(), dir.y(), dir.z());

	// now we need to see if we collided with anything and pick the best ray to put the camera at
	// if nothing hit, we're good
	if(collider.findCollisions(true) || losLength > (FLOAT64)maxLOS)
	{
		collider.sort();
		colliderQuery::CollisionList hits;

		// if our line of sight ray hit, let's look for a better cam position
		// if not, we're still in line of sight
		if(collider.getCollisions(ray, hits, false) || losLength > (FLOAT64)maxLOS)
		{
			Vec3d best;
			FLOAT64 bestDepth;
			SINT32 bestIndex = -1;
			SINT32 rayIndex = GetRandom(0, cardinalRays - 1);

			for(unsigned int r = 0; r < cardinalRays; ++r)
			{
				// let's try the top ray first
				ray = rays[rayIndex++];

				hits.clear();

				if(collider.getCollisions(ray, hits, false) && !hits.empty())
				{
					colliderQuery::Collision& hit = hits.at(0);

					//std::vector<std::string> cdesc = colliderGetDescription(dGeomGetCategoryBits(hit.g2));
					//OBJECT* o2 = (OBJECT*)dGeomGetData(hit.g2);

					if(bestIndex == -1)
					{
						best			= hit.pos;
						bestDepth	= hit.depth;
						bestIndex	= rayIndex;
					}
					else if(hit.depth > bestDepth)
					{
						best			= hit.pos;
						bestDepth	= hit.depth;
						bestIndex	= rayIndex;
					}
				}
				else
				{
					dGeomRayGet(ray, rayStart, rayDir);

					dir.set(rayDir[0], rayDir[1], rayDir[2]);
					start.set(rayStart[0], rayStart[1], rayStart[2]);

					dir *= rayLen;
					start += dir;

					best			= start;
					bestDepth	= rayLen;
					bestIndex	= rayIndex;

					break;
				}

				if(rayIndex >= cardinalRays)
					rayIndex = 0;
			}

			// we now move best towards the lookat point a little
			// to make sure we're not inside a wall
			best = physicsPhysicalToGame(best);
			dir = lookAt - best;
			dir.normalize();
			dir *= .5f;
			best += dir;

			// update cam pos
			camPos = best;
			//camPos = physicsPhysicalToGame(camPos);

			//snprintf(debug, 255, "Best cam position: index = %d length = %2.f position = %.2f,%.2f,%.2f", bestIndex, bestDepth, camPos.x(), camPos.y(), camPos.z());
		}
		else
		{
			//snprintf(debug, 255, "Line of sight ok, using passed cam pos: %.2f,%.2f,%.2f", camPos.x(), camPos.y(), camPos.z());
		}
	}
	else
	{
		//snprintf(debug, 255, "No ray collisions, using passed cam pos: %.2f,%.2f,%.2f", camPos.x(), camPos.y(), camPos.z());
	}

	/*
	if(strcmp(lastDebug, debug))
	{
		uiChatPrintLine(debug);
		strsafecpy(lastDebug, debug);
	}
	*/
}

bool fpTPSDeathPivot(piVECTOR* pos, piMATRIX* att)
{
	vehVehicle* v	= persGetPersonaVehiclePtr();
	bool dead		= fpFPSIsDead();

	if(!dead)
		return false;

	if(_playerRagdoll)
		_focusPos = _playerRagdoll->GetRootPos();

	 if(_focusPos.length2())
	 {
		_lookAt = _focusPos;
		_lookAt.z() -= 1.0f;
	}
	else
	{
		_lookAt.set(v->ModelData->VehiclePos);
		_lookAt.z() -= 1.0f; //_lookAt.z() += fpFPSGetEyeAdjust(-1, -1, false);
	}

	if(_lookAt.length2())
	{
		Vec3d p = _lookAt;

		pos->x = -float(p.x() - WorldOriginOffset.x);
		pos->y = -float(p.y() - WorldOriginOffset.y);
		pos->z = float(p.z());

		if(v)
		{
			piVECTOR TempAtt;

			TempAtt.x = (FLOAT32)v->ModelData->RenderAtt.Pitch;
			TempAtt.y = (FLOAT32)v->ModelData->RenderAtt.Roll;
			TempAtt.z = (FLOAT32)v->ModelData->RenderAtt.Yaw;

			mathEul_ToMatrix(att,EulOrdXYZs,&TempAtt);
		}

		return true;
	}

	return false;
}

void fpTPSDeathCameraReset()
{
	_lookAt.set(0,0,0);
	_camPos.set(0,0,0);
	_focusPos.set(0,0,0);
	_playerRagdoll = nullptr;
}

#define RAGDOLL_FALLING_THRESHOLD 30
#define RAGDOLL_FALLING_STOP 16

bool fpTPSDeathCamera(piVECTOR* pos, piVECTOR* aim)
{
	static SINT32 _nextUpdate = 0;
	static UINT32 _startFall = 0;
	static UINT32 _groundZ = 0;
	static bool init = false;
	static piVECTOR lastAim;
	static SINT32 respawnTime = 0;

	if(!fpFPSIsDead())
		return true;

	vehVehicle* v							= persGetPersonaVehiclePtr();
	fpPersonData* pData				= (v) ? (fpPersonData*)v->ModelData->PersonData: nullptr;
	UINT32 aboveGround					= (pData) ? (UINT32)pData->heightAboveGround : 0;
	UINT32 maxLOS						= 100;

	if(!pos)
		init = false;

	if(!init && v)
	{
		_camPos.set(v->ModelData->VehiclePos);
		_lookAt.set(v->ModelData->VehiclePos);
		_lookAt.z() += .5f;

		// move the camera out and up a bit
		_camPos += Vec3d(GetRandom(-5.0f, 5.0f), 5.0f, 3.0f);

		if(aboveGround > RAGDOLL_FALLING_THRESHOLD)
			persSetExternalCameraPos(GetRandom(-180.0f, 180.0f), 45, 8);

		// set out camera pivot point callback
		persSetExternalCameraPivot(fpTPSDeathPivot);
        persSetExternalCameraTarget(PI, 0.0f, 0.0f, 10.0f);

		_nextUpdate = 0;
		_startFall = aboveGround;
		_groundZ = (UINT32)_lookAt.z() - _startFall;

		respawnTime = frametime + 1000;
		lastAim.set(0,0,0);

		init = true;
	}

	if(_playerRagdoll)
		_focusPos = _playerRagdoll->GetRootPos();

	if(_focusPos.length2())
	{
		_lookAt = _focusPos;
		//_lookAt.z() += .5f;
	}
	else
	{
		_lookAt.set(v->ModelData->VehiclePos);
		_lookAt.z() += fpFPSGetEyeAdjust(-1, -1, false);
	}

	if(_startFall > RAGDOLL_FALLING_THRESHOLD)
	{
		// keep these in sync when falling and we have a focal point
		if(_focusPos.length2() && v)
		{
			v->ModelData->VehiclePos.x = _lookAt.x();
			v->ModelData->VehiclePos.y = _lookAt.y();
			v->ModelData->VehiclePos.z = _lookAt.z();
		}

		aboveGround = (_lookAt.z() > _groundZ) ? (UINT32)_lookAt.z() - _groundZ : 0;

		//uiChatPrintLine("Death Zs above ground: %u", aboveGround);

		if(aboveGround > RAGDOLL_FALLING_STOP)
		{
			if(_playerRagdoll && _playerRagdoll->IsActive())
			{
				GR2PhysicsBoneRef bone = nullptr;
				FLOAT64VECTOR vel;
				static float chance = .01f;
				static float push = 1.75f;

				// add some drama
				if(GetRandom(0.0f, 1.0f) < chance)
				{
					bone	= _playerRagdoll->GetBone("Bip01 L Hand");
					vel		= bone->getLinearVel();
					vel.x += (double)GetRandom(-push, push);
					vel.y += (double)GetRandom(-push, push);
					vel.z += (double)GetRandom(-push, push);
					bone->setLinearVel(vel);
				}

				if(GetRandom(0.0f, 1.0f) < chance)
				{
					bone	= _playerRagdoll->GetBone("Bip01 R Hand");
					vel		= bone->getLinearVel();
					vel.x += (double)GetRandom(-push, push);
					vel.y += (double)GetRandom(-push, push);
					vel.z += (double)GetRandom(-push, push);
					bone->setLinearVel(vel);
				}

				if(GetRandom(0.0f, 1.0f) < chance)
				{
					bone	= _playerRagdoll->GetBone("Bip01 L Foot");
					vel		= bone->getLinearVel();
					vel.x += (double)GetRandom(-push, push);
					vel.y += (double)GetRandom(-push, push);
					vel.z += (double)GetRandom(-push, push);
					bone->setLinearVel(vel);
				}

				if(GetRandom(0.0f, 1.0f) < chance)
				{
					bone	= _playerRagdoll->GetBone("Bip01 R Foot");
					vel		= bone->getLinearVel();
					vel.x += (double)GetRandom(-push, push);
					vel.y += (double)GetRandom(-push, push);
					vel.z += (double)GetRandom(-push, push);
					bone->setLinearVel(vel);
				}
			}

			persSetExternalCameraTarget(360.0f, 0.0f, 0.0f, 30.0f);
			return true;
		}

		if(_nextUpdate == 0 && pos) // first time through
		{
			//_camPos= _lookAt + Vec3d(GetRandom(-5.0f, 5.0f), 5.0f, 3.0f);

			_camPos.x() = double(pos->x + WorldOriginOffset.x);
			_camPos.y() = double(pos->y + WorldOriginOffset.y);
			_camPos.z() = double(pos->z);
		}

		_startFall = 0;
		_groundZ = 0;
	}

	if(frametime >= _nextUpdate)
	{
		_calculateCamPos(_camPos, _lookAt, maxLOS);

		static SINT32 period = 500;

		// set out next update
		_nextUpdate = frametime + period;
	}

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
				vehExitVehicle(1,0,0);

				// make it exit right away
				v->VehicleDeadTime = frametime - 4000;
			}
		}
	}

	return true;
}

void fpTPSAddForce(const FLOAT64VECTOR& pos, FLOAT64 energy)
{

}

TestRagdoll::TestRagdoll() : gr2PhysicsModel()
{
	// our base skeleton
	InitSkeleton(fpTPSSkeletonFile);

	/********************************************************************************/
	// calculate our bone lengths and radii
	/********************************************************************************/

	// create a body for each bone
	// head
	double headRad		= ITOM(3.0f);
	double headLen		= ITOM(8.0f);

	// chest
	double chestRad		= ITOM(4.0f);
	double chestLen		= GetBoneDist("Bip01 L UpperArm", "Bip01 R UpperArm");

	// upper arm
	double uarmRad		= ITOM(2.5f);
	double uarmLen		= GetBoneDist("Bip01 L UpperArm", "Bip01 L Forearm");

	// fore arm
	double farmRad		= ITOM(2.0f);
	double farmLen		= GetBoneDist("Bip01 L Forearm", "Bip01 L Hand");

	// hand
	double handRad		= ITOM(1.0f);
	double handLen		= GetBoneDist("Bip01 L Hand", "Bip01 L Finger11");

	// hips
	double hipsRad		= ITOM(4.0f);
	double hipsLen		= chestLen;

	// torso
	double torsoRad		= ITOM(4.0f);
	double torsoLen		= GetBoneDist("Bip01 Spine", "Bip01 Neck") - ITOM(2.0f);

	// thigh
	double thighRad		= ITOM(4.0f);
	double thighLen		= GetBoneDist("Bip01 L Thigh", "Bip01 L Calf");

	// calf
	double calfRad		= ITOM(3.0f);
	double calfLen		= GetBoneDist("Bip01 L Calf", "Bip01 L Foot");

	// foot
	double footRad		= ITOM(2.0f);
	double footLen		= GetBoneDist("Bip01 L Foot", "Bip01 L Toe0");

	/********************************************************************************/
	// add our bones
	/********************************************************************************/

	// collidible bones
	gr2PhysicsBone* head			= addAxialBone("Bip01 Head", POB(5), headLen, headRad);
	gr2PhysicsBone* shoulders		= addCenterBone("Bip01 Neck", POB(7), chestLen, chestRad);
	gr2PhysicsBone* torso			= addCenterBone("Bip01 Spine2", POB(10), torsoLen, torsoRad);
	gr2PhysicsBone* hips			= addCenterBone("Bip01 Spine", POB(18), hipsLen, hipsRad);
	gr2PhysicsBone* lupperarm		= addAxialBone("Bip01 L UpperArm", POB(4), uarmLen, uarmRad);
	gr2PhysicsBone* lforearm		= addAxialBone("Bip01 L Forearm", POB(3), farmLen, farmRad);
	gr2PhysicsBone* lhand			= addAxialBone("Bip01 L Hand", POB(4), handLen, handRad);
	gr2PhysicsBone* rupperarm		= addAxialBone("Bip01 R UpperArm", POB(4), uarmLen, uarmRad);
	gr2PhysicsBone* rforearm		= addAxialBone("Bip01 R Forearm", POB(3), farmLen, farmRad);
	gr2PhysicsBone* rhand			= addAxialBone("Bip01 R Hand", POB(4), handLen, handRad);
	gr2PhysicsBone* lthigh			= addAxialBone("Bip01 L Thigh", POB(6), thighLen, thighRad);
	gr2PhysicsBone* lcalf			= addAxialBone("Bip01 L Calf", POB(5), calfLen, calfRad);
	gr2PhysicsBone* lfoot			= addAxialBone("Bip01 L Foot", POB(8), footLen, footRad);
	gr2PhysicsBone* rthigh			= addAxialBone("Bip01 R Thigh", POB(6), thighLen, thighRad);
	gr2PhysicsBone* rcalf			= addAxialBone("Bip01 R Calf", POB(5), calfLen, calfRad);
	gr2PhysicsBone* rfoot			= addAxialBone("Bip01 R Foot", POB(8), footLen, footRad);

	// local adjustments
	head->SetOffset(ITOM(4.0f));
	shoulders->SetOffset(-ITOM(1.0f));
	torso->SetOffset(-ITOM(4.0f));
	hips->SetOffset(-ITOM(1.0f));

	// non-collidible bones
	granny_skeleton* gs = _skeleton->GetSkeleton();

	for(int i = 0; i < gs->BoneCount; i++)
	{
		if(!GetBone(gs->Bones[i].Name))
			addStaticBone(gs->Bones[i].Name);
	}

	// set parent overrides here
	gr2StaticBone* pelvis = GetStaticBone("Bip01 Pelvis");
	if(pelvis) pelvis->SetParent(hips->GetBone());

	// tell our bones to init themselves
	InitBones();

	/********************************************************************************/
	// set friction models
	/********************************************************************************/

	// set up our friction models
	{
		// we don't want the head digging in
		head->SetFrictionModel(1000.0f, 0.0f, 200);

		// scale this down pretty fast
		shoulders->SetFrictionModel(25000.0f, 1000.0f, 800);

		// we want feet to stick at first then losen up fast
		lfoot->SetFrictionModel(30000.0f, 3000.0f, 1000);
		rfoot->SetFrictionModel(30000.0f, 3000.0f, 1000);

		// legs slip pretty fast
		lthigh->SetFrictionModel(25000.0f, 4000.0f, 800);
		lcalf->SetFrictionModel(15000.0f, 1000.0f, 800);
		rthigh->SetFrictionModel(25000.0f, 4000.0f, 800);
		rcalf->SetFrictionModel(15000.0f, 1000.0f, 800);

		// arms and hands don't need much resistance
		lupperarm->SetFrictionModel(15000.0f, 500.0f, 800);
		lforearm->SetFrictionModel(8000.0f, 300.0f, 500);
		lhand->SetFrictionModel(2000.0f, 200.0f, 300);
		rupperarm->SetFrictionModel(15000.0f, 500.0f, 800);
		rforearm->SetFrictionModel(8000.0f, 300.0f, 500);
		rhand->SetFrictionModel(2000.0f, 200.0f, 300);

		// these are the main grips
		torso->SetFrictionModel(30000.0f, 20000.0f, 1000);
		hips->SetFrictionModel(30000.0f, 20000.0f, 1000);
	}

    // setup our bone joints
    /*
        TBD
     
	//
	// setup our joints
	//

	Vec3d xAxis = Vec3d(1.0f, 0, 0);
	Vec3d yAxis = Vec3d(0, 1.0f, 0);
	Vec3d zAxis = Vec3d(0, 0, 1.0f);

	if(1)
	{
		dJointID jid = nullptr;

		// link all of the bodies together via joints
		// neck
		addJoint(head, shoulders, dJointTypeUniversal, &zAxis, -.4f, .1f, &yAxis, -.2f, .2f);

		// shoulders
		addJoint(lupperarm, shoulders, dJointTypeBall);
		addJoint(rupperarm, shoulders, dJointTypeBall);

		// shoulder restrictors
		// axis 1 must be perpendicular to axis 3
		// axis 2's vector isn't actually used, ODE calculates it based
		// on axis 1 and 3; the hi/lo params are used to restrict the axis though for
		// axis 2; 1 = rotation, 2 = up/down 3 = front back
		// axis1 = twist, axis2 = up/down, axis3 = front/back
		jid = addJoint(lupperarm, shoulders, dJointTypeAMotor, &xAxis, -.5f, .5f, &zAxis, -.5f, .5f, &yAxis, -.5f, .5f);
		dJointSetAMotorMode(jid, dAMotorEuler);

		// y = up/down
		jid = addJoint(rupperarm, shoulders, dJointTypeAMotor, &xAxis, -.5f, .5f, &zAxis, -.1f, .1f, &yAxis, -.5f, .5f);
		dJointSetAMotorMode(jid, dAMotorEuler);

		lupperarm->SetStiffnessModel(3000.0f, 200.0f, 500);
		rupperarm->SetStiffnessModel(3000.0f, 200.0f, 500);

		// Motors can also be used to accurately model dry (or Coulomb) friction in joints.
		// Simply set the desired velocity to zero and set the maximum force to some constant value - then all joint motion will be impeded by that force.

		// elbows
		addJoint(lforearm, lupperarm, dJointTypeHinge, &zAxis, -1.5f, .1f);
		addJoint(rforearm, rupperarm, dJointTypeHinge, &zAxis, -1.5f, .1f);

		lforearm->SetStiffnessModel(1000.0f, 100.0f, 500);
		rforearm->SetStiffnessModel(1000.0f, 100.0f, 500);

		// wrists
		addJoint(lhand, lforearm, dJointTypeHinge, &zAxis, -.5f, .5f);
		addJoint(rhand, rforearm, dJointTypeHinge, &zAxis, -.5f, .5f);

		// chest
		addJoint(torso, shoulders, dJointTypeHinge, &zAxis, -.3f, .1f);

		// axis1 = twist, axis2 = left/right, axis3 = front/back
		//addJoint(torso, shoulders, dJointTypeBall);
		//jid = addJoint(torso, shoulders, dJointTypeAMotor, &xAxis, -.1f, .1f, &yAxis, -.1f, -.1f, &zAxis, -.1f, .1f);
		//dJointSetAMotorMode(jid, dAMotorEuler);

		// waist
		addJoint(hips, torso, dJointTypeHinge, &zAxis, -.3f, .1f);

		// axis1 = twist, axis2 = left/right, axis3 = front/back
		//addJoint(hips, torso, dJointTypeBall);
		//jid = addJoint(hips, torso, dJointTypeAMotor, &xAxis, -.1f, .1f, &yAxis, -.1f, -.1f, &zAxis, -.1f, .1f);
		//dJointSetAMotorMode(jid, dAMotorEuler);

		//-.2f, 1.6f, &bAxis, -.2f, 1.0f
		// hips
		addJoint(lthigh, hips, dJointTypeUniversal, &zAxis, -.2f, 1.4f, &yAxis, -.1f, .8f);
		addJoint(rthigh, hips, dJointTypeUniversal, &zAxis, -.2f, 1.4f, &yAxis, -.8f, .1f);

		lthigh->SetStiffnessModel(10000.0f, 100.0f, 1000);
		rthigh->SetStiffnessModel(10000.0f, 100.0f, 1000);

		// knees
		addJoint(lcalf, lthigh, dJointTypeHinge, &zAxis, -2.0f, 0);
		addJoint(rcalf, rthigh, dJointTypeHinge, &zAxis, -2.0f, 0);

		lcalf->SetStiffnessModel(10000.0f, 100.0f, 1000);
		rcalf->SetStiffnessModel(10000.0f, 100.0f, 1000);


		// ankles
		addJoint(lfoot, lcalf, dJointTypeHinge, &zAxis, -.5f, .2f);
		addJoint(rfoot, rcalf, dJointTypeHinge, &zAxis, -.5f, .2f);

		lfoot->SetStiffnessModel(10000.0f, 100.0f, 1000);
		rfoot->SetStiffnessModel(10000.0f, 100.0f, 1000);
	}
	*/

	// set our root bone
	_rootBone = hips;
}

TestRagdoll::~TestRagdoll()
{
}

void TestRagdoll::AnimInit(gr2AnimTrooper* anim)
{
	Init();

	piMATRIX& t			= (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleAttMat: anim->myVehicle->PosData.AttMat;
	FLOAT64VECTOR& v	= (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleVel: anim->myVehicle->PosData.Vel;
	FLOAT64VECTOR& p	= (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehiclePos: anim->myVehicle->PosData.Pos;

	Vec3d worldPos;
	worldPos.set(p);

	//worldPos = physicsGameToPhysical(worldPos);

	_worldTransform.set(t);
	_worldTransform.setTrans(worldPos);

	_velocity.set(v);

	if(!anim->myVehicle->ModelData)
		_velocity.x() *= -1.0f;

	// update the skeleton so it reverts to the default pose
	//gr2UpdateModel(_skeleton); may not need this because the model gets update right below

	// figure out which model to init with
	gr2Model* visualModel = anim->model.GetActive();

	visualModel->Update(true, true);

	_skeleton->Update(true, true);

	// init our visual model
	InitModel(gr2Model::Load(gr2GetPath(visualModel), _skeleton, 0, 1));

	// init our scenegraph node for drawing
	// need non-osg equivalent
	InitNode(anim->myVehicle->VehicleObjectData->PlaneObject);

	// make sure we're active
	SetActive(true);

	anim->skeleton->Update(true, true);

	// update to the current pose
	//InitPose(anim->skeleton);
	BonesToBodies(anim->skeleton);

	UpdateBones();

	_material = (anim->myVehicle->isInside) ?
					piFindMaterialName("interior") :
					piFindMaterialName("forest1");

	//for(BoneList::iterator it = _bones.begin(); it != _bones.end(); it++)
	//	dBodyDisable((*it)->GetBody());
}

fpTPSRagdoll::fpTPSRagdoll() : gr2PhysicsModel(),
	_delay(0),
	_country(0),
	_decal(-1),
	_nextContactSound(0),
    _nextImpactSound(0),
	_weapJoint(nullptr)
{
	// our base skeleton
	InitSkeleton(fpTPSSkeletonFile);

	/********************************************************************************/
	// calculate our bone lengths and radii
	/********************************************************************************/

	// create a body for each bone
	// head
	double headRad		= ITOM(3.0f);
	double headLen		= ITOM(8.0f);

	// chest
	double chestRad		= ITOM(4.0f);
	double chestLen		= GetBoneDist("Bip01 L UpperArm", "Bip01 R UpperArm");

	// upper arm
	double uarmRad		= ITOM(3.0f);
	double uarmLen		= GetBoneDist("Bip01 L UpperArm", "Bip01 L Forearm");

	// fore arm
	double farmRad		= ITOM(2.5f);
	double farmLen		= GetBoneDist("Bip01 L Forearm", "Bip01 L Hand");

	// hand
	double handRad		= ITOM(1.5f);
	double handLen		= GetBoneDist("Bip01 L Hand", "Bip01 L Finger11");

	// hips
	double hipsRad		= ITOM(4.5f);
	double hipsLen		= chestLen;

	// torso
	double torsoRad		= ITOM(4.5f);
	double torsoLen		= GetBoneDist("Bip01 Spine", "Bip01 Neck") - ITOM(2.0f);

	// thigh
	double thighRad		= ITOM(4.0f);
	double thighLen		= GetBoneDist("Bip01 L Thigh", "Bip01 L Calf");

	// calf
	double calfRad		= ITOM(3.0f);
	double calfLen		= GetBoneDist("Bip01 L Calf", "Bip01 L Foot");

	// foot
	double footRad		= ITOM(2.0f);
	double footLen		= footRad * 2.2f; //GetBoneDist("Bip01 L Foot", "Bip01 L Toe0");

	/********************************************************************************/
	// add our bones
	/********************************************************************************/

	// collidible bones
	gr2PhysicsBone* head			= addAxialBone("Bip01 Head", POB(7), headLen, headRad);
	gr2PhysicsBone* shoulders		= addCenterBone("Bip01 Neck", POB(7), chestLen, chestRad);
	gr2PhysicsBone* torso			= addCenterBone("Bip01 Spine2", POB(10), torsoLen, torsoRad);
	gr2PhysicsBone* hips			= addCenterBone("Bip01 Spine", POB(16), hipsLen, hipsRad);
	gr2PhysicsBone* lupperarm		= addAxialBone("Bip01 L UpperArm", POB(4), uarmLen, uarmRad);
	gr2PhysicsBone* lforearm		= addAxialBone("Bip01 L Forearm", POB(3), farmLen, farmRad);
	gr2PhysicsBone* lhand			= addAxialBone("Bip01 L Hand", POB(2), handLen, handRad);
	gr2PhysicsBone* rupperarm		= addAxialBone("Bip01 R UpperArm", POB(4), uarmLen, uarmRad);
	gr2PhysicsBone* rforearm		= addAxialBone("Bip01 R Forearm", POB(3), farmLen, farmRad);
	gr2PhysicsBone* rhand			= addAxialBone("Bip01 R Hand", POB(2), handLen, handRad);
	gr2PhysicsBone* lthigh			= addAxialBone("Bip01 L Thigh", POB(6), thighLen, thighRad);
	gr2PhysicsBone* lcalf			= addAxialBone("Bip01 L Calf", POB(4), calfLen, calfRad);
	gr2PhysicsBone* lfoot			= addAxialBone("Bip01 L Foot", POB(8), footLen, footRad);
	gr2PhysicsBone* rthigh			= addAxialBone("Bip01 R Thigh", POB(6), thighLen, thighRad);
	gr2PhysicsBone* rcalf			= addAxialBone("Bip01 R Calf", POB(5), calfLen, calfRad);
	gr2PhysicsBone* rfoot			= addAxialBone("Bip01 R Foot", POB(4), footLen, footRad);

	// we want some of the bones to collide with our sibling bones
	//lupperarm->SetCollideWithSiblings();
	lforearm->SetCollideWithSiblings();
	lhand->SetCollideWithSiblings();
	//rupperarm->SetCollideWithSiblings();
	rforearm->SetCollideWithSiblings();
	rhand->SetCollideWithSiblings();
	lcalf->SetCollideWithSiblings();
	lfoot->SetCollideWithSiblings();
	rcalf->SetCollideWithSiblings();
	lfoot->SetCollideWithSiblings();

	// debug collider colors
	{
		head->SetColliderColor(colorCYANALPHA);
		shoulders->SetColliderColor(colorBLUEALPHA);
		torso->SetColliderColor(colorCYANALPHA);
		hips->SetColliderColor(colorBLUEALPHA);
		lupperarm->SetColliderColor(colorYELLOWALPHA);
		lforearm->SetColliderColor(colorGREENALPHA);
		lhand->SetColliderColor(colorYELLOWALPHA);
		rupperarm->SetColliderColor(colorYELLOWALPHA);
		rforearm->SetColliderColor(colorGREENALPHA);
		rhand->SetColliderColor(colorYELLOWALPHA);
		lthigh->SetColliderColor(colorMAGENTAALPHA);
		lcalf->SetColliderColor(colorREDALPHA);
		lfoot->SetColliderColor(colorMAGENTAALPHA);
		rthigh->SetColliderColor(colorMAGENTAALPHA);
		rcalf->SetColliderColor(colorREDALPHA);
		rfoot->SetColliderColor(colorMAGENTAALPHA);
	}

	// local adjustments
	head->SetOffset(ITOM(4.0f));
	shoulders->SetOffset(-ITOM(1.0f));
	torso->SetOffset(-ITOM(4.0f));
	hips->SetOffset(-ITOM(1.0f));

	// non-collidible bones
	granny_skeleton* gs = _skeleton->GetSkeleton();

	for(int i = 0; i < gs->BoneCount; i++)
	{
		if(!GetBone(gs->Bones[i].Name))
			addStaticBone(gs->Bones[i].Name);
	}

	// set parent overrides here
	gr2StaticBone* pelvis = GetStaticBone("Bip01 Pelvis");
	if(pelvis) pelvis->SetParent(hips->GetBone());

	// tell our bones to init themselves
	InitBones();

	/********************************************************************************/
	// set friction models
	/********************************************************************************/

	// set up our friction models
	{
		// we don't want the head digging in
		head->SetFrictionModel(1000.0f, 0.0f, 200);

		// scale this down pretty fast
		shoulders->SetFrictionModel(25000.0f, 5000.0f, 800);

		// we want feet to stick at first then losen up fast
		lfoot->SetFrictionModel(20000.0f, 1000.0f, 1000);
		rfoot->SetFrictionModel(20000.0f, 1000.0f, 1000);

		// legs slip pretty fast
		lthigh->SetFrictionModel(25000.0f, 3000.0f, 800);
		lcalf->SetFrictionModel(15000.0f, 2000.0f, 800);
		rthigh->SetFrictionModel(25000.0f, 3000.0f, 800);
		rcalf->SetFrictionModel(15000.0f, 2000.0f, 800);

		// arms and hands don't need much resistance
		lupperarm->SetFrictionModel(15000.0f, 800.0f, 800);
		lforearm->SetFrictionModel(8000.0f, 300.0f, 500);
		lhand->SetFrictionModel(2000.0f, 800.0f, 300);
		rupperarm->SetFrictionModel(15000.0f, 800.0f, 800);
		rforearm->SetFrictionModel(8000.0f, 300.0f, 500);
		rhand->SetFrictionModel(2000.0f, 800.0f, 300);

		// these are the main grips
		torso->SetFrictionModel(30000.0f, 25000.0f, 1000);
		hips->SetFrictionModel(30000.0f, 25000.0f, 1000);
	}

	/********************************************************************************/
	// setup our joints
	/********************************************************************************/

	Vec3d xAxis = Vec3d(1.0f, 0, 0);
	Vec3d yAxis = Vec3d(0, 1.0f, 0);
	Vec3d zAxis = Vec3d(0, 0, 1.0f);

	// setup our bone joints
	if(1)
	{
		dJointID jid = nullptr;

		// link all of the bodies together via joints
		// neck
		addJoint(head, shoulders, dJointTypeUniversal, &zAxis, -30, 10, &yAxis, -20, 20);
		//addJoint(head, shoulders, dJointTypeUniversal, &zAxis, -.4f, .1f, &yAxis, -.2f, .2f);

		// shoulders
		addJoint(lupperarm, shoulders, dJointTypeBall);
		addJoint(rupperarm, shoulders, dJointTypeBall);

		// shoulder restrictors
		// axis 1 must be perpendicular to axis 3
		// axis 2's vector isn't actually used, ODE calculates it based
		// on axis 1 and 3; the hi/lo params are used to restrict the axis though for
		// axis 2; 1 = rotation, 2 = up/down 3 = front back

		// axis1 = up/down, axis2 = front/back, axis3 = twist
		jid = addJoint(lupperarm, shoulders, dJointTypeAMotor, &yAxis, -70, 70, &zAxis, -10, 100, &xAxis, -80, 80);
		//jid = addJoint(lupperarm, shoulders, dJointTypeAMotor, &xAxis, -10, 10, &zAxis, -10, 10, &yAxis, -10, 10);
		//dJointSetAMotorMode(jid, dAMotorEuler);

		// axis1 = up/down, axis2 = front/back, axis3 = twist
		jid = addJoint(rupperarm, shoulders, dJointTypeAMotor, &yAxis, -70, 70, &zAxis, -10, 100, &xAxis, -80, 80);
		//dJointSetAMotorMode(jid, dAMotorEuler);

		lupperarm->SetStiffnessModel(10000.0f, 100.0f, 500);
		rupperarm->SetStiffnessModel(10000.0f, 100.0f, 500);

		// Motors can also be used to accurately model dry (or Coulomb) friction in joints.
		// Simply set the desired velocity to zero and set the maximum force to some constant value - then all joint motion will be impeded by that force.

/*
// elbows
	addJoint(lforearm, lupperarm, dJointTypeHinge, &lforearm->GetPosition(false), &dAxis, -.1f, 1.5f);
	addJoint(rforearm, rupperarm, dJointTypeHinge, &rforearm->GetPosition(false), &uAxis, -.1f, 1.5f);

	lforearm->SetStiffnessModel(1000.0f, 100.0f, 500);
	rforearm->SetStiffnessModel(1000.0f, 100.0f, 500);

	// wrists
	addJoint(lhand, lforearm, dJointTypeHinge, &lhand->GetPosition(false), &fAxis, -.8f, .5f);
	addJoint(rhand, rforearm, dJointTypeHinge, &rhand->GetPosition(false), &bAxis, -.8f, .5f);
*/

		// elbows
		addJoint(lforearm, lupperarm, dJointTypeHinge, &zAxis, -120, 5);
		addJoint(rforearm, rupperarm, dJointTypeHinge, &zAxis, -120, 5);

		lforearm->SetStiffnessModel(5000.0f, 100.0f, 500);
		rforearm->SetStiffnessModel(5000.0f, 100.0f, 500);

		// wrists
		addJoint(lhand, lforearm, dJointTypeHinge, &zAxis, -30, 30);
		addJoint(rhand, rforearm, dJointTypeHinge, &zAxis, -30, 30);

		// chest
		//addJoint(torso, shoulders, dJointTypeHinge, &zAxis, -10, 5);

		// axis1 = twist, axis2 = left/right, axis3 = front/back
		addJoint(torso, shoulders, dJointTypeBall);
		jid = addJoint(torso, shoulders, dJointTypeAMotor, &xAxis, -10, 10, &yAxis, 0, 0, &zAxis, -10, 5);
		//dJointSetAMotorMode(jid, dAMotorEuler);

		// waist
		//addJoint(hips, torso, dJointTypeHinge, &zAxis, -10, 5);

		// axis1 = twist, axis2 = left/right, axis3 = front/back
		addJoint(hips, torso, dJointTypeBall);
		jid = addJoint(hips, torso, dJointTypeAMotor, &xAxis, -10, 10, &yAxis, 0, 0, &zAxis, -20, 5);
		//dJointSetAMotorMode(jid, dAMotorEuler);

		//-.2f, 1.6f, &bAxis, -.2f, 1.0f
		// hips
		addJoint(lthigh, hips, dJointTypeUniversal, &zAxis, -5, 130, &yAxis, -5, 30);
		addJoint(rthigh, hips, dJointTypeUniversal, &zAxis, -5, 130, &yAxis, -30, 5);

		lthigh->SetStiffnessModel(5000.0f, 100.0f, 1000);
		rthigh->SetStiffnessModel(5000.0f, 100.0f, 1000);

		// knees
		addJoint(lcalf, lthigh, dJointTypeHinge, &zAxis, -120, 0);
		addJoint(rcalf, rthigh, dJointTypeHinge, &zAxis, -120, 0);

		lcalf->SetStiffnessModel(5000.0f, 100.0f, 1000);
		rcalf->SetStiffnessModel(5000.0f, 100.0f, 1000);


		// ankles
		addJoint(lfoot, lcalf, dJointTypeHinge, &zAxis, -35, 15);
		addJoint(rfoot, rcalf, dJointTypeHinge, &zAxis, -35, 15);

		lfoot->SetStiffnessModel(3000.0f, 100.0f, 1000);
		rfoot->SetStiffnessModel(3000.0f, 100.0f, 1000);
	}

	// set our root bone
	_rootBone = hips;
}

fpTPSRagdoll::~fpTPSRagdoll()
{
}

void fpTPSRagdoll::AnimInit(gr2AnimTrooper* anim, const Vec3d* force, int timeout, int delay)
{
	if(timeout >= 0 && delay >= 0)
		timeout += delay;

	Init(timeout);

	piMATRIX& t = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleAttMat: anim->myVehicle->PosData.AttMat;
	FLOAT64VECTOR& v = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleVel: anim->myVehicle->PosData.Vel;
	FLOAT64VECTOR& p = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehiclePos: anim->myVehicle->PosData.Pos;

	Vec3d worldPos;
	worldPos.set(p);

	//worldPos = physicsGameToPhysical(worldPos);

	_worldTransform.set(t);
	_worldTransform.setTrans(worldPos);
	_velocity.set(v);
	//_velocity *= .7f;

	if(!anim->myVehicle->ModelData)
		_velocity.x() *= -1.0f;

	// update the skeleton so it reverts to the default pose
	//gr2UpdateModel(_skeleton); may not need this because the model gets update right below

	// figure out which model to init with
	gr2Model* visualModel = anim->model.GetActive();
	gr2Model* corpseModel = anim->model.Get(GR2ANIM_CORPSE_LOD);

	visualModel->Update(true, true);
	_skeleton->Update(true, true);

	// init our visual model
	InitModel(gr2Model::Load(visualModel->GetPath(), _skeleton, 0, 1, -1, false));

	// make this model look like the incoming model
	_country	= anim->myVehicle->VehicleType->WeaponSpec.country();
	_decal		= anim->myVehicle->GetDecal();
	_handle		= (const char*)anim->myVehicle->Handle;
	_corpse		= (corpseModel) ? corpseModel->GetPath() : _model->GetPath(); //gr2GetPath(corpseModel);

	//gr2ModelPersonalize(_model, _country, (SINT32)BRANCH_LAND, (SCHAR*)_handle.c_str(), _decal);

	// init our scenegraph node for drawing
	// need non-osg equivalent
	InitNode(anim->myVehicle->VehicleObjectData->PlaneObject); //&vehicle->Position(), &vehicle->Attitude());

	// make sure we're active
	SetActive(true, timeout);

	// make sure the incoming skeleton is up to date
	anim->skeleton->Update(true, true);

	// update to the current pose
	BonesToBodies(anim->skeleton);

	// add weapon, fixup face, decals, add kit
	Personalize(anim);

	// get our visual bones into position
	UpdateBones();

	// add some initial effects
	AddDrama(force);

	if(delay > 0)
		_delay = frametime + delay;

	_nextImpactSound = frametime;
	_nextContactSound = frametime;

	_material = (anim->myVehicle->isInside) ? piFindMaterialName("interior") : piFindMaterialName("forest1");

	static bool disable = false;

	if(disable)
	{
		for(BoneList::iterator it = _bones.begin(); it != _bones.end(); it++)
			dBodyDisable((*it)->GetBody());
	}
}

void fpTPSRagdoll::AddDrama(const Vec3d* force)
{
	// add some random force some of the time
	if(force && force->length2())
	{
		const int pushCount = 16;

		static const char* pushNames[pushCount] =
		{
			"Bip01 Spine",
			"Bip01 Spine2",
			"Bip01 Head",
			"Bip01 Neck",
			"Bip01 L UpperArm",
			"Bip01 L ForeArm",
			"Bip01 L Hand",
			"Bip01 R UpperArm",
			"Bip01 R ForeArm",
			"Bip01 R Hand",
			"Bip01 L Thigh",
			"Bip01 L Calf",
			"Bip01 L Foot",
			"Bip01 R Thigh",
			"Bip01 R Calf",
			"Bip01 R Foot"
		};

		bool moving = (_velocity.length2() > 1.0f);
		gr2PhysicsBone* bone = nullptr;
		Vec3d localForce = *force;
		Vec3d tempForce;

		if(moving)
			localForce *= .5f;

		for(int i = 0; i < pushCount; ++i)
		{
			bone = GetBone(pushNames[i]);

			if(bone && bone->IsCollidable())
			{
				tempForce.set(localForce);
				tempForce *= (double)((.5f + GetRandom(0.0f, .5f))/(float)pushCount);
				//tempForce /= pushCount;

				if(!moving && i < 2)
				{
					static float afor = 200.0f;
					Vec3d angForce = Vec3d(GetRandom(-afor, afor), GetRandom(-afor, afor), GetRandom(-afor, afor));
					static bool aforce = false;
					if(aforce)
					{
						float a1 = (GetRandom(0.0f, 1.0f) < .5f) ? -afor : afor;
						float a2 = (GetRandom(0.0f, 1.0f) < .5f) ? -afor : afor;
						float a3 = (GetRandom(0.0f, 1.0f) < .5f) ? -afor : afor;
						angForce.set(a1, a2, a3);
					}

					bone->SetAngularVel(angForce);
					bone->SetLinearVel(tempForce);
				}
				else
				{
					bone->SetLinearVel(tempForce);
				}

				//uiChatPrintLine("Add force of %.2f, %.2f, %.2f with a length of %.2f", force->x(), force->y(), force->z(), force->length());
			}
		}
	}

    /*
     TBD - additional joints
    gr2PhysicsBone* lforearm	= GetBone("Bip01 L Forearm");
    gr2PhysicsBone* rforearm	= GetBone("Bip01 R Forearm");
    gr2PhysicsBone* pelvis		= GetBone("Bip01 Pelvis");
     */
    
	gr2PhysicsBone* lhand		= GetBone("Bip01 L Hand");
	gr2PhysicsBone* rhand		= GetBone("Bip01 R Hand");
	gr2PhysicsBone* lfoot		= GetBone("Bip01 L Foot");
	gr2PhysicsBone* rfoot		= GetBone("Bip01 R Foot");

	// if we have a weapon, attach it to our hand right hand
	if(_attached && GetRandom(0.0f, 1.0f) < .8f)
	{
		_weapJoint = dJointCreateFixed(gr2PhysicsBody::DefaultWorld(), 0);
		dJointAttach(_weapJoint, rhand->GetBody(), _attached->GetBone(0)->GetBody());
		dJointSetFixed(_weapJoint);
	}

	Vec3d hforce = Vec3d(0,0,0);
	Vec3d tempPos;
	static float hfmod = 15.0f;

	tempPos = lfoot->GetPosition(false);
	hforce = lhand->GetPosition(false) - tempPos;
	hforce.normalize();
	hforce *= GetRandom(3.0f, hfmod);
	hforce += lhand->GetLinearVel();
	lhand->SetLinearVel(hforce);
	//lforearm->SetLinearVel(hforce);

	tempPos = rfoot->GetPosition(false);
	hforce = rhand->GetPosition(false) - tempPos;
	hforce.normalize();
	hforce *= GetRandom(3.0f, hfmod);
	hforce += rhand->GetLinearVel();
	rhand->SetLinearVel(hforce);
	//rforearm->SetLinearVel(hforce);

	// determine our mods
	double speed = _velocity.length();

	// faster we go, the less friction we apply
	// 6 should be the sprint speed for troops
	float frictMod = 1.0f - ((float)speed / 6.0f);
	float stiffMod = frictMod;

	for(BoneList::iterator it = _bones.begin(); it != _bones.end(); it++)
	{
		if((*it)->IsCollidable())
		{
			(*it)->SetMods(stiffMod, frictMod);
			(*it)->SetLinearVel(_velocity);
		}
	}

	float test = GetRandom(0.0f, 1.0f);

	if(test < .2f)
	{
		gr2PhysicsBone* arm = GetBone("Bip01 L Forearm");
		arm->SetMods(stiffMod * GetRandom(0.0f, .5f), frictMod);
	}
	else if( test < .4f)
	{
		gr2PhysicsBone* arm = GetBone("Bip01 R Forearm");
		arm->SetMods(stiffMod * GetRandom(0.0f, .5f), frictMod);
	}
	else if( test < .6f)
	{
		gr2PhysicsBone* calf = GetBone("Bip01 L Calf");
		calf->SetMods(stiffMod * GetRandom(0.0f, .5f), frictMod);
	}
	else if( test < .8f)
	{
		gr2PhysicsBone* calf = GetBone("Bip01 R Calf");
		calf->SetMods(stiffMod * GetRandom(0.0f, .5f), frictMod);
	}
}

void fpTPSRagdoll::Personalize(gr2AnimTrooper* anim)
{
	anim->skeleton->TransferAttachments(_skeleton);

	// for testing
	_loadWeapShapes();

	if(anim->currentPose != TPS_POSETYPE_SWIM &&
	   anim->currentWeapon >= 0 &&
	   anim->currentWeapon < MAX_NUM_FPS_WEAPONS &&
	   cfgPrefsGetBool("app.ragdollweapons", true))
	{
		fpsWeaponTypes fpsWeapType = anim->weaponArray[anim->currentWeapon];

		const WeaponShape* weapCollider = _getWeaponShape(anim->currentWeaponType, fpsWeapType);

		// add our 3rd person weapon model
		if(weapCollider && anim->weapons[anim->currentWeapon].GetActive())
			Attach(new fpTPSWeapon(anim, nullptr, weapCollider));
	}
}

	// update
void fpTPSRagdoll::BeginStep()
{
	if(frametime > _delay)
	{
		if(IsActive())
			_lastCollision = 0;

		/*
		int end = _initTime + 1000;
		float scaler = float(frametime - _initTime) / float(end - frametime);

		if(scaler < 0.0f) scaler = 0;
		if(scaler > 1.0f) scaler = 1.0f;

		float velocity = 1.0f + (scaler * 150.0f);

		dJointSetHingeParam(getBone("Bip01 L Forearm")->getJoint(), dParamFMax, velocity);
		dJointSetHingeParam(getBone("Bip01 R Forearm")->getJoint(), dParamFMax, velocity);
		dJointSetHingeParam(getBone("Bip01 L Calf")->getJoint(), dParamFMax, velocity);
		dJointSetHingeParam(getBone("Bip01 R Calf")->getJoint(), dParamFMax, velocity);
		*/

		gr2PhysicsModel::BeginStep();
	}
}

void fpTPSRagdoll::Update()
{
	if(frametime > _delay)
	{
		dJointAddHingeTorque(GetBone("Bip01 L Forearm")->GetJoint(), tforce);
		dJointAddHingeTorque(GetBone("Bip01 R Forearm")->GetJoint(), tforce);
		dJointAddHingeTorque(GetBone("Bip01 L Calf")->GetJoint(), tforce);
		dJointAddHingeTorque(GetBone("Bip01 R Calf")->GetJoint(), tforce);

		gr2PhysicsModel::Update();
	}
}

#define MAX_WEAP_RELEASE_VEL 3.0f

void fpTPSRagdoll::EndStep()
{
	if(frametime > _delay)
	{
		gr2PhysicsModel::EndStep();

		if(_weapJoint && GetRandom(0.0f, 1.0f) < .05f)
		{
			if(_attached)
			{
				Vec3d lvel = _attached->GetBone(0)->GetLinearVel();

				//uiChatPrintLine("Weapon release velocity is: %.2f", lvel.length());

				if(lvel.length2() > MAX_WEAP_RELEASE_VEL * MAX_WEAP_RELEASE_VEL)
				{
					lvel.normalize();
					lvel *= MAX_WEAP_RELEASE_VEL;

					// clamp linear velocity
					_attached->GetBone(0)->SetLinearVel(lvel);
				}
			}

			dJointDestroy(_weapJoint);
			_weapJoint = nullptr;
		}
	}
}

void fpTPSRagdoll::Deactivate()
{
	if(_playerRagdoll == this)
		_playerRagdoll = nullptr;

	_lastCollision = 0;

	// calculate last collision
	BoneList::iterator it = _bones.begin();
	for(; it != _bones.end(); ++it)
		_lastCollision |= (*it)->LastCollisions();

#ifdef USE_CORPSES
	if(colliderIsType(_lastCollision, COLLIDER_CONTACT_MASK) && !killed)
	{
		// don't leave a corpse if we're on a vehicle
		if(!colliderIsType(_lastCollision, COLLIDER_DRIVABLE_MASK))
		{
			// we go ahead and show a corpse for a short period anyway so the ragdoll doesn't just disappear real quick
			SINT32 lifeTime	= (!cfgPrefsGetBool("app.corpses", true)) ? 20000: 0;
			Corpse* c = gr2AnimAddCorpse(this, "Bip01 Spine", true, lifeTime);

			if(c)
			{
				// if our last hit was a building or bridge, look up the TO object so we can check damage states later
				if(colliderIsType(_lastCollision, COLLIDER_TRN_BUILDING|COLLIDER_TRN_BRIDGE))
				{
					// look up its TO object
					gr2AnimCorpseTO(c);
				}

				// make this corpse have the same face and decals as the incoming model
				//gr2ModelPersonalize(c->model, _country, (SINT32)BRANCH_LAND, (SCHAR*)_handle.c_str(), _decal);

				if((_lastCollision ^ COLLIDER_DRIVABLE_MASK) == 0)
				{
					// we're on a vehicle
					// let the corpse know
				}
			}
		}
	}
#endif

	gr2PhysicsModel::Deactivate();

	Expire();

	// unattach from the weapon if we haven't already
	Unattach();
}

void fpTPSRagdoll::OnUnattach(gr2PhysicsModel* m)
{
	// break the joint if one still exists
	if(_weapJoint)
		dJointDestroy(_weapJoint);

	_weapJoint = nullptr;

	gr2PhysicsModel::OnUnattach(m);
}

bool fpTPSRagdoll::ProcessFlag(unsigned int f, physicsObject* o)
{
	gr2PhysicsBone* bone = dynamic_cast<gr2PhysicsBone*>(o);

	if(bone)
	{
		if(f & COLLIDER_CONTACT_MASK)
		{
			if(frametime >= _nextImpactSound)
			{
				static double minVel = 3.2f;
				Vec3d bv = bone->GetLinearVel();

				if(bv.length2() >= (minVel * minVel))
				{
					int id = fxSndAddFiniteWorldSound("sounds/troop/body/impact.wav",
													 0,
													 15,
													 IDE_TP_PRIORITY_LOW,
													 bone->getPosition());

					fxSndSetVolume(id, .8f);

					if(!(_allCollisions & COLLIDER_CONTACT_MASK) && GetRandom(0.0f, 1.0f) < .5f)
					{
						fxSndAddFiniteWorldSound("sounds/troop/body/grunt.wav",
														 3,
														 15,
														 IDE_TP_PRIORITY_LOW,
														 bone->getPosition());
					}

					_nextImpactSound = frametime + 1200;
				}
			}

			if(frametime >= _nextContactSound)
			{
				static double minVel = .25f;
				Vec3d bv = bone->GetLinearVel();

				if(bv.length2() >= (minVel * minVel) && GetRandom(0.0f, 1.0f) < .5f)
				{
					int numSounds = 4;

#ifdef BETA_RELEASE
					numSounds = _numFallSounds;
#endif

					int id = GetRandom(0, numSounds);
					char path[50+1];

					snprintf(path, 50, "sounds/troop/body/fall%d.wav", id);

					fxSndAddFiniteWorldSound(path,
														 0,
														 20,
														 IDE_TP_PRIORITY_LOW,
														 bone->getPosition());
				}

				_nextContactSound = frametime + 500;
			}
		}
	}

	return gr2PhysicsModel::ProcessFlag(f, o);
}

void fpTPSWeapon::BeginStep()
{
	if(IsActive())
		_lastCollision = 0;

	gr2PhysicsModel::BeginStep();
}

void fpTPSWeapon::EndStep()
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

fpTPSWeapon::fpTPSWeapon(gr2AnimTrooper* anim, const Vec3d* force, const WeaponShape* collider) :
	gr2PhysicsModel(),
	_damp(false)
{
	// defaults to expiring when we no longer have enabled bones
	int expires = 10000;

	//gr2Model* weaponModel = anim->weaponModels[anim->currentWeapon][TPS_LOD_2];
	//if(!weaponModel) weaponModel = anim->weaponModels[anim->currentWeapon][TPS_LOD_1];
	gr2Model* weaponModel = anim->weapons[anim->currentWeapon].GetActive();

	// init our weapon's skeleton
	InitSkeleton(gr2GetPath(weaponModel));

	// collidible bones
	granny_skeleton* gs = gr2GetSkeleton(_skeleton);

	for(int i = 0; i < gs->BoneCount; i++)
	{
		if(i == 0)
		{
			_rootBone = addBone(gs->Bones[i].Name);

			if(!collider)
			{
				// lookup shape info
				fpsWeaponTypes weapon = anim->weaponArray[anim->currentWeapon];
				collider = _getWeaponShape(anim->currentWeaponType, weapon);
			}

			// make our collider
			if(makeCollider(collider))
			{
				_rootBone->Init();
				_rootBone->SetEnabled(false);
				_rootBone->SetFrictionModel(1000.0f, 500.0f, 500);

				// keep up us from rotating to fast
				//dBodySetFiniteRotationMode(bone->getBody(), 1);

				dBodySetMaxAngularSpeed(_rootBone->GetBody(), 100000.0);
			}
			else
			{
				expires = 1;
			}
		}
		else
		{
			gr2StaticBone* bone = addStaticBone(gs->Bones[i].Name);
			if(bone) bone->Init();
		}
	}

	Init();

	piMATRIX& t = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleAttMat: anim->myVehicle->PosData.AttMat;
    
    /*
     TBD - incorporate velocity?  why commented out?
     
	FLOAT64VECTOR& v = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleVel: anim->myVehicle->PosData.Vel;
     */
	FLOAT64VECTOR& p = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehiclePos: anim->myVehicle->PosData.Pos;

	Vec3d worldPos;
	worldPos.set(p);

	//worldPos = physicsGameToPhysical(worldPos);

	_worldTransform.set(t);
	_worldTransform.setTrans(worldPos);
	//_velocity.set(v);
	//_velocity *= .01f;

	_model = _skeleton;

	// init our scenegraph node for drawing
	// need non-osg equivalent
	InitNode(anim->myVehicle->VehicleObjectData->PlaneObject); //&vehicle->Position(), &vehicle->Attitude());

	// make sure we're active
	SetActive(true, expires);

	// update to the current pose
	//InitPose(anim->weapons[anim->currentWeapon].GetActive());
	BonesToBodies(anim->weapons[anim->currentWeapon].GetActive());

	// add some random force some of the time
	/*
	if(false && force && force->length2())
	{
		Vec3d weapForce(*force);

		weapForce += _owner->getVelocity();
		weapForce *= .2f;

		if(_rootBone && _rootBone->isCollidable())
		{
			_rootBone->SetLinearVel(weapForce);

			const float aforce = 1.0f;

			weapForce.x() = GetRandom(-aforce, aforce);
			weapForce.y() = GetRandom(-aforce, aforce);
			weapForce.z() = GetRandom(-aforce, aforce);

			_rootBone->SetAngularVel(*force);
		}
	}
	*/

	UpdateBones();

	_material = (anim->myVehicle->isInside) ? piFindMaterialName("interior") : piFindMaterialName("forest1");
}

fpTPSWeapon::~fpTPSWeapon()
{
	Unattach();

	_lastCollision = 0;

	// calculate last collision
	BoneList::iterator it = _bones.begin();
	for(; it != _bones.end(); ++it)
		_lastCollision |= (*it)->LastCollisions();

	Deactivate();

	// we may need to delete this model's geom and body?

#ifdef USE_CORPSES
	if(colliderIsType(_lastCollision, COLLIDER_CONTACT_MASK) && !killed)
	{
		if(!colliderIsType(_lastCollision, COLLIDER_DRIVABLE_MASK) && cfgPrefsGetBool("app.corpses", true))
		{
			Corpse* c = gr2AnimAddCorpse(this, nullptr, false, 30000); // , bone->getName(), false
			c->shadow = false;
		}
	}
#endif
}

// bone to bodies
void fpTPSWeapon::InitPose(gr2Model* poseModel)
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

void fpTPSWeapon::damp()
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

bool fpTPSWeapon::makeCollider(const WeaponShape* collider)
{
	if(!_rootBone || !collider)
		return false;

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
			// rotate it along the specified axis so that it fits our body better
			Vec3d axis(shape.rotation.x(), shape.rotation.y(), shape.rotation.z());
			Matrixd rot = Matrixd::rotate(PI * .5f, axis); // mlDEG_TO_RAD(shape.rotation.w())
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

void fpTPSWeapon::OnExpire(DRAW_STATE& state)
{
	state = gr2PhysicsModel::DS_DELETE;
}

void fpTPSWeapon::OnRemove()
{
	delete (fpTPSWeapon*)this;
}

bool fpTPSWeapon::ProcessFlag(unsigned int f, physicsObject* o)
{
	//* for now don't play any sounds; easy to dev trooper sounds
	GR2PhysicsBoneRef bone = dynamic_cast<GR2PhysicsBoneRef>(o);

	if(bone)
	{
		if((f & COLLIDER_CONTACT_MASK) && !(_allCollisions & COLLIDER_CONTACT_MASK))
		{
			static double minVel = 1.5f;
			Vec3d bv = bone->GetLinearVel();

			if(bv.length2() >= (minVel * minVel))
			{
				fxSndAddFiniteWorldSound("sounds/troop/body/fall6.wav",
												 0,
												 15,
												 IDE_TP_PRIORITY_LOW,
												 bone->getPosition());
			}

			Unattach();
		}
	}
	//*/

	return gr2PhysicsModel::ProcessFlag(f, o);
}



fpTPSKitObject::fpTPSKitObject(gr2AnimTrooper* anim, const Vec3d* force, const WeaponShape* collider) :
	gr2PhysicsModel(),
	_damp(false)
{
	// defaults to expiring when we no longer have enabled bones
	int expires = 10000;

	//gr2Model* weaponModel = anim->weaponModels[anim->currentWeapon][TPS_LOD_2];
	//if(!weaponModel) weaponModel = anim->weaponModels[anim->currentWeapon][TPS_LOD_1];
	gr2Model* weaponModel = anim->weapons[anim->currentWeapon].GetActive();

	// init our weapon's skeleton
	InitSkeleton(gr2GetPath(weaponModel));

	// collidible bones
	granny_skeleton* gs = gr2GetSkeleton(_skeleton);

	for(int i = 0; i < gs->BoneCount; i++)
		addBone(gs->Bones[i].Name);

	// setup our collision capsules
	BoneList::iterator it = _bones.begin();

	for(; it != _bones.end(); ++it)
	{
		gr2PhysicsBone* bone = *it;

		// find root bone
		if(bone->GetBone() == 0)
		{
			_rootBone = bone;

			if(!collider)
			{
				// lookup shape info
				fpsWeaponTypes weapon = anim->weaponArray[anim->currentWeapon];
				collider = _getWeaponShape(anim->currentWeaponType, weapon);
			}

			// make our collider
			if(makeCollider(collider))
			{
				bone->Init();
				bone->SetEnabled(false);
				bone->SetFrictionModel(1000.0f, 500.0f, 500);

				// keep up us from rotating to fast
				//dBodySetFiniteRotationMode(bone->getBody(), 1);

				dBodySetMaxAngularSpeed( bone->GetBody(), 100000.0);
			}
			else
			{
				expires = 1;
			}

			break;
		}
	}

	Init();

	piMATRIX& t = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleAttMat: anim->myVehicle->PosData.AttMat;
    /*
      TBD - incoporate velocity.  Why commented out below?
     
	FLOAT64VECTOR& v = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehicleVel: anim->myVehicle->PosData.Vel;
     */
    
	FLOAT64VECTOR& p = (anim->myVehicle->ModelData) ? anim->myVehicle->ModelData->VehiclePos: anim->myVehicle->PosData.Pos;

	Vec3d worldPos;
	worldPos.set(p);

	worldPos = physicsGameToPhysical(worldPos);

	_worldTransform.set(t);
	_worldTransform.setTrans(worldPos);
	//_velocity.set(v);
	//_velocity *= .01f;

	_model = _skeleton;

	// init our scenegraph node for drawing
	// need non-osg equivalent
	InitNode(anim->myVehicle->VehicleObjectData->PlaneObject); //&vehicle->Position(), &vehicle->Attitude());

	// make sure we're active
	SetActive(true, expires);

	// update to the current pose
	//InitPose(anim->weaponModels[anim->currentWeapon][anim->currentLOD]);
	InitPose(anim->weapons[anim->currentWeapon].GetActive());

	// add some random force some of the time
	/*
	if(false && force && force->length2())
	{
		Vec3d weapForce(*force);

		weapForce += _owner->getVelocity();
		weapForce *= .2f;

		if(_rootBone && _rootBone->isCollidable())
		{
			_rootBone->SetLinearVel(weapForce);

			const float aforce = 1.0f;

			weapForce.x() = GetRandom(-aforce, aforce);
			weapForce.y() = GetRandom(-aforce, aforce);
			weapForce.z() = GetRandom(-aforce, aforce);

			_rootBone->SetAngularVel(*force);
		}
	}
	*/

	UpdateBones();

	_material = (anim->myVehicle->isInside) ? piFindMaterialName("interior") : piFindMaterialName("forest1");
}

fpTPSKitObject::~fpTPSKitObject()
{
	Unattach();

	_lastCollision = 0;

	// calculate last collision
	BoneList::iterator it = _bones.begin();
	for(; it != _bones.end(); ++it)
		_lastCollision |= (*it)->LastCollisions();

	Deactivate();

	// we may need to delete this model's geom and body?

#ifdef USE_CORPSES
	if(colliderIsType(_lastCollision, COLLIDER_CONTACT_MASK) && !killed)
	{
		if(!colliderIsType(_lastCollision, COLLIDER_DRIVABLE_MASK) && cfgPrefsGetBool("app.corpses", true))
		{
			Corpse* c = gr2AnimAddCorpse(this, nullptr, false, 30000); // , bone->getName(), false
			c->shadow = false;
		}
	}
#endif
}

void fpTPSKitObject::BeginStep()
{
	if(IsActive())
		_lastCollision = 0;

	gr2PhysicsModel::BeginStep();
}

void fpTPSKitObject::EndStep()
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

// bone to bodies
void fpTPSKitObject::InitPose(gr2Model* poseModel)
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

void fpTPSKitObject::damp()
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

bool fpTPSKitObject::makeCollider(const WeaponShape* collider)
{
	if(!_rootBone || !collider)
		return false;

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

void fpTPSKitObject::OnExpire(DRAW_STATE& state)
{
	state = gr2PhysicsModel::DS_DELETE;
}

void fpTPSKitObject::OnRemove()
{
	delete (fpTPSWeapon*)this;
}

bool fpTPSKitObject::ProcessFlag(unsigned int f, physicsObject* o)
{
	//* for now don't play any sounds; easy to dev trooper sounds
	GR2PhysicsBoneRef bone = dynamic_cast<GR2PhysicsBoneRef>(o);

	if(bone)
	{
		if((f & COLLIDER_CONTACT_MASK) && !(_allCollisions & COLLIDER_CONTACT_MASK))
		{
			static double minVel = 1.5f;
			Vec3d bv = bone->GetLinearVel();

			if(bv.length2() >= (minVel * minVel))
			{
				fxSndAddFiniteWorldSound("sounds/troop/body/fall6.wav",
												 0,
												 15,
												 IDE_TP_PRIORITY_LOW,
												 bone->getPosition());
			}

			Unattach();
		}
	}
	//*/

	return gr2PhysicsModel::ProcessFlag(f, o);
}


#ifdef BETA_RELEASE
void fpTPSSetFallSounds(int numSounds)
{
	_numFallSounds = numSounds;
}
void fpTPSSetFrictionModel(int start, int stop, int duration)
{
	RagdollList::iterator it = _ragdolls.begin();

	for(; it != _ragdolls.end(); ++it)
	{
		fpTPSRagdoll* ragdoll = *it;

		gr2PhysicsModel::BoneList& bones = ragdoll->GetBones();
		gr2PhysicsModel::BoneList::iterator bit = bones.begin();

		for(; bit != bones.end(); ++bit)
			(*bit)->SetFrictionModel((float)start, (float)stop, duration);
	}
}
#endif

#ifdef TPS_ROPE
static bool _rodeDraw(OBJECT* o);
static OBJECT* _getRopeObject();

// head of rope
fpTPSRope::fpTPSRope(dBody* anchor) :
	physicsBody(COLLIDER_WEAPON, COLLIDER_HARDTERRAIN_MASK),
	_node(nullptr),
	_parent(nullptr),
	_child(nullptr),
	_links(0),
	_nextLinkCheck(0)
{
	double length	= ITOM(12.0f);

	// setup our game object
	_createNode();

	Vec3d velocity = Vec3Ode(anchor->getLinearVel());
	Vec3d jointPos = Vec3Ode(anchor->getPosition());
	Vec3d position	 = jointPos;

	//position = physicsPhysicalToGame(position);

	Vec3d up(0,0,-1.0f);

	velocity.normalize();

	Matrixd rotate;
	rotate.makeRotate(up, velocity);

	// reverse it
	velocity *= -1.0f;

	// scale it by length/2
	velocity *= (length/2.0f);

	// move it back
	position += velocity;

	// setup our physics
	_initBody(position, rotate);

	// update our game object
	_updateNode();

	//setRotation(QuatFromAxisAndAngle(Ammo->Velocity,AmmoType->GrenadeOrient));
	//setRestitution(Bounce);
	//setModelPosition(Position);
	//setLinearVel(Velocity);

	// connect it to our anchor point
	Connect(_body, anchor, jointPos);

	// add it to the physics loop
	physicsAddObject(this);
}

// subsequent rope links
fpTPSRope::fpTPSRope(fpTPSRope* rope) :
	physicsBody(COLLIDER_WEAPON, COLLIDER_HARDTERRAIN_MASK),
	_node(nullptr),
	_parent(nullptr),
	_child(nullptr),
	_links(0),
	_nextLinkCheck(0)
{
	// setup the link
	SetParent(rope);
	rope->SetChild(this);

	double length	= ITOM(12.0f);

	// setup our game object
	_createNode();

	Vec3d velocity = rope->GetLinearVel();
	Vec3d jointPos = rope->GetPosition();
	Vec3d position	 = jointPos;

	//position = physicsPhysicalToGame(position);

	Vec3d up(0,0,-1.0f);

	velocity.normalize();

	Matrixd rotate;
	rotate.makeRotate(up, velocity);

	// reverse it
	velocity *= -1.0f;

	// scale it by length/2
	velocity *= (length/2.0f);

	// move it back
	jointPos += velocity;
	position += (velocity * 2.0f);

	// setup our physics
	_initBody(position, rotate);

	// update our game object
	_updateNode();

	//setRotation(QuatFromAxisAndAngle(Ammo->Velocity,AmmoType->GrenadeOrient));
	//setRestitution(Bounce);
	//setModelPosition(Position);
	//setLinearVel(Velocity);

	// connect it to our anchor point
	Connect(_body, rope->Body(), jointPos);

	// add it to the physics loop
	physicsAddObject(this);
}

fpTPSRope::~fpTPSRope()
{
}

void fpTPSRope::Draw()
{
	double halfLength	= 1.0f;//ITOM(12.0f)/2.0f;

	// setup endpoints
	piVECTOR start = {0, 0, halfLength};
	piVECTOR stop = {0, 0, -halfLength};

	// draw our rope line here
	draw3dLine(&start, &stop, colorBLUEALPHA);

	//if(_child)
	//	_child->Draw();
}

fpTPSRope* fpTPSRope::Tail()
{
	if(_child)
		return _child->Tail();

	return this;
}

void fpTPSRope::EndStep()
{
	physicsBody::EndStep();

	// move our game object to our new position
	_updateNode();

	if(!_parent && _links < 30 && frametime >= _nextLinkCheck) // we're the head
	{
		Vec3d lvel = GetLinearVel();

		if(lvel.length2() >= (3.0f * 3.0f))
		{
			// add another link
			fpTPSRope* newLink = new fpTPSRope(Tail());

			_links++;
			_nextLinkCheck = frametime + 50;
		}
	}
}

dJointID fpTPSRope::Connect(dBody* from, dBody* to, Vec3d pos)
{
	dWorldID world		= DefaultWorld();
	dJointID jointID	= dJointCreateBall(world, 0);

	dJointAttach(jointID, from->id(), to->id());
	dJointSetBallAnchor(jointID, pos.x(), pos.y(), pos.z());

	return jointID;
}

void fpTPSRope::_createNode()
{
	_node = _getRopeObject();
	_node->physicalModel = this;
}

void fpTPSRope::_updateNode()
{
	if(_node)
	{
		Matrixf transform = GetTransform();
		Vec3d position = transform.getTrans();

		const FLOAT64VECTOR* woo = trnGetWorldOriginOffsetPtr();

		position.x() -= woo->x;
		position.y() -= woo->y;
		position.z() -= woo->z;

		_node->pos.x = position.x();
		_node->pos.y = position.y();
		_node->pos.z = position.z();

		transform.setTrans(position);
		transform.get(_node->mat);
	}
}

void fpTPSRope::_initBody(Vec3d position, Matrixd rotation)
{
	double length	= ITOM(12.0f);
	double radius	= ITOM(.5f);
	double weight	= LTOK(.2f);

	// create our collider geom
	// create capsule
	// create transform geom
	// adjust transform down so we're centered on the position
	// add it to our collider
	dGeomID shape = dCreateGeomTransform(collider->space());

	// our actual collision capsule
	dGeomID capsule = dCreateCapsule(0, radius, length);

	// capsules are oriented up and rotated to the Y below
	// setting the position to the x offset achieves a
	// final the result we want which is a capsule moved
	// along the X axis after its rotated by the shape
	// transform geom below
	dGeomSetPosition(capsule, 0, 0, -(length/2.0f));

	dGeomTransformSetCleanup(shape, 1);
	dGeomTransformSetInfo(shape, 1);
	dGeomTransformSetGeom(shape, capsule);

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
		Matrixf rot = Matrixf::rotate((float)angle, boneAxis);
		dMatrix3 dmat;
		MatrixTodMatrix(rot, dmat);
		dGeomSetRotation(shape, dmat); // should this be capsule and not shape?
	}
	*/

	_geom->addGeom(shape);
	//_geom->createCapsule(radius, length);

	// attach our body to the capsule collider
	_geom->attachToBody(_body);

	// make sure our collider knows about it
	collider->addGeom(shape, _geom->getCategoryMask(), _geom->getCollideMask());

	//setAngularVel(angvel);

	/*
	setMass(Weight, ModelCM,
				AmmoType->Ixx,
				AmmoType->Iyy,
				AmmoType->Izz,
				AmmoType->Ixy,
				AmmoType->Ixz,
				AmmoType->Iyz
				AmmoType->Object->child);
	*/

	dMassSetSphereTotal(&_mass, weight, length/2);
	dBodySetMass(_body->id(), &_mass);

	_body->setMass(&_mass);

	// rotate our body into position along the head's velocity
	Matrixf transform = GetTransform(false);

	transform.setTrans(0,0,0);
	transform *= rotation;

	// set our body's rotation
	SetTransform(transform);

	// need to adjust position by half the length -velocity normalized
	SetPosition(position, false);
}

OBJECT* _getRopeObject()
{
	OBJECT* newObject, *childObject;

	obmainCreateObjectFull (&newObject, "ropeObject", 0, 0);

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

	newObject->drawcallback = _rodeDraw; //gr2PhysicsDrawCallback;
	newObject->cullcallback = nullptr;//gr2AnimCullCallback;

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

bool _rodeDraw(OBJECT* o)
{
	fpTPSRope* rope = dynamic_cast<fpTPSRope*>(o->physicalModel);

	if(rope)
	{
		static SINT32 matID = -1;

		if(matID == -1)
			matID = piFindMaterialName("forest1");

		// set out material
		piSetActiveMaterial(matID);
		piSetWorldMatrix(&rope->Node()->mat);
		piSetRenderState(piRENDERSTATE_CULLMODE, piCULL_CW);

		rope->Draw();

		piSetRenderState(piRENDERSTATE_CULLMODE, piCULLMODE_DEFAULT);
		oglSetWorldMatrix(&IdentityMatrix);
	}

	return true;
}

#endif
