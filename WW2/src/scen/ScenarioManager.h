#pragma once

#include <map>
#include <set>

#include "lua/luaMain.h"
#include "chat/chatHandle.h"
#include "math/Vec3d.h"
#include "math/Matrixf.h"
#include "tut/TutorialManager.h"
#include "js/jsContent.h"

// ----------- defines
#define SCEN_MGR ScenarioManager::GetInstance()

// ----------- predeclares
class ServerTrackedObject;
class TerrainSTO;
class TransformEffect;
struct vehType;
struct vehVehicle;

// ----------- types
typedef std::vector<std::string> SCENPATH_LIST;
typedef std::map<std::string,std::string> SCENCONF_MAP;
typedef std::vector<Vec3d> SCENPOS_LIST;

/************************************************************************************************
	ScenarioObject
*************************************************************************************************/

struct ScenarioObject
{
	enum TYPE
	{
		ANY,
		TERRAIN,
		PLACED,
		SPAWN,
		EMITTER,
		SOUND,
		COMMAND,
		INCLUDE,
		AREA,
		VEHICLE,
		EVENT,
		BOMBRUN,
		BARRAGE,
		CHAT,
		ALERT,
		CLONE,
		BLOCK,
		BOUNDARY,
		AMMO,
        COAST,
		WAYPOINT,
		LOCATION,
		CALL,
		MESSAGE,
		LIMIT,
        RIVER,
		VOLUME,
		HANDLER,
		FIREPOINT,
		FPSFX,
		WAVE,
        OCEAN,
		CLUTTER,
		STRAT, // for controlling terrain object ownership
		PERMISSION, // for productization
		WEAPONPACK
	};

	enum LOAD_FLAGS
	{
		ON_START			= 1<<0, // when the scenario starts
		ON_TERRAIN_LOAD		= 1<<1,	// when terrain tiles needs to be loaded
		ON_TERRAIN_LOADED	= 1<<2,	// when terrain tiles have been loaded
		ON_CONTENT_LOAD		= 1<<3,	// when content needs to be loaded
		ON_SPAWN			= 1<<4,	// every time we spawn
		ON_RUN_OBJECT		= 1<<5	// only when run object is called
	};

	ScenarioObject() :
		type(ANY),
		flags(0),
		team(0),
		round(0),
		stage(0),
		resource(NULL),
		runtime(0),
		duration(0), // anytime
		loadFlags(ON_RUN_OBJECT)
	{
	}

	ScenarioObject(TYPE t, const char* n, const char* d, UINT32 lf = ON_RUN_OBJECT) :
		type(t),
		name(n),
		data(d),
		flags(0),
		team(0),
		round(0),
		stage(0),
		resource(NULL),
		runtime(0),
		duration(0), // anytime
		loadFlags(lf)
	{
	}

	virtual ~ScenarioObject()
	{
	}

	virtual void Update(){}

	virtual bool DoLoadStage(UINT32 flags)
	{
		return ((loadFlags & flags) > 0);
	}

	// object type
	TYPE type;

	// filters
	UINT32 team;
	UINT32 round;
	UINT32 stage;

	// object specific flags
	UINT32 flags;

	// when can this object be loaded
	UINT32 loadFlags;

	// when can we run this object again?
	SINT32 runtime;
	SINT32 duration;

	// name of object
	std::string name;

	// object data
	std::string data;

	// resource link
	void* resource;
};

typedef std::vector<ScenarioObject*> SCENOBJ_LIST;

struct ScenarioWorldObject : public ScenarioObject
{
	ScenarioWorldObject() : ScenarioObject()
	{
		attitude.Roll = attitude.Pitch = attitude.Yaw = 0.0f;
	}

	ScenarioWorldObject(TYPE t, const char* n, const char* d) : ScenarioObject(t, n, d)
	{
		attitude.Roll = attitude.Pitch = attitude.Yaw = 0.0f;
	}

	virtual ~ScenarioWorldObject()
	{
	}

	// usually position
	Vec3d position;

	// usually rotation
	FLOAT64ATT attitude;
};

typedef std::vector<ScenarioWorldObject*> SCENWOB_LIST;

struct ScenarioCoast : public ScenarioObject
{
    ScenarioCoast(const char* n, const char* s, float x, float y, float z, float rot) :
		ScenarioObject(COAST, n, s, ScenarioObject::ON_TERRAIN_LOAD)
    {
        rotation = rot;
        position.set(x, y, z);
    }

    float rotation;
    Vec3d position;
};

struct ScenarioRiver : public ScenarioObject
{
    ScenarioRiver(const char* n, const char* s, float x, float y, float z, float rot) :
		ScenarioObject(RIVER, n, s, ScenarioObject::ON_TERRAIN_LOAD)
    {
        rotation = rot;
        position.set(x, y, z);
    }

    float rotation;
    Vec3d position;
};

struct ScenarioOcean : public ScenarioObject
{
    ScenarioOcean(const char* n, const char* s, float x, float y, float z, float rot) :
		ScenarioObject(OCEAN, n, s, ScenarioObject::ON_TERRAIN_LOAD)
    {
        rotation = rot;
        position.set(x, y, z);
    }

    float rotation;
    Vec3d position;
};

struct ScenarioStrat : public ScenarioObject
{
    ScenarioStrat(const char* n, const char* d) :
		ScenarioObject(STRAT, n, d)
    {
		if(strcasecmp(data.c_str(), "uk") == 0)
			flags = 1;
		else if(strcasecmp(data.c_str(), "us") == 0)
			flags = 2;
		else if(strcasecmp(data.c_str(), "fr") == 0)
			flags = 3;
		else if(strcasecmp(data.c_str(), "de") == 0)
			flags = 4;
    }
};

struct ScenarioFPSFX : public ScenarioWorldObject
{
    ScenarioFPSFX(const char* n, const char* d) :
		ScenarioWorldObject(FPSFX, n, d)
    {
    }

	void DoFX();
};

struct ScenarioWaveObject : public ScenarioWorldObject
{
	struct WaveData
	{
		SINT32 startOffset;
		ScenarioWorldObject* object;
		Matrixf objAtt; // original attitude matrix
		Vec3f objPos; // original world offsetted position
	};

    ScenarioWaveObject(const char* n, const char* d) :
		ScenarioWorldObject(WAVE, n, d)
    {
		loadFlags = ScenarioObject::ON_RUN_OBJECT;
    }

	virtual void Update();

	// list of objects that need to bob up and down in the waves
	std::vector<WaveData> bobbers;

	FLOAT64 pitchRate;
	FLOAT64 rollRate;
	FLOAT64 bobRate;
};

struct ScenarioCall : public ScenarioObject
{
    ScenarioCall(const char* n, const char* d) :
		ScenarioObject(CALL, n, d)
    {
    }

	std::string code;
};

struct ScenarioBombRun : public ScenarioObject
{
	ScenarioBombRun() : ScenarioObject()
	{
		type = BOMBRUN;
	}

	ScenarioBombRun(const char* n, const char* d) :
		ScenarioObject(ScenarioObject::BOMBRUN, n, d)
	{
	}

	virtual ~ScenarioBombRun()
	{
	}

	Vec3d start;
	Vec3d end;
	Vec3d position;
	Vec3d move;
};

struct ScenarioBombBarrage : public ScenarioObject
{
	ScenarioBombBarrage() : ScenarioObject(),
		radius(0.0f)
	{
		type = BARRAGE;
	}

	ScenarioBombBarrage(const char* n, const char* d) :
		ScenarioObject(BARRAGE, n, d),
		radius(0.0f)
	{
	}

	virtual ~ScenarioBombBarrage()
	{
	}

	Vec3d center;
	FLOAT32 radius;
};

/************************************************************************************************
	ScenarioSound
*************************************************************************************************/

struct ScenarioSound : public ScenarioWorldObject
{
	ScenarioSound() : ScenarioWorldObject(),
		id(-1),
		volume(1.0f),
		minDist(0.0f),
		maxDist(0.0f)
	{
		type = SOUND;
	}

	ScenarioSound(const char* n, const char* d) : ScenarioWorldObject(SOUND, n, d),
		id(-1),
		volume(1.0f),
		minDist(0.0f),
		maxDist(0.0f)
	{
	}

	virtual ~ScenarioSound()
	{
	}

	FLOAT32 volume;
	FLOAT32 minDist;
	FLOAT32 maxDist;
	SINT32 id;
};

/************************************************************************************************
	ScenarioFirepoint
*************************************************************************************************/

struct ScenarioFirepoint : public ScenarioWorldObject
{
	ScenarioFirepoint(const char* n, const char* d) :
		ScenarioWorldObject(FIREPOINT, n, d),
		turret(-1),
		mount(-1),
		hVariance(0),
		vVariance(0)
	{
	}

	virtual ~ScenarioFirepoint()
	{
	}

	void Update(bool firing);

	SINT32 turret;
	SINT32 mount;
	FLOAT32 hVariance;
	FLOAT32 vVariance;
};

typedef std::set<ScenarioSound*> SCENSND_LIST;

/************************************************************************************************
	ScenarioWaypoint
*************************************************************************************************/

struct ScenarioWaypoint : public ScenarioWorldObject
{
	ScenarioWaypoint(const char* n, const char* d) :
		ScenarioWorldObject(WAYPOINT, n, d)
	{
		loadFlags = ScenarioObject::ON_SPAWN;
	}

	virtual ~ScenarioWaypoint()
	{
	}

	std::string color;
};

/************************************************************************************************
	ScenarioArea
*************************************************************************************************/

struct ScenarioArea : public ScenarioObject
{
	ScenarioArea(const char* n, const char* d) :
		ScenarioObject(AREA, n, d, ScenarioObject::ON_CONTENT_LOAD),
		plane(0)
	{
	}

	virtual ~ScenarioArea()
	{
	}

	// world positions making up an area in the world (for triggers, etc...)
	// 2 points represents a bounding box; 3 or more represents a polygon
	SCENPOS_LIST points;

	int plane;
};

typedef std::vector<ScenarioArea*> SCENAREA_LIST;


/************************************************************************************************
	ScenarioVehicle
*************************************************************************************************/

struct ScenarioVehicleMask : public ScenarioObject
{
	ScenarioVehicleMask(const char* n, const char* d) :
		ScenarioObject(VEHICLE, n, d)
	{
		spec.fromShort(0);
	}

	virtual ~ScenarioVehicleMask()
	{
	}

	bool IsMatch(const vehType* vtype);

	// weapon mask describing what vehicles this represents
	WEAPON_SPEC spec;
};

typedef std::vector<ScenarioVehicleMask*> SCENVEH_LIST;

/************************************************************************************************
	ScenarioBlock
*************************************************************************************************/

struct ScenarioBlock : public ScenarioObject
{
	ScenarioBlock(const char* n) :
		ScenarioObject(BLOCK, n, "", ScenarioObject::ON_CONTENT_LOAD),
		height(10.0f),
		width(.5f)
	{
	}

	ScenarioBlock() :
		ScenarioObject(BLOCK, "", "", ScenarioObject::ON_CONTENT_LOAD),
		height(10.0f),
		width(.5f)
	{
	}

	virtual ~ScenarioBlock()
	{
	}

	double height;
	double width;
	Vec3d pt1;
	Vec3d pt2;
};

typedef std::vector<ScenarioBlock> SCENBLOCK_LIST;

/************************************************************************************************
	ScenarioVolume
*************************************************************************************************/

struct ScenarioVolume : public ScenarioWorldObject
{
	ScenarioVolume(const char* n, const char* d) :
		ScenarioWorldObject(VOLUME, n, d),
		shape(0)
	{
		loadFlags = ScenarioObject::ON_CONTENT_LOAD;
	}

	ScenarioVolume() :
		ScenarioWorldObject(VOLUME, "", ""),
		shape(0)
	{
		loadFlags = ScenarioObject::ON_START;
	}

	virtual ~ScenarioVolume()
	{
	}

	int shape;
	Vec3d dimensions;
};

typedef std::vector<ScenarioVolume> SCENVOL_LIST;

/************************************************************************************************
	ScenarioBoundary
*************************************************************************************************/

struct ScenarioBoundary : public ScenarioObject
{
	ScenarioBoundary(const char* n) :
		ScenarioObject(BOUNDARY, n, "", ScenarioObject::ON_CONTENT_LOAD)
	{
	}

	virtual ~ScenarioBoundary()
	{
	}

	SCENBLOCK_LIST blocks;
};


/************************************************************************************************
	ScenarioTrigger
*************************************************************************************************/

struct ScenarioEvent : public ScenarioObject
{
	ScenarioEvent(const char* n) : ScenarioObject(EVENT, n, ""),
		nextEvent(0),
		lastEvent(0),
		eventInterval(0),
		runInterval(0),
		run(-1),
		runs(1)
	{
	}

	virtual ~ScenarioEvent()
	{
	}

	// timing data
	FLOAT32 chance; // how likely each event interval to run
	SINT32 lastEvent;
	SINT32 nextEvent;
	SINT32 eventInterval; // how long between events
	SINT32 runInterval; // how long between runs
	SINT32 run; // current run
	SINT32 runs; // number of runs
};

typedef std::vector<ScenarioEvent*> SCENEVT_LIST;

/************************************************************************************************
	ScenarioPermission
*************************************************************************************************/

struct ScenarioPermission : public ScenarioObject
{
	ScenarioPermission(const char* n, const char* d) :
		ScenarioObject(PERMISSION, n, d)
	{
	}

	virtual ~ScenarioPermission()
	{
	}

	bool IsPermitted(); // pass in a permission name

	static bool IsPermitted(const char* permission); // pass in a permission name
	static bool IsPermitted(UINT64 permission); // pass in a permission name
	static UINT64 GetValue(const char* name); // returns a permission value given a permission name
};

/************************************************************************************************
	Scenario
*************************************************************************************************/

struct Scenario : public jsContentData
{
	Scenario() : jsContentData()
	{
	}

	virtual ~Scenario()
	{
	}

	void Init(const char* p);
	const char* Config(const char* name);
	bool GetParent(jsContentData& data);

	// scenario content that this scenario was created from
	std::string path;

	// scenario type; determines which lua script to run
	std::string name;

	// filename of custom map to load; optional
	std::string terrain;

	// actually mdl to use; might be a parent
	// of the specified terrain directory
	std::string map;

	// terrain inherits data from
	std::string inherits;

	// global offset for all scenario objects; everything will be offset
	Vec3d offset;
};

struct ScenarioPlayer
{
	ScenarioPlayer() : handle(0), lastUpdate(0)
	{
	}

	ScenarioPlayer(ChatHandle h) : handle(h), lastUpdate(0)
	{
	}

	virtual ~ScenarioPlayer()
	{
	}

	bool IsActive();

	std::string GetAttribute(const char* name);

	// queue's an update to be sent up to the server
	// sets the state
	void Set(const char* name, SINT32 value);
	void Set(const char* name, UINT32 value);
	void Set(const char* name, bool value);
	void Set(const char* name, float value);
	void Set(const char* name, const char* value);

	// queue an add to be sent up to the server
	// adds to the existing state (must be a number)
	void Add(const char* name, SINT32 value);
	void Add(const char* name, UINT32 value);
	void Add(const char* name, bool value);
	void Add(const char* name, float value);
	void Add(const char* name, const char* value);

	// receive an update of a player from the server
	void Receive(const char* name, const char* value);

	WEAPON_SPEC GetWeaponSpec();
	SINT32 GetTeam();
	SINT32 GetKills();
	SINT32 GetDeaths();
	SINT32 GetScore();
	FLOAT64 GetX();
	FLOAT64 GetY();
	bool GetReady();
	bool GetSpawned();
	RANK_NO GetRank();
	vehVehicle* GetVehicle();

	bool IsMe();

	ChatHandle handle;
	SINT32 lastUpdate;
	std::map<std::string, std::string> attributes;
};

typedef std::map<ChatHandle, ScenarioPlayer> SCENPLAYER_MAP;

/************************************************************************************************
	ScenarioManager
*************************************************************************************************/

// tolua_begin
class ScenarioManager
{
public:

	// flags for controlling game behavior
	// below the ui and lua layers
	enum GAME_CONTROL
	{
		ALL_OFF					= 0,
		WHO_KILLED_ME			= 1<<0, // show floating death icons in game
		ENEMY_DEATHS			= 1<<1, // show floating death icons in game
		ENEMY_ICONS				= 1<<2, // show enemy icons
		FAST_ENEMY_ICONS		= 1<<3, // show icons for sprinting enemy
		FIRING_ENEMY_ICONS		= 1<<4, // show icons for firing enemy
		HIDE_PRONE_ENEMY_ICONS	= 1<<5, // hide icons for prone enemy
		ENEMY_ICON_TYPES_ONLY	= 1<<6, // only show enemy vehicle type icons only
		RUN_EVENTS				= 1<<7,	// do we run events?
		ENEMY_INTEL				= 1<<8,	// do enemy intel reports
		RESUPPLY_ME				= 1<<9, // resupply my weapons?
		RESPAWN_ME				= 1<<10 // respawn me right away
	};

    // Not dependent on _scenario being NULL
    enum GLOBAL_GAME_CONTROL
    {
        GLOBAL_NONE         = 0,
        GLOBAL_GENERATE_MAP = 1<<0
    };

    ScenarioManager();
	~ScenarioManager();

	// load any resources
	void Init();

	// frame update
	void Update();

	// player update
	void UpdatePlayer(const char* callsign, const char* state, const char* value);

	// is a scenario running
	bool IsActive() const;

	// have we completed the scenario
	bool IsScenarioComplete() const;

// tolua_end

	bool IsCustomTerrain() const;
	bool IsTerrainLoaded() const;

	bool Start(const char* path, bool init = true);
	bool Stop();

	// this scenario is completed
	void SetComplete();
	void SetConfig(const char* name, const char* value);
	void SetRespawnWeapon(const WEAPON_SPEC& spec);
	bool Spawn(const vehType* vehtype);
	bool Respawn();

	// set or remove a game flag; returns true if a change was made
	bool SetFlag(SINT32 flag, bool on = true); // tolua_export
	// is this flag on?
	bool HasFlag(SINT32 flag); // tolua_export

	// number of players; pass in 0 to get the total players count
	int GetPlayerCount(int team = 0); // tolua_export
	int GetReadyCount(int team = 0); // tolua_export

	SCENPLAYER_MAP& GetPlayers();
	ScenarioPlayer* GetPlayer(const char* callsign);
	ScenarioPlayer* GetPlayer(const wchar_t* callsign = NULL);

	void SetPlayerState(const char* name, const char* value);
	void AddPlayerState(const char* name, const char* value);

	// get the current round/stage info
	int GetRound(); // tolua_export
	int GetStage(); // tolua_export

	Scenario* GetScenario();

	// pass -1 to simply increment
	int SetRound(int r = -1); // tolua_export
	int SetStage(int s = -1); // tolua_export
	void SetPhase(int r, int s);

	// set how we see this vehicle's icon
	void SetVehicleIcon(vehVehicle* vehicle);

	// is this vehicle valid for this scenario
	bool ValidVehicle(const vehType* vtype);

	// look up a valid vehicle mask for this vehtype
	// if we have one; null otherwise
	const ScenarioVehicleMask* VehicleMask(const vehType* vtype, bool best = true, bool spawnable = true);

	// see if we have a limit for this vehicle id or type
	// returns -1 for no limit
	int VehicleLimit(const char* id);
	int VehicleLimit(const vehType* vtype);
	int VehicleCount(const vehType* vtype);
	bool VehiclePermitted(const vehType* vtype);

	// get a random spawn position for this vehicle type
	const ScenarioWorldObject* VehicleSpawn(const vehType* vtype);

	// does this block apply to my vehicle
	bool PlayerRelevant(const char* name); // tolua_export
	bool PlayerRelevant(ScenarioObject* ob); // tolua_export
	bool RoundRelevant(ScenarioObject* ob);

	// offline data for testing
	void SetOfflineData(UINT32 t, UINT32 r, UINT32 s);

	int SetTeam(int t);
	int GetTeam(); // tolua_export
	int GetOtherTeam(); // tolua_export

	// is this player on the same team as me
	bool SameTeam(vehVehicle* player);

	// create a custom map path
	const char* MapPath(const char* file, const char* ender = NULL);
	const char* MapName();
	const char* MapModelName();

	// get our custom map's pos and scale
	bool InGameMapPos(FLOAT32& x, FLOAT32& y, FLOAT32& scale);

	// localizations
	const wchar_t* GetLoc(const char* key);

	// setup up our terrain
	const char* CustomTerrainMap();
	void LoadTerrain();
	void SetupTerrainPaths(const char* check);
	void DetermineMapModel();
    void SetTerrainReadyToLoad(bool load = true);
    void PushGlobalFlag(unsigned int flag);
    void PopGlobalFlag(unsigned int flag);

	// setup any terrain objects or fx
	void RunObject(const char* name, int load); // tolua_export
	void RunObject(ScenarioObject::TYPE type, int load); // tolua_export
	void RunObject(const char* name, int load, const char* data); // tolua_export
	void RunObject(ScenarioObject::TYPE type, int load, const char* data); // tolua_export
	void LoadObject(ScenarioObject* ob, UINT32 loadFlags, const char* data = NULL);
	void UnloadObject(ScenarioObject* ob);
	bool FindObjects(SCENOBJ_LIST& list, const char* name, ScenarioObject::TYPE type = ScenarioObject::ANY, bool relevant = false);
	bool FindObjects(SCENOBJ_LIST& list, ScenarioObject::TYPE type, bool relevant = false);
	ScenarioObject* FindObject(const char* name, ScenarioObject::TYPE type = ScenarioObject::ANY, bool relevant = false);

	void LoadObjects(UINT32 loadFlags = ScenarioObject::ON_START); // tolua_export
	//void LoadAreas(); // tolua_export
	//void LoadBoundaries(); // tolua_export
	//ScenarioArea* FindArea(const char* name);
	//void LoadIncludes(); // tolua_export

	// primary content handlers
	bool SaveContent(const char* path);
	void LoadContent(); // tolua_export
	void UnloadContent(); // tolua_export

	// content creator functions
	bool CreateVehicleSTO(ScenarioObject* ob);
	bool CreateTerrainSTO(ScenarioObject* ob);
	bool CreatePlacedSTO(ScenarioObject* ob);
	bool CreateEmitter(ScenarioObject* ob);
	bool CreateSound(ScenarioObject* ob);
	bool CreateAmmo(const char* name, Vec3d& position, Vec3d& dir);
    bool CreateCoastSTO(ScenarioObject* ob);
    bool CreateRiverSTO(ScenarioObject* ob);
    bool CreateOceanSTO(ScenarioObject* ob);

	// process an event
	void RunEvent(const char* name); // tolua_export
	void RunEvent(ScenarioEvent* evt);

	// run any scenario dot commands for the specified tutoral event type
	void RunCommands(TUT_EVENT_TYPE type);

#ifdef BETA_RELEASE
	// are we in edit mode; provides some visual feedback in beta releases
	void AddEditPoint(float x, float y, float z);
	void ExportEditPoints();
	void SetEditMode(bool mode);
	void SetTerrain(const char* name);
	void SetContent(const char* name);
    bool GetEditMode() {return _editMode;}
#endif

	// add a sound to our list of sounds to monitor
	void AddSound(ScenarioSound* sound);

	// add content
	int AddObject(ScenarioObject* ob);

	// lua add content
	int AddContent(const char* line); // tolua_export

	// gets the master instance of the manager
	static ScenarioManager* GetInstance(); // tolua_export

private:

#ifdef BETA_RELEASE
	bool _editMode;
#endif

	// c++ layer scenario flags
	// used to control behavior below
	// the ui and lua layers
	UINT32 _flags;
    UINT32 _globalflag;

	UINT32 _round;
	UINT32 _stage;

	// offline data
	UINT32 _offlineTeam; // for dev
	UINT32 _offlineRound; // for dev
	UINT32 _offlineStage; // for dev

	SINT32 _soundUpdate;
	SINT32 _playerUpdate;
	SINT32 _lastUpdate;
	SINT32 _startTime;
    SINT32 _startTimeTerrainLoad;

	bool _updatePlayers;
	bool _objectsLoaded;
    bool _terrainReadyToLoad;
	bool _scenarioIsComplete;

	WEAPON_SPEC _respawnWeapon;

	// currently running scenario
	Scenario* _scenario;

	// our custom map (if we have one)
	TerrainSTO* _map;

	// map paths to search
	SCENPATH_LIST mapPaths;

	// all loaded objects
	SCENOBJ_LIST objects;

	// available vehicle spawns
	SCENWOB_LIST spawns;

	// list of events we're monitoring
	SCENEVT_LIST events;

	// list of sounds we're monitoring
	SCENSND_LIST sounds;

	// a list of named objects
	SCENOBJ_LIST named;

	// a list of areas we've added
	SCENOBJ_LIST areas;

	// a list of areas we've added
	SCENOBJ_LIST boundaries;

	// list of vehicles we can spawn
	SCENVEH_LIST vehicles;

	// list of objects that need updating
	SCENOBJ_LIST updaters;

	// list of players in our scenario
	SCENPLAYER_MAP players;

}; // tolua_export

