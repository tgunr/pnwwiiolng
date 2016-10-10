#include <syscfg.h> // Path is project specific.

#include "fp/fpFPSinternal.h"
#include "fp/fpPPO.h"
#include "client/clientSTO.h"
#include "xml/XMLLoader2.h"
#include "physics/colliderMain.h"
#include "math/matheul.h"
#include "ogrp/ogrptype.h"
#include "trn/trncd.h"
#include "trn/trnfx.h"
#include "trn/trnmain.h"
#include "draw/draw3d.h"
#include "imap/ww2func.h"
#include "ui/uiGameEvents.h"
#include "ui/uiMain.h"
#include "ogl/oglMaterial.h"
#include "ob/obMgr.h"
#include "trn/trnop.h"
#include "ob/obpool.h"
#include "pers/persmain.h"
#include "math/Matrixf.h"
#include "math/Matrixd.h"
#include "math/math3d.h"
#include "ml/mlutility.h"
#include "trn/trnvars.h"
#include "ob/obdraw.h"
#include "fp/fpFPS.h"
#include "spt/sptmain.h"
#include "pers/persmain.h"
#include "map/mapArenaDescription.h"
#include "cfg/cfgPrefs.h"
#include "wwii/wwiiPermissions.h"
#include "client/GameClient.h"
#include "wwii/wwiiPPOLoader.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "client/GameClient.h"
#include "weap/weapSpec.h"

// WWIIOL specific.
#include "strt/strtCPFlags.h"
#include "omg/omgChat.hxx"

#define PPO_SPEC "vehicles/ground/groundman/ppo.xml"

using PPO_SINK_MAP = std::map<UINT32,SINT32>;

extern PPOSpecLoader specLoader;

std::vector<PPOSpec *> activePPOList;
UINT16 activePPOIndex = 0;

static int _badMat			= -1;
static int _goodMat			= -1;
static int _offMat			= -1;
static bool _resetMaterials = true;
static std::wstring _fail	= L"";

static PlayerPlacedObject* _placedObject = nullptr;

static PPO_SINK_MAP _ppoSinkMap;

static ref_ptr<oglMaterial> ppomaterial;

static bool _ppoDrawCallback(OBJECT* o);
static bool _inTargetFacilityRange(FLOAT32 distance, SINT32 side);
static bool _checkFacilityRange(FLOAT32 distance, StratDB::StratObject* so, SINT32 side, bool friendly);
static bool _fbCheck(vehVehicle* player, StratDB::StratObject* fac, FLOAT32 radius);

SINT32 globalMSCoolDownEnd = 0;

struct AddDrawCallback
{
    OBJECT* o;

    void SetCallback(OBJECT* o)
    {
        if(o)
        {
            if(!(o->flags & OBJECT_NOTOBJECT))
            {
                o->flag2 |= OBJECT_HASALPHA;
                o->drawcallback = _ppoDrawCallback;
            }

            SetCallback(o->child);
            SetCallback(o->attached);
            SetCallback(o->sibling);
        }
    }

    void Apply()
    {
        SetCallback(o);
    }
};

PlayerPlacedObject::PlayerPlacedObject(PPOSpec* s) :
	obj(nullptr),
	valid(false),
	lastValid(false),
	failOnInvalid(false),
	interruptNextUpdate(false),
	pauseBuild(false),
	abox(nullptr),
	spec(s),
	buildAt(0),
	lastUpdate(0),
	buildUI(nullptr),
	updateCallback(nullptr),
	executeCallback(nullptr),
	validityCallback(nullptr)
{
	if(!spec) return;

	const char* modelName = spec->modelName.c_str();

	//collider = new colliderQuery(0, false);

	//collider->addGeomToCollideWith((dGeomID)colliderWorldSpace());
	//collider->setMaxContactsPerHit(16);
	//collider->setNormalFilter(0.0f, 0.0f, 1.0f);

	//vehVehicle* player = playerGetVehicle();
	//if(!player) return;

	//const FLOAT64ATT& att = player->ModelData->VehicleAtt;
	//const FLOAT64VECTOR& pos = player->ModelData->VehiclePos;

	piMATRIX transform = IdentityMatrix;

	//GetTransform(transform);

	PoolModel* masterModel = obmgrGetPoolModel(modelName);
	if(!masterModel) return;

	obj = obmgrCreateModelInstance(modelName, transform, true);
	if(!obj) return;

	struct AddDrawCallback addcallback;
	addcallback.o = obj;
	addcallback.Apply();

	// update our object's bounding box
	trnopRecalcBoundsAndNormals(obj); // COMMENT: possible crash in ldmdlCalcNormals2() in here, #500+ in 1.33.4

	//obmainSetFlag2InObjectTree(obj, OBJECT_HASALPHA);
	obmainClearFlagsInObjectTree(obj, OBJECT_DYNASHADOW);

	// look for any country flags in this object
	obmainSetFlag3InObjectTree(obj, OBJECT_HANDLES_COUNTRY);

	vehVehicle* player = persGetPersonaVehiclePtr();

	if(player)
		trnFXSetCountryFlags(obj, (SINT32)player->VehicleType->WeaponSpec.wCountry);

	finalPos.set(0,0,0);
	finalAtt.set(0,0,0);

	// reset the ppo material to be bad at first
	ppomaterial->SetMaterial(_badMat);
}

PlayerPlacedObject::~PlayerPlacedObject()
{
	// make sure and kill the UI if its up
	Interrupt();

	// this call destroys the associated ode collider as well
	if(obj)
		obpoolFreeObjectCallback(obj);

	obj = nullptr;
	valid = false;
	lastValid = false;
}

void PlayerPlacedObject::UpdateColliders()
{
	if(obj == nullptr)
		return;

	BoundBox bbox = BoundBox(obj, FALSE);

	const piVECTOR& bl = bbox.Min();
	const piVECTOR& tr = bbox.Max();

	if(collider.geoms().empty())
	{
		// limit how many contacts we generate
		collider.setMaxContactsPerHit(2);

		dSpaceID space = collider.space();

		// create our ray colliders for placement handling
		for(int i = 0; i < 6; ++i)
			collider.addGeom(dCreateRay(space, 12.0f), 0, COLLIDER_HARDTERRAIN_MASK);

		// now create our box collider for avoidance checks

		// our box transform
		abox = dCreateGeomTransform(space);

		double xLen = fabs(bl.x - tr.x) + (spec->avoidance * 2.0f);
		double yLen = fabs(bl.y - tr.y) + (spec->avoidance * 2.0f);
		double zLen = fabs(bl.z - tr.z) + (spec->avoidance * 2.0f);

		// our actual collision box
		dGeomID box = dCreateBox(0, xLen, yLen, zLen);

		// init our transform geom
		dGeomTransformSetCleanup(abox, 1);
		dGeomTransformSetInfo(abox, 1);

		double zOffset = (zLen - spec->avoidance) * .5f;

		// set our offset position
		dGeomSetPosition(box, 0, 0, zOffset);

		// save our box in the transform geom
		dGeomTransformSetGeom(abox, box);

		// set what we collide with and add it to our collider query
		collider.addGeom(abox, 0, spec->avoidFlags);
	}

	// update all collider positions

	vehVehicle* player = playerGetVehicle();
	if(!player) return;

	Vec3d playerPos				= Vec3d(player->ModelData->VehiclePos);
	Vec3d position				= playerPos;
	Matrixf attitude			= Matrixf(player->ModelData->VehicleAttMat);
	Vec3d rayPos				= Vec3d(0, 1, 0);
	Vec3d dir					= Vec3d(0, 0, -1.0f); // direction pointer
	dGeomID	 geom				= nullptr;
	double viewOffset			= (player->VehicleType->Type == VEH_TYPE_PERSON) ? -spec->viewOffset: spec->viewOffset * 2.0f;
	piVECTOR forward			= {0,(float)viewOffset,0};

	// now orient the vector to our facing
	math3dMultiplyRotation(&forward, &forward, &player->ModelData->VehicleAttMat);

	position.x() += forward.x;
	position.y() += forward.y;
	position.z() += forward.z;

	Vec3d adjust(spec->rayAdjust);

	if(spec->rotation)
	{
		double r = (player->VehicleType->Type == VEH_TYPE_PERSON) ? spec->rotation: -spec->rotation;

		Matrixd rotate;
		rotate.makeRotate(mlDEG_TO_RAD(r), 0, 0, 1.0f);

		attitude = attitude * rotate;
	}

	colliderQuery::dGeomList& geoms = collider.geoms();
	int rayCount = geoms.size() - 1;

	// update our rays, but not the last geom (the avoidance box), we do that when we process the center ray
	for(int i = 0; i < rayCount; i++)
	{
		geom = geoms.at(i);

		// set the ray pos
		switch(i)
		{
			case 0: rayPos.set(0,0,0);							break; // center
			case 1: rayPos.set(-adjust.x(), adjust.y(), 0);		break; // front left
			case 2: rayPos.set(adjust.x(), adjust.y(), 0);		break; // front right
			case 3: rayPos.set(-adjust.x(), -adjust.y(), 0);	break; // back left
			case 4: rayPos.set(adjust.x(), -adjust.y(), 0);		break; // back right
			case 5:
			{
				// this is our line of sight ray
				// it has special handling
				playerPos.z() += 1.0f; // move it up some

				// create our look at pos
				Vec3d losPos = Vec3d(position.x(), position.y(), position.z() + .5f);

				// calculate our look at direction
				Vec3d to = losPos - playerPos;

				// set our line of sight length
				dGeomRaySetLength(geom, to.length());

				// normalize the direction
				to.normalize();

				// convert to physical coords
				playerPos = physicsGameToPhysical(playerPos);

				// set our position and direction
				dGeomRaySet(geom, playerPos.x(), playerPos.y(), playerPos.z(), to.x(), to.y(), to.z());

				// done
				continue;
			}
		}

		if(i == rayCount - 1)
			continue;

		// rotate into world orientation
		rayPos = rayPos * attitude;

		// put us into the world
		rayPos += position;

		// put us into the physical world
		rayPos = physicsGameToPhysical(rayPos);

		// update our ray
		dGeomRaySet(geom, rayPos.x(), rayPos.y(), rayPos.z() + 6.0f, dir.x(), dir.y(), dir.z());

		// when processing the center ray, update our avoidance box collider
		//  too since its positioned directly over our position
		if(i == 0)
		{
			geom = abox;

			dMatrix3 dMat;
			MatrixTodMatrix(attitude, dMat);

			dGeomSetPosition(geom, rayPos.x(), rayPos.y(), rayPos.z());
			dGeomSetRotation(geom, dMat);
		}
	}
}

static bool _foundMaterial(std::vector<std::string>& texnames, const char* texname)
{
	if(texnames.empty())
		return false;

	for(unsigned int i = 0; i < texnames.size(); i++)
	{
		if(!stricmp(texnames[i].c_str(), texname))
			return true;
	}

	return false;
}

static bool _badSurface(UINT32 flags, OBJECT* o)
{
	SINT32 sid = -1;
	mlGetObjectBumpinessAndFriction(nullptr,nullptr,nullptr,nullptr,nullptr,&sid,o);

	return (sid >= 0) ? ((flags & (1<<sid)) == 0): false;
}

// vectors must be normalized
static bool _okAngle(Vec3d& from, Vec3d& to, double tolerance)
{
	if(!tolerance)
		return true;

	double cosTheta = from * to;

	// cap it
	cosTheta = (cosTheta < -1.0f) ? -1.0f : (cosTheta > 1.0f) ? 1.0f : cosTheta;

	double angle = fabs(mlRAD_TO_DEG(acos(cosTheta)));

	// kill any rotation and z adjust if we tilted too far
	// also flag it as invalid
	if(angle > tolerance)
		return false;

	return true;
}

void PlayerPlacedObject::SetInvalid(const char* key, bool force)
{
	if(valid || force)
	{
		valid = false;
		_fail = uiLocString(key);
	}
}

SINT32 PlayerPlacedObject::CoolDown()
{
    SINT32 *coolDownEnd = &globalMSCoolDownEnd;
    
    if (spec && spec->stotype != ServerTrackedObject::_UMS)
        coolDownEnd = &spec->coolDownEnd;
    
	return *coolDownEnd;
}

bool PlayerPlacedObject::CalculatePlacement(piVECTOR& pos, piVECTOR& att, bool checksOnly)
{
	// reset here; various hits will turn this back off below
	valid = true;

	Vec3d worldNormal;
	FLOAT64 zTotal = 0;
	UINT32 zCount = 0;
	dGeomID geom = nullptr;

	UINT32 allHits = 0;
	bool noAdjust = false;
	bool pause = false;
	Vec3d centerNormal;

	if(collider.findCollisions(true))
	{
		collider.sort();

		colliderQuery::dGeomList& geoms = collider.geoms();
		int rayCount = geoms.size() - 1;

		for(int r = 0; r < rayCount; ++r)
		{
			//if(r == rayCount - 1) // our line of sight ray
			//	continue;

			hits.clear();

			geom = collider.geom(r);

			if(collider.getCollisions(geom, hits, false))
			{
				if(r == rayCount - 1 && !valid) // our line of sight ray
					continue;

				colliderQuery::Collision* hit = nullptr;

				for(unsigned int i = 0; i < hits.size(); ++i)
				{
					colliderQuery::Collision& current = hits[i];

					UINT32 cat = dGeomGetCategoryBits(current.g2);

					if(r == rayCount - 1) // our line of sight ray
					{
						if(colliderIsType(cat, COLLIDER_TILE|COLLIDER_TRN_GROUND))
							continue;

						SetInvalid("ppos.cantsee");
						break;
					}

					OBJECT* o = colliderData<OBJECT>(current.g2);

					if(o)
					{
						if(_foundMaterial(spec->badTextures, o->texname))
							SetInvalid("ppos.badlocation");
						if(valid && _badSurface(spec->surfaceFlags, o))
							SetInvalid("ppos.badlocation");
					}

					//UINT32 diff = (cat ^ spec->buildFlags);

					if(current.normal.z() > 0 && (!hit || current.pos.z() > hit->pos.z()))
					{
						if(colliderIsType(cat, spec->buildFlags) || _foundMaterial(spec->okTextures, o->texname))
							hit = &current;
					}

					// save everything we hit
					allHits |= cat;
				}

				if(hit)
				{
					/*
					piVECTOR mark;

					Vec3d mpos = physicsPhysicalToGame(hit->pos);

					mark.x = (FLOAT32)(mpos.x() - WorldOriginOffset.x);
					mark.y = (FLOAT32)(mpos.y() - WorldOriginOffset.y);
					mark.z = (FLOAT32)(-mpos.z() - WorldOriginOffset.z);

					draw3dDebugMarkPoint(&mark, colorGREENALPHA, .5f);
					*/

					if(spec->rayTolerance)
					{
						if(r == 0)
						{
							centerNormal = hit->normal;
						}
						else if(valid)
						{
							if(!_okAngle(centerNormal, hit->normal, spec->rayTolerance))
								SetInvalid("ppos.badangle");
						}
					}

					worldNormal = worldNormal + hit->normal;
					zTotal += hit->pos.z();
					++zCount;
				}
				else if(r < rayCount - 1) // don't consider the line of sight ray
				{
					// if any ray doesn't hit, we bail on all other rays
					noAdjust = true;
					break;
				}
			}
			else if(r < rayCount - 1) // don't consider the line of sight ray
			{
				noAdjust = true;
			}
		}

		// if we hit anything besides ground, we're not in a valid location
		if((allHits ^ spec->buildFlags))
		{
			if(colliderIsType(allHits, COLLIDER_VEHICLE_MASK))
			{
				SetInvalid("ppos.badvehicle");
				pause = true;
			}
			else
			{
				SetInvalid("ppos.badlocation");
			}
		}
		else if(valid) // otherwise check the avoidance box
		{
			hits.clear();

			if(collider.getCollisions(abox, hits, false))
			{
				for(unsigned int i = 0; i < hits.size(); ++i)
				{
					UINT32 diff = (dGeomGetCategoryBits(hits[i].g2) ^ spec->buildFlags);

					if(diff)
					{
                        /*
                         TBD - this doesn't actually do anything.  Probably something unfinished?
                         
						OBJECT* ohit = colliderData<OBJECT>(hits[0].g2);
						trnCDTerrainObject* thit = colliderSpaceData<trnCDTerrainObject>(hits[0].g2);
                         */
                        
						std::vector<std::string> desc = colliderGetDescription(diff);

						SetInvalid("ppos.badavoidance");
						break;
					}
				}
			}
		}
	}

	// if we have hits, calculate
	vehVehicle* player = playerGetVehicle();

	if(!player)
	{
		SetInvalid("ppos.badplayer");
		return false;
	}
	else if(valid && (!zCount || (!spec->allowInterior && player->isInside)))
	{
		SetInvalid("ppos.noinside");
	}

	if(noAdjust)
	{
		SetInvalid("ppos.badlocation");
		zCount = 0;
	}

	if(valid)
	{
		vehVehicle* current = nullptr;
		Vec3d currentPos;
		Vec3d diff;

		// check for troopers in the way
		vehGetVehicleHead(&current);

		if(current != nullptr)
		{
			for (;current != nullptr; current = current->getNext())
			{
				if(current != player && current->VehicleType->Type == VEH_TYPE_PERSON)
				{
					currentPos.set(current->PosData.Pos);
					diff = finalPos - currentPos;

					if(diff.length2() <= (spec->avoidance * spec->avoidance))
					{
						SetInvalid("ppos.badvehicle");
						pause = true;
						break;
					}
				}
			}
		}
	}

	if(valid && buildAt > 0 && spec->maxSpeed)
	{
		FLOAT32 speed2 = (float)mathExactVectorLength2(&player->ModelData->VehicleVel);

		if(speed2 > (spec->maxSpeed * spec->maxSpeed))
		{
			SetInvalid("ppos.badspeed");
			fpPPOAlert();
			interruptNextUpdate = true;
		}
	}

	if(valid && Online)
	{
		if(spec->targetDistance && !_inTargetFacilityRange((float)spec->targetDistance, player->Side))
		{
			if(spec->targetDistance >= 0)
				SetInvalid("ppos.badtargetrange.tooclose");
			else
				SetInvalid("ppos.badtargetrange.toofar");
		}

		StratDB::StratObject* strat = nullptr;

		// enemy check
		if(valid)
		{
			strat = StratDB::FindStratObject((sdm_full_obj_id&)player->ModelData->closestEnemyFAC);

			if(spec->enemyFacDistance && !_checkFacilityRange((float)spec->enemyFacDistance, strat, player->Side, false))
			{
				if(spec->enemyFacDistance >= 0)
					SetInvalid("ppos.badenemyfac.tooclose");
				else
					SetInvalid("ppos.badenemyfac.toofar");
			}

			// can't place near fbs
			if(valid && strat && !_fbCheck(player, strat, 20.0f + (float)spec->avoidance))
				SetInvalid("ppos.badlocation");
		}

		// friendly check
		if(valid)
		{
			strat = StratDB::FindStratObject((sdm_full_obj_id&)player->ModelData->closestFriendlyFAC);

			if(spec->friendlyFacDistance && !_checkFacilityRange((float)spec->friendlyFacDistance, strat, player->Side, true))
			{
				if(spec->friendlyFacDistance >= 0)
					SetInvalid("ppos.badfriendlyfac.tooclose");
				else
					SetInvalid("ppos.badfriendlyfac.toofar");
			}

			// can't place near fbs
			if(valid && strat && !_fbCheck(player, strat, 20.0f + (float)spec->avoidance))
				SetInvalid("ppos.badlocation");
		}


	}

	double viewOffset			= (player->VehicleType->Type == VEH_TYPE_PERSON) ? -spec->viewOffset: spec->viewOffset * 2.0f;

	FLOAT64VECTOR worldPos		= player->ModelData->VehiclePos;
	piMATRIX worldAtt			= player->ModelData->VehicleAttMat;
	piVECTOR forward			= {0,(float)viewOffset,0};
	Matrixf attitude			= Matrixf(worldAtt);
	Matrixd rotate;

	if(spec->rotation)
	{
		double r = (player->VehicleType->Type == VEH_TYPE_PERSON) ? spec->rotation: -spec->rotation;
		rotate.makeRotate(mlDEG_TO_RAD(r), 0, 0, 1.0f);
		attitude = attitude * rotate;
	}

	// now orient the vector to our facing
	math3dMultiplyRotation(&forward, &forward, &worldAtt);

	worldPos.x += forward.x;
	worldPos.y += forward.y;
	worldPos.z += forward.z;

	// set our position and averaged z
	if(zCount)
	{
		worldNormal.normalize();

		// check the tilt angle if need be
		if(valid && spec->tiltTolerance)
		{
			Vec3d up = Vec3d(0,0,1.0f);

			if(!_okAngle(up, worldNormal, spec->tiltTolerance))
				SetInvalid("ppos.badangle");
		}

		// average the Zs
		worldPos.z = zTotal / zCount;
	}

	// save our world position in the returned pos
	pos.x = (FLOAT32)(worldPos.x - WorldOriginOffset.x);
	pos.y = (FLOAT32)(worldPos.y - WorldOriginOffset.y);
	pos.z = (FLOAT32)-(worldPos.z - WorldOriginOffset.z);

	if(valid && spec->treeCheck && sptIsRadiusInTreeBBox(pos, spec->treeCheck))
		SetInvalid("ppos.badtrees");

	// external source might check something else and set the valid flag true or false
	if(validityCallback)
		valid = validityCallback();

	if(!valid && buildAt && !pauseBuild && pause)
		uiGameEvent(GAMEEVENT_PPO, 1, _fail.c_str());

	pauseBuild = pause;

	if(checksOnly)
		return true;

	// for creating rotation matrices
	Vec3d from;
	Vec3d to;

	// forward
	from.set(0.0f, 1.0f, 0.0);
	to.set(0.0f, 1.0f, 0.0f);

	// orient our forward vector so we can rotate our terrain object around the z
	to = to * attitude;
	to.z() = 0;
	to.normalize();

	// create a orientation around the z
	rotate.makeRotate(from, to);

	// now we'll rotate around the x and y based on the world normal
	from.set(0.0f, 0.0f, 1.0f);

	// save our current rotation
	attitude = rotate;

	if(zCount)
	{
		rotate.makeIdentity();
		rotate.makeRotate(from, worldNormal);

		// add in the new rotation
		attitude = attitude * rotate;
	}

	// put it into a game matrix so we can convert it to an attitude
	attitude.get(worldAtt);

	// rotate us by the average of the hit normals as well as the vehicle's yaw
	// convert it to the expected attitude vector
	mathEul_FromMatrix(&att, &worldAtt, EulOrdXYZr);

	// mirror it as usual...does anyone know why?
	att.x = -att.x;
	att.y = -att.y;

	finalPos = worldPos;
	finalAtt = att;

	return true;
}

void PlayerPlacedObject::SetUpdateCallback(PPOCallback uc)
{
	updateCallback = uc;
}

void PlayerPlacedObject::SetExecuteCallback(PPOCallback ec)
{
	executeCallback = ec;
}

void PlayerPlacedObject::SetValidityCallback(PPOCallback vc)
{
	validityCallback = vc;
}

static bool _ppoDrawCallback(OBJECT* o)
{

	if(fpFPSIsSwimming())
		return true;

	if(ogl::UseDeprecatedPath() && ppomaterial.valid())
	{
		oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);

		ppomaterial->AddTexture(0, o->texid);
       		ppomaterial->Apply();

		int id = ppomaterial->GetMaterial();
		const piMATERIAL& mat = oglGetMaterial(id);

		oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(mat.diffuse.r, mat.diffuse.g, mat.diffuse.b, mat.diffuse.a));

		oglSetWorldMatrix(&o->mat);
		obdrawDrawMesh(o);

		oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
		oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, 1));
	}
	else
	{
		// TODO
	}

	return true;
}

bool PlayerPlacedObject::Update()
{
	piVECTOR pos	= {0, 0, 0};
	piVECTOR att	= {0, 0, 0};

    SINT32 *coolDownEnd = &globalMSCoolDownEnd;
    
    if (spec && spec->stotype != ServerTrackedObject::_UMS)
        coolDownEnd = &spec->coolDownEnd;
    
	if(!lastUpdate)
		lastUpdate = frametime;

	if(buildAt > 0)
	{
		// run these checks even while building
		CalculatePlacement(pos, att, true);

		if(Modified())
			ppomaterial->SetMaterial((valid) ? _goodMat: _badMat);

		if(frametime < buildAt) // we're building
		{
			if(pauseBuild)
				buildAt += frametime - lastUpdate;

			if(buildUI)
			{
				char script[128+1];
				float progress = float(frametime - (buildAt - spec->buildTime)) / (float)spec->buildTime;
				snprintf(script, 128, "updatePPOPopup(%f)", progress);
				uiRunScript(buildUI, script);
			}
		}
		else
		{
			Execute();
			return true;
		}
	}
	else // update position and validity
	{
		// make sure the UI is not up if we get here
		Interrupt();

		UpdateColliders();

		if(CalculatePlacement(pos, att) && obj != nullptr)
			ogrpMoveAndRotateObjectGroup(obj, &pos, &att);

		if(frametime <= *coolDownEnd)
		{
			_fail = uiLocString("ppos.coolingdown");
			valid = false;
		}

		if(Online && !PlayerClient()->IsPermitted(CAN_USE_PPO_TOOL))
		{
			_fail = uiLocString("ppos.noaccess");
			valid = false;
		}

		if(Modified())
			ppomaterial->SetMaterial((valid) ? _goodMat: _badMat);

		if(frametime <= *coolDownEnd) // we're cooling off
		{
			// we did our cool down start
			SINT32 coolDownStart = *coolDownEnd - spec->coolDown;
			SINT32 coolDownProgress = frametime - coolDownStart;

			if(coolDownProgress < 0)
			{
				// we have a start time before we got into the game
				// this can happen with shared (network) cooldown timers
				SINT32 adjust = -coolDownProgress;

				SINT32 now = frametime + adjust;

				coolDownStart = 0;
				coolDownProgress = now;
			}

			// Used to offset progress by however much progress we already have

			float progress = float(coolDownProgress) / float(spec->coolDown);

			char script[128+1];
			snprintf(script, 128, "game.oncooldown(%f)", progress);
			uiRunScript(script);
			//uiChatPrintLine("PPO Cooldown progress is %d%%", SINT32(100 * progress));
		}
		else
		{
			uiRunScript("game.oncooldown(1)");
		}
	}

	if(interruptNextUpdate)
	{
		interruptNextUpdate = false;
		Interrupt();
	}

	lastUpdate = frametime;

	// call our update callback if we have one
	if(updateCallback)
		return updateCallback();

	return (!valid && failOnInvalid) ? true: false;
}

bool PlayerPlacedObject::Build()
{
	if(valid && buildAt == 0)
	{
		buildAt = frametime + spec->buildTime;
		buildUI = uiLoadView("buildppo");

		if(buildUI)
		{
			char script[128+1];
			snprintf(script, 128, "initPPOPopup(%d)", spec->id);
			uiRunScript(buildUI, script);
		}

		return true;
	}
	else if(!valid)
	{
		fpPPOAlert();
	}

	return false;
}

void PlayerPlacedObject::Interrupt()
{
	buildAt = 0;

	if(buildUI)
	{
		buildUI->Close();
		buildUI = nullptr;

		// reset the cursor state if we closed the modal ui
		uiSetCursor((int)uiViewport()->HasModalChild("", true));
	}
}

bool PlayerPlacedObject::Execute()
{
	Interrupt();

    SINT32 *coolDownEnd = &globalMSCoolDownEnd;
    
    if (spec && spec->stotype != ServerTrackedObject::_UMS)
        coolDownEnd = &spec->coolDownEnd;

    // might need to do a max object count here
	// if too many objects, make invalid

	// if we have an execute callback and it returns false, then
	// don't do any execution code because it handled it
	if(executeCallback && !executeCallback())
		return true;

	if(valid)
	{
	    WorldPosition_m position(finalPos.x(), finalPos.y(), finalPos.z());
		AttitudeVector attitude(finalAtt.x, finalAtt.y, finalAtt.z);

        vehVehicle* player = persGetPersonaVehiclePtr();
		if(Online)
		{
         
            if (spec->stotype != ServerTrackedObject::_UMS)
            {
                // create a ppo sto
                PlayerPlacedSTO sto(0, static_cast<UINT16>(spec->id), position, attitude, static_cast<UINT16>(spec->stotype), (UINT32)player->Country);

                // send the request to the host
                sto.request(PlayerClient()->ChatConnection());
            }
            else
            {
                // create a ppo sto
                MobileSpawnSTO sto(0, static_cast<UINT16>(spec->id), position, attitude, (UINT32)player->Country);
                
                // send the request to the host
                sto.request(PlayerClient()->ChatConnection());
                
            }
		}
		else
		{
            
            if (spec->stotype != ServerTrackedObject::_UMS)
            {
                PlayerPlacedSTO* sto = new PlayerPlacedSTO(0, static_cast<UINT16>(spec->id), position, attitude, static_cast<UINT16>(spec->stotype), (UINT32)player->Country);
                ServerTrackedObject::addToIndex(sto);
                trnCDUpdateCollideLists(true);
                //sto->Sink();
            }
            else
            {
                MobileSpawnSTO* sto = new MobileSpawnSTO(0, static_cast<UINT16>(spec->id), position, attitude, (UINT32)player->Country);
                ServerTrackedObject::addToIndex(sto);
                trnCDUpdateCollideLists(true);
                //sto->Sink();
            }
		}

        *coolDownEnd = frametime + spec->coolDown;

		// handle troop specific stuff
		fpFPSCancelPPO();

		// temp string buffer
		char nkey[50+1];

		// create name key based on id
		snprintf(nkey, 50, "ppos.ob%d", spec->id);

		const std::wstring& name = uiLocString(nkey);

		fpPPOAlert("ppos.built", name.c_str());

		return true;
	}
	else
	{
		fpPPOAlert();
	}

	return false;
}

bool PlayerPlacedObject::Modified()
{
	if(valid != lastValid)
	{
		lastValid = valid;
		return true;
	}

	return false;
}

void fpPPOInitiateBuildAction(UINT32 & state, UINT32 & control, SINT32 & stance)
{
    PlayerPlacedObject* ppo = fpPPOGet();
    
    if(ppo)
    {
        Write_Log (LOG_FPS, LOG_INFO, "Building ppo!");
        
        if(ppo->Build())
        {
            state &= static_cast<UINT32>(~STATE_HOLD_FIRE);
            control &= static_cast<UINT32>(~FPS_FIRE);
            
            stance = fpFPSGetPlayerPose();
            
            if(stance != POSE_CROUCH)
            {
                control |= static_cast<UINT32>(FPS_CROUCH);
                control &= static_cast<UINT32>(~FPS_PRONE);
            }
            
            if(stance == POSE_TRANSITION)
                stance = POSE_CROUCH;
        }
    }
    
    //fpsTrooper.firedWeapon = 1;

}

void fpPPOSet(PlayerPlacedObject* to)
{
    // do nothing if already set
    if (to != nullptr && to == _placedObject)
        return;
    
	if(_placedObject)
		delete _placedObject;

	_placedObject = to;
}

PlayerPlacedObject* fpPPOGet()
{
	return _placedObject;
}

/*
  The user has elected to switch to the next PPO type.
  Usually due to pressing the mapped "cycle ammo" key <BKSP> default.
 */
void fpPPOSwitchPPO(bool adjust)
{
	// don't bother doing a thing unless we have more than one choice
	if (activePPOList.size() > 1)
	{
		
        PPOCallback up = nullptr, ex = nullptr, val = nullptr;
		if (_placedObject)
        {
            up = _placedObject->updateCallback;
            ex = _placedObject->executeCallback;
            val = _placedObject->validityCallback;
			fpPPOKill(false);
        }

		if (adjust)
		{
			if (++activePPOIndex == activePPOList.size())
				activePPOIndex = 0;
		}

		fpPPOSet(fpPPOCreate(activePPOList[activePPOIndex]->id, up, ex, val));

	}
}

/*
 Called when you wish to reset the list of possible PPOs that the current vehicle can build.
 Should be called when the BUILD weapon is first selected.
 */
void fpPPOReset()
{
	static WEAPON_SPEC troop = NULL_WEAP_SPEC;
	static bool firsttime = true;

	if(firsttime)
	{
		firsttime = false;
		troop = weapCreateSpec("GV_troop.1");
	}

	activePPOIndex = 0;

	mlVehicleData* VehicleData = persGetPersonaModelDataPtr();
    vehType* vtype = VehicleData->VehicleMasterPtr->VehicleType;

	// only allow FRUs for infrantry when mission leader
	bool allowFrus = vtype->WeaponSpec.wClass != troop.wClass || (Online && PlayerClient()->IsMissionLeader());

    // make sure this class/type has ppo types available to it
    if (specLoader.getAllowedPPOs(vtype->WeaponSpec.wClass, vtype->WeaponSpec.wType, activePPOList, !allowFrus))
	{
		fpPPOSwitchPPO(false);
	}	
}

bool fpPPOUpdate()
{
	// update sinkers
	PPO_SINK_MAP::iterator it = _ppoSinkMap.begin();

	for(; it != _ppoSinkMap.end(); it++)
	{
		ServerTrackedObject* sto = wwiiGetSTO(it->first);

		if(sto)
		{
			if(frametime >= it->second)
			{
				wwiiRemoveSTO(sto->uid());
			}
			else
			{
				PlayerPlacedSTO* psto = dynamic_cast<PlayerPlacedSTO*>(sto);
				if(psto) psto->Update();
			}
		}
	}

	if(_placedObject && _placedObject->Update())
	{
		fpPPOSet(nullptr);
		return true;
	}

	return false;
}

void fpPPOKill(bool release)
{
    if (release)
    {
        activePPOIndex = 0;
        activePPOList.clear();
    }
	fpPPOCancel(release);
	fpPPOSet(nullptr);
}

void fpPPOCancel(bool release)
{
	if(_placedObject)
	{
		_placedObject->Interrupt();
        if (release)
            fpFPSCancelPPO();
	}
}

void fpPPOResetMaterial()
{
	_resetMaterials = true;
}

void fpPPOCreateMaterial()
{
	if(!ppomaterial.valid())
		ppomaterial = new oglMaterial();

	if(_badMat == -1)
	{
		// create a new temp material object
		piMATERIAL mat;
		memset(&mat, 0, sizeof(piMATERIAL));

		// set default good material diffuse
		mat.diffuse.set(0, 1.0f, 0, .5f);

		// create our good material
		_goodMat = oglCreateMaterial(&mat);

		// set default bad material diffuse
		mat.diffuse.set(1.0f, 0, 0, .5f);

		// create our bad material
		_badMat = oglCreateMaterial(&mat);

		mat.diffuse.set(0, 0, 0, 0);

		_offMat = oglCreateMaterial(&mat);

		_resetMaterials = true;
	}

	if(_resetMaterials)
	{
		MATERIAL* mats = nullptr;
		oglGetMaterials(&mats);

		UINT32 good	= (UINT32)cfgPrefsGetInt(cfgPrefsGetGlobalName(cfgPPO_OK), piRGB_MAKE(0, 255, 0));

		float r = piRGBA_GETRED(good)/(float)0xff;
		float g = piRGBA_GETGREEN(good)/(float)0xff;
		float b = piRGBA_GETBLUE(good)/(float)0xff;

		mats[_goodMat].dirty = true;
		mats[_goodMat].material.diffuse.set(r, g, b, .5f);

		UINT32 bad	= (UINT32)cfgPrefsGetInt(cfgPrefsGetGlobalName(cfgPPO_NOK), piRGB_MAKE(255, 0, 0));

		r = piRGBA_GETRED(bad)/(float)0xff;
		g = piRGBA_GETGREEN(bad)/(float)0xff;
		b = piRGBA_GETBLUE(bad)/(float)0xff;

		mats[_badMat].dirty = true;
		mats[_badMat].material.diffuse.set(r, g, b, .5f);

		_resetMaterials = false;
	}
}

PlayerPlacedObject* fpPPOCreate(int type,  PPOCallback updateCB, PPOCallback executeCB, PPOCallback validityCB, bool save)
{
	static int _lastType = -1;

	if(type != _lastType)
	{
		_lastType = type;

		// make sure the hud is correct
		// assume we're ok to place
		uiRunScript("game.oncooldown(1)");
	}

	_fail = L"";

	PPOSpec* spec = specLoader.ppoGetSpec(type);
	if(!spec) return nullptr;

	// make sure our materials are created
	fpPPOCreateMaterial();

	if(Online && spec->stotype == ServerTrackedObject::_UMS)
	{
		const Chat::MissionStatus* ms = PlayerClient()->MissionStatus();

		if(ms)
		{
			SINT32 remaining	= SINT32(ms->fru().cooldownRemaining());
			globalMSCoolDownEnd	= (remaining > 0) ? frametime + remaining : 0;
		}
	}

	PlayerPlacedObject* ppo = new PlayerPlacedObject(spec);

	// make sure the object got created (bad spec or something)
	if(ppo && !ppo->obj)
	{
		delete ppo;
		ppo = nullptr;
	}

    ppo->SetUpdateCallback(updateCB);
    ppo->SetExecuteCallback(executeCB);
    ppo->SetValidityCallback(validityCB);

	if(save)
		fpPPOSet(ppo);

	return ppo;
}

void fpPPOAlert(const wchar_t* message)
{
	if(message)
		uiGameEvent(GAMEEVENT_PPO, 1, message);
	else if(!_fail.empty())
		uiGameEvent(GAMEEVENT_PPO, 1, _fail.c_str());
}

void fpPPOAlert(const char* key, ...)
{
	wchar_t message[128+1];

	std::wstring loc = uiLocString(key);

	va_list marker;
	va_start(marker, key);
	vswprintf(message, 128, loc.c_str(), marker);
	va_end(marker);

	fpPPOAlert(message);
}

PlayerPlacedSTO::~PlayerPlacedSTO()
{
	PPO_SINK_MAP::iterator found = _ppoSinkMap.find(uid());

	if(found != _ppoSinkMap.end())
		_ppoSinkMap.erase(found);
}

bool PlayerPlacedSTO::applyJoules(const SINT64& joules, class Player* player)
{
	return TerrainSTO::applyJoules(joules, player);
}

bool PlayerPlacedSTO::load()
{
	bool loaded = TerrainSTO::load();

	if(loaded && _to)
		obmainSetFlag3InThisTree(_to, OBJECT_PLAYER_PLACED);

	return loaded;
}

const char* PlayerPlacedSTO::model() const
{
	PPOSpec* spec = specLoader.ppoGetSpec(m_subType);

	if(spec)
		return spec->modelName.c_str();

	return "";
}

const char* MobileSpawnSTO::model() const
{
    PPOSpec* spec = specLoader.ppoGetSpec(m_subType);
    
    if(spec)
        return spec->modelName.c_str();
    
    return "";
}

void PlayerPlacedSTO::Sink()
{
	// make sure we're not on the list
	UINT32 id = uid();

	PPO_SINK_MAP::iterator found = _ppoSinkMap.find(id);

	if(found == _ppoSinkMap.end())
	{
		_pos.x				= m_position.x();
		_pos.y				= m_position.y();
		_pos.z				= m_position.z();
		_lastUpdate			= frametime;
		_ppoSinkMap[id]		= frametime + 30000;

		if(_to && _to->child)
		{
			trnCDTerrainObject* TO = trncdFindTerrainObject(_to->child);

			if(TO)
			{
				TO->uFlags |= TRN_OBJECT_INACTIVE;

				obmainClearFlagsInObjectTree(TO->CollisionObject, OBJECT_COLLISION);
				obmainClearFlagsInObjectTree(TO->DamageCollisionObject, OBJECT_COLLISION);

				if(TO->physicsColliderSpace)
					dGeomDisable((dGeomID)TO->physicsColliderSpace);

				if(TO->physicsBoxCollider)
					dGeomDisable((dGeomID)TO->physicsBoxCollider);

				trnCDUpdateCollideLists(true);
			}
		}
	}
}

static LOGICAL _sinkppo(struct OBJECT* o, void* userdata)
{
	o->mat._43 += (float)(*((FLOAT64*)userdata));
	return TRUE;
}

void PlayerPlacedSTO::Update()
{
	FLOAT64 delta = (FLOAT64(frametime) - FLOAT64(_lastUpdate)) / 1000.0f;
	const FLOAT64 SINK_RATE = .1f;

	FLOAT64 move = SINK_RATE * delta;

	obmainFindCallback(_to, _sinkppo, &move);

	_lastUpdate = frametime;
}

/************************************************************************************************
	_inTargetFacilityRange
*************************************************************************************************/
static bool _inTargetFacilityRange(FLOAT32 distance, SINT32 side)
{
 	FULL_OBJ_ID foi = PlayerClient()->SelectedMission().m_target;
	StratDB::StratObject* so = StratDB::FindStratObject((sdm_full_obj_id&)foi);

	if(!so)
		return true;

	bool friendly = (so->Side() == side);

	if ( so->StratClass() == SDM_CLASS_FACILITY || so->StratType() == SDM_CP_BRIDGE )
		return _checkFacilityRange(distance, so, side, friendly);
	else if(so->StratClass() != SDM_CLASS_CP)
		return false;

	StratDB::StratObject::ChildList cl = so->ChildStratObjects();

	if(cl.empty())
		return false;

	StratDB::StratObject::ChildList::iterator B = cl.begin();
	StratDB::StratObject::ChildList::iterator E = cl.end();

	vehVehicle* player = persGetPersonaVehiclePtr();
	if(!player) return false;

	StratDB::StratObject* best = nullptr;
	double bestDistance2 = 0;

	for(; B != E; ++B)
	{
		StratDB::StratObject* child = *B;

		// Find closest facility or open fb
		if(child->StratClass() == SDM_CLASS_FACILITY &&
		  (child->StratType() != FACILITY_FIREBASE &&
		  (child->StratFlags() & CP_STAT_OPEN)))
		{
			if(friendly != (side == child->Side()))
				continue;

			Vec3d pPos = Vec3d(player->ModelData->VehiclePos);
			Vec3d sPos = Vec3d((double)child->PosX(), (double)child->PosY(), pPos.z());
			Vec3d diff = sPos - pPos;

			if(!best || diff.length2() < bestDistance2)
			{
				best = child;
				bestDistance2 = diff.length2();
			}
		}
	}

	if(best && _checkFacilityRange(distance, best, side, friendly))
		return true;

    return false;
}

/************************************************************************************************
	_nearFacility
*************************************************************************************************/
static bool _checkFacilityRange(FLOAT32 distance, StratDB::StratObject* so, SINT32 side, bool friendly)
{
	if(!so && distance >= 0)
		return true;
	else if(!so && distance < 0)
		return false;
	else if(so->StratClass() != SDM_CLASS_FACILITY)
		return false;
	else if(so->StratType() == FACILITY_FIREBASE && !(so->StratFlags() & CP_STAT_OPEN))
		return false;

	bool stratFriendly = (so->Side() == side);

	// If we don't know where it is, it won't prohibit you spawning
	if(friendly && !stratFriendly && distance >= 0) return true;
	else if(friendly && !stratFriendly && distance < 0) return false;
	else if(!friendly && stratFriendly && distance >= 0) return true;
	else if(!friendly && stratFriendly && distance < 0) return false;

	vehVehicle* player = persGetPersonaVehiclePtr();
	if(!player) return false;

	Vec3d pPos = Vec3d(player->ModelData->VehiclePos);
	Vec3d sPos = Vec3d((double)so->PosX(), (double)so->PosY(), pPos.z());
	Vec3d diff = sPos - pPos;

	FLOAT64 facDis2 = diff.length2();
	FLOAT32 adist = fabs(distance);

	if(distance >= 0)
		return (facDis2 >= (adist * adist));
	else
		return (facDis2 <= (adist * adist));
}

static bool _fbCheck(vehVehicle* player, StratDB::StratObject* fac, FLOAT32 radius)
{
	if(!player || !fac)
		return true;

	StratDB::StratObject* cp = fac->ParentStratObject();
	StratDB::StratObjList fbs;

	if(cp && StratDB::FindStratObjects(fbs, SDM_CLASS_FACILITY, SDM_FACILITY_FIREBASE, -1, cp))
	{
		Vec3d diff;
		Vec3d bPos;
		Vec3d pPos = Vec3d(player->ModelData->VehiclePos);

		for(size_t i = 0; i < fbs.size(); ++i)
		{
			StratDB::StratObject* fb = fbs[i];
			const StratDB::StratObject::ChildList& buildings = fb->ChildStratObjects();

			for(size_t j = 0; j < buildings.size(); ++j)
			{
				StratDB::StratObject* building = buildings[j];
				bPos.set(building->PosX(), building->PosY(), pPos.z());
				diff = bPos - pPos;

				if(diff.length2() < radius * radius)
					return false;
			}
		}
	}

	return true;
}
