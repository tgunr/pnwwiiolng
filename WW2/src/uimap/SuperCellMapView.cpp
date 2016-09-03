#include <syscfg.h> // Path is project specific.
#include <algorithm>
#include <map>
#include <list>
#include <string>
#include <vector>
#include <math.h>
#include <assert.h>

#include "pi/pi.h"
#include "gl/glplat.h"
#include "math/math3d.h"
#include "include/vars.h"
#include "ogl/ogl.h"
#include "trn/trntypes.h"

#include "phui/KeyCodes.h"
#include "phui/GraphicsContext.h"
#include "phui/Util.h"
#include "phui/Referenced.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Javascript.h"
#include "phui/Message.h"
#include "uimap/SuperCellMapView.h"
#include "sdm/sdm.h"
#include "tut/TutorialManager.h"
#include "imap/ww2func.h"
#include "ui/uiMain.h"
#include "uimap/CPRenderer.h"
#include "sdm/StratDB.h"
#include "scen/ScenarioManager.h"
#include "sys/sysmain.h"
#include <condition_variable>
#include "include/pnThread.h"

#define SQUARE(x) ((x)*(x))
#define PAN_SCALE_MAX 5.0f				// max scale when panning
#define PAN_SCALE_RAMP 200000.0f		// lower = higher scale values, higher = lower scale values
#define PAN_TIME_WIDTH(x) ((x < .5f) ? 800: 1500) // time it takes to move horizontally
#define PAN_TIME_HEIGHT 750				// time it takes to move vertically

static SINT32 _mapinstances = 0;

class  SuperCellMapView::Renderer::LoadThread : public PNThread, public phui::Referenced
{
public:
	LoadThread(SuperCellMapView::Renderer* r);
	virtual ~LoadThread();

	void LoadSuperCell(SuperCellMapView::Renderer::SuperCell*);
	void RemoveRendererSuperCells(SuperCellMapView::Renderer*);

	virtual void run();

	std::mutex _queueMutex;
	std::condition_variable _doLoad;

	SuperCellMapView::Renderer* _renderer;
	bool _done;

	typedef std::vector<SuperCell*> SuperCellList;
	SuperCellList _cells;
};

phui::ref_ptr<SuperCellMapView::Renderer::LoadThread> SuperCellMapView::Renderer::_loadthread;

#define WORLD_ARC_WIDTH			180.0
#define WORLD_ARC_HEIGHT		70.1875

/************************************************************************************************
	SuperCellMapView
*************************************************************************************************/

SuperCellMapView::SuperCellMapView() : View(),
    _posx(0),
	_posy(0),
	_scacross(1),
	_minscale(.1f),
	_maxscale(64),
	_mapscale(0),
	_navscale(1.0f),
	_mx(0),
	_my(0),
	_panx(0),
	_pany(0),
	_pandirx(0),
	_pandiry(0),
	_panscale(0),
	_startpanscale(0),
	_stoppanscale(0),
    _panlength(0),
	_alpha(1.0f),
	_pantime(0),
	_panstop(0),
	_autoscale(0),
	_autoscaletime(0),
    _panstage(0),
	_objectScaleCheck(false),
	_mapLocked(false)
{
	_acceptsFocus = _objectClicked = false;
	_dragscx = _dragscy = 0;
	SetBackgroundColor(1,1,1,1);
	_mapObjects = new MapObjectList;

	// keep track of how many maps are currently running
	++_mapinstances;

	//if(gCustomTerrain)
	//	_mapLocked = false;
}

SuperCellMapView::~SuperCellMapView()
{
	_renderers.clear();

	// remove any shared filters from the main message loop
	// map input filters are shared
	phui::MessageFilter* filter = nullptr;

	for(int i = 0; (filter = FilterAt(i)) != nullptr; i++)
		uiMessageLoop()->RemoveMessageFilter(filter);

	// keep track of how many maps are currently running
	--_mapinstances;
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

#define ZOOM_SCALE 100.0f

void SuperCellMapView::DrawSelf(void)
{
	if(_autoscale && frametime >= _autoscaletime)
	{
		MouseScroll(0, 0, _autoscale);
		_autoscaletime = frametime + 50;
	}

	UpdatePan();
	View::DrawSelf();

	float scxf = (float)(_posx / SDM_SUPERCELL_SIZE);
	float scyf = (float)(_posy / SDM_SUPERCELL_SIZE);
	float scale = Width() / (float)_scacross;
	float aspect =  Height() / Width();

//	float scleft	= (float)(scxf - .5 * _scacross);
//	float scright   = (float)(scxf + .5 * _scacross);
//	float sctop		= (float)(scyf + .5 * _scacross * aspect);
//	float scbottom  = (float)(scyf - .5 * _scacross * aspect);

	oglSetMatrixMode(GL_MODELVIEW);
	glScalef(scale, scale, 1);
	glTranslatef(-scxf + .5f * _scacross, -scyf + .5f * _scacross * aspect, 0); // center on position

	int scx1, scx2, scy1, scy2;
	VisibleSupercells(&scx1, &scy1, &scx2, &scy2);

	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(1.0f, 1.0f, 1.0f, _alpha);

	RendererList::iterator it = _renderers.begin();
	for(; it != _renderers.end(); ++it)
	{
		Renderer* r = it->ptr();

		if(r->enabled && _scacross >= r->minscale && _scacross < r->maxscale)
			r->RenderSuperCells(this, scx1, scy1, scx2, scy2);
	}

	_mapObjects->Draw(this);

	// reset color
	ctx->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void SuperCellMapView::MessageReceived(phui::Message* msg, phui::MessageHandler* reply)
{
	bool handled = false;
	long unicode,keycode;

	switch(msg->what)
	{
		case phui::KEY_DOWN:
		{
			msg->FindLong(UI_KEYCODE, &keycode);
			msg->FindLong(UI_KEY_UNICODE, &unicode);
			handled = KeyDown(keycode, (unsigned short)unicode);
		}
		break;

		case phui::KEY_UP:
		{
 			msg->FindLong(UI_KEYCODE, &keycode);
			msg->FindLong(UI_KEY_UNICODE, &unicode);
			handled = KeyUp(keycode, (unsigned short)unicode);
		}
		break;
	}

	if(!handled)
		View::MessageReceived(msg, reply);
}

/************************************************************************************************
	KeyDown
*************************************************************************************************/

bool SuperCellMapView::KeyDown(int keycode, unsigned short unicode)
{
	float posx = _posx;
	float posy = _posy;

	switch (keycode)
	{
		case phui::KEYCODE_UP:
			if(!SCEN_MGR->IsActive())
				posy += (float)(_scacross * .05f * SDM_SUPERCELL_SIZE);
		break;
		case phui::KEYCODE_DOWN:
			if(!SCEN_MGR->IsActive())
				posy -= (float)(_scacross * .05f * SDM_SUPERCELL_SIZE);
		break;
		case phui::KEYCODE_RIGHT:
			if(!SCEN_MGR->IsActive())
				posx += (float)(_scacross * .05f * SDM_SUPERCELL_SIZE);
		break;
		case phui::KEYCODE_LEFT:
			if(!SCEN_MGR->IsActive())
				posx -= (float)(_scacross * .05f * SDM_SUPERCELL_SIZE);
		break;
		case phui::KEYCODE_KP_MINUS:
			if(!uiInGame())
				SetAutoScale(-1);
		break;
		case phui::KEYCODE_KP_PLUS:
			if(!uiInGame())
				SetAutoScale(1);
		break;
		case phui::KEYCODE_SPACE:
			if(TUT_ACTIVE)
				TutorialManager::Input(1, IMAP_CONTINUE_TUTORIAL, 0);
		break;
	}

	SetPositionXY(posx, posy);

	return false;
}

bool SuperCellMapView::KeyUp(int keycode, unsigned short unicode)
{
	switch (keycode)
	{
		case phui::KEYCODE_KP_MINUS:
		case phui::KEYCODE_KP_PLUS:
			if(!uiInGame())
				SetAutoScale(0);
		break;
		case phui::KEYCODE_SPACE:
			if(TUT_ACTIVE)
				TutorialManager::Input(0, IMAP_CONTINUE_TUTORIAL, 0);
		break;
	}

	return false;
}

void SuperCellMapView::SetOverMapObject(MapObject* overobj)
{
	if(_lastover.ptr() && overobj != _lastover.ptr())
		_lastover.ptr()->Over(this, phui::MOUSE_EXIT);

	_lastover = overobj;
}

bool SuperCellMapView::MapObjectList::MouseMove(SuperCellMapView* mv, float x, float y, int state)
{
	float sx=x, sy=y;
	mv->LocalToSuperCellCoordinates(&sx, &sy);

	MapObjectList::reverse_iterator it = rbegin();
	MapObject* lastoverobj = mv->OverMapObject();

	for(; it != rend(); ++it)
	{
		MapObject* mapobj = it->ptr();
		if(!mapobj->_visible) continue;

		if(mapobj->Children.valid())
			if(mapobj->Children->MouseMove(mv, x,y, state)) return true;

		if(mapobj->IsHit(mv, sx, sy))
		{
			state = (mapobj != lastoverobj) ? phui::MOUSE_ENTER : phui::MOUSE_OVER;
			mv->SetOverMapObject(mapobj);
			mapobj->Over(mv, state);
			return true;
		}
	}

	return false;
}

bool SuperCellMapView::Renderer::MouseMove (SuperCellMapView* mv, float x, float y, int state )
{
	if(MapObjects()->MouseMove(mv, x,y,state))
		return true;

	int scx1, scx2, scy1, scy2;
	mv->VisibleSupercells(&scx1, &scy1, &scx2, &scy2);

	for(int scy = scy1; scy < scy2; scy++)
	{
		for(int scx = scx1; scx < scx2; scx++)
		{
			SuperCellMap::iterator it = _scmap.find(supercell_id(scx,scy));
			if(it == _scmap.end()) continue;

			SuperCell* sc = it->second.ptr();
			if(sc)
			{
				if(sc->MapObjects()->MouseMove(mv, x,y,state))
					return true;
			}
		}
	}

	return false;
}

bool SuperCellMapView::MouseMove (float x, float y, int state)
{
	if(!AcceptsMouseClick())
		return true;

	_mx = x; _my = y;
	bool mouseover = false;

	mouseover = MapObjects()->MouseMove(this, x,y,state);

	if(!mouseover)
	{
		RendererList::reverse_iterator it = _renderers.rbegin();
		for(; it != _renderers.rend(); ++it)
		{
			Renderer* r = it->ptr();
			if(r->enabled && _scacross >= r->minscale && _scacross < r->maxscale)
			{
				mouseover = r->MouseMove(this,x,y,state);
				if(mouseover) break;
			}
		}
	}

	if(!mouseover)
		SetOverMapObject(0);

	return View::MouseMove(_mx, _my, state);
}

bool SuperCellMapView::MapObjectList::MouseDown(SuperCellMapView* mv, float x, float y, int button)
{
	float sx=x, sy=y;
	mv->LocalToSuperCellCoordinates(&sx, &sy);

	MapObjectList::reverse_iterator it = rbegin();

	for(; it != rend(); ++it)
	{
		MapObject* mapobj = it->ptr();
		if(!mapobj->_visible) continue;

		if(mapobj->Children.valid())
			if(mapobj->Children->MouseDown(mv, x,y, button)) return true;

		if(mapobj->_selectable && mapobj->IsHit(mv, sx, sy))
		{
			mv->SelectMapObject(mapobj);
			return true;
		}
	}

	return false;
}

bool SuperCellMapView::Renderer::MouseDown(SuperCellMapView* mv, float x, float y, int button)
{
	if(MapObjects()->MouseDown(mv, x,y,button))
		return true;

	int scx1, scx2, scy1, scy2;
	mv->VisibleSupercells(&scx1, &scy1, &scx2, &scy2);

	for(int scy = scy1; scy < scy2; scy++)
	{
		for(int scx = scx1; scx < scx2; scx++)
		{
			SuperCellMap::iterator it = _scmap.find(supercell_id(scx,scy));
			if(it == _scmap.end()) continue;

			SuperCell* sc = it->second.ptr();
			if(sc)
			{
				if(sc->MapObjects()->MouseDown(mv, x,y,button)) return true;
			}
		}
	}

	return false;
}

bool SuperCellMapView::MouseDown (float x, float y, int button)
{
	if(!AcceptsMouseClick())
		return true;

	// stop any panning
	_panx = _pany = 0.0f;
	_panstage = 0;

	if(button) // only the left mouse button can select
		return true;

	_mx = x; _my = y;
	_objectClicked = false;
	_objectScaleCheck = false;

	_objectClicked = MapObjects()->MouseDown(this, x,y,button);

	if(_objectClicked) return true;

	RendererList::reverse_iterator it = _renderers.rbegin();
	for(; it != _renderers.rend(); ++it)
	{
		Renderer* r = it->ptr();
		if(r->enabled && _scacross >= r->minscale && _scacross < r->maxscale)
		{
			_objectClicked = r->MouseDown(this,x,y,button);
			if(_objectClicked) return true;
		}
	}

	return true;
}

/************************************************************************************************
	MouseScroll
*************************************************************************************************/

bool SuperCellMapView::MouseScroll(float x, float y, int dir)
{
	if(!AcceptsMouseClick() || _mapLocked)
		return true;

	float scacross = _scacross;
	float inc = (gCustomTerrain) ? scacross/20.0f : scacross/5.0f;

	scacross += -dir * inc;
	SetScale(scacross);

	_panx = _pany = 0;
	_panstage = 0;

	return true;
}

/************************************************************************************************
	MouseMove
*************************************************************************************************/

bool SuperCellMapView::MouseDrag (float x, float y, int button, bool start)
{
	if(!AcceptsMouseClick() || _mapLocked)
		return true;

	if(start)
	{
		_dragx = x;
		_dragy = y;
		PositionSuperCell(&_dragscx, &_dragscy);
	}

	float scx, scy;
	scx = _dragscx - (x - _dragx) / Width()  * _scacross;
	scy = _dragscy - (y - _dragy) / Height() * _scacross;

	SetPositionSuperCell(scx, scy);

	int type = TUT_MAP_DRAG;
	TUT_MGR->DoEvent(TUT_EVENT_MAP, &type);

	return true;
}

/************************************************************************************************
	VisibleSupercells
*************************************************************************************************/

void SuperCellMapView::VisibleSupercells(int* scx1, int* scy1, int* scx2, int* scy2)
{
	float scxf = (float)(_posx / SDM_SUPERCELL_SIZE);
	float scyf = (float)(_posy / SDM_SUPERCELL_SIZE);
	float aspect = 1;
	if(_height && _width)
		aspect = _height / _width;
	float scleft	= (float)(scxf - .5 * _scacross);
	float scright   = (float)(scxf + .5 * _scacross);
	float sctop		= (float)(scyf + .5 * _scacross * aspect);
	float scbottom  = (float)(scyf - .5 * _scacross * aspect);

	*scx1 = (int)floor(scleft);
	*scx2 = (int)ceil(scright);
	*scy1 = (int)floor(scbottom);
	*scy2 = (int)ceil(sctop);
}

/************************************************************************************************
	SetPositionXY
*************************************************************************************************/

void SuperCellMapView::SetPositionXY(float x, float y, bool recalcPan)
{
	_posx = x;
	_posy = y;

	if(recalcPan && (_panx || _pany))
		SetPanPositionXY(_panx, _pany, _stoppanscale);
}

/************************************************************************************************
	SetPanPositionXY
*************************************************************************************************/

#define PAN_LIMIT_SCALE(x) (x = ((x < _minscale) ? _minscale: (x > _maxscale) ? _maxscale: x))

void SuperCellMapView::SetPanPositionXY(float x, float y, float s)
{
	_panx						= x;
	_pany					= y;
	_panstage				= 1;
	_pantime				= frametime;
	_panlength				= math3dFastSqrt(SQUARE((_panx - _posx)) + SQUARE((_pany - _posy)));

	_panscale				= (_panlength / PAN_SCALE_RAMP) * PAN_SCALE_MAX;	// our stage target scale
	_stoppanscale			= (s) ? s: _scacross;								// scale we end at
	_startpanscale			= _scacross;										// scale we start at
	_objectScaleCheck	= false;

	PAN_LIMIT_SCALE(_stoppanscale);

	if(_panscale < _startpanscale)
		_panscale = _startpanscale;

	if(_panscale < _stoppanscale)
		_panscale = _stoppanscale;

	PAN_LIMIT_SCALE(_panscale);

	if(_scacross != _panscale)
	{
		_panstop = _pantime + PAN_TIME_HEIGHT;
	}
	else
	{
		_panstop	= _pantime + PAN_TIME_WIDTH(_stoppanscale);
		_panstage	= 2; // start moving horizontally
	}

	// get direction and save
	piVECTOR dir;
	piVECTOR start;
	piVECTOR stop;

	start.x = _posx;
	start.y = _posy;
	start.z = 0.0f;

	stop.x = _panx;
	stop.y = _pany;
	stop.z = 0.0f;

	math3dSubtractVectors(&dir, &stop, &start);
	math3dNormalize(&dir);

	_pandirx = dir.x;
	_pandiry = dir.y;
}

/************************************************************************************************
	SetPositionLatLon
*************************************************************************************************/

void SuperCellMapView::SetPositionLatLon(float lat, float lon)
{
	float x = (float)(lon * WORLD_WIDTH / WORLD_ARC_WIDTH);
	float y = (float)(lat * WORLD_HEIGHT / WORLD_ARC_HEIGHT);
	SetPositionXY(x, y);
}

/************************************************************************************************
	SetPositionSuperCell
*************************************************************************************************/

void SuperCellMapView::SetPositionSuperCell(float scx, float scy)
{
	SetPositionXY(SDM_SUPERCELL_SIZE * scx, SDM_SUPERCELL_SIZE * scy);
}

/************************************************************************************************
	SetScale
*************************************************************************************************/

void SuperCellMapView::SetScale(float numsc)
{
	if(_objectScaleCheck && _selectedmapobject.valid())
	{
		_objectScaleCheck = false;

		// recalc
		StratMapObject* smo = dynamic_cast<StratMapObject*>(_selectedmapobject.ptr());

		if(smo)
		{
			int posx, posy;

			if(smo->_cp.StratClass() == SDM_CLASS_CP && smo->_cp.GetAreaXY(posx, posy))
				SetPositionXY((float)posx, (float)posy);
			else
				SetPositionXY((float)smo->_cp.PosX(), (float)smo->_cp.PosY());

			/*
			const StratDB::StratObject* strat = &smo->_cp;

			// Loop until cp object is found
			while(strat)
			{
				if(strat->StratClass() == SDM_CLASS_CP)
					break;

				strat = strat->ParentStratObject();
			}

			// as the default and initial default values
			int panx, pany, count;

			panx = strat->PosX();
			pany = strat->PosY();
			count = 1;

			StratDB::StratObject::ChildList const& children = strat->ChildStratObjects();
			StratDB::StratObject::ChildList::const_iterator it;
			for(it = children.begin() ; it != children.end() ; ++it)
			{
				if((*it)->StratType() != SDM_FACILITY_FIREBASE)
				{
					panx += (*it)->PosX();
					pany += (*it)->PosY();
					++count;
				}
			}

			SetPositionXY(int(panx/count), int(pany/count));
			*/


			/*
			// as the default and initial default values
			int leftx, rightx, topy, bottomy;

			leftx = rightx = strat->PosX();
			topy = bottomy = strat->PosY();

			StratDB::StratObject::ChildList const& children = strat->ChildStratObjects();
			StratDB::StratObject::ChildList::const_iterator it;
			for(it = children.begin() ; it != children.end() ; ++it)
			{
				if((*it)->StratType() != SDM_FACILITY_FIREBASE)
				{
					leftx = std::min(leftx, (*it)->PosX());
					rightx = std::max(rightx, (*it)->PosX());
					topy = std::max(topy, (*it)->PosY());
					bottomy = std::min(bottomy, (*it)->PosY());
				}
			}

			SetPositionXY(leftx + ((rightx - leftx)/2), bottomy + ((topy - bottomy)/2));
			*/
		}
	}

	if(numsc < _minscale) numsc = _minscale;
	if(numsc > _maxscale) numsc = _maxscale;

	int dir = 0;

	if(numsc < _scacross)
		dir = TUT_MAP_ZOOMIN;
	else if(numsc > _scacross)
		dir = TUT_MAP_ZOOMOUT;

	if(dir)
		TUT_MGR->DoEvent(TUT_EVENT_MAP, &dir);

	_scacross = numsc;

	MapScale(true);
}

/************************************************************************************************
	SetAlpha - used to set the map's alpha
*************************************************************************************************/

void SuperCellMapView::SetAlpha(float a)
{
	_alpha = a;
}

/************************************************************************************************
	SetNavScale
*************************************************************************************************/

void SuperCellMapView::SetNavScale(float ns)
{
	_navscale = ns;
}

float SuperCellMapView::MapScale(bool force)
{
	if(_mapscale == 0 || force)
	{
		if(_width == 0.0f || _scacross == 0.0f)
			return 1.0f;

		float iconscale = _width / _scacross;
		float width = 1000.0f/SDM_SUPERCELL_SIZE * iconscale;

		// calculate our current map scale
		_mapscale = pow(10.0f, (float)floor(log10(width/(_width*.9/2.0)))+1);
	}

	return _mapscale * _navscale;
}

/************************************************************************************************
	PositionSuperCell
*************************************************************************************************/

void SuperCellMapView::PositionSuperCell(float* scx, float* scy)
{
	*scx = _posx/SDM_SUPERCELL_SIZE;
	*scy = _posy/SDM_SUPERCELL_SIZE;
}

/************************************************************************************************
	PreferredSize
*************************************************************************************************/

void SuperCellMapView::PreferredSize(float* w, float* h)
{
	*w = 300; *h = 300;
}


void SuperCellMapView::LocalToSuperCellCoordinates(float* x, float* y)
{
	float scale = Width() / (float)_scacross;
	float aspect =  Height() / Width();
	float scxf = (float)(_posx / SDM_SUPERCELL_SIZE);
	float scyf = (float)(_posy / SDM_SUPERCELL_SIZE);

	*x /= scale;
	*y /= scale;

	*x -= (float)(-scxf + .5 * _scacross);
	*y -= (float)(-scyf + .5 * _scacross * aspect);
}

void SuperCellMapView::LocalToWorldCoordinates(float* x, float* y)
{
	LocalToSuperCellCoordinates(x, y);
	*x *= SDM_SUPERCELL_SIZE;
	*y *= SDM_SUPERCELL_SIZE;
}

void SuperCellMapView::LocalToLatLon(float* x, float* y)
{
	LocalToWorldCoordinates(x, y);
	*x *= (float)(WORLD_ARC_WIDTH / WORLD_WIDTH);
	*y *= (float)(WORLD_ARC_HEIGHT / WORLD_HEIGHT);
}

void SuperCellMapView::AddRenderer(Renderer* r)
{
	_renderers.push_back(r);
}

/************************************************************************************************
	Renderer
*************************************************************************************************/

SuperCellMapView::Renderer::Renderer() : enabled(true), minscale(.1f), maxscale(64), _mv(0)
{
	_mapObjects = new MapObjectList;
	if(!_loadthread.valid())
	{
		_loadthread = new LoadThread(this);
		_loadthread->start();
		while(!_loadthread->isRunning()){
#if platOS == OS_WIN32
            SwitchToThread();
#elif platOS == OS_MACOS
            pthread_yield_np();
#elif platOS == OS_LINUX
            pthread_yield();
#else
#error "Oops"
#endif
		}
	}

	_loadthread->Acquire();
}

SuperCellMapView::Renderer::~Renderer()
{
	_loadthread->RemoveRendererSuperCells(this);
	_loadthread->Release();
}

/************************************************************************************************
	RenderSuperCells
*************************************************************************************************/

void SuperCellMapView::Renderer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	_mv = mv;
	for(int scy = scy1; scy <= scy2; scy++) // no more white squares at least
	{
		for(int scx = scx1; scx <= scx2; scx++) // no more white squares at least
		{
			SuperCellMap::iterator it = _scmap.find(supercell_id(scx,scy));
			SuperCell* sc = 0;

			if(it == _scmap.end())
				_scmap[supercell_id(scx,scy)] = sc = NewSuperCell(this, scx, scy);
			else
				sc = it->second.ptr();

			if(sc)
			{
				if(!sc->loaded && !sc->loading)
					_loadthread->LoadSuperCell(sc);
				else if(sc->loaded)
					sc->Draw(mv->GetGraphicsContext());
			}
		}
	}

	MapObjects()->Draw(mv);
}

/************************************************************************************************
	SuperCell
*************************************************************************************************/

SuperCellMapView::Renderer::SuperCell::SuperCell(Renderer* r, int x, int y)
	: loading(false), loaded(false)
	, scx(x), scy(y)
	, renderer(r)
{
	_mapObjects = new MapObjectList;
}

/************************************************************************************************
	LoadThread
*************************************************************************************************/

SuperCellMapView::Renderer::LoadThread::LoadThread(Renderer* r)
	: PNThread()
	, _renderer(r)
	, _done(false)
	, _cells()
{
	_cells.reserve(64);
}

SuperCellMapView::Renderer::LoadThread::~LoadThread()
{
	_done = true;
	{
		std::unique_lock<std::mutex> lock(_queueMutex, std::try_to_lock);
		_doLoad.notify_all();
	}
    join();
}

void SuperCellMapView::Renderer::LoadThread::LoadSuperCell(SuperCell* sc)
{
	std::lock_guard<std::mutex> lock(_queueMutex);

	sc->loading = true;
	_cells.push_back(sc);
	_doLoad.notify_one();
}

void SuperCellMapView::Renderer::LoadThread::RemoveRendererSuperCells(SuperCellMapView::Renderer* r)
{
	std::lock_guard<std::mutex> lock(_queueMutex);
	if ( _cells.empty() == true )
		return;
	SuperCellList::iterator it = _cells.begin();
	while ( it != _cells.end() )
	{
		if( (*it)->renderer == r)
		{
			*it = _cells.back();
			_cells.pop_back();
		}
		else
			++it;
	}
}

void SuperCellMapView::Renderer::LoadThread::run()
{
	for(;;)
	{
		std::unique_lock<std::mutex> lock(_queueMutex);
		_doLoad.wait(lock, [&] { return _done || !_cells.empty(); });
		if(_done)
			break;
		if(_cells.empty())
			continue;

		SuperCell* sc = _cells.front();
		_cells.erase(_cells.begin());
		lock.unlock();

		if ( nullptr != sc && sc->loading == true )
			sc->Load();
	}
}

void SuperCellMapView::UpdatePan()
{
	switch(_panstage)
	{
		case 1: // scaling up
		{
			if(_scacross < _panscale && _pantime < _panstop)
			{
				float panpct	= (float)(frametime - _pantime)/(float)PAN_TIME_HEIGHT;
				float panstep	= fabs(_panscale - _startpanscale) * panpct;

				if((_panscale - _scacross) > panstep)
				{
					_scacross += panstep;
					_pantime = frametime;
				}
				else
				{
					_pantime = _panstop;
				}
			}
			else
			{
				_scacross	= _panscale;
				_pantime	= frametime;
				_panstop	= _pantime + PAN_TIME_WIDTH(_panscale);
				_panstage	= 2;
			}
		}
		break;
		case 2: // moving horizontally
		{
			if(_pantime < _panstop)
			{
				float panpct	= (frametime - _pantime)/(float)PAN_TIME_WIDTH(_panscale);
				float panstep	= _panlength * panpct;

				if(fabs(_posx - _panx) <= panstep && fabs(_posy - _pany) <= panstep)
				{
					SetPositionXY(_panx, _pany, false);
					_pantime = _panstop;
				}
				else
				{
					SetPositionXY(_posx + (_pandirx * panstep), _posy + (_pandiry * panstep), false);
					_pantime = frametime;
				}
			}
			else
			{
				SetPositionXY(_panx, _pany, false);

				_panstage	= 3;
				_pantime	= frametime;
				_panstop	= _pantime + PAN_TIME_HEIGHT;
			}
		}
		break;
		case 3: // scaling down
		{
			if(_scacross > _stoppanscale && _pantime < _panstop)
			{
				float panpct	= (float)(frametime - _pantime)/(float)PAN_TIME_HEIGHT;
				float panstep	= fabs(_panscale - _stoppanscale) * panpct;

				if((_scacross - _stoppanscale) > panstep)
				{
					_scacross -= panstep;
					_pantime = frametime;
				}
				else
				{
					_pantime = _panstop;
				}
			}
			else
			{
				_scacross	= _stoppanscale;
				_pantime	= frametime;
				_panstop	= _pantime;
				_panscale	= _stoppanscale;
				_panstage	= 0;
				_panx		= 0;
				_pany		= 0;
			}
		}
		break;
	}
}

bool SuperCellMapView::Stabilized() const
{
	return (_panstage == 0);
}

void SuperCellMapView::SelectMapObject(MapObject* mapobj)
{
	if(mapobj && !mapobj->_selectable) return;

	if(_selectedmapobject.ptr() != mapobj)
	{
		phui::ref_ptr<MapObject> prevselected = _selectedmapobject.ptr();
		_selectedmapobject = mapobj;

		if(prevselected.valid())
			prevselected->Deselect(this);

		if(mapobj)
			mapobj->Select(this);
	}

	_objectScaleCheck = true;
}

void SuperCellMapView::MapObjectList::remove(MapObject* mapobj)
{
	MapObjectList::iterator it = find(begin(), end(), mapobj);
	if(it != end())
		erase(it);
}

void SuperCellMapView::MapObject::DrawSelf(SuperCellMapView* mv)
{
	if(Children.valid())
		Children->Draw(mv);
}

void SuperCellMapView::MapObjectList::Draw(SuperCellMapView* mv)
{
	MapObjectList::iterator it = begin();
	for(; it != end();)
	{
		phui::ref_ptr<MapObject> mapobj = it->ptr();
		++it;

		if(mapobj->_blinktotime)
			mapobj->DoBlink(mv);

		if(mapobj->_visible)
			mapobj->DrawSelf(mv);
	}
}

void SuperCellMapView::Renderer::FindObjectsAt(SuperCellMapView* mv, float x, float y, MapObjectList& objs)
{
	MapObjects()->FindObjectsAt(mv, x,y,objs);

	for(int sx = (int)x-1; sx < (int)x+1; sx++)
	{
		for(int sy = (int)y-1; sy < (int)y+1; sy++)
		{
			SuperCellMap::iterator it = _scmap.find(supercell_id(sx,sy));
			if(it == _scmap.end()) continue;

			if(it->second.valid() && it->second->MapObjects())
				it->second->MapObjects()->FindObjectsAt(mv, x,y, objs);
		}
	}
}

void SuperCellMapView::FindObjectsAt(float x, float y, MapObjectList& objs)
{
	LocalToSuperCellCoordinates(&x, &y);

	MapObjects()->FindObjectsAt(this, x,y,objs);

	RendererList::reverse_iterator it = _renderers.rbegin();
	for(; it != _renderers.rend(); ++it)
	{
		Renderer* r = it->ptr();
		r->FindObjectsAt(this, x,y,objs);
	}
}

void SuperCellMapView::MapObjectList::FindObjectsAt(SuperCellMapView* mv, float x, float y, MapObjectList& objs)
{
	iterator it = begin();
	for(; it != end(); ++it)
	{
		MapObject* obj = it->ptr();
		if(obj->_visible)
			obj->FindObjectsAt(mv, x, y, objs);
	}
}

void SuperCellMapView::MapObject::FindObjectsAt(SuperCellMapView* mv, float x, float y, MapObjectList& objs)
{
	if(Children.valid())
		Children->FindObjectsAt(mv, x,y, objs);

	if(IsHit(mv, x,y))
		objs.push_back(this);
}

void SuperCellMapView::MapObject::SetBlink(SuperCellMapView* mv, int seconds)
{
	if(_visible)
	{
		_blinktotime = (seconds * 1000) + frametime; // mv->ParentLoop()->FrameTime()
		_nextblinktime = 0;
	}
}

void SuperCellMapView::MapObject::DoBlink(SuperCellMapView* mv)
{
	//int frametime = mv->ParentLoop()->FrameTime();

	if(frametime <= _blinktotime)
	{
		if(frametime >= _nextblinktime)
		{
			_visible = !_visible;
			_nextblinktime = frametime + 500;
		}
	}
	else
	{
		_blinktotime = _nextblinktime = 0;
		_visible = true;
	}
}

SINT32 SuperCellMapView::Instances()
{
	return _mapinstances;
}
