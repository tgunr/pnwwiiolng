#include <syscfg.h> // Path is project specific.
#include <stdio.h>	// var args
#include <stdarg.h>	// var args

#include "tafat/TafatEditor.h"
#include "tut/TutorialManager.h"
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
#include "uiChat.h"
#include "draw/draw3d.h"
#include "math/matheul.h"

// defines
#define TAFAT_SCRIPT_TEMPLATE "default.lua"

// private variables

// singleton
static TafatEditor _tafatEditor;

/************************************************************************************************
	TafatEditor
*************************************************************************************************/

void TafatEditor::TafatScript::SetScript(std::string& s)
{
	script = s;
	modified = true;
}

bool TafatEditor::TafatScript::Load(std::string& path)
{
	Close();

	std::string fullPath = TAFAT_SCRIPT_RPATH;
	fullPath += path;

	if(fioAccess(fullPath.c_str()))
	{
		int fd = fioOpen(fullPath.c_str());
		if(fd > -1)
		{
			size_t size = fioFileSize(fullPath.c_str());
			if(size > 0)
			{
				char* buffer = new char[size+1];
				buffer[size] = '\0';

				if(fioRead(fd, buffer, size) == size)
				{
					filename = path;
					script = buffer;
					loaded = true;
				}

				delete [] buffer;
			}

			fioClose(fd);
		}
	}

	return loaded;
}

bool TafatEditor::TafatScript::Save()
{
	bool saved = false;

	if(modified && !filename.empty())
	{
		std::string fullPath = TAFAT_SCRIPT_RPATH;
		fullPath += filename;

		int fd = fioOpenWrite(fullPath.c_str());
		if(fd > -1)
		{
			if(fioWrite(fd, script.c_str(), strlen(script.c_str())) > 0)
				saved = true;

			fioClose(fd);
		}

		modified = false;
	}

	return saved;
}

bool TafatEditor::TafatScript::Close(bool save)
{
	if(loaded)
	{
		if(save)
			Save();

		loaded = false;
		modified = false;
		filename = "";
		script = "";

		return true;
	}

	return false;
}

bool TafatEditor::TafatWaypoint::NearMe(double x, double y, double z)
{
	Vec3d hit = Vec3d(x, y, z);
	Vec3d diff = position - hit;

	return (diff.length() <= 3.0f) ? true: false;
}

TafatEditor::TafatWaypoint* TafatEditor::TafatRoute::Selected()
{
	if(selected >= 0 && selected < waypoints.size())
		return &waypoints[selected];

	return NULL;
}

void TafatEditor::TafatRoute::Draw(bool hilite)
{
	FLOAT64VECTOR worldPos;
	FLOAT64VECTOR woo;

	piVECTOR v1;
	piVECTOR v2;

	trnGetWorldOriginOffset(&woo);

	int wsize = (SINT32)waypoints.size();

	for(int i = 0; i < wsize; i++)
	{
		TafatEditor::TafatWaypoint& waypoint = waypoints.at(i);

		waypoint.position.get(worldPos);

		v1.x = (FLOAT32)(worldPos.x - woo.x);
		v1.y = (FLOAT32)(worldPos.y - woo.y);
		v1.z = (FLOAT32)(-worldPos.z - woo.z);

		// draw connection
		if(i + 1 < wsize)
		{
			TafatEditor::TafatWaypoint& waypoint2 = waypoints.at(i + 1);

			waypoint2.position.get(worldPos);

			v2.x = (FLOAT32)(worldPos.x - woo.x);
			v2.y = (FLOAT32)(worldPos.y - woo.y);
			v2.z = (FLOAT32)(-worldPos.z - woo.z);

			draw3dLine(&v1, &v2, (hilite) ? colorYELLOWALPHA: colorBLACKALPHA);
		}

		UINT32 color = (i == selected) ? colorMAGENTAALPHA: colorBLUEALPHA;

		if(!hilite) color = colorBLACKALPHA;

		// mark us
		draw3dDebugMarkPoint(&v1, color, .5f);

		// vertical marker
		v2 = v1;
		v2.z += (i == selected) ? -4.0f: -2.0f;

		draw3dLine(&v1, &v2, color);

		if(i == 0) // first waypoint
		{
			piVECTOR vecAtt;
			piMATRIX matAtt;

		    vecAtt.x = 0;
			vecAtt.y = 0;
			vecAtt.z = heading;

			mathEul_ToMatrix(&matAtt, EulOrdXYZs, &vecAtt);

			Matrixd rotate = Matrixd(matAtt);

			Vec3d attitude = Vec3d(0 ,1.0f, 0);
			attitude = rotate * attitude;

			v2.x = v1.x + -attitude.x();
			v2.y = v1.y + attitude.y();
			v2.z = v1.z = v1.z - 1.0f;

			draw3dLine(&v1, &v2, (hilite) ? colorCYANALPHA: colorBLACKALPHA);
		}
		else if(i == wsize - 1 && lastHop >= 0 && lastHop < wsize) // last waypoint
		{
			TafatEditor::TafatWaypoint& waypoint2 = waypoints.at(lastHop);

			waypoint2.position.get(worldPos);

			v2.x = (FLOAT32)(worldPos.x - woo.x);
			v2.y = (FLOAT32)(worldPos.y - woo.y);
			v2.z = (FLOAT32)(-worldPos.z - woo.z);

			v2.z -= .5f;
			v1.z -= .5f;

			draw3dLine(&v1, &v2, (hilite) ? colorORANGEALPHA: colorBLACKALPHA);
		}
	}
}

void TafatEditor::TafatRoute::Update()
{
	/*
	FLOAT64VECTOR v1;
	FLOAT64VECTOR v2;

	for(int i = 0; i < waypoints.size(); i++)
	{
		TafatEditor::TafatWaypoint& waypoint = waypoints.at(i);

		if(waypoint.drawID >= 0)
		{
			if(i + 1 < waypoints.size())
			{
				TafatEditor::TafatWaypoint& waypoint2 = waypoints.at(i + 1);

				waypoint.position.get(v1);
				waypoint2.position.get(v2);

				draw3dUpdateDebugObjects(waypoint.drawID, &v1, &v2, color);
			}
		}
	}
	*/
}

void TafatEditor::TafatRoute::Select()
{
	/*
	FLOAT64VECTOR v1;
	FLOAT64VECTOR v2;

	for(int i = 0; i < waypoints.size(); i++)
	{
		TafatEditor::TafatWaypoint& waypoint = waypoints.at(i);

		if(waypoint.drawID == -1)
		{
			if(i + 1 < waypoints.size())
			{
				TafatEditor::TafatWaypoint& waypoint2 = waypoints.at(i + 1);

				waypoint.position.get(v1);
				waypoint2.position.get(v2);

				char name[31+1];
				snprintf(name, 31, "WP%d", i);

				waypoint.drawID = draw3dSetWorldDebugLine(&v1, &v2, name, color);
				draw3dSetDebugObjectFlipZ(waypoint.drawID, TRUE);
			}
		}
	}
	*/
}

void TafatEditor::TafatRoute::Deselect()
{
	/*
	for(int i = 0; i < waypoints.size(); i++)
	{
		TafatEditor::TafatWaypoint& waypoint = waypoints.at(i);

		if(waypoint.drawID >= 0)
		{
			draw3dRemoveDebugObject(waypoint.drawID);
			waypoint.drawID = -1;
		}
	}
	*/
}

bool TafatEditor::TafatRoutePlan::Load(std::string& path)
{
	Close();

	loaded = false;

	std::string fullPath = TAFAT_ROUTE_RUN;
	fullPath += path;

	// start default vm
	luaGetVM();

	// load our default state
	luaRunScript("include/state.lua");

	// run the route script
	if(luaRunScript(fullPath.c_str()))
	{
		filename = path;
		loaded = true;

		// include the tafat editor script to load the routes
		luaRunScript("include/tafated.lua");
	}

	//	stop our lua vm
	luaStopVM();

	return loaded;
}

bool TafatEditor::TafatRoutePlan::Save()
{
	// write out lua
	if(!filename.empty()) // modified &&
	{
		bool saved = false;

		std::string fullPath = TAFAT_ROUTE_RPATH;
		fullPath += filename;

		int fd = fioOpenWrite(fullPath.c_str());
		if(fd > -1)
		{
			char line[255+1];

			snprintf(line, 255, "-- routes x,y,z,info\n");
			fioWrite(fd, line, strlen(line));

			snprintf(line, 255, "state.routes = \n");
			fioWrite(fd, line, strlen(line));

			snprintf(line, 255, "{\n");
			fioWrite(fd, line, strlen(line));

			for(int r = 0; r < routes.size(); r++)
			{
				TafatRoute& route = routes.at(r);

				snprintf(line, 255, "\t{\n");
				fioWrite(fd, line, strlen(line));

				const char* rcomma = (r < routes.size() - 1) ? ",": "";

				for(int w = 0; w < route.waypoints.size(); w++)
				{
					TafatWaypoint& waypoint = route.waypoints.at(w);

					const char* wcomma = (w < route.waypoints.size() - 1) ? ",": "";
					double x = waypoint.position.x();
					double y = waypoint.position.y();
					double z = waypoint.position.z();
					double info = 0.0f;

					if(w == 0)
					{
						info = route.heading;
					}
					else if(w == route.waypoints.size() - 1)
					{
						if(r == 0) // drone route
							info = 1.0f;
						else
							info = (double)route.lastHop + 1.0f;
					}

					snprintf(line, 255, "\t\t{%.0f, %.0f, %.0f, %.2f, 0}%s\n", x, y, z, info, wcomma);
					fioWrite(fd, line, strlen(line));
				}

				snprintf(line, 255, "\t}%s\n", rcomma);
				fioWrite(fd, line, strlen(line));
			}

			snprintf(line, 255, "}\n");
			fioWrite(fd, line, strlen(line));

			saved = true;

			fioClose(fd);
		}

		return saved;
	}

	return false;
}

bool TafatEditor::TafatRoutePlan::Close(bool save)
{
	if(loaded)
	{
		if(save)
			Save();

		loaded = false;
		modified = false;
		filename = "";
		routes.clear();

		return true;
	}

	return false;
}

TafatEditor::TafatRoute* TafatEditor::TafatRoutePlan::Selected()
{
	if(selected >= 0 && selected < routes.size())
		return &routes[selected];

	return NULL;
}

bool TafatEditor::Start()
{
	if(IsEditing())
		return false;

	if(!uiInGame())
	{
		uiChatPrintLine("You can only enter edit mode while in the game world.");
		return false;
	}

	_cursor = uiCursorState();

	uiSetCursor(1);

	_ui = uiLoadView("tafatedit");

	return true;
}

bool TafatEditor::Stop()
{
	if(IsEditing())
	{
		_ui->Close();
		_ui = NULL;
	}

	_script.Close();
	_routePlan.Close();

	uiSetCursor(_cursor);

	return true;
}

bool TafatEditor::Toggle()
{
	if(!TAFAT_EDITOR->IsEditing())
		return TAFAT_EDITOR->Start();
	else
		return TAFAT_EDITOR->Stop();
}

void TafatEditor::Draw()
{
	int rsize = _routePlan.routes.size();

	for(int i = 0; i < rsize; i++)
	{
		TafatRoute& route = _routePlan.routes.at(i);

		if(_drawall || i == _routePlan.selected)
			route.Draw(i == _routePlan.selected);
	}
}

void TafatEditor::Update()
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		if(_mouseDown == 1 && _mouseDrag == 1)
		{
			SINT32 mx = 0;
			SINT32 my = 0;

			sysGetMousePosition(&mx, &my);

			if(trncdCollideWithMouse(1, mx, my))
			{
				Vec3d pos = _lastClick;
				pos.z() += 1.0f;
				UpdateWaypointPosition(pos);
			}
			else
			{
				_mouseDrag = 0;
			}
		}

		route->Update();
	}
}

bool TafatEditor::IsEditing()
{
	return (_ui != NULL);
}

void TafatEditor::UpdateUI()
{
	uiRunScript(uiFindView("tafatedit"), "refreshEditor()");
}

bool TafatEditor::NewScript()
{
	std::string defaultPath = TAFAT_SCRIPT_TEMPLATE;

	// load default
	if(_script.Load(defaultPath))
	{
		_script.filename = "";
		_script.modified = false;
	}
	else
	{
		_script.filename = "";
		_script.loaded = true;
		_script.modified = false;
		_script.script = "";
	}

	return true;
}

bool TafatEditor::LoadScript(std::string& path)
{
	if(_script.Load(path))
	{
		//UpdateUI();
		return true;
	}

	return false;
}

bool TafatEditor::SaveScript()
{
	if(_script.Save())
	{
		//UpdateUI();
		return true;
	}

	return false;
}

bool TafatEditor::NewRoutePlan()
{
	_routePlan.Close(true);

	_routePlan.filename = "";
	_routePlan.loaded = true;
	_routePlan.modified = true;
	_routePlan.selected = -1;

	// add our initial drone route
	AddRoute();

	return true;
}

bool TafatEditor::LoadRoutePlan(std::string& path)
{
	if(_routePlan.Load(path))
	{
		//UpdateUI();
		return true;
	}

	return false;
}

bool TafatEditor::SaveRoutePlan()
{
	if(_routePlan.Save())
	{
		_routePlan.modified = false;
		//UpdateUI();
		return true;
	}

	return false;
}

TafatEditor::TafatScript* TafatEditor::GetScript()
{
	if(_script.loaded)
		return &_script;

	return NULL;
}

TafatEditor::TafatRoutePlan* TafatEditor::GetPlan()
{
	if(_routePlan.loaded)
		return &_routePlan;

	return NULL;
}

TafatEditor::TafatRoute* TafatEditor::GetRoute(int index)
{
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		if(index >= 0 && index < plan->routes.size())
		{
			return &plan->routes[index];
		}
		else
		{
			return plan->Selected();
		}
	}

	return NULL;
}

TafatEditor::TafatWaypoint* TafatEditor::GetWaypoint(int index)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		if(index >= 0 && index < route->waypoints.size())
		{
			return &route->waypoints[index];
		}
		else
		{
			return route->Selected();
		}
	}

	return NULL;
}

// route management
int TafatEditor::AddRoute(int after)
{
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		TafatRoute route = TafatRoute();
		int rcount = plan->routes.size();

		if(rcount > 0 && after >= 0 && after < rcount)
		{
			// after element
			plan->routes.insert(plan->routes.begin() + after, route);
			SelectRoute(after + 1);
		}
		else
		{
			// at the end

			// if this is the first route, its the drone route
			if(rcount == 0)
			{
				route.drone = true;
				route.lastHop = 0;
			}

			plan->routes.push_back(route);
			SelectRoute(plan->routes.size() - 1);
		}

		//UpdateUI();

		return plan->selected;
	}

	return -1;
}

void TafatEditor::SelectRoute(int index)
{
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		TafatRoute* current		= GetRoute();
		TafatRoute* newRoute	= GetRoute(index);

		if(current != NULL && current != newRoute)
			current->Deselect();

		if(index < 0)
			plan->selected = 0;
		else if(index >= plan->routes.size())
			plan->selected = plan->routes.size() - 1;
		else
			plan->selected = index;

		current = GetRoute();

		if(current)
			current->Select();
	}
}

void TafatEditor::RemoveRoute(int index)
{
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		if(index == -1)
			index = plan->selected;

		// cant remove the drone route
		if(index == 0)
			return;

		TafatEditor::TafatRouteList::iterator it = plan->routes.begin();

		for(int i = 0; it != plan->routes.end(); it++, i++)
		{
			if(i == index)
			{
				plan->routes.erase(it);
				//UpdateUI();
				break;
			}
		}

		if(!plan->routes.empty())
			SelectRoute(0);
		else
			SelectRoute(-1);
	}
}

void TafatEditor::SetDroneRoute(bool isDrone, int index)
{
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		if(index == -1)
			index = plan->selected;

		TafatEditor::TafatRouteList::iterator it = plan->routes.begin();

		for(int i = 0; it != plan->routes.end(); it++, i++)
		{
			if(i == index)
			{
				it->drone = isDrone;
				//UpdateUI();
				break;
			}
		}
	}
}

void TafatEditor::SetRouteColor(UINT32 c, int index)
{
	/*
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		if(index == -1)
			index = plan->selected;

		TafatEditor::TafatRouteList::iterator it = plan->routes.begin();

		for(int i = 0; it != plan->routes.end(); it++, i++)
		{
			if(i == index)
			{
				it->color = c;
				UpdateUI();
				break;
			}
		}
	}
	*/
}

UINT32 TafatEditor::GetRouteColor(int index)
{
	/*
	TafatRoutePlan* plan = GetPlan();

	if(plan)
	{
		if(index == -1)
			index = plan->selected;

		TafatEditor::TafatRouteList::iterator it = plan->routes.begin();

		for(int i = 0; it != plan->routes.end(); it++, i++)
		{
			if(i == index)
				return it->color;
		}
	}
	*/

	return 0;
}

// waypoint management
int TafatEditor::AddWaypoint(double x, double y, double z, int after)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		TafatWaypoint waypoint = TafatWaypoint(x,y,z);

		if(route->waypoints.size() > 0 && after >= 0 && after < route->waypoints.size())
		{
			// after element
			route->waypoints.insert(route->waypoints.begin() + after + 1, waypoint);
			SelectWaypoint(after + 1);
		}
		else
		{
			// at the end
			route->waypoints.push_back(waypoint);
			SelectWaypoint(route->waypoints.size() - 1);
		}

		//UpdateUI();

		return route->selected;
	}

	return -1;
}

// waypoint management
int TafatEditor::Add3DWaypoint(double x, double y, double z, int after)
{
	// bail if this is not the drone route
	if(_routePlan.selected != 0)
		return -1;

	return AddWaypoint(x, y, z, after);
}

void TafatEditor::SelectWaypoint(int index)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		if(index < 0)
			route->selected = 0;
		else if(index >= route->waypoints.size())
			route->selected = route->waypoints.size() - 1;
		else
			route->selected = index;
	}
}

void TafatEditor::RemoveWaypoint(int index)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		if(index == -1)
			index = route->selected;

		TafatEditor::TafatWaypointList::iterator it = route->waypoints.begin();

		for(int i = 0; it != route->waypoints.end(); it++, i++)
		{
			if(i == index)
			{
				//if(it->drawID >= 0)
				//	draw3dRemoveDebugObject(it->drawID);

				route->waypoints.erase(it);
				//UpdateUI();
				break;
			}
		}

		if(index <= route->lastHop)
			route->lastHop--; // move the lasthop down one because we deleted a waypoint below it

		if(!route->waypoints.empty())
			SelectWaypoint(route->selected - 1);
		else
			SelectWaypoint(-1);
	}
}

void TafatEditor::ContextWaypoint(int index)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		if(index >= 0 && index < route->waypoints.size())
			route->context = index;
		else
			route->context = -1;
	}
}

void TafatEditor::UpdateWaypointPosition(Vec3d pos, int index)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		if(index == -1)
			index = route->selected;

		TafatEditor::TafatWaypointList::iterator it = route->waypoints.begin();

		for(int i = 0; it != route->waypoints.end(); it++, i++)
		{
			if(i == index)
			{
				it->position = pos;
				_routePlan.modified = true;
				break;
			}
		}
	}
}

void TafatEditor::SetHeading()
{
	TafatRoute* route = GetRoute();

	if(route && !route->waypoints.empty())
	{
		// look up last click
		Vec3d dir = _lastClick - route->waypoints.at(0).position;

		dir.normalize();

		route->heading = atan2(dir.x(), dir.y());
	}
}

void TafatEditor::SetHeading(double heading)
{
	TafatRoute* route = GetRoute();
	if(route) route->heading = heading;
}

void TafatEditor::SetLastHop(int index)
{
	TafatRoute* route = GetRoute();

	if(route)
	{
		route->lastHop = -1;

		int wsize = route->waypoints.size();

		if(wsize > 1 && index >= 0 && index < wsize - 1)
			route->lastHop = index;
	}
}

void TafatEditor::OnClick(int button, int state, double x, double y, double z)
{
	if(state == 1)
	{
		bool contextual = (TAFAT_EDITOR->Context() >= 0);

		if(contextual)
			return;

		// clear our context
		ContextWaypoint(-1);

		_mouseDown = button;

		if(_mouseDown == 2 || _mouseDrag)
			_lastClick.set(x, y, z);

		// see if we're close to any waypoints
		TafatRoute* route = GetRoute();

		if(route)
		{
			TafatEditor::TafatWaypointList::iterator it = route->waypoints.begin();

			for(int i = 0; it != route->waypoints.end(); it++, i++)
			{
				TafatEditor::TafatWaypoint& waypoint = *it;

				if(waypoint.NearMe(x, y, z))
				{
					if(_mouseDown == 1)
					{
						if(!_mouseDrag)
							SelectWaypoint(i);

						_mouseDrag = _mouseDown;
					}
					else if(_mouseDown == 2)
					{
						ContextWaypoint(i);
					}

					break;
				}
			}
		}
	}
	else
	{
		_mouseDown = 0;
		_mouseDrag = 0;
	}
}

bool TafatEditor::MouseDown()
{
	return (_mouseDown > 0);
}

int TafatEditor::Context()
{
	TafatRoute* route = GetRoute();

	if(route && route->context >= 0 && route->context < route->waypoints.size())
	{
		phui::View* menu = uiFindView("tafat_rcmenu");

		if(menu != NULL)
			return route->context;
	}

	return -1;
}

Vec3d& TafatEditor::LastClick()
{
	return _lastClick;
}

TafatEditor* TafatEditor::GetInstance()
{
	return &_tafatEditor;
}
