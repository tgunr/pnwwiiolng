#include <syscfg.h> // Path is project specific.

#ifdef BETA_RELEASE
#include <stdio.h>	// var args
#include <stdarg.h>	// var args

#include "tut/ContentEditor.h"
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
#include "math/Matrixd.h"
#include "trn/trncd.h"
#include "sys/sysmain.h"
#include "fio/fio.h"

// defines
#define CONTENT_EDITOR_SCRIPT "content.lua"

// private variables

// singleton
static ContentEditor _contentEditor;

/************************************************************************************************
	ContentEditor
*************************************************************************************************/

void ContentEditor::Script::SetScript(std::string& s)
{
	script = s;
	modified = true;
}

bool ContentEditor::Script::Load(std::string& path)
{
	Close();

	std::string fullPath = CONTENT_EDITOR_SCRIPT_RPATH;
	fullPath += path;

    int fd = fioOpen(fullPath.c_str());
    if(fd > -1)
    {
        size_t size = fioFileSize(fullPath.c_str());
        if(size > 0)
        {
            std::unique_ptr<char[]> buffer = std::make_unqiue<char[]>(size+1);
            buffer[size] = '\0';

            if(fioRead(fd, buffer.get(), size) == size)
            {
                filename = path;
                script = buffer.get();
                loaded = true;
            }
        }

        fioClose(fd);
    }

	return loaded;
}

bool ContentEditor::Script::Save()
{
	bool saved = false;

	if(modified && !filename.empty())
	{
		std::string fullPath = CONTENT_EDITOR_SCRIPT_RPATH;
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

bool ContentEditor::Script::Close(bool save)
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

bool ContentEditor::Point::NearMe(double x, double y, double z)
{
	Vec3d hit = Vec3d(x, y, z);
	Vec3d diff = position - hit;

	return (diff.length() <= 3.0f) ? true: false;
}

ContentEditor::Point* ContentEditor::Line::Selected()
{
	if(selected >= 0 && selected < points.size())
		return &points[selected];

	return NULL;
}

void ContentEditor::Line::Draw(bool hilite)
{
	FLOAT64VECTOR worldPos;
	FLOAT64VECTOR woo;

	piVECTOR v1;
	piVECTOR v2;

	trnGetWorldOriginOffset(&woo);

	int wsize = (SINT32)points.size();

	for(int i = 0; i < wsize; i++)
	{
		ContentEditor::Point& point = points.at(i);

		point.position.get(worldPos);

		v1.x = (FLOAT32)(worldPos.x - woo.x);
		v1.y = (FLOAT32)(worldPos.y - woo.y);
		v1.z = (FLOAT32)(-worldPos.z - woo.z);

		// draw connection
		if(i + 1 < wsize)
		{
			ContentEditor::Point& point2 = points.at(i + 1);

			point2.position.get(worldPos);

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
			ContentEditor::Point& point2 = points.at(lastHop);

			point2.position.get(worldPos);

			v2.x = (FLOAT32)(worldPos.x - woo.x);
			v2.y = (FLOAT32)(worldPos.y - woo.y);
			v2.z = (FLOAT32)(-worldPos.z - woo.z);

			v2.z -= .5f;
			v1.z -= .5f;

			draw3dLine(&v1, &v2, (hilite) ? colorORANGEALPHA: colorBLACKALPHA);
		}
	}
}

void ContentEditor::Line::Update()
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

void ContentEditor::Line::Select()
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

void ContentEditor::Line::Deselect()
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

bool ContentEditor::Plan::Load(std::string& path)
{
	Close();

	loaded = false;

	std::string fullPath = "";//CONTENT_EDITOR_ROUTE_RUN;
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
		luaRunScript("include/content.lua");
	}

	//	stop our lua vm
	luaStopVM();

	return loaded;
}

bool ContentEditor::Plan::Save()
{
	// write out lua
	if(!filename.empty()) // modified &&
	{
		bool saved = false;

		std::string fullPath = "";//CONTENT_EDITOR_ROUTE_RPATH;
		fullPath += filename;

		int fd = fioOpenWrite(fullPath.c_str());
		if(fd > -1)
		{
			char buffer[255+1];

			snprintf(buffer, 255, "-- routes x,y,z,info\n");
			fioWrite(fd, buffer, strlen(buffer));

			snprintf(buffer, 255, "state.routes = \n");
			fioWrite(fd, buffer, strlen(buffer));

			snprintf(buffer, 255, "{\n");
			fioWrite(fd, buffer, strlen(buffer));

			for(int r = 0; r < lines.size(); r++)
			{
				Line& line = lines.at(r);

				snprintf(buffer, 255, "\t{\n");
				fioWrite(fd, buffer, strlen(buffer));

				const char* rcomma = (r < lines.size() - 1) ? ",": "";

				for(int w = 0; w < line.points.size(); w++)
				{
					Point& point = line.points.at(w);

					const char* wcomma = (w < line.points.size() - 1) ? ",": "";
					double x = point.position.x();
					double y = point.position.y();
					double z = point.position.z();
					double info = 0.0f;

					if(w == 0)
					{
						info = line.heading;
					}
					else if(w == line.points.size() - 1)
					{
						if(r == 0) // drone route
							info = 1.0f;
						else
							info = (double)line.lastHop + 1.0f;
					}

					snprintf(buffer, 255, "\t\t{%.0f, %.0f, %.0f, %.2f, 0}%s\n", x, y, z, info, wcomma);
					fioWrite(fd, buffer, strlen(buffer));
				}

				snprintf(buffer, 255, "\t}%s\n", rcomma);
				fioWrite(fd, buffer, strlen(buffer));
			}

			snprintf(buffer, 255, "}\n");
			fioWrite(fd, buffer, strlen(buffer));

			saved = true;

			fioClose(fd);
		}

		return saved;
	}

	return false;
}

bool ContentEditor::Plan::Close(bool save)
{
	if(loaded)
	{
		if(save)
			Save();

		loaded = false;
		modified = false;
		filename = "";
		lines.clear();

		return true;
	}

	return false;
}

ContentEditor::Line* ContentEditor::Plan::Selected()
{
	if(selected >= 0 && selected < lines.size())
		return &lines[selected];

	return NULL;
}

bool ContentEditor::Start()
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

	_ui = uiLoadView("contentedit");

	return true;
}

bool ContentEditor::Stop()
{
	if(IsEditing())
	{
		_ui->Close();
		_ui = NULL;
	}

	_script.Close();
	_plan.Close();

	uiSetCursor(_cursor);

	return true;
}

bool ContentEditor::Toggle()
{
	if(!CONTENT_EDITOR->IsEditing())
		return CONTENT_EDITOR->Start();
	else
		return CONTENT_EDITOR->Stop();
}

void ContentEditor::Draw()
{
	int rsize = _plan.lines.size();

	for(int i = 0; i < rsize; i++)
	{
		Line& line = _plan.lines.at(i);

		if(_drawall || i == _plan.selected)
			line.Draw(i == _plan.selected);
	}
}

void ContentEditor::Update()
{
	Line* line = GetLine();

	if(line)
	{
		if(_mouseDown == 1 && _mouseDrag == 1)
		{
			SINT32 mx = 0;
			SINT32 my = 0;

			sysGetMousePosition(&mx, &my);

			if(trncdCollideWithMouse(mx, my))
			{
				Vec3d pos = _lastClick;
				pos.z() += 1.0f;
				UpdatePoint(pos);
			}
			else
			{
				_mouseDrag = 0;
			}
		}

		line->Update();
	}
}

bool ContentEditor::IsEditing()
{
	return (_ui != NULL);
}

void ContentEditor::UpdateUI()
{
	uiRunScript(uiFindView("contentedit"), "refreshEditor()");
}

bool ContentEditor::NewScript()
{
	std::string defaultPath = "";//CONTENT_SCRIPT_TEMPLATE;

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

bool ContentEditor::LoadScript(std::string& path)
{
	if(_script.Load(path))
	{
		//UpdateUI();
		return true;
	}

	return false;
}

bool ContentEditor::SaveScript()
{
	if(_script.Save())
	{
		//UpdateUI();
		return true;
	}

	return false;
}

bool ContentEditor::NewPlan()
{
	_plan.Close(true);

	_plan.filename = "";
	_plan.loaded = true;
	_plan.modified = true;
	_plan.selected = -1;

	// add our initial drone route
	AddLine();

	return true;
}

bool ContentEditor::LoadPlan(std::string& path)
{
	if(_plan.Load(path))
	{
		//UpdateUI();
		return true;
	}

	return false;
}

bool ContentEditor::SavePlan()
{
	if(_plan.Save())
	{
		_plan.modified = false;
		//UpdateUI();
		return true;
	}

	return false;
}

ContentEditor::Script* ContentEditor::GetScript()
{
	if(_script.loaded)
		return &_script;

	return NULL;
}

ContentEditor::Plan* ContentEditor::GetPlan()
{
	if(_plan.loaded)
		return &_plan;

	return NULL;
}

ContentEditor::Line* ContentEditor::GetLine(int index)
{
	Plan* plan = GetPlan();

	if(plan)
	{
		if(index >= 0 && index < plan->lines.size())
		{
			return &plan->lines[index];
		}
		else
		{
			return plan->Selected();
		}
	}

	return NULL;
}

ContentEditor::Point* ContentEditor::GetPoint(int index)
{
	Line* line = GetLine();

	if(line)
	{
		if(index >= 0 && index < line->points.size())
		{
			return &line->points[index];
		}
		else
		{
			return line->Selected();
		}
	}

	return NULL;
}

// route management
int ContentEditor::AddLine(int after)
{
	Plan* plan = GetPlan();

	if(plan)
	{
		Line line = Line();
		int rcount = plan->lines.size();

		if(rcount > 0 && after >= 0 && after < rcount)
		{
			// after element
			plan->lines.insert(plan->lines.begin() + after, line);
			SelectLine(after + 1);
		}
		else
		{
			// at the end

			// if this is the first route, its the drone route
			if(rcount == 0)
			{
				line.drone = true;
				line.lastHop = 0;
			}

			plan->lines.push_back(line);
			SelectLine(plan->lines.size() - 1);
		}

		//UpdateUI();

		return plan->selected;
	}

	return -1;
}

void ContentEditor::SelectLine(int index)
{
	Plan* plan = GetPlan();

	if(plan)
	{
		Line* current	= GetLine();
		Line* newLine	= GetLine(index);

		if(current != NULL && current != newLine)
			current->Deselect();

		if(index < 0)
			plan->selected = 0;
		else if(index >= plan->lines.size())
			plan->selected = plan->lines.size() - 1;
		else
			plan->selected = index;

		current = GetLine();

		if(current)
			current->Select();
	}
}

void ContentEditor::RemoveLine(int index)
{
	Plan* plan = GetPlan();

	if(plan)
	{
		if(index == -1)
			index = plan->selected;

		// cant remove the drone route
		if(index == 0)
			return;

		ContentEditor::LineList::iterator it = plan->lines.begin();

		for(int i = 0; it != plan->lines.end(); it++, i++)
		{
			if(i == index)
			{
				plan->lines.erase(it);
				//UpdateUI();
				break;
			}
		}

		if(!plan->lines.empty())
			SelectLine(0);
		else
			SelectLine(-1);
	}
}

void ContentEditor::SetLineColor(UINT32 c, int index)
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

UINT32 ContentEditor::GetLineColor(int index)
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
int ContentEditor::AddPoint(double x, double y, double z, int after)
{
	Line* line = GetLine();

	if(line)
	{
		Point point = Point(x,y,z);

		if(line->points.size() > 0 && after >= 0 && after < line->points.size())
		{
			// after element
			line->points.insert(line->points.begin() + after + 1, point);
			SelectPoint(after + 1);
		}
		else
		{
			// at the end
			line->points.push_back(point);
			SelectPoint(line->points.size() - 1);
		}

		//UpdateUI();

		return line->selected;
	}

	return -1;
}

// waypoint management
int ContentEditor::Add3DPoint(double x, double y, double z, int after)
{
	// bail if this is not the drone route
	if(_plan.selected != 0)
		return -1;

	return AddPoint(x, y, z, after);
}

void ContentEditor::SelectPoint(int index)
{
	Line* line = GetLine();

	if(line)
	{
		if(index < 0)
			line->selected = 0;
		else if(index >= line->points.size())
			line->selected = line->points.size() - 1;
		else
			line->selected = index;
	}
}

void ContentEditor::RemovePoint(int index)
{
	Line* line = GetLine();

	if(line)
	{
		if(index == -1)
			index = line->selected;

		ContentEditor::PointList::iterator it = line->points.begin();

		for(int i = 0; it != line->points.end(); it++, i++)
		{
			if(i == index)
			{
				//if(it->drawID >= 0)
				//	draw3dRemoveDebugObject(it->drawID);

				line->points.erase(it);
				//UpdateUI();
				break;
			}
		}

		if(index <= line->lastHop)
			line->lastHop--; // move the lasthop down one because we deleted a waypoint below it

		if(!line->points.empty())
			SelectPoint(line->selected - 1);
		else
			SelectPoint(-1);
	}
}

void ContentEditor::ContextPoint(int index)
{
	Line* line = GetLine();

	if(line)
	{
		if(index >= 0 && index < line->points.size())
			line->context = index;
		else
			line->context = -1;
	}
}

void ContentEditor::UpdatePoint(Vec3d pos, int index)
{
	Line* line = GetLine();

	if(line)
	{
		if(index == -1)
			index = line->selected;

		ContentEditor::PointList::iterator it = line->points.begin();

		for(int i = 0; it != line->points.end(); it++, i++)
		{
			if(i == index)
			{
				it->position = pos;
				_plan.modified = true;
				break;
			}
		}
	}
}

void ContentEditor::SetHeading()
{
	Line* line = GetLine();

	if(line && !line->points.empty())
	{
		// look up last click
		Vec3d dir = _lastClick - line->points.at(0).position;

		dir.normalize();

		line->heading = atan2(dir.x(), dir.y());
	}
}

void ContentEditor::SetPPO(const char* name)
{
	if(_currentPPO)
	{
		delete _currentPPO;
		_currentPPO = NULL;
	}

	if(name)
	{
		_ppoSpec.id						= 1;
		_ppoSpec.modelName				= name;
		_ppoSpec.rayAdjust.x = 0.0;
		_ppoSpec.rayAdjust.y = 0.0;
		_ppoSpec.rayAdjust.z = 0.0;
		_ppoSpec.avoidance				= 0; // buffer in meters to add to the avoidance box
		_ppoSpec.rayTolerance			= 0; // ray tolerance; angle between corner and center ray normals allowed
		_ppoSpec.tiltTolerance			= 0; // tilt tolerance; tilt allowed from straight up
		_ppoSpec.viewOffset				= 20.0f; // how far to move the object in front of us for viewing purposes
		_ppoSpec.rotation				= 0; // degrees to rotate the object around the z
		_ppoSpec.coolDown				= 0; // how long in milliseconds to cool down after building this object
		_ppoSpec.buildTime				= 0; // build time in miliseconds
		_ppoSpec.buildFlags				= COLLIDER_ALL; // what this ppo can be built on; everything else must be avoided (collider flags)
		_ppoSpec.avoidFlags				= 0; // what this ppo cant be in contact with to build (collider flags)
		_ppoSpec.surfaceFlags			= COLLIDER_ALL; // which infantry surfaces are okay for this object type (0 - 5)
		_ppoSpec.allowInterior			= false; // can we build this while inside a building
		_ppoSpec.treeCheck				= true;		// do we check for tree collisions
		_ppoSpec.enemyFacDistance		= 0; // distance from the nearest enemy facility (positive >= negative <=)
		_ppoSpec.friendlyFacDistance	= 0; // distance from the nearest friendly facility (positive >= negative <=)
		_ppoSpec.targetDistance			= 0;	// distance from the target facility (positive >= negative <=)
		_ppoSpec.maxSpeed				= 1000;		// threshold in meters per second <=
		_ppoSpec.badTextures.clear();

		_currentPPO = new PlayerPlacedObject(&_ppoSpec);
	}
}

void ContentEditor::SetHeading(double heading)
{
	Line* line = GetLine();
	if(line) line->heading = heading;
}

void ContentEditor::SetLastHop(int index)
{
	Line* line = GetLine();

	if(line)
	{
		line->lastHop = -1;

		int wsize = line->points.size();

		if(wsize > 1 && index >= 0 && index < wsize - 1)
			line->lastHop = index;
	}
}

void ContentEditor::OnClick(int button, int state, double x, double y, double z)
{
	if(state == 1)
	{
		bool contextual = (CONTENT_EDITOR->Context() >= 0);

		if(contextual)
			return;

		// clear our context
		ContextPoint(-1);

		_mouseDown = button;

		if(_mouseDown == 2 || _mouseDrag)
			_lastClick.set(x, y, z);

		// see if we're close to any waypoints
		Line* line = GetLine();

		if(line)
		{
			ContentEditor::PointList::iterator it = line->points.begin();

			for(int i = 0; it != line->points.end(); it++, i++)
			{
				ContentEditor::Point& point = *it;

				if(point.NearMe(x, y, z))
				{
					if(_mouseDown == 1)
					{
						if(!_mouseDrag)
							SelectPoint(i);

						_mouseDrag = _mouseDown;
					}
					else if(_mouseDown == 2)
					{
						ContextPoint(i);
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

bool ContentEditor::MouseDown()
{
	return (_mouseDown > 0);
}

int ContentEditor::Context()
{
	Line* line = GetLine();

	if(line && line->context >= 0 && line->context < line->points.size())
	{
		phui::View* menu = uiFindView("content_rcmenu");

		if(menu != NULL)
			return line->context;
	}

	return -1;
}

Vec3d& ContentEditor::LastClick()
{
	return _lastClick;
}

ContentEditor* ContentEditor::GetInstance()
{
	return &_contentEditor;
}

#endif // BETA_RELEASE
