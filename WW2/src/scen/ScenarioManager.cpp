#include <syscfg.h> // Path is project specific.
#include <stdarg.h>	// var args

#include "scen/scenMain.h"
#include "scen/ScenarioManager.h"
#include "wwii/wwiiServerTrackedObject.h"
#include "client/clientSTO.h"
#include "ui/uiDotCommands.h"
#include "fx/TransformEffect.h"
#include "ob/obMgr.h"
#include "ob/obcs.h"
#include "draw/draw3d.h"
#include "math/mathnum.h"
#include "ml/mlutility.h"
#include "fx/fxSnd.h"
#include "fio/fio.h"
#include "ui/uiMain.h"
#include "ui/uiGameEvents.h"
#include "trn/trnClutter.h"
#include "trn/speedgrass.h"
#include "trn/trntod.h"
#include "ui/uiChat.h"
#include "spt/Random.h"
#include "shader/glslWater.h"
#include "trn/trnsky.h"
#include "trn/trntypes.h"
#include "trn/trnvars.h"
#include "osgImport/ShadowMap.h"
#include "spt/sptmain.h"
#include "weap/weapammo.h"
#include "physics/physicsmain.h"
#include "pers/persmain.h"
#include "fp/fpFPS.h"
#include "client/GameClient.h"
#include "inst/instDescriptor.h"
#include "map/mapArenaDescription.h"
#include "pers/pershud.h"
#include "gr2/gr2Anim.h"
#include "veh/vehupdate.h"
#include "veh/vehcomms.h"
#include "ob/obpool.h"

// ca2 custom terrain
#include "trn/trnScenario.h"
#include "trn/trnmain.h"
#include "fx/fxWeather.h"
#include "ml/mltypes.h"

#ifdef BETA_RELEASE
#include "pi/piinfo.h"
#endif

/*
#include "include/vars.h"
#include "Random.h"
#include "veh/vehmain.h"
#include "imap/iac.h"
#include "ui/uiMain.h"
#include "trnvars.h"
#include "ml/mlhead.h"
#include "ui/uiChat.h"
#include "fx/fxSnd.h"
#include "ui/uiSound.h"
#include "math/matheul.h"
#include "xml/XMLLoader2.h"
*/

// types
struct content_command
{
	void reset()
	{
		command[0] = 0;
		team	= 0;
		round	= 0;
		stage	= 0;
	}

	char command[32+1];
	UINT32 team;
	UINT32 round;
	UINT32 stage;
};

// defines
#define MAPS_PATH "scenarios/terrain/"
#define MAPS_COMMON_PATH MAPS_PATH "common/"
#define ANON_OBJECT_NAME "x_anonymous"

// private variables
#ifdef BETA_RELEASE
std::vector<Vec3d> _editArea;
std::vector<std::pair<const char*,int> > _editLines;
static int _lineCount = 0;
#endif

// static functions
#if(0)
static void _normalizeAngle(double& angle);
#endif

void _extractNumbers(const char* source, double& x, double& y, double& z);

#ifdef BETA_RELEASE
void _addEditLine(const char* name, int id, Vec3d pos, UINT32 color);
void _addEditLink(const char* name, int id, Vec3d pt1, Vec3d pt2, UINT32 color);
void _removeEditLine(const char* name, int id);
#endif

// singleton
static ScenarioManager _scenMgr;

/************************************************************************************************
	Scenario
*************************************************************************************************/

void Scenario::Init(const char* p)
{
	// scenario file that this scenario was created from
	path = p;

	const char* key = nullptr;

	key = Config("name");
	if(key) name = key;

	key = Config("terrain");
	if(key) terrain = key;

	key = Config("offset");
	if(key) _extractNumbers(key, offset.x(), offset.y(), offset.z());
}

const char* Scenario::Config(const char* name)
{
	jsAttributeMap::iterator key = find(name);

	if(key != end())
		return key->second.c_str();

	return nullptr;
}

bool Scenario::GetParent(jsContentData& data)
{
	std::string locPath = "loc://";
	locPath += path;

	size_t index = locPath.rfind("/");

	if(index != std::string::npos)
		locPath.erase(index);

	locPath += "/index.xml";

	return jsContent::GetData(locPath.c_str(), &data, false);
}

bool ScenarioPlayer::IsActive()
{
	return true; //((frametime - lastUpdate) <= (10 * 1000));
}

std::string ScenarioPlayer::GetAttribute(const char* name)
{
	std::string value;

	if(attributes.find(name) != attributes.end())
		value = attributes[name];

	return value;
}

// queue's an update to be sent up to the server
void ScenarioPlayer::Set(const char* name, SINT32 value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%d", value);
	Set(name, strValue);
}

void ScenarioPlayer::Set(const char* name, UINT32 value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%u", value);
	Set(name, strValue);
}

void ScenarioPlayer::Set(const char* name, bool value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%s", (value) ? "true" : "false");
	Set(name, strValue);
}

void ScenarioPlayer::Set(const char* name, float value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%f", value);
	Set(name, strValue);
}

void ScenarioPlayer::Set(const char* name, const char* value)
{
	char buffer[255+1];
	snprintf(buffer, 255, "scenario.SetPlayerState('%s', '%s')", name, value);
	TUT_MGR->RunCode(buffer);
}

// queue's an update to be sent up to the server
void ScenarioPlayer::Add(const char* name, SINT32 value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%d", value);
	Add(name, strValue);
}

void ScenarioPlayer::Add(const char* name, UINT32 value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%u", value);
	Add(name, strValue);
}

void ScenarioPlayer::Add(const char* name, bool value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%s", (value) ? "1" : "-1");
	Add(name, strValue);
}

void ScenarioPlayer::Add(const char* name, float value)
{
	char strValue[50+1];
	snprintf(strValue, 50, "%f", value);
	Add(name, strValue);
}

void ScenarioPlayer::Add(const char* name, const char* value)
{
	char buffer[255+1];
	snprintf(buffer, 255, "scenario.AddPlayerState('%s', '%s')", name, value);
	TUT_MGR->RunCode(buffer);
}

// receive an update of a player from the server
void ScenarioPlayer::Receive(const char* name, const char* value)
{
	attributes[name] = value;
	lastUpdate = frametime;
}

// get our current weapon
WEAPON_SPEC ScenarioPlayer::GetWeaponSpec()
{
	WEAPON_SPEC spec(0, 0, 0, 0);

	std::string key = GetAttribute("weapon");

	if(!key.empty())
	{
		SINT32 iKey = atoi(key.c_str());
		spec.fromShort((UINT16)iKey);
	}

	return spec;
}

SINT32 ScenarioPlayer::GetTeam()
{
	std::string team = GetAttribute("team");

	if(!team.empty())
		return atoi(team.c_str());

	return 0;
}

SINT32 ScenarioPlayer::GetKills()
{
	std::string kills = GetAttribute("kills");

	if(!kills.empty())
		return atoi(kills.c_str());

	return 0;
}

SINT32 ScenarioPlayer::GetDeaths()
{
	std::string deaths = GetAttribute("deaths");

	if(!deaths.empty())
		return atoi(deaths.c_str());

	return 0;
}

SINT32 ScenarioPlayer::GetScore()
{
	std::string score = GetAttribute("score");

	if(!score.empty())
		return atoi(score.c_str());

	return 0;
}

bool ScenarioPlayer::GetReady()
{
	std::string ready = GetAttribute("ready");

	if(!ready.empty() && strcasecmp(ready.c_str(), "true") == 0)
		return true;

	return false;
}

bool ScenarioPlayer::GetSpawned()
{
	std::string spawned = GetAttribute("spawned");

	if(!spawned.empty() && strcasecmp(spawned.c_str(), "true") == 0)
		return true;

	return false;
}

RANK_NO ScenarioPlayer::GetRank()
{
	SINT32 score = GetScore();
	SINT32 maxRank = persrankMax();
	SINT32 scorePerRank = 5;
	SINT32 scoredRank = 0;

	for(SINT32 r = maxRank - 1; r >= 0; r--)
	{
		const persRank* rank = persrankFind(r);

		if(rank && rank->Type() == persRank::COMMAND)
		{
			scoredRank = r;
			break;
		}
	}

	// each level takes 5 points
	SINT32 scoreRank = SINT32(score / scorePerRank);

	if(scoreRank > scoredRank)
		scoreRank = scoredRank;

	return (RANK_NO)scoreRank;
}

vehVehicle* ScenarioPlayer::GetVehicle()
{
	return vehFindVehicleByChatHandle(handle);
}

bool ScenarioPlayer::IsMe()
{
	return (handle == PlayerClient()->Personas().m_callsign);
}

FLOAT64 ScenarioPlayer::GetX()
{
	std::string x = GetAttribute("x");

	if(!x.empty())
		return atof(x.c_str());

	return 0.0f;
}

FLOAT64 ScenarioPlayer::GetY()
{
	std::string y = GetAttribute("y");

	if(!y.empty())
		return atof(y.c_str());

	return 0.0f;
}

/************************************************************************************************
	ScenarioManager
*************************************************************************************************/

ScenarioManager::ScenarioManager() :
#ifdef BETA_RELEASE
	_editMode(false),
#endif
    _flags(0),
    _globalflag(GLOBAL_NONE),
    _round(0),
    _stage(0),
    _offlineTeam(1),
    _offlineRound(1),
    _offlineStage(1),
	_soundUpdate(0),
	_playerUpdate(0),
    _lastUpdate(0),
    _updatePlayers(true),
    _terrainReadyToLoad(false),
    _scenarioIsComplete(true),
    _scenario(nullptr),
    _map(nullptr)
{
	_respawnWeapon = NULL_WEAP_SPEC;
}

ScenarioManager::~ScenarioManager()
{
}

void ScenarioManager::Init()
{
}

bool ScenarioManager::Start(const char* path, bool init)
{
	if(IsActive())
		Stop();

	_flags = 0;
	_round = (!Online) ? _offlineRound : 1;
	_stage = (!Online) ? _offlineStage : 1;

	// make sure we're on the right prepath
	TUT_MGR->SetPrePath("scenarios/");

	_scenario = scenMainLoad(path);

    if(!_scenario)
    {
        uiChatPrintLine("scenMainLoad: %s FAILED", path);
        return false;
    }

	const char* rules = _scenario->Config("rules");
	if(!rules) rules = _scenario->name.c_str();

	// create a path to our rules script
	std::string script = "rules/";
	script += rules;
	//script += ".lua";

#if platOS == OS_MACOS
	script = "rules/mac";
#endif

	if(_scenario && TUT_MGR->Start(script.c_str()))
	{
		gEZMode = 2; // ez mode
		_scenarioIsComplete = false;

#ifdef BETA_RELEASE
#if DEBUG_WW2
    // Do nothing
#else
		if(!Online)
			SetEditMode(true);
#endif
#endif

		// set spawn override
		OverrideSpawner = SCENARIO_OVERRIDE;

		if(!_scenario->terrain.empty())
		{
			// custom terrain doesn't need a physics offset
			FLOAT64VECTOR offset = {0, 0, 0};
			physicsSetWorldOffset(offset);

			// load up any inheritable map directories
			SetupTerrainPaths(_scenario->terrain.c_str());

			// determine which map model to use
			DetermineMapModel();

			// last fallback parent is the common directory
			mapPaths.push_back("common");

			// we're using custom terrain, let the rest of the code know it
			gCustomTerrain = TRUE;

			// tell our lua script about our terrain
			SetConfig("terrain", _scenario->terrain.c_str());

			// make sure our nospawn terrain is loaded
			trnmainDoRawUpdate(FALSE);
		}

		// pass any config variables to the lua script
		jsAttributeMap::iterator key = _scenario->begin();

		for(; key != _scenario->end(); key++)
		{
			const std::string& name		= key->first;
			const std::string& value	= key->second;

			SetConfig(name.c_str(), value.c_str());
		}

		// temp buffer for adding scenario variables
		char buffer[25+1];

		ChatHandle myHandle = 0;

		if(Online)
		{
			const Instance::Descriptor& instance = PlayerClient()->GetInstance();

			// convert our instance id into a string
			strprintf(buffer, "%" PR64BitUT "", instance.id());

			// tell our script what our instance id is
			// this is needed for proper network comms
			SetConfig("instance", buffer);

			// convert our instance seed into a string
			//snprintf(buffer, 25, "%u", instance.seed());

			// tell our script what our instance seed is
			// this allows shared "random" events to be seeded
			//SetConfig("seed", buffer);

			// grab my handle so we tell if we're the creator or not
			myHandle = PlayerClient()->Personas().m_callsign;

			SetConfig("creator", (instance.handle() == myHandle) ? "true" : "false");
			SetConfig("init", (init) ? "true" : "false");

			ARENA_VARS::iterator itr = g_arenaDescription.find("scenario_server");

			if(itr != g_arenaDescription.end())
				SetConfig("server", itr->second.c_str());
		}
		else
		{
			int id = GetRandom(10000, 20000);
			char buff[32+1];
			snprintf(buff, 32, "%d", id);
			SetConfig("instance", buff);
			SetConfig("creator", "true");
			SetConfig("init", "true");

			myHandle = chatGetHandle(L"offline");
		}

		// set up our player data in the lua scenario lib
		std::wstring callsign = chatGetWideCallsign(myHandle);
		SetConfig("callsign", UTF8(callsign.c_str()));

		// go ahead and add us to our player list
		players[myHandle] = ScenarioPlayer(myHandle);

		// force a load of any scenario content
		LoadContent();

		// default time of day and weather
		UINT32 hour = 12 * 3600;
		SINT32 weather = 1;
		SINT32 fog = 1;

		const char* hourOfDay = _scenario->Config("hour");
		if(hourOfDay && sscanf(hourOfDay, "%u", &hour) == 1)
			hour *= 3600;

		// now set our weather
		const char* w = _scenario->Config("weather");
		if(w) sscanf(w, "%d", &weather);

		const char* f = _scenario->Config("fog");
		if(f) sscanf(f, "%d", &fog);

		// kill our multiplier, time stands still
		trnTODSetTimeMultiplier(0);

		// set the time of day
		trnTODSetTimeOfDay(hour);

		// set our weather and fog
		fxWeatherSetState(weather, 14);
		//trnskyChangeSetting(weather, fog, true);

		// run any associated commands
		RunCommands(TUT_EVENT_START);

		// run events by default
		SetFlag(ScenarioManager::RUN_EVENTS, true);

		LoadObjects(ScenarioObject::ON_START);

		// start our scenario networking
		SetConfig("network", "start");

		// we're starting round 1, stage 1 at this point
		RunObject("onroundstart", 1);
		RunObject("onstagestart", 1);

		// run onspawn objects
		if(uiInGame())
		{
			LoadTerrain();
			LoadObjects(ScenarioObject::ON_SPAWN);
		}

		ScenarioPlayer* me = GetPlayer();
		if(me) me->Set("team", GetTeam());
	}
	else
	{
		Stop();
	}

	return true;
}

void ScenarioManager::Update()
{
	if(!IsActive()) return;

    // Force a load one frame after _terrainReadyToLoad is set
    if(_terrainReadyToLoad && frametime > _startTimeTerrainLoad)
    {
		//uiConsoleprintf("SCENARIO: Spawning terrain from scenario update!");
        // Once again, our world origin offset bites us in the ass.
        _terrainReadyToLoad = false;
        LoadTerrain();
    }

	bool inGame = uiInGame();

	if(inGame)
	{
		if((_flags & ScenarioManager::RUN_EVENTS))
		{
			// events need to be processed every frame
			for(size_t t = 0; t < events.size(); t++)
			{
				ScenarioEvent* evt = events.at(t);

				if(!RoundRelevant(evt))
					continue;

				if(!evt->eventInterval || frametime < evt->nextEvent)
					continue;

				RunEvent(evt);
			}

			// handle positional unnamed sounds here once a second
			if(frametime >= _soundUpdate)
			{
				vehVehicle* player = persGetPersonaVehiclePtr();
				Vec3d myPos = Vec3d(player->ModelData->VehiclePos);

				SCENSND_LIST remove;
				SCENSND_LIST::iterator it = sounds.begin();

				for(; it != sounds.end(); it++)
				{
					ScenarioSound* sound = *it;

					if(!RoundRelevant(sound))
						continue;

					Vec3d diff	= sound->position - myPos;

					// try to load it if we're in range
					if(diff.length2() <= sound->maxDist * sound->maxDist)
						LoadObject(sound, ScenarioObject::ON_RUN_OBJECT);

					// if we were able to play our sound
					// erase it from the sounds we are monitoring
					// it will get popped back on the list
					// once the sound stops playing
					if(sound->id >= 0)
						remove.insert(sound);
				}

				// remove any playing sounds
				if(!remove.empty())
				{
					it = remove.begin();

					for(; it != remove.end(); it++)
						sounds.erase(*it);
				}

				_soundUpdate = frametime + 500;
			}
		}

		if(!updaters.empty())
		{
			for(size_t u = 0; u < updaters.size(); u++)
			{
				ScenarioObject* ob = updaters[u];

				if(RoundRelevant(ob))
					ob->Update();
			}
		}
	}

	if(frametime >= _playerUpdate)
	{
		if(inGame)
		{
			ScenarioPlayer* me = GetPlayer();

			if(me)
			{
				vehVehicle* myVehicle = me->GetVehicle();

				if(myVehicle)
				{
					me->Set("x", (FLOAT32)myVehicle->ModelData->VehiclePos.x);
					me->Set("y", (FLOAT32)myVehicle->ModelData->VehiclePos.y);
				}
			}
		}

		if(_updatePlayers)
		{
			uiRunScript("game.onscenarioplayers()");
			_updatePlayers = false;
		}

		_playerUpdate = frametime + 1000;
	}

	/*
	static UINT32 _stoCount = 0;
	static UINT32 _objCount = 0;

	UINT32 stoCount = wwiiCountSTOs();
	UINT32 objCount = objects.size();

	if(stoCount != _stoCount || objCount != _objCount)
	{
		uiChatPrintLine("SCEN COUNTS: objects %u -> %u    stos %u -> %u", _objCount, objCount, _stoCount, stoCount);

		_stoCount = stoCount;
		_objCount = objCount;
	}
	*/


	_lastUpdate = frametime;
}

// player update coming from the server
void ScenarioManager::UpdatePlayer(const char* callsign, const char* state, const char* value)
{
	// let the UI know we have updated info about players
	_updatePlayers = true;

	ChatHandle handle = chatGetHandle(UTF16(callsign));
	SCENPLAYER_MAP::iterator found = players.find(handle);

	// team == 0 means remove the player
	if(strcasecmp(state, "team") == 0 && atoi(value) == 0)
	{
		if(found != players.end())
			players.erase(found);

		return;
	}

	ScenarioPlayer* player = nullptr;

	if(found != players.end())
		player = &found->second;

	if(!player)
	{
		players[handle] = ScenarioPlayer(handle);
		player = &players[handle];
	}

	// save our attribute
	player->Receive(state, value);
}

bool ScenarioManager::Stop()
{
#ifdef BETA_RELEASE
    SetEditMode(false);
#endif

	// run any associated slash commands
	RunCommands(TUT_EVENT_STOP);

	// unload any scenario content
	UnloadContent();

	// clear our list of scenarios players
	players.clear();

	// reset our parent terrain paths
	mapPaths.clear();

	// kill current lua script
	TUT_MGR->Stop(false);

	// kill our custom map if we have one
	if(_map)
	{
		_map->setExpired();
		_map = nullptr;
	}

	_scenario = nullptr;

	// kill spawn override
	OverrideSpawner = NO_OVERRIDE;

	// reset back to normal terrain handling
	gCustomTerrain = FALSE;

	// disable ez mode in case the scenario turned it on
	gEZMode = 0;

	// make sure the physics offset gets reset
	FLOAT64VECTOR offset = { 18*SUPERCELL_WIDTH, 235*SUPERCELL_HEIGHT, 0};
	physicsSetWorldOffset(offset);

    trnClutterKill();
    sptmainInit();
    speedgrassInit();
    glslInitWaterTexture();
    trnskyInitSkyBox();
    gr2AnimExpireCorpses();

    trnmainSetDraw(true);

	// reset time of day
	trnTODInit();

    if(!Online)
        fxWeatherSetState(0, 0);

	_scenarioIsComplete = true;
	_flags = 0;
	_round = 0;
	_stage = 0;

	HUD_MGR->Reset();

	// set this back to false in case the scenario set it
	fpFPSSetAutoReload(FALSE);

	return true;
}

int ScenarioManager::VehicleLimit(const char* id)
{
	ScenarioObject* limit = FindObject(id, ScenarioObject::LIMIT);
	if(limit) return limit->flags;

	return -1;
}

int ScenarioManager::VehicleLimit(const vehType* vtype)
{
	return VehicleLimit((const char*)vtype->Id);
}

int ScenarioManager::VehicleCount(const vehType* vtype)
{
	int count = 0;

	if(!vtype)
		return count;

	/*
	const std::vector<MissionMember>& members = PlayerClient()->MissionMembers();

	for(size_t i = 0; i < members.size(); i++)
	{
		const MissionMember& member = members.at(i);

		if(member.m_weapon == vtype->WeaponSpec)
			++count;
	}
	*/

	SCENPLAYER_MAP::iterator it = players.begin();

	for(; it != players.end(); it++)
	{
		if(it->second.GetWeaponSpec() == vtype->WeaponSpec)
			++count;
	}

	return count;
}

bool ScenarioManager::VehiclePermitted(const vehType* vtype)
{
	SCENOBJ_LIST list;

	if(FindObjects(list, ScenarioObject::WEAPONPACK))
	{
		for(size_t i = 0; i < list.size(); i++)
		{
			ScenarioVehicleMask* weapPack = dynamic_cast<ScenarioVehicleMask*>(list.at(i));
			if(!weapPack || !weapPack->IsMatch(vtype)) continue;

			// it is relevant, look to see if they have the proper permission
			return ScenarioPermission::IsPermitted(weapPack->name.c_str());
		}
	}

	return true;
}

bool ScenarioVehicleMask::IsMatch(const vehType* vtype)
{
	// bail if this mask is not relevant to this vehicle type
	if(spec.wCountry > 0 && spec.wCountry != vtype->WeaponSpec.wCountry)
		return false;

	if(spec.wCategory > 0 && spec.wCategory != vtype->WeaponSpec.wCategory)
		return false;

	if(spec.wClass > 0 && spec.wClass != vtype->WeaponSpec.wClass)
		return false;

	if(spec.wType > 0 && spec.wType != vtype->WeaponSpec.wType)
		return false;

	return true;
}

// returns the closest match to our vehicle type
const ScenarioVehicleMask* ScenarioManager::VehicleMask(const vehType* vtype, bool best, bool spawnable)
{
	const ScenarioVehicleMask* match = nullptr;
	UINT32 specificity = 0;

	for(size_t i = 0; i < vehicles.size(); i++)
	{
		ScenarioVehicleMask* mask = vehicles.at(i);

		if(!RoundRelevant(mask))
			continue;

		if(spawnable && FindObject(mask->name.c_str(), ScenarioObject::SPAWN, true) == nullptr)
			continue;

		if(!mask->IsMatch(vtype))
			continue;

		/*
		if(mask->spec.wCountry > 0 && mask->spec.wCountry != vtype->WeaponSpec.wCountry)
			continue;

		if(mask->spec.wCategory > 0 && mask->spec.wCategory != vtype->WeaponSpec.wCategory)
			continue;

		if(mask->spec.wClass > 0 && mask->spec.wClass != vtype->WeaponSpec.wClass)
			continue;

		if(mask->spec.wType > 0 && mask->spec.wType != vtype->WeaponSpec.wType)
			continue;
		*/

		// if we got to here, it's valid

		if(!match)
		{
			match = mask;

			// as soon as we find one, break out if we don't care if its the best
			if(!best) break;

			if(mask->spec.wCountry > 0) specificity++;
			if(mask->spec.wCategory > 0) specificity++;
			if(mask->spec.wClass > 0) specificity++;
			if(mask->spec.wType > 0) specificity++;
		}
		else
		{
			// see if this one is more specific than the current best
			UINT32 currentSpecificity = 0;

			if(mask->spec.wCountry > 0) currentSpecificity++;
			if(mask->spec.wCategory > 0) currentSpecificity++;
			if(mask->spec.wClass > 0) currentSpecificity++;
			if(mask->spec.wType > 0) currentSpecificity++;

			if(currentSpecificity > specificity)
			{
				match = mask;
				specificity = currentSpecificity;
			}
		}
	}

	return match;
}

void ScenarioManager::AddSound(ScenarioSound* sound)
{
	if(!sound)
		return;

	// has potential for creating duplicate entries
	sounds.insert(sound);
}

int ScenarioManager::SetTeam(int t)
{
	_offlineTeam = t;
	return _offlineTeam;
}

void ScenarioManager::SetOfflineData(UINT32 t, UINT32 r, UINT32 s)
{
	SetTeam(t);
	_offlineRound = r;
	_offlineStage = s;
}

int ScenarioManager::GetTeam()
{
	if(Online)
	{
		return (int)wwiiSide((UINT32)PlayerClient()->PersonaSelected().m_countryID);
	}
	else
	{
		vehVehicle* player = persGetPersonaVehiclePtr();

		if(player)
			return (int)wwiiSide((UINT32)player->VehicleType->WeaponSpec.country());

		return _offlineTeam;
	}

	return 0;
}

int ScenarioManager::GetOtherTeam()
{
	return (GetTeam() == 1) ? 2 : 1;
}

bool ScenarioManager::SameTeam(vehVehicle* player)
{
	if(!player) return false;
	return (GetTeam() == player->Side);
}

bool ScenarioManager::RoundRelevant(ScenarioObject* ob)
{
	if(!ob)
		return false;
	else if(ob->team && ob->team != GetTeam())
		return false;
	else if(ob->round && _round != ob->round)
		return false;
	else if(ob->stage && _stage != ob->stage)
		return false;

	return true;
}

bool ScenarioManager::PlayerRelevant(ScenarioObject* ob)
{
	if(!RoundRelevant(ob))
		return false;

	return PlayerRelevant(ob->name.c_str());
}

// is this name relevant to the player vehicle
bool ScenarioManager::PlayerRelevant(const char* name)
{
	if(!uiInGame())
		return false;

	ScenarioObject* ob = FindObject(name, ScenarioObject::VEHICLE);

	if(ob)
	{
		ScenarioVehicleMask* mask = dynamic_cast<ScenarioVehicleMask*>(ob);
		if(!mask) return true;

		vehVehicle* player = persGetPersonaVehiclePtr();
		if(!player) return false;

		WEAPON_SPEC& spec = player->VehicleType->WeaponSpec;

		if(mask->spec.wCountry > 0 && mask->spec.wCountry != spec.wCountry)
			return false;

		if(mask->spec.wCategory > 0 && mask->spec.wCategory != spec.wCategory)
			return false;

		if(mask->spec.wClass > 0 && mask->spec.wClass != spec.wClass)
			return false;

		if(mask->spec.wType > 0 && mask->spec.wType != spec.wType)
			return false;
	}

	return true;
}

static bool _configValue(const char* name, const char* wantedName, const char* value, const char* wantedValue = "true")
{
	if(strcasecmp(name, wantedName) == 0 && strcasecmp(value, wantedValue) == 0)
		return true;

	return false;
}

void ScenarioManager::SetConfig(const char* name, const char* value)
{
	if(_configValue(name, "enemydeaths", value))
		SetFlag(ScenarioManager::ENEMY_DEATHS);
	else if(_configValue(name, "enemyicons", value))
		SetFlag(ScenarioManager::ENEMY_ICONS);
	else if(_configValue(name, "fastenemyicons", value))
		SetFlag(ScenarioManager::FAST_ENEMY_ICONS);
	else if(_configValue(name, "firingenemyicons", value))
		SetFlag(ScenarioManager::FIRING_ENEMY_ICONS);
	else if(_configValue(name, "hideproneenemyicons", value))
		SetFlag(ScenarioManager::HIDE_PRONE_ENEMY_ICONS);
	else if(_configValue(name, "enemyicontypeonly", value))
		SetFlag(ScenarioManager::ENEMY_ICON_TYPES_ONLY);
	else if(_configValue(name, "autoreload", value))
		fpFPSSetAutoReload(TRUE);
	else if(strcasecmp(name, "ezmode") == 0)
		gEZMode = (UINT32)atoi(value);

	TUT_MGR->DoEvent(TUT_EVENT_SET, name, value);
}

void ScenarioManager::SetRespawnWeapon(const WEAPON_SPEC spec)
{
	_respawnWeapon = spec;
}

bool ScenarioManager::Respawn()
{
	return Spawn(vehTypeGetBySpec(_respawnWeapon));
}

bool ScenarioManager::Spawn(const vehType* vehtype)
{
	if(!vehtype)
		return false;

	//uiConsoleprintf("SCENARIO: >>>>>>>>>>>>>>>>>");

	FLOAT64VECTOR at = { 0.0f, 0.0f, 0.0f };
	FLOAT64 yaw = 0;

	const ScenarioWorldObject* spawn = SCEN_MGR->VehicleSpawn(vehtype);
	if(!spawn && Online) return false;

	if(spawn)
	{
		//uiConsoleprintf("SCENARIO: Spawn object found!");
		at.x	= spawn->position.x();
		at.y	= spawn->position.y();
		at.z	= spawn->position.z();
		yaw		= spawn->attitude.Yaw;

		if(vehtype->Type != VEH_TYPE_PERSON)
		{
			// yaw needs to be adjusted for non-troopers
			yaw = mlRAD_TO_DEG(yaw);
			yaw -= 180.0f;
			gr2NormalizeAngle(yaw);
			yaw = mlDEG_TO_RAD(yaw);
		}
	}
	else
	{
		//uiConsoleprintf("SCENARIO: Spawn object not found!");
	}

	// send the request
	if(Online)
	{
		PlayerClient()->RequestDirectSpawn(vehtype->WeaponSpec, &at, yaw);
	}
	else
	{
		//uiConsoleprintf("SCENARIO: Spawning at %.2f, %.2f, %.2f", &at.x, &at.y, &at.z);
		mlSetSpawnStuff(0,0,-1,1);
		mlSetMobileSpawnStuff(0, at.x, at.y, at.z, yaw);
		WEAPON_SERIAL blah={0};
		vehcommSendVehicleRequest(vehtype->VehicleTypeNum,&blah);
	}

	// save our respawn weapon spec
	SetRespawnWeapon(vehtype->WeaponSpec);

	if(Online)
		uiSetDespawnInfo("scenarios/scenario");
	else
		uiSetDespawnInfo("offline");

	return true;
}

void ScenarioManager::SetComplete()
{
	_scenarioIsComplete = true;
}

void ScenarioManager::PushGlobalFlag(unsigned int flag)
{
    _globalflag |= flag;
}

void ScenarioManager::PopGlobalFlag(unsigned int flag)
{
    _globalflag &= ~flag;
}

bool ScenarioManager::SetFlag(SINT32 flag, bool on)
{
	if(!_scenario)
		return false;

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

bool ScenarioManager::HasFlag(SINT32 flag)
{
	if(!_scenario)
		return false;

	return (_flags & flag) > 0;
}

int ScenarioManager::GetPlayerCount(int team)
{
	if(team == 0)
		return (int)players.size();

	SINT32 count = 0;

	SCENPLAYER_MAP::iterator it = players.begin();

	for(; it != players.end(); it++)
	{
		if(it->second.GetTeam() == team)
			count++;
	}

	return count;
}

int ScenarioManager::GetReadyCount(int team)
{
	SINT32 count = 0;

	SCENPLAYER_MAP::iterator it = players.begin();

	for(; it != players.end(); it++)
	{
		ScenarioPlayer& player = it->second;

		if((team == 0 || player.GetTeam() == team) && player.GetReady())
			count++;
	}

	return count;
}

SCENPLAYER_MAP& ScenarioManager::GetPlayers()
{
	return players;
}

ScenarioPlayer* ScenarioManager::GetPlayer(const char* callsign)
{
	if(callsign && *callsign)
		return GetPlayer(UTF16(callsign));
	else
		return GetPlayer();
}

ScenarioPlayer* ScenarioManager::GetPlayer(const wchar_t* callsign)
{
	ChatHandle handle = 0;

	if(callsign == nullptr)
	{
		if(!Online)
			handle = chatGetHandle(L"offline");
		else
			handle = PlayerClient()->Personas().m_callsign;
	}
	else
	{
		handle = chatGetHandle(callsign);
	}

	if(!handle)
		return nullptr;

	SCENPLAYER_MAP::iterator found = players.find(handle);

	if(found != players.end())
		return &found->second;

	return nullptr;
}

int ScenarioManager::GetRound()
{
	return _round;
}

int ScenarioManager::GetStage()
{
	return _stage;
}

Scenario* ScenarioManager::GetScenario()
{
	return _scenario;
}

void ScenarioManager::SetPlayerState(const char* name, const char* value)
{
	ScenarioPlayer* me = GetPlayer();
	if(me) me->Set(name, value);
}

void ScenarioManager::AddPlayerState(const char* name, const char* value)
{
	ScenarioPlayer* me = GetPlayer();
	if(me) me->Add(name, value);
}

int ScenarioManager::SetRound(int r)
{
	int lastRound = _round;
	int nextRound = (r == -1) ? ++_round : r;
	int nextStage = (r != _round) ? 1 : _stage;

	if(nextRound != lastRound)
		SetPhase(nextRound, nextStage);

	return GetRound();
}

int ScenarioManager::SetStage(int s)
{
	int lastStage = _stage;
	int nextStage = (s == -1) ? ++_stage : s;

	if(nextStage != lastStage)
		SetPhase(_round, nextStage);

	return GetStage();
}

void ScenarioManager::SetPhase(int r, int s)
{
	if(r == _round && s == _stage)
		return;

	if(r != _round)
	{
		RunObject("onstageend", 1);
		RunObject("onroundend", 1);
	}
	else if(s != _stage)
	{
		RunObject("onstageend", 1);
	}

	// unload the last round/stage objects
	for(size_t i = 0; i < objects.size(); i++)
	{
		ScenarioObject* ob = objects.at(i);

		if(ob->round == _round && ob->stage == _stage)
			UnloadObject(ob);
	}

	int lastRound = _round;
	int lastStage = _stage;

	_round = r;
	_stage = s;

	// load next current round/stage
	for(size_t i = 0; i < objects.size(); i++)
	{
		ScenarioObject* ob = objects.at(i);

		if(ob->round == _round && ob->stage == _stage)
			LoadObject(ob, ScenarioObject::ON_CONTENT_LOAD);
	}

	if(r != lastRound)
	{
		RunObject("onroundstart", 1);
		RunObject("onstagestart", 1);
	}
	else if(s != lastStage)
	{
		RunObject("onstagestart", 1);
	}

	// run onspawn objects
	if(uiInGame())
	{
		RunObject("onspawn", 1);
		LoadObjects(ScenarioObject::ON_SPAWN);
	}
}

// set how we see this vehicle's icon
void ScenarioManager::SetVehicleIcon(vehVehicle* vehicle)
{
	vehVehicle* player = persGetPersonaVehiclePtr();
	if(!player) return;

	// a few scenario specific checks for enemy players when scenarios are running
	if(vehicle->isSTO() || vehicle->Side == player->Side || !SCEN_MGR->IsActive())
		return;

	vehicle->DrawIconHandle	= FALSE;
	vehicle->DrawIconRank	= FALSE;
	vehicle->DrawIconSquad	= FALSE;
	vehicle->DrawIconId 		= FALSE;

	bool show = false;

	// show enemy icons all the time?
	if(HasFlag(ScenarioManager::ENEMY_ICONS))
		show = true;

	SINT32 lastFired = frametime - vehicle->LastFireTime;

	// show firing enemies?
	if(!show && HasFlag(ScenarioManager::FIRING_ENEMY_ICONS) && lastFired <= 2000)
		show = true;

	if(vehicle->VehicleType->Type == VEH_TYPE_PERSON)
	{
		gr2AnimTrooper* anim = vehicle->GetAnim();

		if(anim)
		{
			if(!show && anim->currentMove == TPS_MOVETYPE_SPRINT && HasFlag(ScenarioManager::FAST_ENEMY_ICONS))
				show = true;

			if(show && anim->currentPose == TPS_POSETYPE_PRONE && HasFlag(ScenarioManager::HIDE_PRONE_ENEMY_ICONS))
				show = false;
		}
	}

	if(show)
	{
		vehicle->DrawIconHandle		= TRUE;
		vehicle->DrawIconId 			= FALSE;
		vehicle->IconTransparency	= 1.0f;
	}
}

bool ScenarioManager::ValidVehicle(const vehType* vtype)
{
	return (VehicleMask(vtype, false) != nullptr);
}

const ScenarioWorldObject* ScenarioManager::VehicleSpawn(const vehType* vtype)
{
	const ScenarioVehicleMask* mask = VehicleMask(vtype);

	if(mask)
	{
		SCENWOB_LIST vspawns;

		for(size_t i = 0; i < spawns.size(); i++)
		{
			ScenarioWorldObject* sp = spawns.at(i);

			// check name and team/round/stage info
			if(sp->name.compare(mask->name) == 0 && RoundRelevant(sp))
				vspawns.push_back(sp);
		}

		// return a random one
		if(!vspawns.empty())
            return vspawns[PNRandom::random(0, vspawns.size() - 1)];
	}

	// didn't find one
	return nullptr;
}

bool ScenarioManager::IsActive() const
{
	return (_scenario != nullptr);
}

bool ScenarioManager::IsScenarioComplete() const
{
	return _scenarioIsComplete;
}

bool ScenarioManager::IsCustomTerrain() const
{
	return (IsActive() && !_scenario->terrain.empty());
}

// returns true is not custom terrain or we have custom terrain and its loaded
bool ScenarioManager::IsTerrainLoaded() const
{
	return (IsCustomTerrain() && _map == nullptr) ? false : true;
}

int ScenarioManager::AddObject(ScenarioObject* ob)
{
	objects.push_back(ob);

	// if we're a named object, also put us in the named list
	// so we can do slightly faster lookups
	if(!ob->name.empty())
		named.push_back(ob);

	return objects.size() - 1; // return our index
}

static content_command& _parse_command(std::string& command)
{
	static content_command cc;
	static char buffer[128+1];
	strsafecpy(buffer, command.c_str());

	char* ptr = strchr(buffer, '.');

	// make all dots a space so we can easily sscanf it
	while(ptr)
	{
		*ptr = ' ';
		ptr = strchr(ptr, '.');
	}

	cc.reset();

	// now we'll look up the data
	sscanf(buffer, "%32s %u %u %u", cc.command, &cc.team, &cc.round, &cc.stage);

	return cc;
}

static double _radian_degrees(double r)
{
	if(r >= 0 || r <= 180)
	{
		// eastern hemisphere
		r = -(r - 180);
	}
	else
	{
		// western hemisphere
		r = (r - 180);
	}

	return mlDEG_TO_RAD(r);
}

static UINT32 _extract_color(const char* color)
{
	UINT32 r, g, b;
	r = g = b = 0;

	if(strcasecmp(color, "allied") == 0)
	{
		return (SCEN_MGR->GetTeam() == 1) ? gIconFriendlyColor : gIconEnemyColor;
	}
	else if(strcasecmp(color, "axis") == 0)
	{
		return (SCEN_MGR->GetTeam() == 2) ? gIconFriendlyColor : gIconEnemyColor;
	}
	else if(strcasecmp(color, "enemy") == 0)
	{
		return gIconEnemyColor;
	}
	else if(strcasecmp(color, "friendly") == 0)
	{
		return gIconFriendlyColor;
	}
	else if(sscanf(color, "#%02x%02x%02x", &r, &g, &b) == 3)
	{
		return UINT32(piRGB_MAKE(r, g, b));
	}

	return colorGREENALPHA;
}

int ScenarioManager::AddContent(const char* line)
{
	// our work buffer
	char name[255+1];
	char data[255+1];

	// copy the data inout buffer so we can manipulate it
	strsafecpy(name, line);

	char* ptr = strchr(name, ' ');
	if(ptr) *ptr = 0;

	std::string command = name;
	std::string arguments = "";

	if(ptr)
		arguments = (ptr + 1);

	name[0] = 0;
	data[0] = 0;

	// determine command data
	// parses name, team, round and stage filters if any;
	// _parse_command returns a reference but we copy it
	// because some content calls may send back thru
	// another AddContent
	content_command cc = _parse_command(command);

	// this is our new object to add;
	// content types below will set this
	// if valid data is found
	ScenarioObject* newOb = nullptr;

    if(strcasecmp(cc.command, "river") == 0)
    {
		// position
		float x = 0;
		float y = 0;
		float z = 0;
        float rot = 0;

		sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &rot);

		// we have a valid entry
		newOb = new ScenarioRiver(name, data, x, y, z, mlDEG_TO_RAD(rot));
    }
    else if(strcasecmp(cc.command, "coast") == 0)
    {
		// position
		float x = 0;
		float y = 0;
		float z = 0;
        float rot = 0;

		sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &rot);

		// we have a valid entry
		newOb = new ScenarioCoast(name, data, x, y, z, mlDEG_TO_RAD(rot));
    }
    else if(strcasecmp(cc.command, "ocean") == 0)
    {
		// position
		float x = 0;
		float y = 0;
		float z = 0;
        float rot = 0;

		sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &rot);

		// we have a valid entry
		newOb = new ScenarioOcean(name, data, x, y, z, mlDEG_TO_RAD(rot));
    }
    else if(strcasecmp(cc.command, "strat") == 0)
    {
		if(sscanf(arguments.c_str(), "%255s %255s", name, data) == 2)
		{
			// we have a valid entry
			newOb = new ScenarioStrat(name, data);

			// we'll set our ownership after the terrain is loaded
			newOb->loadFlags = ScenarioObject::ON_TERRAIN_LOADED;
		}
    }
    else if(strcasecmp(cc.command, "wave") == 0)
    {
		// position
		float x1 = 0;
		float y1 = 0;
		float x2 = 0;
		float y2 = 0;
		float p = 0;
		float r = 0;
		float y = 0;
        FLOAT32 pitchRate = 0;
		FLOAT32 rollRate = 0;
		FLOAT32 bobRate = 0;
		data[0] = 0;

		if(sscanf(arguments.c_str(), "%255s %f %f %f %f %f %f %f %f %f %f", name, &x1, &y1, &x2, &y2, &p, &r, &y, &pitchRate, &rollRate, &bobRate) == 11)
		{
			// we only care about the xy plane when describing the wavefront
			Vec3d v1 = Vec3d(x1, y1, 0);
			Vec3d v2 = Vec3d(x2, y2, 0);

			// calculate the normal parallel to the waves (what we'll rotate around)
			Vec3d axis = v1 - v2;
			axis.normalize();

			ScenarioWaveObject* wo = new ScenarioWaveObject(name, data);

			// re-use position vector to describe the rotation axis to use
			wo->position = axis;

			// set the rotation limits
			wo->attitude.Pitch = mlDEG_TO_RAD(p); // forward/back rotations along the y
			wo->attitude.Roll = mlDEG_TO_RAD(r); // left/right rotations along the x

			// set our bob limit
			wo->attitude.Yaw = y; // yaw is used to describe the bob in meters up/down

			// how long of a wave are dealing with
			wo->pitchRate = pitchRate;
			wo->rollRate = rollRate;
			wo->bobRate = bobRate;

			newOb = wo;

			// this type of object needs updating
			updaters.push_back(newOb);
		}
    }
	else if(strcasecmp(cc.command, "place") == 0)
	{
		// filename,worldx,worldy,zoffset,roll,pitch,yaw,scalex,scaley,scalez,damage

		// position
		float x = 0;
		float y = 0;
		float z = 0;

		// rotation
		float roll	= 0;
		float pitch = 0;
		float yaw	= 0;

		// scale
		float sx	= 0;
		float sy = 0;
		float sz	= 0;

		// flags
		UINT32 flags = 0;

		// grab our data; we need at least the building name and the position info
		// if we don't have a named object, try an anonymous object
		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %f %f %f %f %f %f %u", name, data, &x, &y, &z, &roll, &pitch, &yaw, &sx, &sy, &sz, &flags) < 12)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%255s %f %f %f %f %f %f %f %f %f %u", data, &x, &y, &z, &roll, &pitch, &yaw, &sx, &sy, &sz, &flags);
		}

		// we have a valid entry
		ScenarioWorldObject* ob = new ScenarioWorldObject(ScenarioObject::PLACED, name, data);

		ob->flags			= flags;
		ob->loadFlags		= ScenarioObject::ON_TERRAIN_LOADED;
		ob->attitude.Roll	= mlDEG_TO_RAD(roll);
		ob->attitude.Pitch	= mlDEG_TO_RAD(pitch);
		ob->attitude.Yaw		= mlDEG_TO_RAD(yaw);
		ob->position.set(x, y, z);

		newOb = ob;
	}
	else if(strcasecmp(cc.command, "clutter") == 0)
	{
		// filename,worldx,worldy,zoffset,roll,pitch,yaw,scalex,scaley,scalez,damage

		// position
		float x = 0;
		float y = 0;
		float z = 0;

		// rotation
		float roll	= 0;
		float pitch = 0;
		float yaw	= 0;

		// scale
		float sx	= 0;
		float sy	= 0;
		float sz	= 0;

		// flags
		int flags = 0;

		name[0] = 0;
		sscanf(arguments.c_str(), "%255s %f %f %f %f %f %f %f %f %f %u", data, &x, &y, &z, &roll, &pitch, &yaw, &sx, &sy, &sz, &flags);

		// we have a valid entry
		ScenarioWorldObject* ob = new ScenarioWorldObject(ScenarioObject::CLUTTER, name, data);

		ob->flags			= flags;
		ob->loadFlags		= ScenarioObject::ON_TERRAIN_LOADED;
		ob->attitude.Roll	= mlDEG_TO_RAD(roll);
		ob->attitude.Pitch	= mlDEG_TO_RAD(pitch);
		ob->attitude.Yaw		= mlDEG_TO_RAD(yaw);
		ob->position.set(x, y, z);

		newOb = ob;
	}
	else if(strcasecmp(cc.command, "terrain") == 0)
	{
		// position
		float x = 0;
		float y = 0;
		float z = 0;

		// rotation
		float yaw = 0;

		// grab our data; we need at least the building name and the position info
		// if we don't have a named object, try an anonymous object
		sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &yaw);

		// we have a valid entry
		ScenarioWorldObject* ob = new ScenarioWorldObject(ScenarioObject::TERRAIN, name, data);

		ob->flags			= 0;
		ob->loadFlags		= ScenarioObject::ON_TERRAIN_LOAD;
		ob->attitude.Yaw		= mlDEG_TO_RAD(yaw);
		ob->attitude.Pitch	= 0;
		ob->attitude.Roll	= 0;
		ob->position.set(x, y, z);

		newOb = ob;
	}
	else if(strcasecmp(cc.command, "block") == 0)
	{
		// position
		float x1 = 0;
		float y1 = 0;
		float z1 = 0;
		float x2 = 0;
		float y2 = 0;
		float z2 = 0;

		// rotation
		float width		= .5f;
		float height	= 10.0f;

		// grab our data; we need at least the building name and the position info
		if(sscanf(arguments.c_str(), "%255s %f %f %f %f %f %f %f %f", name, &x1, &y1, &z1, &x2, &y2, &z2, &width, &height) < 9)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%f %f %f %f %f %f %f %f", &x1, &y1, &z1, &x2, &y2, &z2, &width, &height);
		}

		// we have a valid entry
		ScenarioBlock* block = new ScenarioBlock(name);

		block->pt1.set(x1, y1, z1);
		block->pt2.set(x2, y2, z2);
		block->width = width;
		block->height = height;

		newOb = block;
	}
	else if(strcasecmp(cc.command, "volume") == 0)
	{
		int shape	= 0;
		float x		= 0;
		float y		= 0;
		float z		= 0;
		float yaw	= 0;
		float dx	= 0;
		float dy	= 0;
		float dz	= 0;
		data[0]		= 0;

		if(sscanf(arguments.c_str(), "%255s %d %f %f %f %f %f %f %f", name, &shape, &x, &y, &z, &yaw, &dx, &dy, &dz) >= 7)
		{
			// we have a valid entry
			ScenarioVolume* volume = new ScenarioVolume(name, data);

			volume->position.set(x, y, z);
			volume->dimensions.set(dx, dy, dz);

			volume->attitude.Yaw		= _radian_degrees(yaw);
			volume->shape			= shape;

			newOb = volume;
		}
	}
	else if(strcasecmp(cc.command, "emitter") == 0)
	{
		// name,worldx,worldy,worldz
		float x = 0;
		float y = 0;
		float z = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f", name, data, &x, &y, &z) < 5)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%255s %f %f %f", data, &x, &y, &z);
		}

		// create an emitter object and add it
		ScenarioWorldObject* emitter = new ScenarioWorldObject(ScenarioObject::EMITTER, name, data);

		// if we don't have a name, load it on start
		if(emitter->name.empty())
			emitter->loadFlags = ScenarioObject::ON_CONTENT_LOAD;

		emitter->position.set(x, y, -z);

		newOb = emitter;
	}
	else if(strcasecmp(cc.command, "fpsfx") == 0)
	{
		int fxType = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %d", name, data, &fxType) >= 1)
		{
			newOb = new ScenarioFPSFX(name, data);
			newOb->flags = (UINT32)fxType;
		}
	}
	else if(strcasecmp(cc.command, "bombingrun") == 0)
	{
		float x1 = 0;
		float y1 = 0;
		float z1 = 0;
		float x2 = 0;
		float y2 = 0;
		float z2 = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %f %f %f", name, data, &x1, &y1, &z1, &x2, &y2, &z2) == 8)
		{
			ScenarioBombRun* bombRun = new ScenarioBombRun(name, data);

			bombRun->start.set(x1, y1, z1);
			bombRun->end.set(x2, y2, z2);

			newOb = bombRun;
		}
	}
	else if(strcasecmp(cc.command, "barrage") == 0)
	{
		float x = 0;
		float y = 0;
		float z = 0;
		float distance = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &distance) == 6)
		{
			ScenarioBombBarrage* barrage = new ScenarioBombBarrage(name, data);

			barrage->center.set(x, y, z);
			barrage->radius = distance;

			newOb = barrage;
		}
	}
	else if(strcasecmp(cc.command, "event") == 0)
	{
		FLOAT32 chance			= 1.0f; // how likely each event interval to run
		SINT32 eventInterval	= 0; // how long between events
		SINT32 runInterval		= 0; // how long between runs
		SINT32 runs				= 1; // number of runs

		// name,chance,einterval,rinterval,runs
		if(sscanf(arguments.c_str(), "%255s %f %d %d %d", name, &chance, &eventInterval, &runInterval, &runs) >= 1)
		{
			// create an emitter object and add it
			ScenarioEvent* evt = new ScenarioEvent(name);

			// set our timing data
			evt->chance			= chance;
			evt->lastEvent		= 0;
			evt->nextEvent		= 0;
			evt->eventInterval	= eventInterval;
			evt->runInterval	= runInterval;
			evt->runs			= runs;
			evt->run			= -1;

			// save our new area
			events.push_back(evt);

			// return our index
			newOb = evt;
		}
	}
	else if(strcasecmp(cc.command, "sound") == 0)
	{
		// name,worldx,worldy,worldz
		float x			= 0;
		float y			= 0;
		float z			= 0;
		float minDist	= 0;
		float maxDist	= 0;
		float volume	= 0;
		UINT32 loop		= 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %u %f %f %f %f %f", name, data, &volume, &loop, &x, &y, &z, &minDist, &maxDist) < 9)
		{
			name[0] = 0;

			if(sscanf(arguments.c_str(), "%255s %f %u %f %f %f %f %f", data, &volume, &loop, &x, &y, &z, &minDist, &maxDist) < 8)
			{
				x = y = z = minDist = maxDist = volume = 0.0f;
				loop = 0;

				if(sscanf(arguments.c_str(), "%255s %255s %f %u", name, data, &volume, &loop) < 4)
				{
					name[0] = 0;
					sscanf(arguments.c_str(), "%255s %f %u", data, &volume, &loop);
				}
			}
		}

		// create an emitter object and add it
		ScenarioSound* sound = new ScenarioSound(name, data);

		sound->flags		= loop;
		sound->volume	= volume;
		sound->minDist	= minDist;
		sound->maxDist	= maxDist;
		sound->position.set(x, y, z);

		// we need to monitor this anonymous world sound
		if(sound->name.empty() && sound->position.length2())
			AddSound(sound);

		newOb = sound;
	}
	else if(strcasecmp(cc.command, "area") == 0)
	{
		// handler,worldx,worldy,worldz
		float x = 0;
		float y = 0;
		float z = 0;
		int plane = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %d", name, data, &x, &y, &z, &plane) >= 5)
		{
			// grab our data; we need at least the handler and the position info
			for(size_t i = 0; i < areas.size(); i++)
			{
				ScenarioObject* ob = areas.at(i);

				if(ob->name.compare(name) == 0)
				{
					ScenarioArea* area = dynamic_cast<ScenarioArea*>(ob);
					if(!area) continue;

					area->points.push_back(Vec3d(x, y, z));

					// return which index we are
					return i;
				}
			}

			ScenarioArea* area = new ScenarioArea(name, data);

			// add our point
			area->points.push_back(Vec3d(x, y, z));
			area->plane = plane;

			areas.push_back(area);

			// return our index
			newOb = area;
		}
	}
	else if(strcasecmp(cc.command, "location") == 0)
	{
		// handler,worldx,worldy,worldz
		float x = 0;
		float y = 0;
		float z = 0;
		float radius = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &radius) == 6)
		{
			ScenarioWorldObject* location = new ScenarioWorldObject(ScenarioObject::LOCATION, name, data);

			// add our point
			location->loadFlags = ScenarioObject::ON_CONTENT_LOAD;
			location->position.set(x, y, z);
			location->attitude.Yaw = radius;

			// return our index
			newOb = location;
		}
	}
	else if(strcasecmp(cc.command, "boundary") == 0)
	{
		// handler,worldx,worldy,worldz
		float x = 0;
		float y = 0;
		float z = 0;

		if(sscanf(arguments.c_str(), "%255s %f %f %f", name, &x, &y, &z) == 4)
		{
			// grab our data; we need at least the handler and the position info
			for(size_t i = 0; i < boundaries.size(); i++)
			{
				ScenarioObject* ob = boundaries.at(i);

				if(ob->name.compare(name) == 0)
				{
					ScenarioBoundary* boundary = dynamic_cast<ScenarioBoundary*>(ob);
					if(!boundary) continue;

					ScenarioBlock block = ScenarioBlock(boundary->name.c_str());
					block.pt1.set(x, y, z);
					block.height = 100.0f;

					boundary->blocks.push_back(block);

					// return which index we are
					return i;
				}
			}

			ScenarioBoundary* boundary = new ScenarioBoundary(name);

			// add our first block
			ScenarioBlock block = ScenarioBlock(boundary->name.c_str());;
			block.pt1.set(x, y, z);
			block.height = 100.0f;

			// save it
			boundary->blocks.push_back(block);
			boundaries.push_back(boundary);

			// return our index
			newOb = boundary;
		}
	}
	else if(strcasecmp(cc.command, "spawn") == 0)
	{
		// spawngroup,worldx,worldy,worldz, yaw
		float x		= 0;
		float y		= 0;
		float z		= 0;
		float yaw	= 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %f", name, data, &x, &y, &z, &yaw) < 6)
		{
			data[0] = 0;
			sscanf(arguments.c_str(), "%255s %f %f %f %f", name, &x, &y, &z, &yaw);
		}

		ScenarioWorldObject* spawn = new ScenarioWorldObject(ScenarioObject::SPAWN, name, data);

		spawn->position.set(x, y, z);
		spawn->attitude.Yaw = _radian_degrees(yaw);
		spawns.push_back(spawn);

		newOb = spawn;
	}
	else if(strcasecmp(cc.command, "waypoint") == 0)
	{
		// name,locstring,worldx,worldy,worldz
		float x		= 0;
		float y		= 0;
		float z		= 0;

		char color[10+1];
		strsafecpy(color, "#FF0000");

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %10s", name, data, &x, &y, &z, color) >= 5)
		{
			ScenarioWaypoint* wp = new ScenarioWaypoint(name, data);

			wp->position.set(x, y, z);
			wp->color = color;

			newOb = wp;
		}
	}
	else if(strcasecmp(cc.command, "clone") == 0)
	{
		// spawngroup,worldx,worldy,worldz, yaw
		float x		= 0;
		float y		= 0;
		float z		= 0;
		float roll	= 0;
		float pitch	= 0;
		float yaw	= 0;

		if(sscanf(arguments.c_str(), "%255s %255s %f %f %f %f %f %f", name, data, &x, &y, &z, &roll, &pitch, &yaw) < 8)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%255s %f %f %f %f %f %f", data, &x, &y, &z, &roll, &pitch, &yaw);
		}

		ScenarioWorldObject* clone = new ScenarioWorldObject(ScenarioObject::CLONE, name, data);

		clone->position.set(x, y, z);
		clone->attitude.Roll		= mlDEG_TO_RAD(roll);
		clone->attitude.Pitch	= mlDEG_TO_RAD(pitch);
		clone->attitude.Yaw		= mlDEG_TO_RAD(yaw);
		clone->loadFlags			= ScenarioObject::ON_TERRAIN_LOADED;

		newOb = clone;
	}
	else if(strcasecmp(cc.command, "vehicle") == 0 || strcasecmp(cc.command, "mask") == 0)
	{
		// spawngroup,country,category,class,type
		char id[31+1];
		UINT32 country	= 0;
		UINT32 category = 0;
		UINT32 klass	= 0;
		UINT32 type		= 0;

		id[0] = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %u %u %u %u", name, data, &country, &category, &klass, &type) < 6)
		{
			data[0] = 0;

			if(sscanf(arguments.c_str(), "%255s %u %u %u %u", name, &country, &category, &klass, &type) < 5)
			{
				SINT32 limit = -1;

				if(sscanf(arguments.c_str(), "%255s %31s %d", name, id, &limit) >= 2)
				{
					WEAPON_SPEC spec = weapFindWeaponByName(id);

					// if we get to here and the spec was null, bail
					if(spec.isNull())
						return -1;

					if(limit > 0)
					{
						char limitContent[255+1];
						snprintf(limitContent, 255, "limit %s %d", id, limit);
						AddContent(limitContent);
					}

					country		= spec.wCountry;
					category	= spec.wCategory;
					klass		= spec.wClass;
					type		= spec.wType;
				}
			}
		}

		ScenarioVehicleMask* mask = new ScenarioVehicleMask(name, data);

		// reset it
		mask->spec.wCountry		= (UINT8)country;
		mask->spec.wCategory		= (UINT8)category;
		mask->spec.wClass		= (UINT8)klass;
		mask->spec.wType			= (UINT8)type;

		// save our new spawn location
		vehicles.push_back(mask);

		// return our index
		newOb = mask;
	}
	else if(strcasecmp(cc.command, "weaponpack") == 0)
	{
		// spawngroup,country,category,class,type
		UINT32 country	= 0;
		UINT32 category = 0;
		UINT32 klass	= 0;
		UINT32 type		= 0;

		data[0] = 0;

		if(sscanf(arguments.c_str(), "%255s %u %u %u %u", name, &country, &category, &klass, &type) < 5)
		{
			char id[31+1];
			id[0] = 0;

			if(sscanf(arguments.c_str(), "%255s %31s", name, id) == 2)
			{
				WEAPON_SPEC spec = weapFindWeaponByName(id);

				// if we get to here and the spec was null, bail
				if(spec.isNull())
					return -1;

				country		= spec.wCountry;
				category	= spec.wCategory;
				klass		= spec.wClass;
				type		= spec.wType;
			}
		}

		ScenarioVehicleMask* weaponPack = new ScenarioVehicleMask(name, data);

		weaponPack->type					= ScenarioObject::WEAPONPACK;
		weaponPack->spec.wCountry		= (UINT8)country;
		weaponPack->spec.wCategory		= (UINT8)category;
		weaponPack->spec.wClass			= (UINT8)klass;
		weaponPack->spec.wType			= (UINT8)type;

		// return our index
		newOb = weaponPack;
	}
	else if(strcasecmp(cc.command, "ammo") == 0)
	{
		SINT32 duration	= 0;

		// command,type
		if(sscanf(arguments.c_str(), "%255s %255s %d", name, data, &duration) >= 2)
		{
			newOb = new ScenarioObject(ScenarioObject::AMMO, name, data);
			newOb->duration = duration;
		}
	}
	else if(strcasecmp(cc.command, "firepoint") == 0)
	{
		int turret	= 0;
		int mount	= 0;
		int flags	= 0;
		float pitch	= 0;
		float yaw	= 0;
		float hVar	= 0; // horizontal variance
		float vVar	= 0; // vertical variance

		// command,type
		if(sscanf(arguments.c_str(), "%255s %255s %d %d %f %f %f %f %d", name, data, &turret, &mount, &pitch, &yaw, &vVar, &hVar, &flags) >= 4)
		{
			ScenarioFirepoint* fp = new ScenarioFirepoint(name, data);

			fp->turret			= turret;
			fp->mount			= mount;
			fp->flags			= (UINT32)flags; // snap or smoothly move to next turret orientation
			fp->hVariance		= hVar;
			fp->vVariance		= vVar;
			fp->attitude.Pitch	= pitch;
			fp->attitude.Yaw		= yaw;

			newOb = fp;
		}
	}
	else if(strcasecmp(cc.command, "handler") == 0)
	{
		// command,type
		if(sscanf(arguments.c_str(), "%255s %255s", name, data) == 2)
			newOb = new ScenarioObject(ScenarioObject::HANDLER, name, data, ScenarioObject::ON_CONTENT_LOAD);
	}
	else if(strcasecmp(cc.command, "limit") == 0)
	{
		SINT32 limit = 0;

		// command,type
		if(sscanf(arguments.c_str(), "%255s %d", name, &limit) == 2)
		{
			newOb = new ScenarioObject(ScenarioObject::LIMIT, name, "");
			newOb->flags = limit;
		}
	}
	else if(strcasecmp(cc.command, "chat") == 0)
	{
		SINT32 duration	= -1;

		// command,type
		if(sscanf(arguments.c_str(), "%255s %255s %d", name, data, &duration) >= 2)
		{
			newOb = new ScenarioObject(ScenarioObject::CHAT, name, data);
			newOb->duration = duration;
		}
	}
	else if(strcasecmp(cc.command, "alert") == 0)
	{
		SINT32 type = GAMEEVENT_ALERT;
		SINT32 duration	= -1;

		// command,type
		if(sscanf(arguments.c_str(), "%255s %255s %d %d", name, data, &duration, &type) >= 2)
		{
			newOb = new ScenarioObject(ScenarioObject::ALERT, name, data);
			newOb->flags = (type > GAMEEVENT_INVALID && type < GAMEEVENT_NUM_TYPES) ? type : GAMEEVENT_ALERT;
			newOb->duration = duration;
		}
	}
	else if(strcasecmp(cc.command, "message") == 0)
	{
		SINT32 duration	= -1;

		char color[32+1];
		color[0] = 0;

		// command,type
		if(sscanf(arguments.c_str(), "%255s %255s %32s %d", name, data, color, &duration) >= 3)
		{
			newOb = new ScenarioObject(ScenarioObject::MESSAGE, name, data);
			newOb->duration = duration;
			newOb->flags = _extract_color(color);
		}
	}
	else if(strcasecmp(cc.command, "call") == 0)
	{
		// command,name,code to call
		char buffer[512+1];

		// create a work buffer
		strncpy(buffer, arguments.c_str(), 512);

		// find the first space
		char* start = buffer;
		char* end = strchr(start, ' ');

		// name
		if(end)
		{
			*end = 0;

			// copy to ptr
			strncpy(name, start, 255);
		}

		start = ++end;

		// data
		if(start)
		{
			end = strchr(start, ' ');

			if(end)
			{
				*end = 0;
			}

			// copy from data ptr start to data ptr end
			strncpy(data, start, 255);
		}

		start = ++end;

		// code
		if(start)
		{
			ScenarioCall* ob = new ScenarioCall(name, data);
			ob->code = start;
			newOb = ob;
		}
	}
	else if(strcasecmp(cc.command, "command") == 0)
	{
		// command,type
		int type = 0;

		if(sscanf(arguments.c_str(), "%255s %255s %d", name, data, &type) < 3)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%255s %d", data, &type);
		}

        newOb = new ScenarioObject(ScenarioObject::COMMAND, name, data);
		newOb->flags = type;
	}
	else if(strcasecmp(cc.command, "include") == 0)
	{
		if(sscanf(arguments.c_str(), "%255s %255s", name, data) < 2)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%255s", data);
		}

		newOb = new ScenarioObject(ScenarioObject::INCLUDE, name, data, ScenarioObject::ON_CONTENT_LOAD);
	}
	else if(strcasecmp(cc.command, "clientstate") == 0)
	{
		if(sscanf(arguments.c_str(), "%255s %255s", name, data) < 2)
		{
			name[0] = 0;
			sscanf(arguments.c_str(), "%255s", data);
		}

		newOb = new ScenarioObject(ScenarioObject::INCLUDE, name, data, ScenarioObject::ON_START);
	}
	else if(strcasecmp(cc.command, "permission") == 0)
	{
		// command,permission name,data
		if(sscanf(arguments.c_str(), "%255s %255s", name, data) >= 2)
			newOb = new ScenarioPermission(name, data);
	}

	if(newOb)
	{
		newOb->team		= cc.team;
		newOb->round		= cc.round;
		newOb->stage		= cc.stage;

		// save it
		return AddObject(newOb);
	}

	return -1;
}

const wchar_t* ScenarioManager::GetLoc(const char* key)
{
	// try our local scenario localization table
	if(IsActive() && key && *key)
	{
		const wchar_t* loc = nullptr;

		char buffer[80+1];
		int team = GetTeam();

		// try full name, team, round, stage syntax
		snprintf(buffer, 80, "%s.%d.%d.%d", key, team, (int)_round, (int)_stage);
		loc = _scenario->GetLoc(buffer);
		if(loc && *loc) return loc;

		// try name, team syntax
		snprintf(buffer, 80, "%s.%d", key, team);
		loc = _scenario->GetLoc(buffer);
		if(loc && *loc) return loc;

		// otherwise just try the name
		loc = _scenario->GetLoc(key);
		if(loc && *loc) return loc;

		// see if this key is in our attribute map
		Scenario::iterator found = _scenario->find(key);

		if(found != _scenario->end())
		{
			loc = UTF16(found->second.c_str());
			return loc;
	}
	}

	// otherwise fall back to the main ui loc string lookup
	return nullptr;
}

const char* ScenarioManager::MapName()
{
	return (IsActive()) ? _scenario->terrain.c_str() : "na";
}

const char* ScenarioManager::MapModelName()
{
	return (IsActive()) ? _scenario->map.c_str() : "na";
}

const char* ScenarioManager::MapPath(const char* file, const char* ender)
{
	static std::string mapPath = "";

	mapPath = MAPS_PATH;
	mapPath += MapName();
	mapPath += "/";
	mapPath += file;

	if(ender)
		mapPath += ender;

    int fd = fioOpen(mapPath.c_str());

    if (fd == -1) {
		std::string tempPath;

		for(size_t i = 0; i < mapPaths.size(); i++)
		{
			std::string& parent = mapPaths.at(i);

			tempPath = MAPS_PATH;
			tempPath += parent;
			tempPath += "/";
			tempPath += file;

			if(ender)
				tempPath += ender;

			if ((fd = fioOpen(tempPath.c_str())) != -1) {
			    fioClose(fd);
				mapPath = tempPath;
				break;
			}
		}
	} else {
	    fioClose(fd);
	}

	return mapPath.c_str();
}

bool ScenarioManager::InGameMapPos(FLOAT32& x, FLOAT32& y, FLOAT32& scale)
{
	jsAttributeMap::iterator found = _scenario->find("ingamemap");

	if(found != _scenario->end())
	{
		std::string& data = found->second;
		return (sscanf(data.c_str(), "%f %f %f", &x, &y, &scale) > 0);
	}

	return false;
}

static void _killsound(void* data)
{
    // look up sound scenario object and kill it
	ScenarioSound* sound = static_cast<ScenarioSound*>(data);

	if(sound)
	{
		sound->id = -1;

		// if we're an anonymous positional sound
		// pop us back on the map of sounds to monitor
		if(sound->name.empty() && sound->position.length2())
			SCEN_MGR->AddSound(sound);
	}
}

bool ScenarioManager::CreateAmmo(const char* name, Vec3d& position, Vec3d& dir)
{
	if(!uiInGame())
		return false;

	FLOAT64VECTOR zero = {0,0,0};
	FLOAT64VECTOR Pos = {0,0,0};
	weapAmmoType* AmmoType = nullptr;
	piMATRIX FireAtt = IdentityMatrix;

	weapFindAmmoTypeByName(&AmmoType, (UCHAR*)name);
	if(!AmmoType) return false;

	position.get(Pos);

	UINT32 Flags = (WEAP_AMMOFLAG_IS_GHOSTROUND|WEAP_AMMOFLAG_USE_MEDCOLLIDELIST|
					WEAP_AMMOFLAG_DONTCHECKVEHICLES|WEAP_AMMOFLAG_DONT_SEND_DAMAGE);

	if(dir.length2())
	{
		Vec3d forward = Vec3d(0, 1.0f, 0);

		Matrixd attitude;
		attitude.makeRotate(forward, dir);

		// rotate our firepoint to dir
		attitude.get(FireAtt);
	}

	// fire our round
	weapFireRound(&zero, AmmoType, &Pos, &zero, &FireAtt, frametime/ 1000,  Flags, nullptr, &Pos, 0, nullptr);

	return true;
}

bool ScenarioManager::CreateSound(ScenarioObject* ob)
{
	ScenarioSound* sound = dynamic_cast<ScenarioSound*>(ob);
	if(!sound) return false;

	if(sound->id >= 0)
		return true;

	//if(!trigger && !sound->name.empty())
	//	return false;

	sound->id = fxSndGetNewSound();

	if(sound->id == -1)
		return false;

	bool worldSound = (sound->position.length2() > 0);

	fxSndSetSoundName(sound->id, (char*)sound->data.c_str());
	fxSndSetPriority(sound->id, IDE_TP_PRIORITY_LOW);

	if(sound->flags)
		fxSndSetSoundType(sound->id, TYPE_INFINITE_EXTERNAL_SOUND);
	else
		fxSndSetSoundType(sound->id, TYPE_FINITE_EXTERNAL_SOUND);

	fxSndSetPitch(sound->id, 1.0f);
	//fxSndPushFlags(sound.resourceID, flags);

    // Set min and max distances
	if(worldSound)
	{
		// set our sound distance stuff
		fxSndSetVolume(sound->id, sound->volume);
		fxSndSetMinDistance(sound->id, sound->minDist);
		fxSndSetMaxDistance(sound->id, sound->maxDist);

		// set sound position
		fxSndSetPosition(sound->id, sound->position.x(), sound->position.y(), sound->position.z());

		// it has a position, play it in the world
		fxSndSetPlayType(sound->id, DEF_PLAYTYPE_WORLD);
	}
	else
	{
		// set volume
		fxSndSetVolume(sound->id, sound->volume);

		// play ambient sounds in your head
		fxSndSetPlayType(sound->id, DEF_PLAYTYPE_HEAD);
	}

	fxSndSetKillData(sound->id, sound);
	fxSndSetKillCallback(sound->id, _killsound);

	//if(sound.flags)
	//fxSndSetSoundLoop(sound.resourceID, FALSE);

	//fxSndSetEmitterObject(iID, soundObject);

    // A failure does not necessarily imply an error. For example, if a sound
    // is beyond its max distance, this function will fail.
    if(!fxSndInitSound(sound->id))
	{
		sound->id = -1;
        return false;
	}

    return true;
}

bool ScenarioManager::CreateEmitter(ScenarioObject* ob)
{
	ScenarioWorldObject* emitter = dynamic_cast<ScenarioWorldObject*>(ob);
	if(!emitter) return false;

	if(emitter->resource)
		return true;

	//if(!trigger && !emitter->name.empty())
	//	return false;

	const Vec3f n = Vec3f(0, 0, -1);
	TransformEffect* te = TransformEffect::AddEffect(emitter->data.c_str(), n, -1, nullptr);

	if(te)
	{
		te->enableemitters(true);;
		te->setmatrix(Matrixd::translate(emitter->position));
		te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);

		// save our resource so we can kill it later
		emitter->resource = te;

		return true;
	}

	return false;
}

bool ScenarioManager::CreateVehicleSTO(ScenarioObject* ob)
{
	ScenarioWorldObject* vehObj = dynamic_cast<ScenarioWorldObject*>(ob);
	if(!vehObj) return false;

	if(vehObj->resource)
		return true;

	//if(!trigger && !vehObj->name.empty())
	//	return false;

	const vehType* type = vehTypeGetById(vehObj->data.c_str());
	if(!type) return false;

	// adjust position by the world origin offset

    WorldPosition_m position(vehObj->position.x(), vehObj->position.y(), vehObj->position.z());
    AttitudeVector attitude((float)vehObj->attitude.Roll, (float)vehObj->attitude.Pitch, (float)vehObj->attitude.Yaw);

    VehicleSTO* sto = new VehicleSTO(0, type, position, attitude);

	if(sto)
	{
		// no icon
		sto->setIcon("");

		// save a reference to our vehicle
		vehObj->resource = sto;

		// track it
		ServerTrackedObject::addToIndex(sto);

		return true;
	}

	return false;
}

bool ScenarioManager::CreateCoastSTO(ScenarioObject* ob)
{
    ScenarioCoast* water = dynamic_cast<ScenarioCoast*>(ob);

    if(water->resource)
        return true;

    // make sure the model gets loaded first; this might not be needed
	PoolModel* model = obmgrGetPoolModel(water->name.c_str());

    if(model)
    {
		Vec3d pos = water->position;

		if(IsActive())
			 pos += _scenario->offset;

		// sto position and atttitude
		WorldPosition_m position(pos.x(), pos.y(), pos.z());
        AttitudeVector attitude(0, 0, water->rotation);

		// create a new sto and save it
		TerrainSTO* sto = new TerrainSTO(water->name.c_str(), position, attitude);

		if(sto)
		{
			ServerTrackedObject::addToIndex(sto);
			water->resource = sto;

            struct AddCallback
            {
                OBJECT* o;
                ScenarioCoast* h2o;

                void add(OBJECT* o)
                {
                    if(o)
                    {
                        if(!(o->flags & OBJECT_NOTOBJECT))
                        {
                            glslScenarioCoastSetup(o, h2o->data.c_str());
                        }

                        add(o->child);
                        add(o->attached);
                        add(o->sibling);
                    }
                }

                void Apply()
                {
                    add(o);
                }
            };

            OBJECT* o = sto->GetObject();

            struct AddCallback acb;
            acb.o = o;
            acb.h2o = water;

            acb.Apply();

			return true;
		}
	}

    return true;
}

bool ScenarioManager::CreateRiverSTO(ScenarioObject* ob)
{
    ScenarioRiver* water = dynamic_cast<ScenarioRiver*>(ob);

    if(water->resource)
        return true;

    // make sure the model gets loaded first; this might not be needed
	PoolModel* model = obmgrGetPoolModel(water->name.c_str());

    if(model)
    {
		Vec3d pos = water->position;

		if(IsActive())
			 pos += _scenario->offset;

		// sto position and atttitude
		WorldPosition_m position(pos.x(), pos.y(), pos.z());
        AttitudeVector attitude(0, 0, water->rotation);

		// create a new sto and save it
		TerrainSTO* sto = new TerrainSTO(water->name.c_str(), position, attitude);

		if(sto)
		{
			ServerTrackedObject::addToIndex(sto);
			water->resource = sto;

            struct AddCallback
            {
                OBJECT* o;
                ScenarioRiver* h2o;

                void add(OBJECT* o)
                {
                    if(o)
                    {
                        if(!(o->flags & OBJECT_NOTOBJECT))
                        {
                            glslScenarioRiverSetup(o, h2o->data.c_str());
                        }

                        add(o->child);
                        add(o->attached);
                        add(o->sibling);
                    }
                }

                void Apply()
                {
                    add(o);
                }
            };

            OBJECT* o = sto->GetObject();

            struct AddCallback acb;
            acb.o = o;
            acb.h2o = water;

            acb.Apply();

			return true;
		}
	}

    return true;
}

bool ScenarioManager::CreateOceanSTO(ScenarioObject* ob)
{
    ScenarioOcean* water = dynamic_cast<ScenarioOcean*>(ob);

    if(water->resource)
        return true;

    // make sure the model gets loaded first; this might not be needed
	PoolModel* model = obmgrGetPoolModel(water->name.c_str());

    if(model)
    {
		Vec3d pos = water->position;

		if(IsActive())
			 pos += _scenario->offset;

		// sto position and atttitude
		WorldPosition_m position(pos.x(), pos.y(), pos.z());
        AttitudeVector attitude(0, 0, water->rotation);

		// create a new sto and save it
		TerrainSTO* sto = new TerrainSTO(water->name.c_str(), position, attitude);

		if(sto)
		{
			ServerTrackedObject::addToIndex(sto);
			water->resource = sto;

            struct AddCallback
            {
                OBJECT* o;
                ScenarioOcean* h2o;

                void add(OBJECT* o)
                {
                    if(o)
                    {
                        if(!(o->flags & OBJECT_NOTOBJECT))
                        {
                            glslScenarioOceanSetup(o, h2o->data.c_str());
                        }

                        add(o->child);
                        add(o->attached);
                        add(o->sibling);
                    }
                }

                void Apply()
                {
                    add(o);
                }
            };

            OBJECT* o = sto->GetObject();

            struct AddCallback acb;
            acb.o = o;
            acb.h2o = water;

            acb.Apply();

			return true;
		}
	}

    return true;
}

bool ScenarioManager::CreateTerrainSTO(ScenarioObject* ob)
{
	ScenarioWorldObject* worldObj = dynamic_cast<ScenarioWorldObject*>(ob);

    if(!worldObj)
        return false;

	if(worldObj->resource)
		return true;

	// make sure the model gets loaded first; this might not be needed
	PoolModel* model = obmgrGetPoolModel(worldObj->name.c_str());

	if(model)
	{
		// our position + offset
		Vec3d pos = worldObj->position;

		if(IsActive())
			 pos += _scenario->offset;

		// sto position and atttitude
		WorldPosition_m position(pos.x(), pos.y(), pos.z());
        AttitudeVector attitude((float)worldObj->attitude.Roll,
                                (float)worldObj->attitude.Pitch,
                                (float)worldObj->attitude.Yaw);

		// create a new sto and save it
		TerrainSTO* sto = new TerrainSTO(worldObj->name.c_str(), position, attitude);

		if(sto)
		{
			// save it
			ServerTrackedObject::addToIndex(sto);

			if(worldObj->flags)
				sto->ToggleDamage();

			worldObj->resource = sto;

            TerrainScenario::GetInstance().LoadTerrain(worldObj->name, worldObj->data);

			return true;
		}
	}

	return false;
}

bool ScenarioManager::CreatePlacedSTO(ScenarioObject* ob)
{
	ScenarioWorldObject* worldObj = dynamic_cast<ScenarioWorldObject*>(ob);
	if(!worldObj) return false;

	if(worldObj->resource)
		return true;

	//if(!trigger && !worldObj->name.empty())
	//	return false;

	// make sure the model gets loaded first; this might not be needed
	PoolModel* model = obmgrGetPoolModel(worldObj->data.c_str());

	if(model)
	{
		// our position + offset
		Vec3d pos = worldObj->position;

		if(IsActive())
			 pos += _scenario->offset;

		// sto position and atttitude
		WorldPosition_m position(pos.x(), pos.y(), pos.z());
		AttitudeVector attitude((float)worldObj->attitude.Roll, (float)worldObj->attitude.Pitch, (float)worldObj->attitude.Yaw);

		// create a new sto and save it
		TerrainSTO* sto = new TerrainSTO(worldObj->data.c_str(), position, attitude);

		if(sto)
		{
			// set our scale
			//if(ob.v3.length2())
			//	sto->SetScale(ob.v3.x(), ob.v3.y(), ob.v3.z());

			// save it
			ServerTrackedObject::addToIndex(sto);

			if(worldObj->flags)
				sto->ToggleDamage();

			worldObj->resource = sto;

			return true;
		}
	}

	return false;
}

void ScenarioManager::SetTerrainReadyToLoad(bool load)
{
	//uiConsoleprintf("SCENARIO: Setting scenarion terrain ready to load!");
    _terrainReadyToLoad = load;
    _startTimeTerrainLoad = frametime;// + 500;
}

void ScenarioManager::DetermineMapModel()
{
	if(!IsActive())
		return;

	if(obmgrGetPoolModel(_scenario->terrain.c_str()))
	{
		_scenario->map = _scenario->terrain;
		return;
	}

	for(size_t i = 0; i < mapPaths.size(); i++)
	{
		std::string& map = mapPaths.at(i);

		if(obmgrGetPoolModel(map.c_str()))
		{
			_scenario->map = map;
			return;
		}
	}
}

void ScenarioManager::SetupTerrainPaths(const char* check)
{
	std::string mapPath = "";

	mapPath = MAPS_PATH;
	mapPath += check;
	mapPath += "/parent.txt";

    // lookup the parent
    int fd = fioOpen(mapPath.c_str());

    if(fd >= 0)
    {
        char parent[31+1];
        parent[0] = 0;

        if(fioGetString(fd, parent, 31))
        {
            mapPaths.push_back(parent);

            // look up its parent
            SetupTerrainPaths(parent);
        }

        fioClose(fd);
    }
}

const char* ScenarioManager::CustomTerrainMap()
{
	//if(IsActive())
	//	return Config("uimap");

	return MapPath("map.bmp");
}

void ScenarioManager::LoadTerrain()
{
	// setup any terrain objects
	LoadObjects(ScenarioObject::ON_TERRAIN_LOAD);

	// force this to build our colliders

	//uiConsoleprintf("SCENARIO: update terrain collide lists!");
	mlCheckForCollisionListUpdate(true);

	// load base map if we have one
	if(IsActive() && !_map && !_scenario->terrain.empty())
	{
		// set spawn override
		//OverrideSpawner = SCENARIO_OVERRIDE;

		// make sure our nospawn terrain is loaded
		//trnmainDoRawUpdate(FALSE);

		// we have a base terrain, load it at 0,0,0
        SCENOBJ_LIST list;
        FindObjects(list, _scenario->map.c_str(), ScenarioObject::TERRAIN);

        if(!list.empty())
        {
            _map = static_cast<TerrainSTO*>(list[0]->resource);
        }
        else
        {
		    ScenarioWorldObject map;

		    map.type		= ScenarioObject::TERRAIN;
		    map.data		= "maintile";
		    map.position	= _scenario->offset;
		    map.name        = _scenario->map;

		    if(CreateTerrainSTO(&map) && map.resource)
		    {
			    _map = static_cast<TerrainSTO*>(map.resource);
			    //uiConsoleprintf("SCENARIO: terrain is loaded!");
	            mlCheckForCollisionListUpdate(true);
                TerrainScenario::GetInstance().LoadTerrain(map.name, map.data);
		    }
        }

        trnClutterKill();
        sptmainInit();
        speedgrassInit();
        glslInitWaterTexture();
        trnskyInitSkyBox();
        gr2AnimExpireCorpses();

        trnmainSetDraw(false);

        mlCheckForCollisionListUpdate(true);

        unsigned int flags = 0;

        if(_globalflag & GLOBAL_GENERATE_MAP)
            flags = TS_NO_GRASS;

        TerrainScenario::GetInstance().LoadEnvironment(flags);
	}

	if(persIsPersonaVehicleType(VEH_TYPE_PERSON))
		fpFPSAreaChecks(nullptr, 0.0f, true);

	// setup any terrain objects that require the terrain to be created first
	LoadObjects(ScenarioObject::ON_TERRAIN_LOADED);
}

bool ScenarioManager::SaveContent(const char* path)
{
	/*
	int fd = fioOpenWrite(path);
	if(fd < 0) return false;

	char buffer[255+1];

	for(int i = 0; i < objects.size(); i++)
	{
		ScenarioObject& ob = objects.at(i);
		buffer[0] = 0;

		switch(ob.type)
		{
			case ScenarioObject::PLACED:
			{
				sprintf(buffer, "place %s %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %d\n",
					ob.name.c_str(),
					ob.v1.x(),
					ob.v1.y(),
					ob.v1.z(),
					ob.v2.x(),
					ob.v2.y(),
					ob.v2.z(),
					ob.v3.x(),
					ob.v3.y(),
					ob.v3.z(),
					(ob.flags1) ? 1 : 0);
			}
			break;
			case ScenarioObject::SPAWN:
			{
				sprintf(buffer, "spawn %s %.2f %.2f %.2f %.2f\n",
					ob.name.c_str(),
					ob.v1.x(),
					ob.v1.y(),
					ob.v1.z(),
					ob.v2.z());
			}
			break;
			case ScenarioObject::COMMAND:
			{
				//sprintf(buffer, "", );
			}
			break;
			case ScenarioObject::INCLUDE:
			{
				sprintf(buffer, "include %s\n", ob.name.c_str());
			}
			break;
			case ScenarioObject::EMITTER:
			{
				sprintf(buffer, "emitter %s %.2f %.2f %.2f\n",
					ob.name.c_str(),
					ob.v1.x(),
					ob.v1.y(),
					ob.v1.z());
			}
			break;
			case ScenarioObject::SOUND:
			{
				sprintf(buffer, "sound %s %.2f %.2f %.2f %.2f %.2f %.2f %d\n",
					ob.name.c_str(),
					ob.v1.x(),
					ob.v1.y(),
					ob.v1.z(),
					ob.v2.x(),
					ob.v2.y(),
					ob.v2.z(),
					(ob.flags1) ? 1 : 0);
			}
			break;
		}

		if(strlen(buffer))
			fioWrite(fd, buffer, strlen(buffer));
	}

	for(int a = 0; a < areas.size(); a++)
	{
		ScenarioArea& area = areas.at(a);
		buffer[0] = 0;

		for(int p = 0; p < area.points.size(); p++)
		{
			Vec3d& point = area.points.at(p);

			sprintf(buffer, "area %s %.2f %.2f %.2f\n",
				area.data.c_str(),
				point.x(),
				point.y(),
				point.z());

			if(strlen(buffer))
				fioWrite(fd, buffer, strlen(buffer));
		}
	}

	for(int v = 0; v < vehicles.size(); v++)
	{
		ScenarioVehicleMask& mask = vehicles.at(v);
		buffer[0] = 0;

		sprintf(buffer, "vehicle %s %u %u %u %u\n",
			mask.data.c_str(),
			(UINT32)mask.spec.wCountry,
			(UINT32)mask.spec.wCategory,
			(UINT32)mask.spec.wClass,
			(UINT32)mask.spec.wType);

		if(strlen(buffer))
			fioWrite(fd, buffer, strlen(buffer));
	}

	fioClose(fd);
	*/

	return true;
}

void ScenarioManager::RunObject(const char* name, int load)
{
	RunObject(name, load, nullptr);
}

void ScenarioManager::RunObject(const char* name, int load, const char* data)
{
	SCENOBJ_LIST list;

	if(FindObjects(list, name) && !list.empty())
	{
		for(size_t i = 0; i < list.size(); i++)
		{
			if(load)
				LoadObject(list.at(i), ScenarioObject::ON_RUN_OBJECT, data);
			else
				UnloadObject(list.at(i));
		}
	}
}

void ScenarioManager::RunObject(ScenarioObject::TYPE type, int load)
{
	RunObject(type, load, nullptr);
}

void ScenarioManager::RunObject(ScenarioObject::TYPE type, int load, const char* data)
{
	SCENOBJ_LIST list;

	if(FindObjects(list, type) && !list.empty())
	{
		for(size_t i = 0; i < list.size(); i++)
		{
			if(load)
				LoadObject(list.at(i), ScenarioObject::ON_RUN_OBJECT, data);
			else
				UnloadObject(list.at(i));
		}
	}
}

// data is content type specific data and may be null
void ScenarioManager::LoadObject(ScenarioObject* ob, UINT32 loadFlags, const char* data)
{
	if(ob->team && ob->team != GetTeam())
		return;
	else if(ob->round && _round != ob->round)
		return;
	else if(ob->stage && _stage != ob->stage)
		return;

	// is this the object's load stage?
	if(!ob->DoLoadStage(loadFlags))
		return;

	// honor objects that define a duration between runs
	if(loadFlags & ScenarioObject::ON_RUN_OBJECT)
	{
		if(ob->runtime && frametime < ob->runtime)
			return;

		// if we have a delay between runs, set it here
		if(ob->duration)
			ob->runtime = (ob->duration < 0) ? (1000 * 60 * 60 * 24): frametime + (ob->duration * 1000);
	}

	switch(ob->type)
	{
		case ScenarioObject::CLONE:
		{
			CreateVehicleSTO(ob);
		}
		break;
		case ScenarioObject::PLACED:
		case ScenarioObject::CLUTTER:
		{
			CreatePlacedSTO(ob);
		}
		break;
		case ScenarioObject::EMITTER:
		{
			CreateEmitter(ob);
		}
		break;
		case ScenarioObject::SOUND:
		{
			CreateSound(ob);
		}
		break;
		case ScenarioObject::CALL:
		{
			if(!data)
				return;

			ScenarioCall* c = dynamic_cast<ScenarioCall*>(ob);
			if(!c) return;

			// run this code; could be dangerous?

			if(strcasecmp(data, c->data.c_str()) == 0)
				TUT_MGR->RunCode(c->code.c_str());
		}
		break;
		case ScenarioObject::HANDLER:
		{
			// run this code; could be dangerous?
			char code[128+1];
			snprintf(code, 128, "RegisterState(\"%s\", \"%s\");", ob->name.c_str(), ob->data.c_str());

			TUT_MGR->RunCode(code);
		}
		break;
		case ScenarioObject::CHAT:
		{
			uiChatPrintLine(uiLocString(ob->data.c_str()).c_str());
		}
		break;
		case ScenarioObject::MESSAGE:
		{
			if(!uiInGame())
				return;

			phui::View* hud = uiFindView("hud");
			if(!hud) return;

			UINT32 r = piRGBA_GETRED(ob->flags);
			UINT32 g = piRGBA_GETGREEN(ob->flags);
			UINT32 b = piRGBA_GETBLUE(ob->flags);

			char script[255+1];
			snprintf(script, 255, "OnMessage(\"%s\", \"#%02x%02x%02x\")", ob->data.c_str(), r, g, b);

			// run our message script
			uiRunScript(hud, script);
		}
		break;
		case ScenarioObject::ALERT:
		{
			uiGameEvent(ob->flags, 1, uiLocString(ob->data.c_str()).c_str());
		}
		break;
		case ScenarioObject::FIREPOINT:
		{
			ScenarioFirepoint* fp = dynamic_cast<ScenarioFirepoint*>(ob);
			if(fp) fp->Update(true);
		}
		break;
		case ScenarioObject::AMMO:
		{
			Vec3d position;
			Vec3d dir = Vec3d(0, 0, -1.0f);

			if(data)
			{
                /*
                 TBD: Clearly someone meant to process more than just 3 pieces of data here even
                      if it intended to be optional - but as is, its not processing more than three.

                 OLD Code:

				FLOAT32 x, y, z, dx, dy, dz;
				x = y = z = dx = dy = dz = 0.0f;

				int parsed = sscanf(data, "%f %f %f", &x, &y, &z, &dx, &dy, &dz);

				if(parsed >= 3)
					position.set((double)x, (double)y, (double)z);
				if(parsed > 3)
					dir.set((double)dx, (double)dy, (double)dz);
                 */

                FLOAT32 x, y, z;
                x = y = z = 0.0f;

                int parsed = sscanf(data, "%f %f %f", &x, &y, &z);

                if(parsed == 3)
                    position.set((double)x, (double)y, (double)z);
			}

			if(!position.length2())
			{
				vehVehicle* player = persGetPersonaVehiclePtr();
				if(!player) return;

				// for now just use the player's position
				position.set(player->ModelData->VehiclePos);
			}

			position.z() += .1f;

			// fire off the ammo
			CreateAmmo(ob->data.c_str(), position, dir);
		}
		break;
		case ScenarioObject::WAYPOINT:
		{
			ScenarioWaypoint* sw = dynamic_cast<ScenarioWaypoint*>(ob);
			if(!sw) return;

			FLOAT64VECTOR pos;
			sw->position.get(pos);

			// create a new waypoint
			PersHUDManager::HUDWaypoint* hw = new PersHUDManager::HUDWaypoint(pos, "scenario", 0.0f);
			if(!hw) return;

			// scenario waypoints dont use the tail
			hw->useTail = false;

			// do one update
			hw->Update();

			UINT32 color = _extract_color(sw->color.c_str());

			UINT32 r = piRGBA_GETRED(color);
			UINT32 g = piRGBA_GETGREEN(color);
			UINT32 b = piRGBA_GETBLUE(color);

			char script[128+1];
			snprintf(script, 128, "setDisplay('%s','#%02x%02x%02x')", sw->data.c_str(), r, g, b);
			uiRunScript(hw->ui, script);

			// waypoints are fire and forget so we don't need to save a ptr to the resource
			sw->resource = hw;

			// add it to the hud manager
			HUD_MGR->AddObject(hw);
		}
		break;
		case ScenarioObject::INCLUDE:
		{
			TUT_MGR->RunScript(ob->data.c_str());
		}
		break;
		case ScenarioObject::AREA:
		{
			if(ob->resource)
				return;

			ScenarioArea* area = dynamic_cast<ScenarioArea*>(ob);
			if(!area) return;

			// set up area tutorial handlers

			// only 2 points is considered a bbox
			// min corner is the first vector and the
			// max corner is the second vector
			if(area->points.size() == 2)
			{
				Vec3d& p1 = area->points.at(0);
				Vec3d& p2 = area->points.at(1);

				Vec3d minpt;
				Vec3d maxpt;

				minpt.x() = (p1.x() <= p2.x() ) ? p1.x(): p2.x();
				minpt.y() = (p1.y() <= p2.y() ) ? p1.y(): p2.y();
				minpt.z() = (p1.z() <= p2.z() ) ? p1.z(): p2.z();
				maxpt.x() = (p1.x() > p2.x() ) ? p1.x(): p2.x();
				maxpt.y() = (p1.y() > p2.y() ) ? p1.y(): p2.y();
				maxpt.z() = (p1.z() > p2.z() ) ? p1.z(): p2.z();

				// these are references from the points vector
				// updated them so we have a proper min/max point
				// for the bounding box
				p1 = minpt;
				p2 = maxpt;
			}

			// create a handler
			TutorialHandler* handler = TUT_MGR->AddAreaHandler(area->data.c_str(), area->name.c_str());

			if(handler)
			{
				handler->data.i = area->plane;

				// add in the points that make up the area in question
				for(size_t i = 0; i < area->points.size(); i++)
					handler->data.l.coords.push_back(area->points.at(i));

				if(area->plane && !area->points.empty())
				{
					handler->data.i = area->plane;
					handler->data.l.d = (float)area->points.at(0).z();
				}

				area->resource = handler;
			}

#ifdef BETA_RELEASE
			if(_editMode)
			{
				UINT32 size = area->points.size();

				for(size_t j = 0; j < size - 1; j++)
				{
					Vec3d pt1 = area->points.at(j);
					Vec3d pt2 = area->points.at(j + 1);

					pt1.z() += .5f;
					pt2.z() += .5f;

					//_addEditLine("area", j, pt1, colorYELLOW);
					_addEditLink("areaconnect", _lineCount++, pt1, pt2, colorYELLOWALPHA);
				}

				// connect the last link
				if(size > 2)
				{
					Vec3d pt1 = area->points.at(size - 1);
					Vec3d pt2 = area->points.at(0);

					pt1.z() += .5f;
					pt2.z() += .5f;

					//_addEditLine("area", size, pt1, colorYELLOW);
					_addEditLink("areaconnect", _lineCount++, pt1, pt2, colorYELLOWALPHA);
				}
			}
#endif
		}
		break;
		case ScenarioObject::LOCATION:
		{
			if(ob->resource)
				return;

			ScenarioWorldObject* location = dynamic_cast<ScenarioWorldObject*>(ob);
			if(!location) return;

			// set up area tutorial handlers
			location->resource = TUT_MGR->AddLocationHandler(
											location->data.c_str(),
											location->name.c_str(),
											location->position.x(),
											location->position.y(),
											location->position.z(),
											(FLOAT32)location->attitude.Yaw);

#ifdef BETA_RELEASE
			if(_editMode)
			{
				/*
				UINT32 size = area->points.size();

				for(size_t j = 0; j < size - 1; j++)
				{
					Vec3d pt1 = area->points.at(j);
					Vec3d pt2 = area->points.at(j + 1);

					pt1.z() += .5f;
					pt2.z() += .5f;

					//_addEditLine("area", j, pt1, colorYELLOW);
					_addEditLink("areaconnect", _lineCount++, pt1, pt2, colorYELLOWALPHA);
				}

				// connect the last link
				if(size > 2)
				{
					Vec3d pt1 = area->points.at(size - 1);
					Vec3d pt2 = area->points.at(0);

					pt1.z() += .5f;
					pt2.z() += .5f;

					//_addEditLine("area", size, pt1, colorYELLOW);
					_addEditLink("areaconnect", _lineCount++, pt1, pt2, colorYELLOWALPHA);
				}
				*/
			}
#endif
		}
		break;
		case ScenarioObject::STRAT:
		{
			ScenarioStrat* strat = dynamic_cast<ScenarioStrat*>(ob);
			if(!strat) return;

			SCENOBJ_LIST list;

			if(!FindObjects(list, strat->name.c_str(), ScenarioObject::PLACED))
				return;

			for(size_t i = 0; i < list.size(); i++)
			{
				ScenarioObject* ob = list.at(i);

				if(!ob->resource)
					continue;

				// create a new sto and save it
				TerrainSTO* sto = static_cast<TerrainSTO*>(ob->resource);
				if(sto) sto->SetCountry(strat->flags);
			}
		}
		break;
		case ScenarioObject::BOUNDARY:
		{
			ScenarioBoundary* boundary = dynamic_cast<ScenarioBoundary*>(ob);
			if(!boundary) return;

			int count = boundary->blocks.size();

			// create blocking volumes for all sides of the boundary polygon
			for(int i = 0; i < count; i++)
			{
				ScenarioBlock& block = boundary->blocks.at(i);
				ScenarioBlock& nextBlock = boundary->blocks.at((i < count - 1) ? i + 1 : 0);

				// set our pt2 to the next block's pt1 (create a link)
				block.pt2 = nextBlock.pt1;

				// load it
				LoadObject(&block, boundary->loadFlags);
			}
		}
		break;
		case ScenarioObject::BLOCK:
		{
			if(ob->resource)
				return;

			ScenarioBlock* block = dynamic_cast<ScenarioBlock*>(ob);
			if(!block) return;

			// create blocking volumes for all sides of the boundary polygon
			Vec3d p1 = block->pt1;
			Vec3d p2 = block->pt2;

			// now we have the 2 points that make up the boundary volume
			Vec3d from = Vec3d(0, 1, 0);
			Vec3d to = p2 - p1;

			double height	= block->height * 2.0f;
			double width	= block->width;
			double length	= to.length();
			double minZ		= (p2.z() < p1.z()) ? p2.z() : p1.z();

			to.z() = 0;
			to.normalize();

			Matrixd rotate;
			rotate.makeRotate(from, to);

			dGeomID box = dCreateBox(colliderTerrainSimpleSpace(), width, length, height);

			dMatrix3 dAttitude;
			MatrixTodMatrix(rotate, dAttitude);

			// rotate our box into place
			dGeomSetRotation(box, dAttitude);

			to *= (length * .5f);

			Vec3d pos = p1 + to;
			pos.z() = minZ;

			pos = physicsGameToPhysical(pos);

			// position it
			dGeomSetPosition(box, pos.x(), pos.y(), pos.z());

			// make our collision object the user data
			dGeomSetData(box, block);

			// set our collide category bits
			colliderSetBits(box, COLLIDER_BLOCK, 0);

			// make our boundary area remember this geom so we can destroy it later
			block->resource = box;

#ifdef BETA_RELEASE
			if(_editMode)
			{
				p1.z() += .5f;
				p2.z() += .5f;

				//_addEditLine("block", _lineCount++, pt1, colorRED);
				_addEditLink("blockconnect", _lineCount++, p1, p2, colorREDALPHA);
			}
#endif
		}
		break;
		case ScenarioObject::VOLUME:
		{
			if(ob->resource)
				return;

			ScenarioVolume* volume = dynamic_cast<ScenarioVolume*>(ob);
			if(!volume) return;

			Vec3d& dim = volume->dimensions;
			dGeomID geom = nullptr;

			switch(volume->shape)
			{
				case dSphereClass:
				{
					geom = dCreateSphere(colliderTerrainSimpleSpace(), dim.x());
				}
				break;
				case dBoxClass:
				{
					geom = dCreateBox(colliderTerrainSimpleSpace(), dim.x(), dim.y(), dim.z());
				}
				break;
			}

			if(!geom)
				return; // unsupported type

			// now we have the 2 points that make up the boundary volume
            //  TBD - but only one is being used.  Why?  Is this an error in the calcualtion?
			// Vec3d front = Vec3d(0,1,0);

			Vec3d up = Vec3d(0,0,1);

			Matrixd rotate;
			rotate.makeRotate(volume->attitude.Yaw, up);

			dMatrix3 dAttitude;
			MatrixTodMatrix(rotate, dAttitude);

			// rotate our box into place
			dGeomSetRotation(geom, dAttitude);

			Vec3d pos = physicsGameToPhysical(volume->position);

			// position it
			dGeomSetPosition(geom, pos.x(), pos.y(), pos.z());

			// make our collision object the user data
			dGeomSetData(geom, volume);

			// set our collide category bits
			colliderSetBits(geom, COLLIDER_VOLUME, 0);

			// make our boundary area remember this geom so we can destroy it later
			volume->resource = geom;

			// this is our tutorial hook
			//TUT_MGR->AddAreaHandler(volume->data.c_str(), volume->name.c_str());
		}
		break;
        case ScenarioObject::COAST:
            {
			    CreateCoastSTO(ob);
        		break;
            }
        case ScenarioObject::RIVER:
            {
                CreateRiverSTO(ob);
                break;
            }
        case ScenarioObject::OCEAN:
            {
                CreateOceanSTO(ob);
                break;
            }
        case ScenarioObject::TERRAIN:
            {
                CreateTerrainSTO(ob);
                break;
            }
        default:
            {
                // Do nothing for now
            }
	}
}

void ScenarioManager::UnloadObject(ScenarioObject* ob)
{
	switch(ob->type)
	{
		case ScenarioObject::CLONE:
		case ScenarioObject::PLACED:
		case ScenarioObject::CLUTTER:
		case ScenarioObject::COAST:
        case ScenarioObject::RIVER:
        case ScenarioObject::OCEAN:
        case ScenarioObject::TERRAIN:
		{
			if(ob->resource)
			{
				ServerTrackedObject* sto = static_cast<ServerTrackedObject*>(ob->resource);

				if(sto && sto->activated())
				{
					bool reset = (sto->type() == ServerTrackedObject::_VEHICLE);

					sto->deactivate();
					wwiiRemoveSTO(sto->uid());

					if(reset)
					{
						obcsResetSortedList();
						obcsResetUnsortedList();
						obcsResetMultiTexture();
						obcsResetSortedCockpitList();
						obcsResetUnsortedCockpitList();
					}
				}

				ob->resource = nullptr;
			}
		}
		break;
		case ScenarioObject::EMITTER:
		{
			if(ob->resource)
			{
				TransformEffect* effect = static_cast<TransformEffect*>(ob->resource);
				if(effect) effect->kill();
				ob->resource = nullptr;
			}
		}
		break;
		case ScenarioObject::SOUND:
		{
			ScenarioSound* sound = dynamic_cast<ScenarioSound*>(ob);

			if(sound)
			{
				if(sound->id >= 0)
					fxSndStopSound(sound->id);

				sound->id = -1;
			}
		}
		break;
		case ScenarioObject::AREA:
		case ScenarioObject::LOCATION:
		{
			TUT_MGR->RemoveHandler(ob->data.c_str());
			ob->resource = nullptr;
		}
		break;
		case ScenarioObject::BOUNDARY:
		{
			ScenarioBoundary* boundary = dynamic_cast<ScenarioBoundary*>(ob);
			if(!boundary) return;

			int count = boundary->blocks.size();

			// create blocking volumes for all sides of the boundary polygon
			for(int i = 0; i < count; i++)
			{
				ScenarioBlock& block = boundary->blocks.at(i);
				UnloadObject(&block);
			}
		}
		break;
		case ScenarioObject::BLOCK:
		{
			if(ob->resource)
				dGeomDestroy((dGeomID)ob->resource);

			ob->resource = nullptr;
		}
		break;
		case ScenarioObject::VOLUME:
		{
			//TUT_MGR->RemoveHandler(ob->data.c_str());

			if(ob->resource)
				dGeomDestroy((dGeomID)ob->resource);

			ob->resource = nullptr;
		}
		break;
		case ScenarioObject::WAYPOINT:
		{
			if(ob->resource)
			{
				// we check to make sure its a valid hud object;
				// because it could get deleted on the players last despawn
				PersHUDManager::HUDObjectList& obs = HUD_MGR->GetObjects();
				PersHUDManager::HUDObjectList::iterator it = obs.begin();

				for(; it != obs.end(); it++)
				{
					PersHUDManager::HUDObject* ho = *it;

					if(ob->resource == ho)
					{
						obs.erase(it);
						delete ho;
						break;
					}
				}
			}

			// reset our ptr
			ob->resource = nullptr;
		}
		break;
		case ScenarioObject::HANDLER:
		{
			// run this code; could be dangerous?
			char code[128+1];
			snprintf(code, 128, "UnregisterState(\"%s\");", ob->name.c_str());

			TUT_MGR->RunCode(code);
		}
		break;

            default:
            // no actions for any other types.
            break;
	}
}

bool ScenarioManager::FindObjects(SCENOBJ_LIST& list, const char* name, ScenarioObject::TYPE type, bool relevant)
{
	for(size_t i = 0; i < named.size(); i++)
	{
		ScenarioObject* ob = named.at(i);

		if(relevant && !RoundRelevant(ob))
			continue;

		if((type == ScenarioObject::ANY || ob->type == type) && strcmp(ob->name.c_str(), name) == 0)
			list.push_back(ob);
	}

	return !list.empty();
}

bool ScenarioManager::FindObjects(SCENOBJ_LIST& list, ScenarioObject::TYPE type, bool relevant)
{
	for(size_t i = 0; i < objects.size(); i++)
	{
		ScenarioObject* ob = objects.at(i);

		if(relevant && !RoundRelevant(ob))
			continue;

		if(ob->type == type)
			list.push_back(ob);
	}

	return !list.empty();
}

ScenarioObject* ScenarioManager::FindObject(const char* name, ScenarioObject::TYPE type, bool relevant)
{
	SCENOBJ_LIST list;

	if(FindObjects(list, name, type, relevant))
	{
		for(size_t i = 0; i < list.size(); i++)
		{
			ScenarioObject* ob = list.at(i);

			if(type == ScenarioObject::ANY || ob->type == type)
				return ob;
		}
	}

	return nullptr;
}

void ScenarioManager::LoadObjects(UINT32 loadFlags)
{
	for(size_t i = 0; i < objects.size(); i++)
	{
		LoadObject(objects[i], loadFlags);
	}

	trnCDUpdateCollideLists(true);
}

/*
void ScenarioManager::LoadIncludes()
{
	for(int i = 0; i < objects.size(); i++)
	{
		ScenarioObject* ob = objects.at(i);

		if(ob->type == ScenarioObject::INCLUDE)
			TUT_MGR->RunScript(ob->data.c_str());
	}
}

ScenarioArea* ScenarioManager::FindArea(const char* name)
{
	for(int a = 0; a < areas.size(); a++)
	{
		ScenarioObject* ob = areas.at(a);

		if(!strcmp(ob->name.c_str(), name))
			return dynamic_cast<ScenarioArea*>(ob);
	}

	return nullptr;
}

void ScenarioManager::LoadAreas()
{
	if(IsActive())
	{
		for(int a = 0; a < areas.size(); a++)
			LoadObject(area, true);
	}
}

void ScenarioManager::LoadBoundaries()
{
	for(int a = 0; a < boundaires.size(); a++)
		LoadObject(area, true);
}
*/

void ScenarioManager::LoadContent()
{
    /*
       TBD - whenever we actually do custom maps, lol, for now - gone.

     bool active = IsActive();

	// global offset for custom maps; everything will be offset
	Vec3d offset = (active) ? _scenario->offset : Vec3d();

     */

	// load our category (mappack) include if one is defined
	jsContentData mapPack;

	// look up our mappack content data
	if(_scenario->GetParent(mapPack) && mapPack.includes != nullptr)
	{
		std::string content;

		for(size_t i = 0; i < mapPack.includes->size(); i++)
		{
			content = "include ";
			content += mapPack.includes->at(i);
			AddContent(content.c_str());
		}
	}

	// tell our scenario script to load our scenario content!
	SetConfig("load", "content");

	// content load stage
	LoadObjects(ScenarioObject::ON_CONTENT_LOAD);

#ifdef BETA_RELEASE
	if(_editMode)
	{
		for(size_t i = 0; i < spawns.size(); i++)
		{
			const ScenarioWorldObject* spawn = spawns.at(i);

			if(strstr(spawn->data.c_str(), "allied"))
				_addEditLine("spawn", i, spawn->position, colorBLUEALPHA);
			else
				_addEditLine("spawn", i, spawn->position, colorREDALPHA);
		}
	}
#endif
}

void ScenarioManager::UnloadContent()
{
	for(size_t i = 0; i < objects.size(); i++)
	{
		ScenarioObject* ob = objects.at(i);
		UnloadObject(ob);
		delete ob;
	}

	spawns.clear();
	events.clear();
	sounds.clear();
	named.clear();
	areas.clear();
	boundaries.clear();
	vehicles.clear();
	objects.clear();
	updaters.clear();

#ifdef BETA_RELEASE
	for(size_t i = 0; i < _editLines.size(); i++)
	{
		std::pair<const char*, int>& line = _editLines.at(i);
		_removeEditLine(line.first, line.second);
	}

	_lineCount = 0;
#endif

    // Force free of queued objects
    obpoolUpdate();
}

void ScenarioManager::RunEvent(const char* name)
{
	for(size_t i = 0; i < events.size(); i++)
	{
		ScenarioEvent* evt = events.at(i);

		if(strcmp(evt->name.c_str(), name) == 0)
		{
			RunEvent(evt);
			break;
		}
	}
}

void ScenarioManager::RunEvent(ScenarioEvent* evt)
{
	if(!evt->resource)
	{
		SCENOBJ_LIST list;

		evt->nextEvent = frametime + evt->eventInterval;

		// try to trigger a resource
		if(GetRandom(0.0f, 1.0f) <= evt->chance)
		{
			// find an object in our named group and load it
			list.clear();

			if(FindObjects(list, evt->name.c_str(), ScenarioObject::ANY, true))
			{
				int lastIndex	= (int)list.size() - 1;
				int index		= (lastIndex == 0) ? 0 : (int)GetRandom(0, lastIndex);

				// get our object to run
				ScenarioObject* ob = list[index];

				LoadObject(ob, ScenarioObject::ON_RUN_OBJECT);

				evt->lastEvent = frametime;
				evt->nextEvent = frametime;
				evt->resource = ob;
			}
		}
	}
	else
	{
		// we're currently tracking a resource; see if we need to free/stop it
		ScenarioObject* ob = static_cast<ScenarioObject*>(evt->resource);

		switch(ob->type)
		{
			case ScenarioObject::SOUND:
			{
				ScenarioSound* sound = dynamic_cast<ScenarioSound*>(ob);

				if(sound && sound->id == -1)
				{
					evt->nextEvent = frametime + evt->eventInterval;
					evt->resource = nullptr;
				}
			}
			break;
			case ScenarioObject::FPSFX:
			{
				ScenarioFPSFX* fx = dynamic_cast<ScenarioFPSFX*>(ob);
				if(!fx) return;

				evt->nextEvent = frametime + evt->runInterval;

				if(evt->run == -1)
				{
					evt->flags = GetRandom(1, evt->runs);
					evt->run = 0;
				}

				if(evt->run >= (int)evt->flags)
				{
					// we're done
					evt->nextEvent = frametime + evt->eventInterval;
					evt->resource = nullptr;
					evt->run = -1;
					fx->position.set(0, 0, 0);
				}
				else
				{
					fx->DoFX();
					evt->run++; // increment our run counter
				}
			}
			break;
			case ScenarioObject::FIREPOINT:
			{
				ScenarioFirepoint* fp = dynamic_cast<ScenarioFirepoint*>(ob);
				if(!fp) return;

				evt->nextEvent = frametime + evt->runInterval;

				if(evt->run == -1)
					evt->run = 0;

				if(evt->run)
				{
					// we're done
					evt->nextEvent = frametime + evt->eventInterval;
					evt->resource = nullptr;
					evt->run = -1;
					fp->Update(false);
				}
				else
				{
					evt->run++; // increment our run counter
				}
			}
			break;
			case ScenarioObject::BOMBRUN:
			{
				ScenarioBombRun* bombRun = dynamic_cast<ScenarioBombRun*>(ob);
				if(!bombRun) return;

				evt->nextEvent = frametime + evt->runInterval;

				if(evt->run == -1)
				{
					evt->run = 0;
					bombRun->move = bombRun->end - bombRun->start;
					bombRun->move /= evt->runs;
					bombRun->position = bombRun->start;
				}

				if(evt->run >= evt->runs)
				{
					// we're done
					evt->nextEvent = frametime + evt->eventInterval;
					evt->resource = nullptr;
					evt->run = -1;
				}
				else
				{
					// move our position and fire off another round
					bombRun->position += bombRun->move;
					Vec3d ammoPos(bombRun->position.x(), bombRun->position.y(), bombRun->position.z() + 500.0f);
					Vec3d ammoDir(0, 0, -1.0f);
					CreateAmmo(bombRun->data.c_str(), ammoPos, ammoDir);
					evt->run++; // increment our run counter
				}
			}
			break;
			case ScenarioObject::BARRAGE:
			{
				ScenarioBombBarrage* barrage = dynamic_cast<ScenarioBombBarrage*>(ob);
				if(!barrage) return;

				evt->nextEvent = frametime + evt->runInterval;

				if(evt->run == -1)
					evt->run = 0;

				if(evt->run >= evt->runs)
				{
					// we're done
					evt->nextEvent = frametime + evt->eventInterval;
					evt->resource = nullptr;
					evt->run = -1;
				}
				else
				{
					// calculate a random direction
					Vec3d dir = Vec3d(GetRandom(-barrage->radius, barrage->radius), GetRandom(-barrage->radius, barrage->radius), 0.0f);

					// calculate our target position
					Vec3d position = barrage->center + dir;
					position.z() += 500.0f;

					dir.set(0, 0, -1.0);

					// fire off a round
					CreateAmmo(barrage->data.c_str(), position, dir);

					// increment our run count
					evt->run++;
				}
			}
			break;
			default:
				evt->nextEvent = frametime + evt->eventInterval;
				evt->resource = nullptr;
				evt->run = -1;
			break;
		}
	}
}

void ScenarioManager::RunCommands(TUT_EVENT_TYPE type)
{
	if(!IsActive())
		return;

	for(size_t i = 0; i < objects.size(); i++)
	{
		ScenarioObject* command = objects.at(i);

		if(command->type == ScenarioObject::COMMAND && (TUT_EVENT_TYPE)command->flags == type)
			uiDotCommand(command->data.c_str());
	}
}

#ifdef BETA_RELEASE

void ScenarioManager::AddEditPoint(float x, float y, float z)
{
	if(!_editMode)
		return;

	_editArea.push_back(Vec3d((double)x, (double)y, (double)z));

	size_t count = _editArea.size();

	if(count > 1 && _editMode)
	{
		size_t index1 = count - 2;
		size_t index2 = count - 1;

		Vec3d pt1 = _editArea.at(index1);
		Vec3d pt2 = _editArea.at(index2);

		pt1.z() += .5f;
		pt2.z() += .5f;

		_addEditLink("editareaconnect", (int)index1, pt1, pt2, colorCYANALPHA);
	}
}

void ScenarioManager::ExportEditPoints()
{
	// auto add this to our clipboard
	std::wstring clipboard;

	wchar_t line[128+1];

	for(size_t i = 0; i < _editArea.size() - 1; i++)
	{
		_removeEditLine("editareaconnect", (int)i);

		Vec3d& pt = _editArea.at(i);

		swprintf(line, 128, L"\"area name OnTrigger %.2f %.2f %.2f\",\n", (float)pt.x(), (float)pt.y(), (float)pt.z());

		clipboard += line;
	}

	// remove this group of lines
	_editArea.clear();

	// send it to the clipboard
	piinfoClipboard(clipboard.c_str());
}

void ScenarioManager::SetEditMode(bool mode)
{
	_editMode = mode;

	if(_editMode)
	{
		draw3dSetLineDrawing(true);

		// add any data
	}
	else
	{
		draw3dSetLineDrawing(false);

		for(size_t i = 0; i < _editLines.size(); i++)
		{
			std::pair<const char*, int>& line = _editLines.at(i);
			_removeEditLine(line.first, line.second);
		}

		_editLines.clear();
	}
}

void ScenarioManager::SetTerrain(const char* name)
{
	if(!IsActive())
		return;

	_scenario->terrain = name;

	mapPaths.clear();

	SetConfig("terrain", name);

	// load up any inheritable map directories
	SetupTerrainPaths(name);
	DetermineMapModel();

	// last fallback parent is the common directory
	mapPaths.push_back("common");

	if(uiInGame())
	{
		if(_map)
			_map->setExpired();

		_map = nullptr;

		LoadTerrain();
	}
}

void ScenarioManager::SetContent(const char* name)
{
	if(!IsActive())
		return;

	(*_scenario)["content"] = name;

	// let our lua state know
	SetConfig("content", name);

	// force a load of any scenario content
	LoadContent();

	if(uiInGame())
		LoadObjects();
}

void _addEditLine(const char* name, int id, Vec3d pos, UINT32 color)
{
	// physics object line
	FLOAT64VECTOR v1, v2, temp;
	Vec3d pv1, pv2;
	Vec3d normal = Vec3d(0,0,1.0f);

	pv1 = pos;
	pv2 = pos + normal;

	pv1.get(v1);
	v1.z = -v1.z;

	pv2.get(v2);
	v2.z = -v2.z;

	char buffer[32];

	snprintf(buffer, 31, "%s%d", name, id);

	if(!color)
		color = colorGREENALPHA;

	draw3dSetWorldDebugLine(&v1, &v2, buffer, color);

	// now we draw the base
	temp = v1;

	v2 = v1;

	v1.x -= .05f;
	v2.x += .05f;

	snprintf(buffer, 31, "%s%dx", name, id);
	draw3dSetWorldDebugLine(&v1, &v2, buffer, color);

	v1 = temp;
	v2 = v1;

	v1.y -= .05f;
	v2.y += .05f;

	snprintf(buffer, 31, "%s%dy", name, id);
	draw3dSetWorldDebugLine(&v1, &v2, buffer, color);

	// save our line reference so we can free it later
	_editLines.push_back(std::make_pair(name, id));
}

void _addEditLink(const char* name, int id, Vec3d pt1, Vec3d pt2, UINT32 color)
{
	// physics object line
	FLOAT64VECTOR v1, v2;

	pt1.get(v1);
	v1.z = -v1.z;

	pt2.get(v2);
	v2.z = -v2.z;

	char buffer[32];
	snprintf(buffer, 31, "%s%d", name, id);

	if(!color)
		color = colorGREENALPHA;

	draw3dSetWorldDebugLine(&v1, &v2, buffer, color);

	// save our line reference so we can free it later
	_editLines.push_back(std::make_pair(name, id));
}

void _removeEditLine(const char* name, int id)
{
	char buffer[32];

	snprintf(buffer, 31, "%s%d", name, id);
	draw3dRemoveDebugObject(draw3dFindDebugObject(buffer));

	snprintf(buffer, 31, "%s%dx", name, id);
	draw3dRemoveDebugObject(draw3dFindDebugObject(buffer));

	snprintf(buffer, 31, "%s%dy", name, id);
	draw3dRemoveDebugObject(draw3dFindDebugObject(buffer));
}

#endif

ScenarioManager* ScenarioManager::GetInstance()
{
	return &_scenMgr;
}

void ScenarioFPSFX::DoFX()
{
	if(!uiInGame())
		return;

	vehVehicle* player	= persGetPersonaVehiclePtr();
	if(!player) return;

	if(flags)
	{
		Vec3d spos = Vec3d(player->ModelData->VehiclePos);

		spos.x() += GetRandom(-10.0f, 10.0f);
		spos.y() += GetRandom(-10.0f, 10.0f);
		spos.z() += GetRandom(.1f, 10.0f);

		SINT32 id = fxSndGetNewSound();
		if(id == -1) return;

		const char* flybys[] =
		{
			"sounds/bulletby.wav",
			"sounds/loshot1.wav",
			"sounds/loshot2.wav",
			"sounds/loshot3.wav"
		};

		int index = GetRandom(0, 3);

		fxSndSetSoundName(id, (SCHAR*)flybys[index]);
		fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
		fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
		fxSndSetPitch(id, 1.0f);
		fxSndSetVolume(id, 5.0f);
		fxSndSetMinDistance(id, 5.0f);
		fxSndSetMaxDistance(id, 10.0f);
		fxSndSetPosition(id, spos.x(), spos.y(), spos.z());
		fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
		fxSndInitSound(id);
	}
	else
	{
		static const float minArea	= 14.0f;
		static const float maxArea	= 50.0f;
		static const float arc		= 35.0f;

		if(position.length2() == 0)
		{
			position.set(player->ModelData->VehiclePos);

			Matrixd attitude	= Matrixd(player->ModelData->VehicleAttMat);
			Vec3d forward		= Vec3d(0, -1.0f, 0);

			Matrixd rotate;
			rotate.makeRotate(mlDEG_TO_RAD(GetRandom(-arc, arc)), 0, 0, 1.0f);

			// rotate to forward
			forward = forward * attitude;

			// rotate it left or right
			forward = forward * rotate;

			// move it out
			forward *= GetRandom(minArea, maxArea);

			// shoot our ray down from here
			position += forward;
		}
		else
		{
			position.x() += GetRandom(-3.0f, 3.0f);
			position.y() += GetRandom(-3.0f, 3.0f);
		}

		const float dist = 50.0f;

		Vec3d down	= Vec3d(0, 0, -1.0f);
		Vec3d start = position;

		start.z() += dist;

		const colliderQuery::CollisionList& hits = collideRayList(COLLIDER_TILE, dist * 2.0f, start, down, true);

		if(!hits.empty())
		{
			const colliderQuery::Collision& hit = hits.at(0);
			const char* fx = nullptr;
			const char* sound = nullptr;

			position = physicsPhysicalToGame(hit.pos);

			if(colliderIsType(hit.g2, COLLIDER_TRN_WATER))
			{
				const int count = 2;

				const char* waterFx[] =
				{
					"waterhit",
					"waterhit"
				};

				const char* waterSounds[] =
				{
					"sounds/hitwater1.wav",
					"sounds/hitwater2.wav"
				};

				int index = GetRandom(0, count - 1);

				fx		= waterFx[index];
				sound	= waterSounds[index];
			}
			else
			{
				const int count = 6;

				const char* landFx[] =
				{
					"sandhit",
					"sandhit",
					"sandhit",
					"sandhit",
					"sandhit",
					"sandhit"
				};

				const char* landSounds[] =
				{
					"sounds/hitdirt1.wav",
					"sounds/hitdirt2.wav",
					"sounds/hitdirt3.wav",
					"sounds/tex/hground1.wav",
					"sounds/tex/hground2.wav",
					"sounds/tex/hground3.wav"
				};

				int index	= GetRandom(0, count - 1);

				fx			= landFx[index];
				sound		= landSounds[index];

			}

			if(sound && *sound)
			{
				SINT32 id = fxSndGetNewSound();
				if(id == -1) return;

				fxSndSetSoundName(id, (SCHAR*)sound);
				fxSndSetPriority(id, IDE_TP_PRIORITY_LOW);
				fxSndSetSoundType(id, TYPE_FINITE_EXTERNAL_SOUND);
				fxSndSetPitch(id, 1.0f);
				fxSndSetVolume(id, 1.5f);
				fxSndSetMinDistance(id, 5.0f);
				fxSndSetMaxDistance(id, 25.0f);
				fxSndSetPosition(id, position.x(), position.y(), position.z());
				fxSndSetPlayType(id, DEF_PLAYTYPE_WORLD);
				fxSndInitSound(id);
			}

			if(fx && *fx)
			{
				Vec3d n = hit.normal;
				n.z() = -n.z();

				TransformEffect* te = TransformEffect::AddEffect(fx, n, 0, nullptr);

				if(te)
				{
					Vec3d fpos = position;
					fpos.z() = -fpos.z();
					te->enableemitters(true);;
					te->setmatrix(Matrixd::translate(fpos));
					te->setflag(TransformEffect::REMOVE_ME_WHEN_DONE);
				}
			}
		}
	}
}

void ScenarioFirepoint::Update(bool firing)
{
	if(firing && !resource)
	{
		SCENOBJ_LIST list;

		if(!SCEN_MGR->FindObjects(list, data.c_str(), ScenarioObject::CLONE))
			return;

		ScenarioObject* found = list[(size_t)GetRandom(0, (int)list.size() - 1)];
		if(!found || !found->resource) return;

		VehicleSTO* sto = static_cast<VehicleSTO*>(found->resource);
		if(!sto) return;

		vehVehicle* vehicle = sto->getVehicle();
		if(!vehicle) return;

		double elevation	= attitude.Pitch + GetRandom(-vVariance, vVariance);
		double traverse		= attitude.Yaw + GetRandom(-hVariance, hVariance);

		// update the turret
		vehicle->StateAnimation.TurretElev[turret]			= (FLOAT32)elevation;
		vehicle->StateAnimation.TurretTraverse[turret]		= (FLOAT32)traverse;

		if(flags)
		{
			// snap into place
			vehicle->StateAnimation.LastTurretElev[turret]		= (FLOAT32)elevation;
			vehicle->StateAnimation.LastTurretTraverse[turret]	= (FLOAT32)traverse;
		}

		// set whether the mount is firing or not
		vehicle->MountIsFiring[mount] = TRUE;

		// save which vehicle we are managing this run
		resource = found;
	}
	else if(!firing || !resource)
	{
		if(resource)
		{
			ScenarioObject* found = static_cast<ScenarioObject*>(resource);
			if(!found || !found->resource) return;

			VehicleSTO* sto = static_cast<VehicleSTO*>(found->resource);
			if(!sto) return;

			vehVehicle* vehicle = sto->getVehicle();
			if(!vehicle) return;

			// set whether the mount is firing or not
			vehicle->MountIsFiring[mount] = FALSE;
		}

		// save which vehicle we are managing this run
		resource = nullptr;
	}

	/*
	// we fire along the y
	Vec3d dir = Vec3d(0, 1.0f, 0);

	Matrixd rotate;

	rotate.makeRotate(fp->attitude.Yaw, 0.0f, 0.0f, 1.0f);
	dir = dir * rotate;

	rotate.makeRotate(fp->attitude.Pitch, 1.0f, 0.0f, 0.0f);
	dir = dir * rotate;

	// fire off the ammo
	CreateAmmo(fp->data.c_str(), fp->position, dir);
	*/
}

void _normalizeAngle(double& angle)
{
	while (angle * angle > 180.0 * 180.0)
	{
		if (angle < 0)
			angle += 360.0f;
		else if (angle >= 0)
			angle -= 360.0f;
	}
}


void _extractNumbers(const char* source, double& x, double& y, double& z)
{
	FLOAT32 _x, _y, _z;
	_x = _y = _z = 0.0f;

	// try to set x,y,z
	if(sscanf(source, "%f,%f,%f", &_x, &_y, &_z) != 3)
	{
		_x = _y = _z = 0.0f;

		// otherwise try x,y
		if(sscanf(source, "%f,%f", &_x, &_y) != 2)
		{
			_x = _y = _z = 0.0f;

			// otherwise try simply z
			if(sscanf(source, "%f", &_z) != 1)
				_x = _y = _z = 0.0f;
		}
	}

	x = _x;
	y = _y;
	z = _z;
}

static void _bob(OBJECT& o, void* userdata)
{
	Matrixf* newMat = (Matrixf*)userdata;
	newMat->get(o.mat);
}

void ScenarioWaveObject::Update()
{
	if(bobbers.empty())
	{
		SCENOBJ_LIST list;

		SCEN_MGR->FindObjects(list, name.c_str(), ScenarioObject::PLACED);

		if(list.empty())
			return;

		for(size_t i = 0; i < list.size(); i++)
		{
			WaveData wd;

			wd.object = dynamic_cast<ScenarioWorldObject*>(list[i]);

			TerrainSTO* sto = static_cast<TerrainSTO*>(wd.object->resource);

			if(sto)
			{
				OBJECT* o = sto->GetObject();

				wd.startOffset = GetRandom(0, 1000);
				wd.objAtt.set(o->mat);
				wd.objPos.set(wd.objAtt.getTrans());
				wd.objAtt.setTrans(0, 0, 0);
			}

			bobbers.push_back(wd);
		}
	}

	for(size_t i = 0; i < bobbers.size(); i++)
	{
		WaveData& wd = bobbers.at(i);

		TerrainSTO* sto = static_cast<TerrainSTO*>(wd.object->resource);
		if(!sto) continue;

		OBJECT* o = sto->GetObject();

		if(!o || o->distsqr > 100.0f)
			continue;

		FLOAT64 pitch	= attitude.Pitch * sin(FLOAT64(frametime + wd.startOffset) * pitchRate);
		FLOAT64 roll	= attitude.Roll * sin(FLOAT64(frametime + wd.startOffset) * rollRate);
		FLOAT64 newz	= (FLOAT64)wd.objPos.z() + attitude.Yaw * sin(FLOAT64(frametime + wd.startOffset) * bobRate);

		Matrixf final;
		Matrixf rotate;

		// add pitch
		rotate.makeRotate((float)pitch, 1.0f, 0, 0);
		final = final * rotate;

		// add roll
		rotate.makeRotate((float)roll, 0, 1.0f, 0);
		final = final * rotate;

        /*
          TBD - rotate it along the y, did this have an actual purpose?

		Vec3d from = Vec3d(0.0f, 1.0f, 0.0f);

		rotate.makeRotate(from, position);
		final = final * rotate;

         */

		// rotate this into the object's world offsetted position
		final = final * wd.objAtt;

		// add in our translation
		final.setTrans((float)wd.objPos.x(), (float)wd.objPos.y(), (float)newz);

		// update our attitude
		obmainFindCallback(o, _bob, &final);
	}
}

bool ScenarioPermission::IsPermitted()
{
	return ScenarioPermission::IsPermitted(name.c_str());
}

bool ScenarioPermission::IsPermitted(const char* permission)
{
	return ScenarioPermission::IsPermitted(ScenarioPermission::GetValue(permission));
}

bool ScenarioPermission::IsPermitted(UINT64 permission)
{
	return PlayerClient()->IsPermitted(permission, false);
}

UINT64 ScenarioPermission::GetValue(const char* name)
{
	if(strcasecmp(name, "product01") == 0)
		return PRODUCT01;
	else if(strcasecmp(name, "product02") == 0)
		return PRODUCT02;
	else if(strcasecmp(name, "product03") == 0)
		return PRODUCT03;
	else if(strcasecmp(name, "product04") == 0)
		return PRODUCT04;
	else if(strcasecmp(name, "product05") == 0)
		return PRODUCT05;
	else if(strcasecmp(name, "product06") == 0)
		return PRODUCT06;
	else if(strcasecmp(name, "product07") == 0)
		return PRODUCT07;
	else if(strcasecmp(name, "product08") == 0)
		return PRODUCT08;
	else if(strcasecmp(name, "product09") == 0)
		return PRODUCT09;
	else if(strcasecmp(name, "product10") == 0)
		return PRODUCT10;
	else if(strcasecmp(name, "product11") == 0)
		return PRODUCT11;
	else if(strcasecmp(name, "product12") == 0)
		return PRODUCT12;
	else if(strcasecmp(name, "product13") == 0)
		return PRODUCT13;
	else if(strcasecmp(name, "product14") == 0)
		return PRODUCT14;
	else if(strcasecmp(name, "product15") == 0)
		return PRODUCT15;
	else if(strcasecmp(name, "product16") == 0)
		return PRODUCT16;
	else if(strcasecmp(name, "product17") == 0)
		return PRODUCT17;
	else if(strcasecmp(name, "product18") == 0)
		return PRODUCT18;
	else if(strcasecmp(name, "product19") == 0)
		return PRODUCT19;
	else if(strcasecmp(name, "product20") == 0)
		return PRODUCT20;

	return 0;
}
