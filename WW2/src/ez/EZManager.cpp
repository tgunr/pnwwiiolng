#include <syscfg.h> // Path is project specific.

#include "ez/EZManager.h"
#include "ez/EZLoader.h"
#include "ez/EZVehicles.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "pers/persmain.h"
#include "math/Matrixd.h"
#include "math/Vec3d.h"
#include "math/matheul.h"
#include "fp/fpFPS.h"
#include "ml/mlutility.h"
#include "trn/trnvars.h"
#include "pers/pershud.h"
#include "ui/uiMain.h"
#include "ui/uiChat.h"
#include "uimap/uiMap.h"
#include "ml/mlEngine.h"
#include "physics/colliderQuery.h"
#include "weap/weapSpec.h"
#include "imap/ww2imap.h"
#include "fp/fpTPSPhysics.h"
#include "fp/fpMain.h"
#include "cfg/cfgPrefs.h"

#define CAMERA_THRESHOLD 1

// current manager, could be null
static EZManager* _ezMgr = nullptr;

// static functions

static void _normalizeAngle(FLOAT64& angle)
{
	while(angle * angle > 180.0 * 180.0)
	{
		if (angle < 0)
			angle += 360.0;
		else if (angle >= 0)
			angle -= 360.0;
	}
}

/************************************************************************************************
	EZManager
*************************************************************************************************/

EZManager::EZManager(vehVehicle* v) :
    _vehicle(v),
	_flags(0),
    _lastUpdate(frametime),
    _category(0),
    _mainTurret(-1),
    _hatchTime(750),
    _view(0),
	_position(0),
    _gamePosition(-1),
	_move(EZ_MOVE_IDLE),
	_turn(EZ_TURN_NONE)
{
}

EZManager::~EZManager()
{
}

bool EZManager::Init(const char* path)
{
	if(_vehicle && path && !HasFlag(EZManager::INITED))
	{
		EZLoader loader(path);
		loader.Load();

		// we now try to find the highest loaded level that is less
		// than or equal to the current game ez mode level
		SINT32 level = 0;

		for(SINT32 l = 1; l <= loader.maxLevelLoaded; l++)
		{
			EZLoader::EZ_LEVEL_MAP::iterator it = loader.levels.find(l);

			// if we found a level and its > than the current level
			// and its <= to the current game ezmode level, then we'll
			// use it; this has the effect of choosing the highest loaded
			// level for the current ez mode game level
			if(it != loader.levels.end() && l > level && l <= (SINT32)gEZMode)
				level = l;
		}

		if(level)
		{
			// add our accumulated positions to our ez manager
			EZManager::POSITION_LIST& positions = loader.levels[level].positions;

			for(size_t i = 0; i < positions.size(); i++)
				AddPosition(positions[i]);

			// save our allowed controls (might be empty which means allow any)
			controls = loader.levels[level].controls;

			// add any level specific config variables
			EZManager::CONFIG_LIST& config = loader.levels[level].config;

			for(size_t i = 0; i < config.size(); i++)
				EZ_MGR->SetConfig(config[i].first.c_str(), config[i].second.c_str());
		}

		// set our flag
		SetFlag(EZManager::INITED, true);

		// return true if we found/loaded some positions
		// you might get no positions here if no ez mode levels
		// weren't found for the current gEZMode game level
		if(!_positions.empty() && !_positions[0].views.empty())
			return true;
	}

	return false;
}

void EZManager::FrameUpdate()
{
	if(!uiInGame() || !_vehicle || !_vehicle->InUse)
		return;

	FLOAT64 delta = FLOAT64(frametime - _lastUpdate) / 1000.0f;

	if(IsCommandView())
	{
		EZCamera& camera = GetCamera();

		/*
		Vec3d dir = Vec3d(0, 1.0f, 0);
		Matrixd attitude = Matrixd(_vehicle->ModelData->VehicleAttMat);

		dir = dir * attitude;
		dir.z() = 0;
		dir.normalize();

		if(_lastDir.length2() && dir != _lastDir)
		{
			FLOAT64 rAngle = _lastDir * dir;

			// cap it
			rAngle = (rAngle < -1.0f) ? -1.0f : (rAngle > 1.0f) ? 1.0f : rAngle;

			FLOAT64 angle = mlRAD_TO_DEG(acos(rAngle));

			if(angle)
			{
				if(rAngle < 0)
					angle = -angle;

				camera.traverse += angle;
			}
		}
		*/

		camera.Update(delta);

		//_lastDir = dir;
	}

	UpdateTurrets();

	_lastUpdate = frametime;
}

void EZManager::TicUpdate(FLOAT64 DeltaTime)
{
	/*
	if(!uiInGame() || !_vehicle || !_vehicle->InUse)
		return;

	FLOAT64 delta = FLOAT64(frametime - _lastUpdate) / 1000.0f;

	if(IsCommandView())
	{
		EZCamera& camera = GetCamera();

		camera.Update(delta);
	}

	UpdateTurrets();

	_lastUpdate = frametime;
	*/
}

void EZManager::UpdateTurrets()
{
	if(!IsCommandView(true))
		return;

	EZView& v = GetView();

	for(size_t i = 0; i < v.turrets.size(); i++)
		v.turrets.at(i).UpdateReticle();
}

void EZManager::PreUpdateTurret(SINT32 p)
{
	if(!IsCommandView(true))
		return;

	EZView& v = GetView();

	for(size_t i = 0; i < v.turrets.size(); i++)
	{
		EZTurret& turret = v.turrets.at(i);

		if(turret.position == p)// && turret.tracking)
		{
			turret.PreUpdate();
			break;
		}
	}
}

void EZManager::PostUpdateTurret(SINT32 p)
{
	if(!IsCommandView(true))
		return;

	EZView& v = GetView();

	for(size_t i = 0; i < v.turrets.size(); i++)
	{
		EZTurret& turret = v.turrets.at(i);

		if(turret.position == p)// && turret.tracking)
		{
			turret.PostUpdate();
			break;
		}
	}
}

bool EZManager::SetFlag(SINT32 flag, bool on)
{
	if(on && !HasFlag(flag))
	{
		_flags |= flag;
		return true;
	}
	else if(!on && HasFlag(flag))
	{
		_flags &= ~flag;
		return true;
	}

	return false;
}

bool EZManager::HasFlag(SINT32 flag)
{
	return (_flags & flag) > 0;
}

EZManager::EZPosition& EZManager::GetPosition(SINT32 p)
{
	if(p >= 0 && p < (int)_positions.size())
		return _positions.at(p);

	return _positions.at(_position);
}

EZManager::EZView& EZManager::GetView()
{
	return GetPosition().GetView();
}

EZManager::EZCamera& EZManager::GetCamera()
{
	return GetView().camera;
}

SINT32 EZManager::GetHatchTime()
{
	return _hatchTime;
}

void EZManager::InstallInputCallbacks()
{
	// install ez mode callbacks
	imapInstallControlCallback(IMAP_EZ_MOVE, ezInputControls);
	imapInstallControlCallback(IMAP_EZ_TURN, ezInputControls);

	imapInstallControlCallback(IMAP_EZ_ACTION1, ezInputControls);
	imapInstallControlCallback(IMAP_EZ_ACTION2, ezInputControls);

	if(IsCommandView())
	{
		imapInstallControlCallback(IMAP_EZ_FREELOOK, ezInputControls);
		imapInstallControlCallback(IMAP_EZ_CAMERA_X, ezInputControls);
		imapInstallControlCallback(IMAP_EZ_CAMERA_Y, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_FORWARD, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_BACK, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_DOWN, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_RIGHT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_FORWARD_LEFT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_RIGHT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_LEFT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_BACK_RIGHT, ezInputControls);
		imapInstallControlCallback(IMAP_LOOK_BACK_LEFT, ezInputControls);
	}

	imapInstallControlCallback(IMAP_MOM_POSITION_1, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_2, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_3, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_4, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_5, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_6, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_7, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_8, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_9, ezInputControls);
	imapInstallControlCallback(IMAP_MOM_POSITION_10, ezInputControls);

	imapInstallControlCallback(IMAP_EZ_NEXTPOS, ezInputControls);
	imapInstallControlCallback(IMAP_EZ_PREVPOS, ezInputControls);
	imapInstallControlCallback(IMAP_EZ_ENGINE, ezInputControls);
}

void EZManager::AddPosition(EZPosition& position)
{
	_positions.push_back(position);
}

bool EZManager::IsPositionDead(SINT32 position)
{
	return GetPosition(position).IsDead();
}

void EZManager::SetReticle(const char* r)
{
	_reticle = r;
}

void EZManager::SetTurret(SINT32 t)
{
	_mainTurret = t;
}

bool EZManager::IsCommandView(bool deathCheck)
{
	EZPosition& position = GetPosition();

	// we're in a command view when the current position's view is external
	if(position.IsCommandView())
	{
		if(deathCheck && position.IsDead())
			return false;

		return true;
	}

	return false;
}

void EZManager::OnFreeLook(SINT32 param, UINT32 id,UINT32 extra)
{
	SetFlag(EZManager::FREELOOK, (param) ? true : false);

	if(IsCommandView())
	{
		if(HasFlag(EZManager::FREELOOK))
			GetCamera().Mark();
		else
			GetCamera().Snap();
	}
}

void EZManager::OnCamera(SINT32 param, UINT32 id,UINT32 extra)
{
	static SINT32 thresholdX = 5;
	static SINT32 thresholdY = 5;

	EZCamera& camera = GetCamera();
	bool tracking = false;

	if(id == IMAP_EZ_CAMERA_X && abs(param) > thresholdX)
	{
		camera.traverse.move += -param;
		tracking = true;
	}
	else if(id == IMAP_EZ_CAMERA_Y && abs(param) > thresholdY)
	{
		camera.elevation.move += -param;
		tracking = true;
	}

	if(tracking)
	{
		EZView& v = GetView();

		for(size_t i = 0; i < v.turrets.size(); i++)
			v.turrets.at(i).tracking = true;
	}
}

bool EZManager::OnCamera(piVECTOR* pos, piVECTOR* aim)
{
	EZCamera& camera = GetCamera();

	Vec3d forward		= Vec3d(0,1.0f,0);
	Vec3d right			= Vec3d(1.0f,0,0);

	Matrixf rotate		= Matrixf::rotate(PI, 0, 0, 1);
	Matrixd attitude	= Matrixd(_vehicle->ModelData->VehicleAttMat);
	Vec3d camPos		= Vec3d(_vehicle->ModelData->VehiclePos);
	Vec3d aimPos		= Vec3d(0, camera.focus, 0);

	FLOAT32 elevation	= (FLOAT32)camera.elevation.angle;
	FLOAT32 traverse	= (FLOAT32)camera.traverse.angle;

	if(!camera.pov && HasFlag(EZManager::VIEW_OVERRIDE))
	{
		elevation = 0.0f;
		traverse = (FLOAT32)_view;
	}

	rotate	= Matrixf::rotate(mlDEG_TO_RAD(elevation), 1, 0, 0);
	forward = forward * rotate;
	right	= right * rotate;

	rotate	= Matrixf::rotate(mlDEG_TO_RAD(traverse), 0, 0, 1);
	forward = forward * rotate;
	right	= right * rotate;

	forward = forward * attitude;
	right = right * attitude;

	if(camera.pov)
	{
		Vec3d aim = forward;
		aim *= camera.focus;

		Vec3d offset = camera.offset;
		offset = offset * attitude;

		camPos += offset;
		aimPos = camPos + aim;
	}
	else
	{
		Vec3d p = forward;
		p *= camera.focus;

		aimPos = camPos + p;

		p = forward;
		p *= (-1.0f * camera.offset.y());
		p += (right * camera.offset.x());

		camPos = camPos + p;
		camPos.z() += camera.offset.z();
	}

	AdjustCamera(camPos, aimPos);

	if(pos)
	{
		pos->x = -float(camPos.x() - WorldOriginOffset.x);
		pos->y = -float(camPos.y() - WorldOriginOffset.y);
		pos->z = float(camPos.z());
	}

	if(aim)
	{
		aim->x = -float(aimPos.x() - WorldOriginOffset.x);
		aim->y = -float(aimPos.y() - WorldOriginOffset.y);
		aim->z = float(aimPos.z());
	}

	return true;
}

void EZManager::OnSpawn()
{
	SetPosition(_position, true);

	// handle any onspawn events for any position/view
	for(size_t p = 0; p < _positions.size(); p++)
	{
		EZManager::VIEW_LIST& views = _positions[p].views;

		for(size_t v = 0; v < views.size(); v++)
		{
			EZView& ev = views[v];

			SetGamePosition(ev.position);

			if(ev.hatch == EZView::EVENT_ONSPAWN)
				mlToggleCanopy(1, 0, 0);

			if(ev.deploy == EZView::EVENT_ONSPAWN)
				mlToggleBombBayDoor(1, 0, 0);
		}

		// reset override position
		SetGamePosition();
	}
}

SINT32 EZManager::GetCategory()
{
	return _category;
}

void EZManager::SetConfig(const char* name, const char* value)
{
	if(strcasecmp(name, "category") == 0)
	{
		if(strcasecmp(value, "air") == 0)
			_category = WW2_FUNC_CATEGORY_AIR;
		else if(strcasecmp(value, "tank") == 0)
			_category = WW2_FUNC_CATEGORY_TANK;
		else if(strcasecmp(value, "truck") == 0)
			_category = WW2_FUNC_CATEGORY_TRUCK;
		else if(strcasecmp(value, "atgun") == 0)
			_category = WW2_FUNC_CATEGORY_ATGUN;
		else if(strcasecmp(value, "person") == 0)
			_category = WW2_FUNC_CATEGORY_PERSON;
		else if(strcasecmp(value, "sea") == 0)
			_category = WW2_FUNC_CATEGORY_SEA;
		else if(strcasecmp(value, "troop") == 0)
			_category = WW2_FUNC_CATEGORY_PERSON;
	}
	else if(strcasecmp(name, "hatchtime") == 0)
	{
		_hatchTime = atoi(value);
	}
	else if(strcasecmp(name, "ezengine") == 0 && strcasecmp(value, "false") == 0)
	{
		SetFlag(EZManager::EZ_ENGINE, false);
	}
}

bool EZManager::SetPosition(SINT32 position, bool force)
{
	if(position < 0 || position >= (SINT32)_positions.size())
		return false;

	if(_positions[position].views.empty())
		return false;

	if(!force && position == _position)
		return false;

	if(!_positions[position].IsCommandView() && _positions[position].IsDead())
		return false;

	if(!_reticle.empty())
	{
		phui::View* camReticle = uiFindView("camera_reticle");
		if(camReticle) camReticle->Close();
	}

	// we're going to load our new position
	// things to do:
	// - unload old reticle
	// - stop any current camera movement

	bool lastPosCV = IsCommandView();

	GetPosition().Unload();

	_position = position;

	GetPosition().Load();

	if(IsCommandView())
	{
		if(!_reticle.empty())
			uiLoadView(_reticle.c_str());

		if(!lastPosCV)
		{
			// last position was not a command view but this one is
			// lets sync the camera to the main turret in case that
			// turret was manipulated by the last position

		}
	}

	return true;
}

SINT32 EZManager::GetGamePosition()
{
	return _gamePosition;
}

void EZManager::SetGamePosition(SINT32 p)
{
	_gamePosition = p;
}

bool EZManager::InputBlock(SINT32 id)
{
	return GetPosition().InputBlock(id);
}

bool EZManager::EasyControl(UINT32 position, const char* control)
{
	// if no controls are defined, allow any control
	if(controls.empty())
		return true;

	for(size_t i = 0; i < controls.size(); i++)
	{
		// if this control is in our allowed list, return true
		if(controls[i].first == (SINT32)position && strcasecmp(controls[i].second.c_str(), control) == 0)
			return true;
	}

	// nothing hit, return false - not allowed
	return false;
}

void EZManager::OnView(SINT32 param, UINT32 id,UINT32 extra)
{
	if(param && IsCommandView(true))
	{
		SetFlag(EZManager::VIEW_OVERRIDE|EZManager::FREELOOK, true);

		switch(id)
		{
			case IMAP_LOOK_FORWARD:			_view = 0.0f; break;
			case IMAP_LOOK_BACK:			_view = 180.0f; break;
			case IMAP_LOOK_DOWN:			_view = 180.0f; break;
			case IMAP_LOOK_FORWARD_RIGHT:	_view = -45.0f; break;
			case IMAP_LOOK_FORWARD_LEFT:	_view = 45.0f; break;
			case IMAP_LOOK_RIGHT:			_view = -90.0f; break;
			case IMAP_LOOK_LEFT:			_view = 90.0f; break;
			case IMAP_LOOK_BACK_RIGHT:		_view = -135.0f; break;
			case IMAP_LOOK_BACK_LEFT:		_view = 135.0f; break;
		}
	}
	else
	{
		SetFlag(EZManager::VIEW_OVERRIDE|EZManager::FREELOOK, false);
	}
}

SINT32 EZManager::ConstrainPosition(SINT32 position)
{
	if(_positions.size() == 0)
		return -1;

	if(_positions.size() == 1)
		return 0;

	if(position >= (SINT32)_positions.size())
		position = 0;
	else if(position < 0)
		position = (SINT32)(_positions.size() - 1);

	return position;
}

void EZManager::OnPosition(SINT32 param, UINT32 id,UINT32 extra)
{
	if(!param || _positions.size() <= 1)
		return;

	switch(id)
	{
		case IMAP_EZ_NEXTPOS:
		case IMAP_EZ_PREVPOS:
		{
			SINT32 move = (id == IMAP_EZ_PREVPOS) ? -1 : 1;
			SINT32 position = ConstrainPosition((SINT32)_position + move);

			while(position != _position)
			{
				if(SetPosition(position))
					break;

				position = ConstrainPosition(position + move);
			}
		}
		break;
		case IMAP_MOM_POSITION_1:
		case IMAP_MOM_POSITION_2:
		case IMAP_MOM_POSITION_3:
		case IMAP_MOM_POSITION_4:
		case IMAP_MOM_POSITION_5:
		case IMAP_MOM_POSITION_6:
		case IMAP_MOM_POSITION_7:
		case IMAP_MOM_POSITION_8:
		case IMAP_MOM_POSITION_9:
		case IMAP_MOM_POSITION_10:
		{
			SetPosition((SINT32)id - IMAP_MOM_POSITION_1);
		}
		break;
	}
}

bool EZManager::IsDriverDead()
{
	return (persGetCrewIsDead(0) > 0);
}

bool EZManager::IsCommanderDead()
{
	return (persGetCrewIsDead(1) > 0);
}

bool EZManager::IsGunnerDead()
{
	return (persGetCrewIsDead(2) > 0);
}

bool EZManager::IsLoaderDead()
{
	return (persGetCrewIsDead(3) > 0);
}


void EZManager::AdjustCamera(Vec3d& camPos, Vec3d& aimPos)
{
	Vec3d physPos	= physicsGameToPhysical(camPos);
	Vec3d vehPos	= physicsGameToPhysical(Vec3d(_vehicle->ModelData->VehiclePos));

	vehPos.z() += 1.0f;

	Vec3d camDir	= physPos - vehPos;
	FLOAT64 camDist = camDir.length();

	camDir.normalize();

	if(_collider.geoms().empty())
	{
		// limit how many contacts we generate
		_collider.setMaxContactsPerHit(2);

		dSpaceID space = _collider.space();

		// create our ray colliders for placement handling
		_collider.addGeom(dCreateRay(space, 1.0f), 0, COLLIDER_HARDTERRAIN_MASK);
	}

	colliderQuery::dGeomList& geoms = _collider.geoms();

	int rayCount = geoms.size();

	// update our ray position, direction and length
	for(int i = 0; i < rayCount; i++)
	{
		dGeomID geom = geoms.at(i);

		// set our line of sight length
		dGeomRaySetLength(geom, camDist);

		// set our position and direction
		dGeomRaySet(geom, vehPos.x(), vehPos.y(), vehPos.z(), camDir.x(), camDir.y(), camDir.z());
	}

	if(_collider.findCollisions(true))
	{
		// looks like we're obstructed
		// loop over the hits and find
		// the closest hist to work with

		const colliderQuery::CollisionList& hits = _collider.getCollisions();
		const colliderQuery::Collision* closest = nullptr;

		for(size_t i = 0; i < hits.size(); i++)
		{
			const colliderQuery::Collision& hit = hits.at(i);

			if(!closest || hit.depth < closest->depth)
				closest = &hit;
		}

		if(closest)
		{
			Vec3d hitDir 	= vehPos - closest->pos;
			FLOAT64 hitDist = hitDir.length();

			// calculate amount to move
			hitDir.normalize();
			hitDir *= ((camDist - hitDist) + 1.0f);

			// adjust final camera position towards the
			// vehicle so that we're no longer obstructed
			camPos += hitDir;
		}
	}

	/*
	// lets calculate our target screen position
	piMATRIX world = *(piGetWorldMatrix());
	piSetWorldMatrix(&IdentityMatrix);

	double x = camPos.x() - WorldOriginOffset.x;
	double y = camPos.y() - WorldOriginOffset.y;
	double z = -camPos.z();

	// screen coordinates
	FLOAT64 sz, sy, sz;
	sx = sy = sz = 0;

	// our screen coords for the position passed in
	piGetWorldToScreen(x, y, z, sx, sy, sz);
	piSetWorldMatrix(&world);
	*/
}

EZManager* EZManager::GetInstance()
{
	return _ezMgr;
}

bool EZManager::CreateManager(vehVehicle* v)
{
	EZManager::FreeManager();

	if(!v)
		return false;

	std::string manager;	// which ez manager class to use
	std::string path;		// source file we used to determine the manager

	if(ezManagerType(v->VehicleType, manager, path))
	{
		if(manager.compare("eztank") == 0)
			_ezMgr = new EZTank(v);
		else if(manager.compare("ezstug") == 0)
			_ezMgr = new EZStug(v);
		else if(manager.compare("ezgun") == 0)
			_ezMgr = new EZATGun(v);
		else if(manager.compare("eztruck") == 0)
			_ezMgr = new EZTruck(v);
		else if(manager.compare("ezboat") == 0)
			_ezMgr = new EZBoat(v);
		else if(manager.compare("ezplane") == 0)
			_ezMgr = new EZPlane(v);
		else if(manager.compare("eztrooper") == 0)
			_ezMgr = new EZTrooper(v);
	}

	if(_ezMgr)
	{
		if(_ezMgr->Init(path.c_str()))
		{
			if(ww2imapSetEZPreset(_ezMgr->GetCategory()))
				ww2imapSetKeymap(v->VehicleType->VehicleTypeNum);

			return true;
		}

		delete _ezMgr;
		_ezMgr = nullptr;
	}

	return false;
}

void EZManager::FreeManager()
{
	if(_ezMgr)
		delete _ezMgr;

	_ezMgr = nullptr;
}

void EZManager::EZCamera::EZAxis::Clamp()
{
	_normalizeAngle(angle);

	// clamp it if needed
	if(angle < minAngle)
		angle = minAngle;
	else if(angle > maxAngle)
		angle = maxAngle;
}

void EZManager::EZCamera::Update(FLOAT64 deltaTime)
{
	if(traverse.move)// || travAdjust)
	{
		/*
		if(!travTime)
			travTime = frametime;

		static FLOAT64 thresholdX = 1000.0f;

		FLOAT64 maxMove = deltaTime * travRate;
		FLOAT64 move	= deltaTime * travMove;

		if((move * move) > (maxMove * maxMove))
			move = maxMove * (travMove < 0) ? -1.0 : 1.0f;

		traverse += move;
		travMove += -move;
		*/

		/*
		FLOAT64 scaledMove = deltaTime * (travRate * ((300 + frametime - travTime)/ thresholdX)) * ((travMove < 0) ? -1.0f : 1.0f);
		FLOAT64 smoothMove = deltaTime * travMove;

		if((scaledMove * scaledMove) <= (smoothMove * smoothMove))
		{
			traverse += scaledMove;
		}
		else
		{
			traverse += smoothMove;
		}
		*/

		traverse.angle += (deltaTime * traverse.move * traverse.rate);

		/*
		if(travAdjust)
		{
			FLOAT64 adjust = deltaTime * travRate;

			if((adjust * adjust) >= (travAdjust * travAdjust))
			{
				traverse += travAdjust;
				travAdjust = 0;
			}
			else
			{
				if(travAdjust < 0)
					adjust = -adjust;

				traverse += adjust;
				travAdjust -= adjust;
			}
		}
		*/

		traverse.Clamp();

		//_normalizeAngle(traverse.angle);

		traverse.move = 0;
	}
	else
	{
		//traverse.time = 0;
	}

	if(elevation.move)
	{
		/*
		if(!elevTime)
			elevTime = frametime;

		static FLOAT64 thresholdY = 1000.0f;
		*/

		/*
		FLOAT64 scaledMove = deltaTime * (elevRate * ((300 + frametime - elevTime)/ thresholdY)) * ((elevMove < 0) ? -1.0f : 1.0f);
		FLOAT64 smoothMove = deltaTime * elevMove;

		if((scaledMove * scaledMove) <= (smoothMove * smoothMove))
		{
			elevation += scaledMove;
		}
		else
		{
			elevation += smoothMove;
		}
		*/

		elevation.angle += (deltaTime * elevation.move * elevation.rate);

		elevation.Clamp();

		/*
		_normalizeAngle(elevation.angle);

		// clamp it if needed
		if(elevation < minAngle)
			elevation = minAngle;
		else if(elevation > maxAngle)
			elevation = maxAngle;
		*/

		elevation.move = 0;
	}
	else
	{
		//elevation.time = 0;
	}
}

void EZManager::EZCamera::Mark()
{
	traverse.mark = traverse.angle;
	elevation.mark = elevation.angle;
}

void EZManager::EZCamera::Snap()
{
	traverse.mark = 0;
	elevation.mark = 0;
}

void EZManager::EZTurret::Init(vehVehicle* v)
{
	vehicle = v;
	FindMount();
}

bool EZManager::EZTurret::FindMount()
{
	if(mount) return true;
	if(!vehicle) return false;

	mlWeaponMount* weapMount = nullptr;
	mlComponent* component = nullptr;

	if(vehicle->ModelData->ComponentList)
	{
		mlComponentListIterator it;
		mlComponentListBegin(vehicle->ModelData->ComponentList, &it);

		for(; it.comp; mlComponentListNext(&it))
		{
			component = it.comp;

			if(component->Type == ML_COMPONENT_WEAPONMOUNT)
			{
				if(!component->SpecialData)
					continue;

				weapMount = (mlWeaponMount*)(component->SpecialData);

				// found a valid mount
				if(weapMount->PositionNum == position)
				{
					mount = weapMount;
					break;
				}
			}
		}
	}

	return (mount != nullptr);
}

void EZManager::EZTurret::PreUpdate()
{
	// update our current data
	//Update();

	// save our last data
	lastScreenPos	= screenPos;
	lastAimPos		= aimPos;
	lastCamPos		= camPos;
	lastAttitude	= attitude;

	// save our turret position in case we need to smooth back
	turretPos.x() = vehicle->ModelData->TurretTraverse[position];
	turretPos.y() = vehicle->ModelData->TurretElevate[position];

	// start moving our turret
	TrackToCamera();
}

void EZManager::EZTurret::Update()
{
	/*
	screenPos.set(0, 0, 0);
	aimPos.set(0, 0, 0);
	attitude.set(0, 1.0f, 0);
	*/

	if(!mount && !FindMount())
		return;

	FLOAT64VECTOR firePoint, fireVelocity, offset;
	piMATRIX attMat;

	// get our firepoint's position and attitude
	mlWeaponGetFireAttPosVel(mount, 0, vehicle->ModelData, &attMat, &offset, &firePoint, &fireVelocity);

	Matrixd rotate = attMat;
	camPos = firePoint;

	Vec3d forward = Vec3d(0, 1.0f, 0);

	forward = forward * rotate;

	// save our turret attitude in the xy plane
	attitude = forward;

	// push the point out
	//forward *= 1000.0f;

	UINT32 collideWith = (COLLIDER_TRN_BUILDING|COLLIDER_TRN_BRIDGE|COLLIDER_TILE_MASK);

	Vec3d physCamPos = physicsGameToPhysical(camPos);

	const colliderQuery::CollisionList& hits = collideRayList(collideWith, 1000.0f, physCamPos, forward);

	if(!hits.empty())
	{
		// our final world turret aim point
		aimPos = physicsPhysicalToGame(hits.at(0).pos);
	}
	else
	{
		//
		forward *= 200.0f;

		// our final world turret aim point
		aimPos = camPos + forward;
	}

	// lets calculate and save our look at screen position
	// so we can adjust turrets towards our look at point
	{
		piMATRIX world = *(piGetWorldMatrix());
		piSetWorldMatrix(&IdentityMatrix);

		double x = aimPos.x() - WorldOriginOffset.x;
		double y = aimPos.y() - WorldOriginOffset.y;
		double z = -aimPos.z();

		// screen coordinates
		FLOAT64 sx, sy, sz;
		sx = sy = sz = 0;

		// our screen coords for the position passed in
		piGetWorldToScreen(x, y, z, sx, sy, sz);
		piSetWorldMatrix(&world);

		//SINT32 isx = (SINT32)sx;
		//SINT32 isy = (SINT32)sy;

		// save it
		screenPos.set(sx, sy, sz);
	}

	//UpdateReticle();
	//TrackToCamera();
}

void EZManager::EZTurret::UpdateReticle()
{
	char name[32];
	snprintf(name, 31, "reticle%d", position);

	phui::View* reticle = uiFindView(name);
	if(!reticle) return;

	TargetView* target = dynamic_cast<TargetView*>(reticle);
	if(!target) return;

	FLOAT64 centerX		= (FLOAT64)WantedWidth * .5;
	FLOAT64 centerY		= (FLOAT64)WantedHeight * .5;

	FLOAT64 xDiff		= fabs(centerX - screenPos.x());
	FLOAT64 yDiff		= fabs(centerY - screenPos.y());

	const FLOAT64 buffer = 3.0f;

	if(xDiff <= buffer && yDiff <= buffer)
	{
		// its close enough so center it
		target->sx = centerX;
		target->sy = centerY;
		target->Update(false);
	}
	else
	{
		// position our reticle to be in align with our barrel
		target->SetTarget(aimPos.x(), aimPos.y(), aimPos.z());
		target->Update();
	}

	if(screenPos.z() >= 1.0 && target->IsVisible())
		target->Hide();
	else if(screenPos.z() < 1.0 && !target->IsVisible())
		target->Show();
}

void EZManager::EZTurret::PostUpdate()
{
	//if(!tracking)
	//	return;

	// update our current data
	Update();

	if(screenPos.z() < 1.0f)
	{
		FLOAT64 centerX = WantedWidth * .5f;
		FLOAT64 centerY	= WantedHeight * .5f;

		// check for traverse overshoots
		if(abs((SINT32)screenPos.x() - (SINT32)centerX) <= CAMERA_THRESHOLD)
		{
			mlTraverseTurretSpeed(0, 0, 0);
		}
		else if((SINT32)lastScreenPos.x() < (SINT32)centerX && (SINT32)screenPos.x() > (SINT32)centerX)
		{
			// we had a traverse left to right overshoot

			// calculate screen x change and move back percentage
			FLOAT64 move		= fabs(screenPos.x() - lastScreenPos.x());
			FLOAT64 pctgMove	= (screenPos.x() - centerX) / move;

			// calculate turret traverse change
			FLOAT64 turretPosX = vehicle->ModelData->TurretTraverse[position];
			FLOAT64 turretDiff = fabs(turretPosX - turretPos.x());

			// center our screen pos
			//screenPos.x() = centerX;

			// move our turret back a little
			vehicle->ModelData->TurretTraverse[position] = (FLOAT32)(turretPosX + (turretDiff * pctgMove));
			vehicle->ModelData->LastTurretTraverse[position] = vehicle->ModelData->TurretTraverse[position];

			mlTraverseTurretSpeed(0, 0, 0);
		}
		else if((SINT32)lastScreenPos.x() > (SINT32)centerX && (SINT32)screenPos.x() < (SINT32)centerX)
		{
			// we had a traverse right to left overshoot

			// calculate screen x change and move back percentage
			FLOAT64 xDiff		= fabs(screenPos.x() - lastScreenPos.x());
			FLOAT64 pctgMove	= (centerX - screenPos.x()) / xDiff;

			// calculate turret traverse change
			FLOAT64 turretPosX = vehicle->ModelData->TurretTraverse[position];
			FLOAT64 turretDiff = fabs(turretPosX - turretPos.x());

			// center our screen pos
			//screenPos.x() = centerX;

			// move our turret back a little
			vehicle->ModelData->TurretTraverse[position] = (FLOAT32)(turretPosX - (turretDiff * pctgMove));
			vehicle->ModelData->LastTurretTraverse[position] = vehicle->ModelData->TurretTraverse[position];

			mlTraverseTurretSpeed(0, 0, 0);
		}

		// check for elevation overshoots
		if(abs((SINT32)screenPos.y() - (SINT32)centerY) <= CAMERA_THRESHOLD)
		{
			mlElevateTurretSpeed(0, 0, 0);
		}
		else if((SINT32)lastScreenPos.y() < (SINT32)centerY && (SINT32)screenPos.y() > (SINT32)centerY)
		{
			// we had a elevation bottom to top overshoot

			// calculate screen y change and move back percentage
			FLOAT64 yDiff		= fabs(screenPos.y() - lastScreenPos.y());
			FLOAT64 pctgMove	= (screenPos.y() - centerY) / yDiff;

			// calculate turret elevation change
			FLOAT64 turretPosY = vehicle->ModelData->TurretElevate[position];
			FLOAT64 turretDiff = fabs(turretPosY - turretPos.y());

			// center our screen pos
			//screenPos.y() = centerY;

			// move our turret back a little
			vehicle->ModelData->TurretElevate[position] = (FLOAT32)(turretPosY - (turretDiff * pctgMove));
			vehicle->ModelData->LastTurretElevate[position] = vehicle->ModelData->TurretElevate[position];

			mlElevateTurretSpeed(0, 0, 0);
		}
		else if((SINT32)lastScreenPos.y() > (SINT32)centerY && (SINT32)screenPos.y() < (SINT32)centerY)
		{
			// we had a elevation top to bottom overshoot

			// calculate screen y change and move back percentage
			FLOAT64 yDiff		= fabs(screenPos.y() - lastScreenPos.y());
			FLOAT64 pctgMove	= (centerY - screenPos.y()) / yDiff;

			// calculate turret elevate change
			FLOAT64 turretPosY = vehicle->ModelData->TurretElevate[position];
			FLOAT64 turretDiff = fabs(turretPosY - turretPos.y());

			// center our screen pos
			//screenPos.y() = centerY;

			// move our turret back a little
			vehicle->ModelData->TurretElevate[position] = (FLOAT32)(turretPosY + (turretDiff * pctgMove));
			vehicle->ModelData->LastTurretElevate[position] = vehicle->ModelData->TurretElevate[position];

			mlElevateTurretSpeed(0, 0, 0);
		}

		//if(update)
		//	Update();
	}

	if(GetStatus() == TURRET_STATUS_INLINE)
	{
		mlTraverseTurretSpeed(0, 0, 0);
		mlElevateTurretSpeed(0, 0, 0);
		tracking = false;
	}
}

EZManager::EZTurret::TURRET_STATUS EZManager::EZTurret::GetStatus()
{
	if(screenPos.z() >= 1.0f) // behind us
		return TURRET_STATUS_TRACKING;

	FLOAT64 centerX					= WantedWidth * .5f;
	FLOAT64 centerY					= WantedHeight * .5f;
	const SINT32 focalThreshold		= 100; // pixels

	SINT32 cxDiff = abs((SINT32)screenPos.x() - (SINT32)centerX);
	SINT32 cyDiff = abs((SINT32)screenPos.y() - (SINT32)centerY);

	if(cxDiff <= focalThreshold && cyDiff <= focalThreshold)
	{
		if(cxDiff <= CAMERA_THRESHOLD && cyDiff <= CAMERA_THRESHOLD * 2)
			return TURRET_STATUS_INLINE;

		return TURRET_STATUS_FOCAL;
	}

	return TURRET_STATUS_TRACKING;
}

void EZManager::EZTurret::TrackToCamera()
{
	// override our game position temporarily
	EZ_MGR->SetGamePosition(position);

	SINT32 speed = 1000;

	// now we're going to move the turret towards the camera
	if(screenPos.length2() && !EZ_MGR->HasFlag(EZManager::FREELOOK))
	{
		FLOAT64 cx = (FLOAT64)WantedWidth * .5;
		FLOAT64 cy = (FLOAT64)WantedHeight * .5;

		UINT32 cxDiff = (UINT32)abs((SINT32)screenPos.x() - (SINT32)cx);

		if(screenPos.z() < 1.0f)
		{
			if(cxDiff > CAMERA_THRESHOLD)
			{
				FLOAT64 rampDown = 60.0f;

				// ramp down the speed of the last 20 pixels
				speed = (cxDiff <= rampDown) ? 200 + SINT32(800.0f * (cxDiff / rampDown)) : 1000;

				if(screenPos.x() > cx)
					mlTraverseTurretSpeed(-speed, 0, 0);
				else if(screenPos.x() < cx)
					mlTraverseTurretSpeed(speed, 0, 0);
			}
			else
			{
				mlTraverseTurretSpeed(0, 0, 0);
			}
		}
		else
		{
			// in this case, we're in the back hemisphere
			if(screenPos.x() > cx)
				mlTraverseTurretSpeed(speed, 0, 0);
			else if(screenPos.x() < cx)
				mlTraverseTurretSpeed(-speed, 0, 0);
		}

		FLOAT64 screenY	= (screenPos.z() >= 1.0f) ? WantedHeight - screenPos.y() : screenPos.y();
		UINT32 cyDiff = (UINT32)abs((SINT32)screenY - (SINT32)cy);

		if(cyDiff > CAMERA_THRESHOLD)
		{
			FLOAT64 rampDown = 30.0f;

			// ramp down the speed of the last 20 pixels
			speed = (cyDiff <= rampDown) ? 200 + SINT32(800.0f * (cyDiff / rampDown)) : 1000;

			if(screenPos.y() > cy)
				mlElevateTurretSpeed(-speed, 0, 0);
			else if(screenPos.y() < cy)
				mlElevateTurretSpeed(speed, 0, 0);
		}
		else
		{
			mlElevateTurretSpeed(0, 0, 0);
		}
	}
	else
	{
		mlTraverseTurretSpeed(0, 0, 0);
		mlElevateTurretSpeed(0, 0, 0);
	}

	// reset our game position
	EZ_MGR->SetGamePosition();
}

void EZManager::EZTurret::Load()
{
	EZManager::EZPosition& pos = EZ_MGR->GetPosition();

	if(!pos.reticle.empty())
	{
		char name[32];
		snprintf(name, 31, "reticle%d", position);
		phui::View* reticle = uiLoadView(pos.reticle.c_str());
		if(reticle) reticle->SetName(name);
	}

	Init(persGetPersonaVehiclePtr());
}

void EZManager::EZTurret::Unload()
{
	char name[32];
	snprintf(name, 31, "reticle%d", position);
	phui::View* retView = uiFindView(name);
	if(retView) retView->Close();

	// override our game position temporarily
	EZ_MGR->SetGamePosition(position);
	{
		// make sure the current turret stops moving
		mlTraverseTurretSpeed(0, 0, 0);
		mlElevateTurretSpeed(0, 0, 0);

		// make sure no guns are firing
		mlPrimaryTriggerDown(0, 0, 0);
		mlSecondaryTriggerDown(0, 0, 0);
	}
	EZ_MGR->SetGamePosition(); // reset
}

bool EZManager::EZView::Load()
{
	persSetPersonaPositionCallback(1, IMAP_MOM_POSITION_1 + position, 0);
	persSetExternalCameraAdjuster(nullptr);

	switch(view)
	{
		case EZManager::VIEW_NORMAL:
		case EZManager::VIEW_INSTRUMENTS:
		case EZManager::VIEW_GUNSIGHT:
			persSetView(view, zoom);
		break;
		default:
			persSetView(0, 0);
			persSetExternalCamera(1, 0, 0);
			persSetExternalCameraAdjuster(ezCameraCallback);
		break;
	}

	if(hatch == EVENT_ONLOAD)
		mlToggleCanopy(1, 0, 0);

	if(deploy == EVENT_ONLOAD)
		mlToggleBombBayDoor(1, 0, 0);

	for(size_t i = 0; i < turrets.size(); i++)
		turrets.at(i).Load();

	return true;
}

bool EZManager::EZView::Unload()
{
	if(hatch == EVENT_ONLOAD)
		mlToggleCanopy(1, 0, 0);

	if(deploy == EVENT_ONLOAD)
		mlToggleBombBayDoor(1, 0, 0);

	for(size_t i = 0; i < turrets.size(); i++)
		turrets.at(i).Unload();

	return true;
}

bool EZManager::EZPosition::Load()
{
	GetView().Load();
	EZ_MGR->SetGamePosition();
	return true;
}

bool EZManager::EZPosition::Unload()
{
	GetView().Unload();
	return true;
}

bool EZManager::EZPosition::InputBlock(SINT32 id)
{
	EZView& v = GetView();

	EZView::INPUT_MAP::iterator found = v.blocks.find(id);

	if(found != v.blocks.end() && found->second == TRUE)
		return true;

	return false;
}

bool EZManager::EZPosition::IsDead()
{
	return (persGetCrewIsDead(GetView().position) > 0);
}

bool EZManager::EZPosition::IsCommandView()
{
	return (GetView().view < 0);
}

void EZManager::EZPosition::NextView()
{
	if(views.size() < 2)
		return;

	int lastView = view;

	if(++view >= (int)views.size())
		view = 0;

	if(view != lastView)
	{
		views[lastView].Unload();
		views[view].Load();
	}
}

EZManager::EZView& EZManager::EZPosition::GetView()
{
	return views[view];
}

UINT32 ezInputControls(SINT32 param, UINT32 id, UINT32 extra)
{
	if(!_ezMgr)
		return 0;

	switch(id)
	{
		case IMAP_EZ_MOVE:			_ezMgr->OnMove(param, id, extra); break;
		case IMAP_EZ_TURN:			_ezMgr->OnTurn(param, id, extra); break;
		case IMAP_EZ_ACTION1:		_ezMgr->OnAction1(param, id, extra); break;
		case IMAP_EZ_ACTION2:		_ezMgr->OnAction2(param, id, extra); break;
		case IMAP_EZ_FREELOOK:		_ezMgr->OnFreeLook(param, id, extra); break;
		case IMAP_EZ_CAMERA_X:		_ezMgr->OnCamera(param, id, extra); break;
		case IMAP_EZ_CAMERA_Y:		_ezMgr->OnCamera(param, id, extra); break;
		case IMAP_EZ_NEXTPOS:		_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_EZ_PREVPOS:		_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_1:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_2:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_3:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_4:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_5:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_6:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_7:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_8:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_9:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_MOM_POSITION_10:	_ezMgr->OnPosition(param, id, extra); break;
		case IMAP_EZ_ENGINE:		_ezMgr->OnEngine(param, id, extra); break;
		case IMAP_LOOK_FORWARD:			_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_BACK:			_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_DOWN:			_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_FORWARD_RIGHT:	_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_FORWARD_LEFT:	_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_RIGHT:			_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_LEFT:			_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_BACK_RIGHT:		_ezMgr->OnView(param, id, extra); break;
		case IMAP_LOOK_BACK_LEFT:		_ezMgr->OnView(param, id, extra); break;
	}

	return 0;
}

bool ezCameraCallback(piVECTOR* pos, piVECTOR* aim)
{
	if(EZ_MGR)
		return EZ_MGR->OnCamera(pos, aim);

	return true;
}