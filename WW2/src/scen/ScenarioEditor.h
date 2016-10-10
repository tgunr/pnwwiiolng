#pragma once

#include <map>

#include "lua/luaMain.h"
#include "chat/chatHandle.h"
#include "math/Vec3d.h"

// ----------- defines

#define TAFAT_EDITOR TafatEditor::GetInstance()
#define TAFAT_SCRIPT_RPATH "lua\\tutorials\\tafat\\"
#define TAFAT_SCRIPT_PATH "DATA\\" TAFAT_SCRIPT_RPATH
#define TAFAT_ROUTE_RPATH "lua\\tutorials\\tafat\\routes\\"
#define TAFAT_ROUTE_RUN "tutorials\\tafat\\routes\\"
#define TAFAT_ROUTE_PATH "DATA\\" TAFAT_ROUTE_RPATH

// ----------- types

/************************************************************************************************
	TafatEditor
*************************************************************************************************/
// tolua_begin
class TafatEditor
{
public:
// tolua_end
	TafatEditor() : _mouseDown(0), _mouseDrag(0), _ui(NULL), _cursor(1), _drawall(false)
	{
	}

	~TafatEditor()
	{
	}

	// contain for script data
	struct TafatScript
	{
		TafatScript() : modified(false), loaded(false)
		{
		}

		virtual ~TafatScript()
		{
		}

		void SetScript(std::string& s);
		bool Load(std::string& path);
		bool Save();
		bool Close(bool save = false);

		bool loaded;
		bool modified;
		std::string filename;
		std::string script;
	};

	// defines a specific waypoint
	struct TafatWaypoint
	{
		TafatWaypoint(double x, double y, double z) : info(0)
		{
			position.set(x, y, z);
		}

		virtual ~TafatWaypoint()
		{
		}

		// is the passed in position near me
		bool NearMe(double x, double y, double z);

		double info;
		Vec3d position;
	};

	using TafatWaypointList = std::vector<TafatEditor::TafatWaypoint>;

	struct TafatRoute
	{
		TafatRoute() : selected(-1), context(-1), heading(0), lastHop(-1), drone(false)
		{
		}

		TafatRoute(bool d, int h) : selected(-1), context(-1), heading(0), lastHop(h), drone(d)
		{
		}

		virtual ~TafatRoute()
		{
			Deselect();
		}

		void Draw(bool hilite);
		void Update();
		void Select();
		void Deselect();

		TafatWaypoint* Selected();

		bool drone;
		double heading;
		SINT32 lastHop;
		SINT32 selected; // left mouse button
		SINT32 context; // right mouse button
		TafatWaypointList waypoints;
	};

	using TafatRouteList = std::vector<TafatEditor::TafatRoute>;

	struct TafatRoutePlan
	{
		TafatRoutePlan() : modified(false), loaded(false), selected(-1)
		{
		}

		virtual ~TafatRoutePlan()
		{
		}

		bool Load(std::string& path);
		bool Save();
		bool Close(bool save = false);

		TafatRoute* Selected();

		SINT32 selected;
		bool loaded;
		bool modified;
		std::string filename;
		TafatRouteList routes;
	};

	bool Start();
	bool Stop();
	bool Toggle();

	void Draw();
	void Update();
	bool IsEditing();
	void UpdateUI();

	bool NewScript();
	bool LoadScript(std::string& path);
	bool SaveScript();

	bool NewRoutePlan();
	bool LoadRoutePlan(std::string& path);
	bool SaveRoutePlan();

	// get defaults
	TafatScript* GetScript(); // loaded script
	TafatRoutePlan* GetPlan(); // loaded plan
	TafatRoute* GetRoute(int index = -1); // selected route if a plan is loaded
	TafatWaypoint* GetWaypoint(int index = -1); // selected waypoint if a plan is loaded

	// route management
	int AddRoute(int after = -1); // tolua_export
	void SelectRoute(int index = -1);
	void RemoveRoute(int index = -1);
	void SetDroneRoute(bool isDrone, int index = -1);

	void SetRouteColor(UINT32 c, int index = -1);
	UINT32 GetRouteColor(int index = -1);

	// waypoint management
	int AddWaypoint(double x, double y, double z, int after = -1); // tolua_export
	int Add3DWaypoint(double x, double y, double z, int after = -1);
	void SelectWaypoint(int index = -1);
	void RemoveWaypoint(int index = -1);
	void ContextWaypoint(int index = -1);

	//void SetWaypointFlags(UINT32 flags, int index = -1);
	void UpdateWaypointPosition(Vec3d pos, int index = -1);

	void SetHeading();
	void SetHeading(double heading); // tolua_export
	void SetLastHop(int index = -1); // tolua_export

	bool DrawAll(){ return _drawall; }
	void SetDrawAll(bool d){ _drawall = d; }

	int Context();
	bool MouseDown();
	void OnClick(int button, int state, double x = 0.0f, double y = 0.0f, double z = 0.0f);
	Vec3d& LastClick();

	// gets the master instance of the manager
	static TafatEditor* GetInstance(); // tolua_export

private:

	// script we're currently editing
	TafatScript _script;

	// route plan we're currently editing
	TafatRoutePlan _routePlan;

	UINT32 _mouseDown; // is mouse down
	UINT32 _mouseDrag; // is mouse dragging a waypoint

	Vec3d _lastClick;

	phui::View* _ui;
	int _cursor;

	bool _drawall;

}; // tolua_export

