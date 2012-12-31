#pragma once

#include <map>
#include "phui/View.h"

#define HUD_MGR PersHUDManager::GetInstance()

class Waypoint;
class ServerTrackedObject;
namespace StratDB { class StratObject ; }

/************************************************************************************************
	persHUD
*************************************************************************************************/
// tolua_begin
enum
{
	HUD_CYCLE_OFF = 0,	// no hud
	HUD_CYCLE_RESET,	// reset hud; old default hud
	HUD_CYCLE_ON,		// show hud; old custom hud
	// add new hud cycle settings here
	HUD_CYCLE_INVALID	// invalid hud cycle setting
};

extern UINT32 persHUDToggleHUD(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero);
extern UINT32 persHUDToggleGameInfo(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero);
extern UINT32 persHUDToggleReadout(SINT32 keyState, UINT32 functionNum, UINT32 alwaysZero);
// tolua_end

extern LOGICAL persHudReceivedUpdate(void);

// tolua_begin
// used by the tutorial system to control hud behavior
extern void persHUDLock(bool l);
extern UINT32 persHUDResetHUD(SINT32 param, UINT32 functionNum, UINT32 alwaysZero);
extern void persHUDShowHUD();
extern void persHUDHideHUD(bool close=false);
// tolua_end

//void persHUDSyncToCycleSetting();

//void persHUDShowReadout();
//void persHUDHideReadout(bool close=false);

//void persHUDShowGameInfo();
//void persHUDHideGameInfo(bool close=false);

class TargetView;

class persHUDTextView : public phui::View
{
public:

	persHUDTextView() : language(-1)
	{
	}

	virtual ~persHUDTextView()
	{
	}

	virtual void DrawSelf();
	void DrawString(const wchar_t*, ...);
	void DrawString(const char*, ...);
	void ResetTextPos();
	virtual void ResizeToText();
	virtual void InitCache();

protected:

	float _textposy, _textposx, _lineheight;
	float _maxx,_miny;

	int language;
	typedef std::map<std::string,std::wstring> StringCache;
	StringCache strings;
};

class PersHUDManager
{
public:

	enum HUD_OBJECT_TYPE
	{
		INVALID,
		DEATH,
		WAYPOINT
	};

	struct HUDObject
	{
		HUDObject(HUD_OBJECT_TYPE t = INVALID, int e = 0);

		virtual ~HUDObject();
		virtual bool Update();
		virtual bool IsVisible();

		int type;
		int expires;
		FLOAT64VECTOR pos;
	};

	struct HUDDeath : public HUDObject
	{
		HUDDeath(ChatHandle& h, FLOAT64VECTOR& p);

		FLOAT32 GetAlpha();
		FLOAT32 GetPlacement(FLOAT64VECTOR& p);

		UINT32 color;
		ChatHandle handle;
	};

	struct HUDWaypoint : public HUDObject
	{
		HUDWaypoint(FLOAT32 o = 1.5f);
		HUDWaypoint(FLOAT64VECTOR& p, const char* f, FLOAT32 o = 1.5f);
		HUDWaypoint(const Waypoint& w, const char* f, FLOAT32 o = 1.5f);
		HUDWaypoint(const StratDB::StratObject* strat, const char* f, FLOAT32 o = 1.5f, bool centerOnChildren = true);
		HUDWaypoint(const ServerTrackedObject* sto, const char* f, FLOAT32 o = 1.5f);

		virtual ~HUDWaypoint();
		virtual bool Update();
		virtual bool CreateUI(const char* file);
		FLOAT64 FindZ();

		bool useTail;
		FLOAT32 minDistance;
		FLOAT32 voffset;
		const char* file;
		TargetView* ui;
	};

	struct HUDPlayerWaypoint : public HUDWaypoint
	{
		HUDPlayerWaypoint(const struct vehVehicle* p, const char* f);

		virtual ~HUDPlayerWaypoint(){}
		virtual bool Update();

		const struct vehVehicle* player;
	};

	struct HUDLeaderWaypoint : public HUDPlayerWaypoint
	{
		HUDLeaderWaypoint(const struct vehVehicle* p);

		virtual ~HUDLeaderWaypoint(){}
		virtual bool Update();
	};

	typedef std::vector<HUDObject*> HUDObjectList;

	PersHUDManager()
	{
	}

	virtual ~PersHUDManager()
	{
	}

	void AddObject(HUDObject* o);
	void Update();
	void Reset();
	bool IsVisible(); // are our hud objects visible?

	HUDObjectList& GetObjects();

	// get our singleton
	static PersHUDManager* GetInstance();

protected:

	HUDObjectList objects;
};
