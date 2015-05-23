#pragma once

#include <atomic>
#include <vector>
#include <list>
#include <map>
#include <string>

#include "phui/View.h"
#include "phui/Referenced.h"

/************************************************************************************************
	SuperCellMapView
*************************************************************************************************/

extern SINT32 frametime;
// tolua_begin
class SuperCellMapView : public phui::View
{
public:
// tolua_end
	SuperCellMapView();
	virtual ~SuperCellMapView();
// tolua_begin
	virtual bool KeyDown(int keycode, unsigned short unicode);
	virtual bool KeyUp(int keycode, unsigned short unicode);
	virtual bool MouseDown (float x, float y, int button);
	virtual bool MouseMove (float x, float y, int state );
	virtual bool MouseScroll(float x, float y, int dir);
	virtual bool MouseDrag (float x, float y, int button, bool start);
// tolua_end
	virtual void DrawSelf();
	virtual void MessageReceived(phui::Message*, phui::MessageHandler*);
// tolua_begin
    // different methods for setting position
    void SetPositionXY(float x, float y, bool recalcPan = true);
    void SetPositionLatLon(float lat, float lon);
    void SetPositionSuperCell(float scx, float scy);
    void SetScale(float numsc); ///< scale is number of supercells displayed horizontally
	void SetPanPositionXY(float x, float y, float s = 0);
	void SetAlpha(float a);
	void SetNavScale(float ns);

	float MapScale(bool force = false);
	float NavScale() const { return _navscale; }
	float Scale() const { return _scacross; }
	float PosX() const { return _posx; }
	float PosY() const { return _posy; }
// tolua_end
	void PositionSuperCell(float* scx, float* scy);
	void PositionXY(float* x, float* y) {*x = _posx; *y = _posy; }
	void MousePosition(float* x, float* y) { *x = _mx; *y = _my; }
	bool ObjectClicked() { return _objectClicked; }

	virtual void PreferredSize(float* w, float* h);
	virtual void MinSize(float* w, float* h) { PreferredSize(w,h); }

	void SetMinScale(float minsc) { _minscale = minsc; }
	void SetMaxScale(float maxsc) { _maxscale = maxsc; }
	void SetAutoScale(int dir){ _autoscale = dir; _autoscaletime = frametime; }

	class MapObject;
	class MapObjectList;
	class Renderer;
	typedef std::vector<phui::ref_ptr<Renderer> > RendererList;

	void AddRenderer(Renderer*);
	RendererList RenderList() { return _renderers; }

	void LocalToSuperCellCoordinates(float* x, float* y);
	void LocalToWorldCoordinates(float* x, float* y);
	void LocalToLatLon(float* x, float* y);

	void VisibleSupercells(int* scx1, int* scy1, int* scx2, int* scy2);
	void UpdatePan();
	bool Stabilized() const; // has the pan settled?

	MapObjectList* MapObjects() { return _mapObjects.ptr(); }

	void SelectMapObject(MapObject*);
	MapObject* SelectedMapObject() { return _selectedmapobject.ptr(); }
	void SetOverMapObject(MapObject*);
	MapObject* OverMapObject() { return _lastover.ptr(); }

	void FindObjectsAt(float x, float y, MapObjectList&);

	// returns the number of active maps right now
	static SINT32 Instances();

protected:
	float _posx, _posy;
	float _scacross;
	float _minscale, _maxscale, _mapscale, _navscale;
	float _mx, _my;
	float _panx, _pany, _pandirx, _pandiry;
	float _panscale, _startpanscale, _stoppanscale;
	float _panlength;
	float _alpha;
	int _pantime, _panstop;
	int _autoscale, _autoscaletime;
	int _panstage;

	// saves whether a click is a map object click or not
	bool _objectClicked, _objectScaleCheck, _mapLocked;
	RendererList _renderers;
	float _dragscx, _dragscy;
	phui::ref_ptr<MapObject> _selectedmapobject, _lastover;
	phui::ref_ptr<MapObjectList> _mapObjects;
}; // tolua_export

/************************************************************************************************
	MapObject
*************************************************************************************************/

class SuperCellMapView::MapObject : public phui::Referenced
{
public:
	MapObject() : _visible(false),
				  _selected(false),
				  _selectable(false),
				  _blinktotime(0),
				  _nextblinktime(0)
	{
	}
	virtual ~MapObject() {}

	virtual void DrawSelf(SuperCellMapView*);
	virtual void Select(SuperCellMapView*) {_selected=true;}
	virtual void Deselect(SuperCellMapView*) {_selected=false;}
	virtual bool IsHit(SuperCellMapView*, float x, float y) const { return false; }
	virtual void Over(SuperCellMapView*,int state) {}
	virtual void SetBlink(SuperCellMapView*, int seconds);
	virtual void DoBlink(SuperCellMapView*);

	void FindObjectsAt(SuperCellMapView*, float x, float y, MapObjectList&);

	bool _visible;
	bool _selected;
	bool _selectable;
	int _blinktotime, _nextblinktime;

	phui::ref_ptr<MapObjectList> Children;
};

/************************************************************************************************
	Renderer
*************************************************************************************************/

class SuperCellMapView::Renderer : public phui::Referenced
{
public:
	Renderer();
	virtual ~Renderer();
	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);

	class SuperCell;
	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy) = 0;

	std::string name;
	bool enabled;
	float minscale, maxscale;

	typedef std::pair<int,int> supercell_id;
	typedef std::map<supercell_id, phui::ref_ptr<SuperCell> > SuperCellMap;
	SuperCellMapView* _mv;

	MapObjectList* MapObjects() { return _mapObjects.ptr(); }

	virtual bool MouseDown(SuperCellMapView*, float x, float y, int button);
	virtual bool MouseMove(SuperCellMapView*, float x, float y, int state);

	void FindObjectsAt(SuperCellMapView*, float x, float y, MapObjectList&);

protected:
	class LoadThread;
	static phui::ref_ptr<LoadThread> _loadthread;
	SuperCellMap _scmap;
	phui::ref_ptr<MapObjectList> _mapObjects;
};

/************************************************************************************************
	SuperCell
*************************************************************************************************/

class SuperCellMapView::Renderer::SuperCell : public phui::Referenced
{
public:
	SuperCell(Renderer*, int scx, int scy);
	virtual ~SuperCell() { }

	std::atomic<bool> loading, loaded; // volatile since we want these from memory not in a register
	int scx, scy;
	Renderer* renderer;

	virtual void Draw(phui::GraphicsContext*) = 0;
	virtual void Load() = 0;

	MapObjectList* MapObjects() { return _mapObjects.ptr(); }

protected:
	phui::ref_ptr<MapObjectList> _mapObjects;
};

class SuperCellMapView::MapObjectList : public phui::Referenced, public std::list<phui::ref_ptr<MapObject> >
{
public:
	void remove(MapObject*);

	void Draw(SuperCellMapView*);

	bool MouseDown(SuperCellMapView*, float x, float y, int button);
	bool MouseMove(SuperCellMapView*, float x, float y, int state);

	void FindObjectsAt(SuperCellMapView*, float x, float y, MapObjectList&);
};
