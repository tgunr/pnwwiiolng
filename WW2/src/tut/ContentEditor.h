#pragma once

#ifdef BETA_RELEASE
#include <map>

#include "lua/luaMain.h"
#include "chat/chatHandle.h"
#include "math/Vec3d.h"
#include "fp/fpPPO.h"

// ----------- defines

#define CONTENT_EDITOR ContentEditor::GetInstance()
#define CONTENT_EDITOR_SCRIPT_RPATH "lua\\content\\"
#define CONTENT_EDITOR_SCRIPT_PATH "DATA\\" CONTENT_EDITOR_SCRIPT_RPATH

// ----------- types
namespace phui
{
	class View;
};

/************************************************************************************************
	ContentEditor
*************************************************************************************************/
// tolua_begin
class ContentEditor
{
public:
// tolua_end
	ContentEditor() :
		_mouseDown(0),
		_mouseDrag(0),
		_ui(NULL),
		_cursor(1),
		_drawall(false),
		_currentPPO(NULL)
	{
	}

	~ContentEditor()
	{
	}

	// contain for script data
	struct Script
	{
		Script() : modified(false), loaded(false)
		{
		}

		virtual ~Script()
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
	struct Point
	{
		Point(double x, double y, double z) : info(0)
		{
			position.set(x, y, z);
		}

		virtual ~Point()
		{
		}

		// is the passed in position near me
		bool NearMe(double x, double y, double z);

		double info;
		Vec3d position;
	};

	using PointList = std::vector<ContentEditor::Point>;

	// describe a series of points; might be a closed loop
	struct Line
	{
		Line() : selected(-1), context(-1), heading(0), lastHop(-1), drone(false)
		{
		}

		Line(bool d, int h) : selected(-1), context(-1), heading(0), lastHop(h), drone(d)
		{
		}

		virtual ~Line()
		{
			Deselect();
		}

		void Draw(bool hilite);
		void Update();
		void Select();
		void Deselect();

		Point* Selected();

		bool drone;
		double heading;
		SINT32 lastHop;
		SINT32 selected; // left mouse button
		SINT32 context; // right mouse button
		PointList points;
	};

	using LineList = std::vector<ContentEditor::Line>;

	struct Plan
	{
		Plan() : modified(false), loaded(false), selected(-1)
		{
		}

		virtual ~Plan()
		{
		}

		bool Load(std::string& path);
		bool Save();
		bool Close(bool save = false);

		Line* Selected();

		SINT32 selected;
		bool loaded;
		bool modified;
		std::string filename;
		LineList lines;
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

	bool NewPlan();
	bool LoadPlan(std::string& path);
	bool SavePlan();

	// get defaults
	Script* GetScript(); // loaded script
	Point* GetPoint(int index = -1); // selected point if a resource is loaded
	Line* GetLine(int index = -1); // selected line if a resource is loaded
	Plan* GetPlan(); // loaded plan

	// route management
	int AddLine(int after = -1); // tolua_export
	void SelectLine(int index = -1);
	void RemoveLine(int index = -1);

	void SetLineColor(UINT32 c, int index = -1);
	UINT32 GetLineColor(int index = -1);

	// waypoint management
	int AddPoint(double x, double y, double z, int after = -1); // tolua_export
	int Add3DPoint(double x, double y, double z, int after = -1);
	void SelectPoint(int index = -1);
	void RemovePoint(int index = -1);
	void ContextPoint(int index = -1);

	//void SetWaypointFlags(UINT32 flags, int index = -1);
	void UpdatePoint(Vec3d pos, int index = -1);

	void SetHeading();
	void SetHeading(double heading); // tolua_export
	void SetLastHop(int index = -1); // tolua_export
	void SetPPO(const char* name = NULL);

	bool DrawAll(){ return _drawall; }
	void SetDrawAll(bool d){ _drawall = d; }

	int Context();
	bool MouseDown();
	void OnClick(int button, int state, double x = 0.0f, double y = 0.0f, double z = 0.0f);
	Vec3d& LastClick();

	// gets the master instance of the manager
	static ContentEditor* GetInstance(); // tolua_export

private:

	// script we're currently editing
	Script _script;

	// plan we're currently editing
	Plan _plan;

	// current placement object
	PPOSpec _ppoSpec;
	PlayerPlacedObject* _currentPPO;

	UINT32 _mouseDown; // is mouse down
	UINT32 _mouseDrag; // is mouse dragging a waypoint

	Vec3d _lastClick;

	phui::View* _ui;
	int _cursor;

	bool _drawall;

}; // tolua_export

#endif // BETA_RELEASE
