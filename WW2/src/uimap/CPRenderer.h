#pragma once

#include <vector>
#include <map>

#include "phui/MessageHandler.h"
#include "phui/Message.h"

#include "uimap/SuperCellMapView.h"
#include "sdm/StratDB.h"
#include "wwii/wwiiTypes.h"
#include "client/ww2Client.h"

/************************************************************************************************
	CPRenderer
*************************************************************************************************/

class CPRenderer;

// DrawString is slooow
// add this to phui later
class phuiDrawString
{
public:
	phuiDrawString();
	~phuiDrawString();

	void DrawString(phui::GraphicsContext* ctx, int font, float r, float g, float b, float a, const char*);
	void DrawString(phui::GraphicsContext* ctx, int font, float r, float g, float b, float a, const wchar_t*);

protected:
	const char* _str;
	int _displayList;
	int _font;
	float _r,_g,_b,_a;
};

struct MapIcon
{
	MapIcon() : id(-1) { uv[0]=0;uv[1]=0;uv[2]=0;uv[3]=1;uv[4]=1;uv[5]=1;uv[6]=0;uv[7]=0;}
	MapIcon(int i, float llx, float lly, float lrx, float lry, float urx, float ury, float ulx, float uly)
	{
		id = i;

		uv[0] = llx; uv[1] = lly;
		uv[2] = lrx; uv[3] = lry;
		uv[4] = urx; uv[5] = ury;
		uv[6] = ulx; uv[7] = uly;
	}

	MapIcon(int i, float* u)
	{
		id = i;

		for(int j = 0; j < 8; ++j)
			uv[j] = u[j];
	}


	int id;
	float uv[8];
};

class StratMapObject : public SuperCellMapView::MapObject
{
public:
	StratMapObject(CPRenderer* p, StratDB::StratObject const* cp, int t)
		: _renderer(p)
		, _mouseover(false), _childselected(false), _mybase(false)
		, _lastflagframe(rand() & 7), _flaginterval(75)
		, _lastflagtime(t + rand() & 255), _lastpulsetime(t + rand() & 255)
		, _objectivealpha(0), _objectivealphaup(true)
		, _mycountry(0), _myside(0), _cpside(0), _objective(0)
		, _bestuniticon(0), _altuniticon(0), _unitcount(0)
		, _bestunit(0), _contended(false)
		, _namecolor(0)
		, _cutoff(0)
		, _font(-1), _fontwidth(-1) // FondAdvance has a bit of overhead, so cache
	{
		_selectable=true;
		_straticon.id = -2;
		_visible = true;
		UpdateDetails(cp);
	}

	virtual void Select(SuperCellMapView*);
	virtual void Deselect(SuperCellMapView*);
	virtual void Over(SuperCellMapView* mv, int state);

	// called each frame, but only executes if something has changed
	CPRenderer* _renderer;

	bool _mouseover, _childselected, _mybase;
	unsigned int _lastflagframe, _flaginterval;
	int _lastflagtime, _lastpulsetime;
	float _objectivealpha;
	bool _objectivealphaup;

	// cached variables
	void UpdateDetails(StratDB::StratObject const*);
	StratDB::StratObject  _cp; // cache this data too many cache misses
	UINT8 _mycountry, _myside, _cpside, _objective;
	MapIcon _straticon;
	int _bestuniticon, _altuniticon;
	int _unitcount;
	UINT32 _bestunit;
	bool _contended;
	float* _namecolor;
	float _cutoff;

	typedef std::vector<MapIcon> MapIcons;
	MapIcons _statusicons;

	int _font, _fontwidth;
	phuiDrawString _cpDrawName;
};

class CPMapObject : public StratMapObject
{
public:
	CPMapObject(CPRenderer* p, StratDB::StratObject const* cp, int t)
		: StratMapObject(p,cp,t) {}

	virtual void DrawSelf(SuperCellMapView*);
	virtual void DrawCP(SuperCellMapView*);
	virtual void DrawCPMarker(SuperCellMapView*);
	virtual bool IsHit(SuperCellMapView*, float x, float y) const;
	virtual void Over(SuperCellMapView*, int state);
	bool ObjectVisible() const;

	virtual void Select(SuperCellMapView*);
	virtual void Deselect(SuperCellMapView*);

	phui::ref_ptr<SuperCellMapView::MapObjectList> ChildrenOutsideSuperCell;
};

class CPMapBridgeObject : public StratMapObject
{
public:
	CPMapBridgeObject(CPRenderer* p, StratDB::StratObject const* cp, int t)
		: StratMapObject(p,cp,t) {}

	virtual bool IsHit(SuperCellMapView*, float x, float y) const;
	virtual void DrawSelf(SuperCellMapView*);
	bool ObjectVisible() const;
};

class FacilityMapObject : public StratMapObject
{
public:
	FacilityMapObject(CPMapObject* parent, CPRenderer* p, StratDB::StratObject const* cp, int t)
		: StratMapObject(p,cp,t), _parent(parent) {}

	virtual void DrawSelf(SuperCellMapView*);
	bool ObjectVisible(float scale) const;
	bool IsHit(SuperCellMapView* mv, float x, float y) const;

	virtual void Deselect(SuperCellMapView*);

	CPMapObject* _parent;
};

class CPRenderer : public SuperCellMapView::Renderer, public phui::MessageHandler
{
public:
    CPRenderer();
	~CPRenderer() {};

	virtual SuperCell* NewSuperCell(Renderer* r, int scx, int scy);
	void FilterStrat(const char* name, const unsigned char* value);

	typedef void (*straticontexfn)(StratDB::StratObject const*, bool selected, MapIcon&);
	straticontexfn _straticon;

	virtual void MessageReceived(phui::Message* m, phui::MessageHandler* reply);

	bool showsmallcps, showlargecps, showews, showfront, showrear, showairfields, showports;
	bool showobjectives, showbrigades, showdeployments, showorders, showmydivision;
	bool showcps, showcivilian, showarmy, showairforce, shownavy, showdepots, showbridges;
	bool shownames, showcpnames, showfacnames, showicons, showflags, showclutter;
	bool cpupdateonclick;

	MapIcon groundAttackIconOn, airAttackIconOn, infAttackIconOn, seaAttackIconOn;
	MapIcon groundAttackIconOff, airAttackIconOff, infAttackIconOff, seaAttackIconOff;
	MapIcon heavyGroundAttackIconOn, heavyAirAttackIconOn, heavyInfAttackIconOn, heavySeaAttackIconOn;
	MapIcon heavyGroundAttackIconOff, heavyAirAttackIconOff, heavyInfAttackIconOff, heavySeaAttackIconOff;
	MapIcon hasPortIcon, hasAFIcon, hasRareInfIcon, hasRareGndIcon, hasRareNavyIcon, hasRareAirIcon;
	MapIcon bridgeIcon, haacIcon, fallbackIcon, overrunIcon, contentionIcon;

	const std::wstring& (*tooltip)(const char*);

	int animFlagIcons[MAX_COUNTRIES];
	MapIcon countryFlags[MAX_COUNTRIES];
	MapIcon countryMarkers[MAX_COUNTRIES];

	float playerposx, playerposy;
	float iconsize, iconscale, cluttercutoff;
	float color[3], selectedcolor[3], alliedcolor[3], axiscolor[3];

	bool addhandler;
	float alphajump;
	int alphainterval;

	virtual void RenderSuperCells(SuperCellMapView*, int scx1, int scy1, int scx2, int scy2);
	class SuperCell;

	ww2::Client* client;

	StratMapObject* FindObject(sdm_full_obj_id const&);
	void AddStratObject(StratMapObject*);
	void RemoveStratObject(StratMapObject*);

protected:
	typedef std::map<sdm_lookup_id, phui::ref_ptr<StratMapObject> > ObjectMap;
	ObjectMap _objectmap;
};

#define CPUPDATE_MSG	'CPup'
#define CPTARGET_MSG	'CPtg'
#define CPUPDATE_SX		"sx"
#define CPUPDATE_SY		"sy"
#define CPUPDATE_ID		"id"
#define CPUPDATE_OWNER	"owner"
#define CPUPDATE_FLAGS	"flags"
