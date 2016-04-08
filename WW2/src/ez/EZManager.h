#pragma once

#include "wwii/wwiiHash.h"

#include <map>
#include "veh/vehMain.h"

// ----------- defines
#define EZ_MGR EZManager::GetInstance()

#define SCREEN_POS_COUNT 5


// ----------- predeclares

// ----------- types

/************************************************************************************************
	EZManager
*************************************************************************************************/

// tolua_begin
class EZManager
{
public:

	enum
	{
		INITED				= 1<<0,
		FREELOOK			= 1<<1,
		START_ENGINES		= 1<<2,
		ENGINES_ON			= 1<<3,
		IN_REVERSE			= 1<<4,
		TOGGLING_GEARBOX	= 1<<5,
		RETICLE				= 1<<6,
		VIEW_OVERRIDE		= 1<<7,
		EZ_ENGINE			= 1<<8
	};

	enum
	{
		VIEW_EXTERNAL = -1,
		VIEW_NORMAL = 0,
		VIEW_INSTRUMENTS,
		VIEW_GUNSIGHT
	};

	enum EZ_ACCEL
	{
		EZ_ACCEL_FORWARD = 1,
		EZ_ACCEL_IDLE = 0,
		EZ_ACCEL_BACKWARD = -1
	};

	enum EZ_TURN
	{
		EZ_TURN_LEFT = -1,
		EZ_TURN_NONE = 0,
		EZ_TURN_RIGHT = 1
	};

	enum EZ_MOVE
	{
		EZ_MOVE_BACKWARD = -1,
		EZ_MOVE_IDLE = 0,
		EZ_MOVE_FORWARD = 1
	};

	enum EZ_ACTION
	{
		EZ_ACTION_NONE = 0,
		EZ_ACTION_FIRE1,
		EZ_ACTION_FIRE2,
		EZ_ACTION_VIEW,
		EZ_ACTION_DEPLOY,
		EZ_ACTION_HATCH
	};

	struct EZCamera
	{
		struct EZAxis
		{
			EZAxis() :
				angle(0),
				move(0),
				rate(1.0),
				//time(0),
				mark(0),
				minAngle(-180.0f),
				maxAngle(180.0f)
			{
			}

			void Clamp();

			FLOAT64 angle;
			FLOAT64 move;
			FLOAT64 rate;
			//FLOAT64 time;
			FLOAT64 mark;
			FLOAT64 minAngle;
			FLOAT64 maxAngle;
		};

		EZCamera() :
			focus(200.0f),
			pov(false)
		{
		}

		void Update(FLOAT64 deltaTime);

		// easy camera ops
		void Mark();
		void Snap();

		EZAxis traverse;
		EZAxis elevation;

		FLOAT64 focus;

		// camera position offset
		Vec3d offset;

		// is this a point of view camera
		bool pov;
	};

	struct EZTurret
	{
		enum TURRET_STATUS
		{
			TURRET_STATUS_INLINE, // synced with camera
			TURRET_STATUS_FOCAL, // tracking to camera but close
			TURRET_STATUS_TRACKING // tracking to camera
		};

		EZTurret() :
			tracking(false),
            position(0),
			mount(nullptr),
			vehicle(nullptr),
			screenPosIndex(0)
		{
			for(int i = 0; i < SCREEN_POS_COUNT; i++)
			{
				screenXPos[i] = 0;
				screenYPos[i] = 0;
			}
		}

		void Init(vehVehicle* v);
		bool FindMount();
		void Load();
		void Unload();
		void PreUpdate();
		void Update();
		void PostUpdate();
		void UpdateReticle();
		void TrackToCamera();
		TURRET_STATUS GetStatus();

		bool tracking;
		int position;
		Vec3d turretPos;
		Vec3d screenPos;
		Vec3d lastScreenPos;
		Vec3d camPos;
		Vec3d lastCamPos;
		Vec3d aimPos;
		Vec3d lastAimPos;
		Vec3d attitude;
		Vec3d lastAttitude;
		mlWeaponMount* mount;
		vehVehicle* vehicle;

		SINT32 screenPosIndex;
		SINT32 screenXPos[SCREEN_POS_COUNT];
		SINT32 screenYPos[SCREEN_POS_COUNT];
	};

	typedef std::vector<EZTurret> TURRET_LIST;
	typedef std::pair<SINT32,std::string> CONTROL_PAIR;
	typedef std::vector<CONTROL_PAIR> CONTROL_LIST;
	typedef std::pair<std::string,std::string> CONFIG_PAIR;
	typedef std::vector<CONFIG_PAIR> CONFIG_LIST;

	struct EZView
	{
		typedef std::unordered_map<SINT32,SINT32> INPUT_MAP;

		enum EVENT
		{
			EVENT_NONE = 0,
			EVENT_ONSPAWN, // when we spawn
			EVENT_ONLOAD // everytime this view is loaded
		};

		EZView() :
			view(0),
			zoom(0),
			deploy(EVENT_NONE),
			hatch(EVENT_NONE)
		{
		}

		bool Load();
		bool Unload();

		int position;
		int view;
		int zoom;
		int deploy;
		int hatch;

		TURRET_LIST turrets;
		INPUT_MAP blocks;
		INPUT_MAP actions;

		EZCamera camera;
	};

	typedef std::vector<EZView> VIEW_LIST;

	struct EZPosition
	{
		EZPosition() : view(0)
		{
		}

		bool Load();
		bool Unload();
		bool InputBlock(SINT32 id);

		bool IsDead();
		bool IsCommandView();

		void NextView();
		EZView& GetView();

		int view; // index into our view list

		std::string name;
		std::string reticle;

		VIEW_LIST views;
	};

	typedef std::vector<EZPosition> POSITION_LIST;

	EZManager(vehVehicle* v);
	virtual ~EZManager();

	// load any resources
	virtual bool Init(const char* path);

	// load our manager's callbacks
	virtual void InstallInputCallbacks();

	// handle config values
	virtual void SetConfig(const char* name, const char* value);
	virtual bool SetPosition(SINT32 p, bool force = false);
	virtual bool InputBlock(SINT32 id);
	virtual bool EasyControl(UINT32 position, const char* control);

	// add a new ez position
	void AddPosition(EZPosition& position);
	bool IsPositionDead(SINT32 position);
	SINT32 ConstrainPosition(SINT32 position);

	void SetReticle(const char* r);
	void SetTurret(SINT32 t);

	bool IsCommandView(bool deathCheck = false);

	// frame update
	virtual void FrameUpdate();
	virtual void TicUpdate(FLOAT64 DeltaTime);
	virtual void UpdateTurrets();

	// turret hooks for tracking
	virtual void PreUpdateTurret(SINT32 p);
	virtual void PostUpdateTurret(SINT32 p);

	// set or remove a game flag; returns true if a change was made
	bool SetFlag(SINT32 flag, bool on = true); // tolua_export
	// is this flag on?
	bool HasFlag(SINT32 flag); // tolua_export

	EZManager::EZPosition& GetPosition(SINT32 p = -1);
	EZManager::EZView& GetView();
	EZManager::EZCamera& GetCamera();
	SINT32 GetHatchTime();

	void AdjustCamera(Vec3d& camPos, Vec3d& aimPos);

	// input callbacks
	virtual void OnMove(SINT32 param, UINT32 id, UINT32 extra) {}
	virtual void OnTurn(SINT32 param, UINT32 id, UINT32 extra) {}
	virtual void OnAction1(SINT32 param, UINT32 id, UINT32 extra) {}
	virtual void OnAction2(SINT32 param, UINT32 id, UINT32 extra) {}
	virtual void OnFreeLook(SINT32 param, UINT32 id,UINT32 extra);
	virtual void OnPosition(SINT32 param, UINT32 id,UINT32 extra);
	virtual void OnEngine(SINT32 param, UINT32 id, UINT32 extra) {}
	virtual void OnView(SINT32 param, UINT32 id,UINT32 extra);
	virtual void OnCamera(SINT32 param, UINT32 id,UINT32 extra);
	virtual bool OnCamera(piVECTOR* pos, piVECTOR* aim);
	virtual void OnSpawn();
	virtual void OnDespawn(){}
	virtual SINT32 GetCategory();

	virtual bool IsDriverDead();
	virtual bool IsCommanderDead();
	virtual bool IsGunnerDead();
	virtual bool IsLoaderDead();

	// position; negative one means use
	// what the current position is
	SINT32 GetGamePosition();
	void SetGamePosition(SINT32 p = -1);

	// gets the master instance of the manager
	static EZManager* GetInstance(); // tolua_export

	static bool CreateManager(vehVehicle* v); // tolua_export
	static void FreeManager(); // tolua_export

protected:

	vehVehicle* _vehicle;

	// c++ layer scenario flags
	// used to control behavior below
	// the ui and lua layers
	UINT32 _flags;

	// last frame update
	SINT32 _lastUpdate;

	// our imap category
	SINT32 _category;

	// our camera reticle
	std::string _reticle;

	// turret info
	SINT32 _mainTurret;
	Vec3d _turretScreenPos;
	Vec3d _turretAttitude;

	SINT32 _hatchTime;

	// last facing
	Vec3d _lastDir;

	// our current view direction
	// only used when the flag VIEW_OVERRIDE
	// is in effect
	FLOAT64 _view;

	// our current view position
	SINT32 _position;

	// this is the position that is repoted to the vehicle physics
	// system so that various functions controlled by the EZManager
	// can perform as expected; quite a few calls in mlMain and other
	// files look up the current persona position but the EZManager
	// may actually be in a different position from the needed position
	// for any particular function. For example, we may be in a gun view,
	// but need to drive forward, we need the _positionOverride to report
	// we are actually in the driver's seat so that driving calls can
	// work as expected, etc...
	SINT32 _gamePosition;

	// direction we are moving, back or forward
	SINT32 _move;

	// which direction we're turning
	SINT32 _turn;

	// used to adjust the camera so that
	// obstructions don't get in the way of seeing
	// while in external view
	colliderQuery _collider;

	// our list of available positions
	POSITION_LIST _positions;

	// allowed vehicle controls for this ez manager and level
	CONTROL_LIST controls;

}; // tolua_export


// public interface functions

// third person camera callback
bool ezCameraCallback(piVECTOR* pos, piVECTOR* aim);

// ez input callback
UINT32 ezInputControls(SINT32 param, UINT32 id, UINT32 extra);
