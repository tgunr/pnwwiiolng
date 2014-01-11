#include <syscfg.h> // Path is project specific.
#include "gl/glew.h"
#include "gl/glplat.h"

#include "phui/Util.h"
#include "phui/GraphicsContext.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/Message.h"
#include "phui/UIXML.h"
#include "phui/Viewport.h"

#include "uimap/CPRenderer.h"
#include "sdm/StratDB.h"
#include "strt/strtTypes.h"
#include "wwii/wwiiHCUnits.h"
#include "map/mapPersonaSummary.h"
#include "map/mapBrigadeSummary.h"
#include "strt/strtCPFlags.h"
#include "chat/chatBrigadeControlList.h"
#include "ui/uiMain.h"
#include "tut/TutorialManager.h"

#include <algorithm>

static bool operator < (const sdm_lookup_id& a, const sdm_lookup_id& b)
{
	if (a.ox != b.ox)
		return a.ox < b.ox;
	else if(a.oy != b.oy)
		return a.oy < b.oy;

	return a.id < b.id;
}

phuiDrawString::phuiDrawString() : _displayList(0), _font(-1), _str(0), _r(-1), _g(-1), _b(-1), _a(-1)
{
}

phuiDrawString::~phuiDrawString()
{
	if(_displayList > 0)
		glDeleteLists(_displayList,1);
}

void phuiDrawString::DrawString(phui::GraphicsContext* ctx
			, int font, float r, float g, float b, float a, const char* str)
{
//	if(_displayList == 0)
//		_displayList = glGenLists(1);

	//if(_font != font || r != _r || g != _g || b != _b || _str != str)
	{
//		_font = font;
//		_r = r; _g = g; _b = b; _a = a;
//		_str = str;

		//glNewList(_displayList, GL_COMPILE);
		ctx->EnableTexture();
		ctx->SetColor(r,g,b, a);
		ctx->DrawString(font, str);
		//glEndList();
	}

	//glCallList(_displayList);
}

void phuiDrawString::DrawString(phui::GraphicsContext* ctx
			, int font, float r, float g, float b, float a, const wchar_t* str)
{
//	if(_displayList == 0)
//		_displayList = glGenLists(1);

	//if(_font != font || r != _r || g != _g || b != _b || _str != str)
	{
//		_font = font;
//		_r = r; _g = g; _b = b; _a = a;
//		_str = str;

		//glNewList(_displayList, GL_COMPILE);
		ctx->EnableTexture();
		ctx->SetColor(r,g,b, a);
		ctx->DrawString(font, str);
		//glEndList();
	}

	//glCallList(_displayList);
}

class CPRenderer::SuperCell : public SuperCellMapView::Renderer::SuperCell
{
public:
	SuperCell(Renderer* r, int x, int y) : SuperCellMapView::Renderer::SuperCell(r, x, y), _renderer(r) {}
 	virtual ~SuperCell() {}

	virtual void Draw(phui::GraphicsContext*)
	{
		MapObjects()->Draw(_renderer->_mv);
	};

	virtual void Load()
	{
		if(loaded) return;

		//int frametime = renderer->_mv->ParentLoop()->FrameTime();

		StratDB::StratObjList objs;
		StratDB::StratObjList::iterator it;

		StratDB::CPsInSuperCell(objs, scx, scy);
		it = objs.begin();

//		MapObjects()->reserve(objs.size());

		for(; it != objs.end(); ++it)
		{
			StratDB::StratObject* stratobj = *it;
			if(!stratobj) continue;

			StratMapObject* mapobj;

			if(stratobj->StratType() == SDM_CP_BRIDGE)
				mapobj = new CPMapBridgeObject((CPRenderer*)renderer, stratobj, frametime);
			else
				mapobj = new CPMapObject((CPRenderer*)renderer, stratobj, frametime);

			MapObjects()->push_back(mapobj);
			((CPRenderer*)_renderer)->AddStratObject(mapobj);
		}

		loaded = true;
		loading = false;
	}


	Renderer* _renderer;
};

void CPRenderer::AddStratObject(StratMapObject* mapobj)
{
	if(!mapobj)
		return;

	if(_objectmap.find(mapobj->_cp.FullId().lookup_id()) == _objectmap.end())
    {
	    _objectmap[mapobj->_cp.FullId().lookup_id()] = mapobj;
    }
}

void CPRenderer::RemoveStratObject(StratMapObject* mapobj)
{
	ObjectMap::iterator it = _objectmap.find(mapobj->_cp.FullId().lookup_id());
	if(it != _objectmap.end())
		_objectmap.erase(it);
}

/************************************************************************************************
	CPRenderer
*************************************************************************************************/

CPRenderer::CPRenderer()
{
	_straticon = 0;

	showcps = showcivilian = showarmy = showairforce = shownavy = showdepots = showbridges = true;
	shownames = showcpnames = showfacnames = false;
	showicons = showflags = showclutter = true;
	showsmallcps = showlargecps = showews = showfront = showrear = showairfields = showports = true;
	showobjectives = showbrigades = showdeployments = showorders = showmydivision = true;

	playerposx = playerposy = 0;

	iconsize = 32;
	cluttercutoff = 0.0f; // 0 = always show, > 0 is the scale to not show facnames

	color[0] = color[1] = color[2] = 1;									// defaults to white
	selectedcolor[0] = 0; selectedcolor[1] = 1; selectedcolor[2] = 0;	// defaults to green
	alliedcolor[0] = .5f; alliedcolor[1] = .8f; alliedcolor[2] = 1.0f;	// defaults to blue
	axiscolor[0] = 1.0f; axiscolor[1] = .7f; axiscolor[2] = .4f;		// defaults to red

	cpupdateonclick = true;
	addhandler = true;

	alphajump = .2f;
	alphainterval = 50;
	client = 0;
}

void CPRenderer::RenderSuperCells(SuperCellMapView* mv, int scx1, int scy1, int scx2, int scy2)
{
	if(addhandler)
	{
		if(mv->ParentLoop())
			mv->ParentLoop()->AddHandler(this);

		addhandler = false;
	}

	SuperCellMapView::Renderer::RenderSuperCells(mv, scx1, scy1, scx2, scy2);
}

void CPRenderer::FilterStrat(const char* name, const unsigned char* value)
{
	if(strcmp("cpupdateonclick", name) == 0)
		cpupdateonclick = phui::UIXMLbool(value);
	else if(strcmp("showcps", name) == 0)
		showcps = phui::UIXMLbool(value);
	else if(strcmp("showcivilian", name) == 0)
		showcivilian = phui::UIXMLbool(value);
	else if(strcmp("showarmy", name) == 0)
		showarmy = phui::UIXMLbool(value);
	else if(strcmp("showairforce", name) == 0)
		showairforce = phui::UIXMLbool(value);
	else if(strcmp("shownavy", name) == 0)
		shownavy = phui::UIXMLbool(value);
	else if(strcmp("showdepots", name) == 0)
		showdepots = phui::UIXMLbool(value);
	else if(strcmp("showbridges", name) == 0)
		showbridges = phui::UIXMLbool(value);
	else if(strcmp("shownames", name) == 0)
		shownames = showcpnames	= phui::UIXMLbool(value);
	else if(strcmp("showcpnames", name) == 0)
		shownames = showcpnames = phui::UIXMLbool(value);
	else if(strcmp("showfacnames", name) == 0)
		showfacnames = phui::UIXMLbool(value);
	else if(strcmp("showclutter", name) == 0)
		showclutter = true; //phui::UIXMLbool(value);
	else if(strcmp("cluttercutoff", name) == 0)
		cluttercutoff = ((float)atof((char*)value));
	else if(strcmp("showicons", name) == 0)
		showicons = phui::UIXMLbool(value);
	else if(strcmp("showflags", name) == 0)
		showflags = phui::UIXMLbool(value);
	else if(strcmp("showsmallcps", name) == 0) // new filters
		showsmallcps = phui::UIXMLbool(value);
	else if(strcmp("showlargecps", name) == 0)
		showlargecps = phui::UIXMLbool(value);
	else if(strcmp("showews", name) == 0)
		showews = phui::UIXMLbool(value);
	else if(strcmp("showfront", name) == 0)
		showfront = phui::UIXMLbool(value);
	else if(strcmp("showrear", name) == 0)
		showrear = phui::UIXMLbool(value);
	else if(strcmp("showairfields", name) == 0)
		showairfields = phui::UIXMLbool(value);
	else if(strcmp("showports", name) == 0)
		showports = phui::UIXMLbool(value);
	else if(strcmp("showobjectives", name) == 0)
		showobjectives = phui::UIXMLbool(value);
	else if(strcmp("showbrigades", name) == 0)
		showbrigades = phui::UIXMLbool(value);
	else if(strcmp("showdeployments", name) == 0)
		showdeployments = phui::UIXMLbool(value);
	else if(strcmp("showorders", name) == 0)
		showorders = phui::UIXMLbool(value);
	else if(strcmp("showmydivision", name) == 0)
		showmydivision = phui::UIXMLbool(value);
}


SuperCellMapView::Renderer::SuperCell* CPRenderer::NewSuperCell(Renderer* r, int scx, int scy)
{
	return new CPRenderer::SuperCell(r, scx, scy);
}

void StratMapObject::Select(SuperCellMapView* mv)
{
	MapObject::Select(mv);

	if(_renderer->_mv->ParentLoop())
	{
		phui::Message msg(CPTARGET_MSG);
		msg.AddLong(CPUPDATE_SX, _cp.SupercellX());
		msg.AddLong(CPUPDATE_SY, _cp.SupercellY());
		msg.AddLong(CPUPDATE_ID, _cp.Id());
		_renderer->_mv->ParentLoop()->PostMessage(&msg, _renderer->_mv, _renderer);
	}
}

void StratMapObject::Deselect(SuperCellMapView* mv)
{
	MapObject::Deselect(mv);

	phui::Message msg(CPTARGET_MSG);

	if(_renderer->_mv->ParentLoop())
		_renderer->_mv->ParentLoop()->PostMessage(&msg, _renderer->_mv, _renderer);
}

void CPMapObject::Select(SuperCellMapView* mv)
{
	StratMapObject::Select(mv);

	if(_renderer->cpupdateonclick && _renderer->_mv->ParentLoop() && TUT_MGR->AllowStrat())
	{
		phui::Message msg(CPUPDATE_MSG);
		msg.AddLong(CPUPDATE_SX, _cp.SupercellX());
		msg.AddLong(CPUPDATE_SY, _cp.SupercellY());
		msg.AddLong(CPUPDATE_ID, _cp.Id());
		_renderer->_mv->ParentLoop()->PostMessage(&msg, _renderer->_mv, _renderer);
	}

	UpdateDetails(StratDB::FindStratObject(*_cp.FullObjID()));

	StratDB::StratObject::ChildList children = _cp.ChildStratObjects();
	if(children.size() > 0 && (!Children.valid() || Children->size() == 0))
	{
		if(!Children.valid()) Children = new SuperCellMapView::MapObjectList;

		StratDB::StratObject::ChildList::iterator it = children.begin();
//		Children->reserve(children.size());

		for(; it != children.end(); ++it)
		{
			StratDB::StratObject* stratobj = *it;
			FacilityMapObject* mapobj = new FacilityMapObject(this, _renderer, stratobj, 0);
			if(stratobj->SupercellX() == _cp.SupercellX() && stratobj->SupercellY() == _cp.SupercellY())
			{
				Children->push_back(mapobj);
			}else
			{
				if(!ChildrenOutsideSuperCell.valid())
					ChildrenOutsideSuperCell = new SuperCellMapView::MapObjectList;

				ChildrenOutsideSuperCell->push_back(mapobj);
				_renderer->MapObjects()->push_back(mapobj);
			}
			_renderer->AddStratObject(mapobj);
		}
	}
}

void FacilityMapObject::DrawSelf(SuperCellMapView* mv)
{
	if(_straticon.id < 0) return;
	if(!ObjectVisible(mv->Scale())) return;

	glPushMatrix();
	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	float iconscale			= mv->Width() / mv->Scale();
	float iconsize			= _renderer->iconsize / iconscale;
//	unsigned int cpflags	= _cp.StratFlags();
	phui::BoxVerts box;

	if(mv->Scale() > 8) iconscale = mv->Width()/8.0f;

	float r,g,b,a;
	mv->GetBackgroundColor(&r, &g, &b, &a);

	ctx->EnableTexture();
	ctx->SetTexture(_straticon.id);

	float alpha = 1;
	float iconx = (float)_cp.PosX()/SDM_SUPERCELL_SIZE - iconsize/2.0f;
	float icony = (float)_cp.PosY()/SDM_SUPERCELL_SIZE - iconsize/2.0f;

	ctx->SetColor(1.0f,1.0f,1.0f,.8f);

	// only open fb's get to this point
	phui::box_verts_with_pos(box, iconx, icony, iconsize, iconsize);
	phui::draw_gl_quad(box.verts, _straticon.uv);

	if(_renderer->countryFlags[_cp.Country()].id > -1)
	{
		phui::box_verts_with_pos(box, iconx - iconsize/6.0f, icony + iconsize/4.0f, iconsize, iconsize);
		ctx->SetTexture(_renderer->countryFlags[_cp.Country()].id);
		phui::draw_gl_quad(box.verts, _renderer->countryFlags[_cp.Country()].uv);
	}

	if(_selected || _mouseover)
	{
		float* fontcolor = (_selected) ? _renderer->selectedcolor: _namecolor;

		glTranslatef(iconx, icony - iconsize/4,0);
		glScalef(1/iconscale, 1/iconscale,1);

		_cpDrawName.DrawString(ctx, mv->GetFont(), fontcolor[0],fontcolor[1],fontcolor[2], 1.0f, _cp.DisplayName());
	}
	ctx->DisableTexture();
	ctx->SetColor(1,1,1,1);
	glPopMatrix();
}

void FacilityMapObject::Deselect(SuperCellMapView* mv)
{
	StratMapObject::Deselect(mv);

	if(_parent && mv->SelectedMapObject() != _parent)
		_parent->Deselect(mv);
}

void CPMapObject::Deselect(SuperCellMapView* mv)
{
	StratMapObject::Deselect(mv);
	if(!Children.valid()) return;

	bool found = find(Children->begin(), Children->end(), mv->SelectedMapObject()) != Children->end();
	if(!found && ChildrenOutsideSuperCell.valid())
		found = find(ChildrenOutsideSuperCell->begin(), ChildrenOutsideSuperCell->end(), mv->SelectedMapObject()) != ChildrenOutsideSuperCell->end();

	if(!found)
	{
		SuperCellMapView::MapObjectList::iterator it = Children->begin();
		for(;it != Children->end(); ++it)
			_renderer->RemoveStratObject(dynamic_cast<StratMapObject*>(it->ptr()));
		Children->clear();

		if(ChildrenOutsideSuperCell.valid())
		{
			it = ChildrenOutsideSuperCell->begin();
			for(;it != ChildrenOutsideSuperCell->end(); ++it)
			{
				_renderer->RemoveStratObject(dynamic_cast<StratMapObject*>(it->ptr()));
				_renderer->MapObjects()->remove(it->ptr());
			}
		}
	}
}

#define SMALLICON 12.0f
#define BIGICON 32.0f

void CPMapObject::DrawCPMarker(SuperCellMapView* mv)
{
	// draw the cp marker
	if(_straticon.id < 0) return;

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	phui::BoxVerts box;
	float fontheight	= ctx->FontHeight(mv->GetFont());
	float iconscale		= mv->Width() / mv->Scale();
	float iconsize		= (fontheight*.6f) / iconscale;
	float iconbuffer	= iconsize * .25f;

	if(mv->Scale() > 8) iconscale = mv->Width()/8.0f;

	float iconx		= (float)_cp.PosX()/SDM_SUPERCELL_SIZE - iconsize/2.0f;
	float icony		= (float)_cp.PosY()/SDM_SUPERCELL_SIZE - iconsize/2.0f;

	glTranslatef(iconx, icony, 0);

	ctx->EnableTexture();
	ctx->SetColor(1.0f,1.0f,1.0f,(_cpside == 2) ? .45f: .5f);
	ctx->SetTexture(_straticon.id);

	float height = fontheight + (iconbuffer * 4) + iconsize;
	phui::box_verts_with_pos(box, 0, (height/iconscale - iconsize)/2, iconsize, iconsize);
	phui::draw_gl_quad(box, _straticon.uv);

	glTranslatef(-iconx, -icony, 0);
}

// this code is ugly and needs cleaning up - ramp
void CPMapObject::DrawCP(SuperCellMapView* mv)
{
	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	glPushMatrix();
	ctx->DisableTexture();

	unsigned int cpflags	= _cp.StratFlags(true);

	int font			= mv->GetFont();
	float fontheight	= ctx->FontHeight(font);
	float fontwidth		= (float)_fontwidth;

	if(font != _font)
	{
 		_font = font;
        fontwidth = ctx->FontAdvance(font, _cp.DisplayName());// this is also expensive
		_fontwidth = (int)fontwidth;
	}

	float iconscale		= mv->Width() / mv->Scale();
	float iconsize		= (fontheight*.6f) / iconscale;
	float iconbuffer	= iconsize * .25f;
	float bigiconsize	= BIGICON / iconscale;

	if(mv->Scale() > 8) iconscale = mv->Width()/8.0f;

//	float alpha		= 1;
	float iconx		= (float)_cp.PosX()/SDM_SUPERCELL_SIZE - iconsize/2.0f;
	float icony		= (float)_cp.PosY()/SDM_SUPERCELL_SIZE - iconsize/2.0f;
	float scaled	= 1/iconscale;

	glTranslatef(iconx, icony, 0);
	glPushMatrix();
	glScalef(scaled, scaled, 1);

	// DRAW THE OBJECTIVE BACKGROUND
	float malpha		= (_selected || _mouseover || cpflags & CP_STAT_ENEMY_LINKED) ? 1.0f: .8f;
	float* fontcolor	= (_selected) ? _renderer->selectedcolor: _namecolor;

	// draw the bg color box
	float bgx	= (_bestunit) ? -(bigiconsize + iconbuffer*2): -iconbuffer;
	float bgy	= -((iconbuffer * 3) + iconsize);
	float bgh	= fontheight + (iconbuffer * 4) + iconsize;
	float bgw	= (_bestunit) ? bigiconsize + fontwidth + iconbuffer*3: fontwidth + iconbuffer*2;

	phui::BoxVerts box;
	phui::box_verts_with_pos(box, bgx - 5, bgy, bgw + 34, bgh);

	if(_contended)
		ctx->SetColor(.5f,.5f,0.0f,.7f);
	else
		ctx->SetColor(.3f,.3f,.3f,(_bestunit) ? 1.0f: .6f);

	ctx->DrawQuad(box.verts);

	// DRAW THE CP NAME
	glTranslatef(19.0f, 0.0f, 0.0f);
	_cpDrawName.DrawString(ctx, font, fontcolor[0],fontcolor[1],fontcolor[2], malpha, _cp.DisplayName());

	glPopMatrix();
	ctx->SetColor(1.0f,1.0f,1.0f,1.0f);

	float offsetx = 0;

	// draw the cp marker
	if(_straticon.id > -1)
	{
		ctx->SetTexture(_straticon.id);
		phui::box_verts_with_pos(box, offsetx, (bgh/iconscale - iconsize)/2, iconsize, iconsize);
		phui::draw_gl_quad(box.verts, _straticon.uv);
	}

	float offsety = -(iconsize + iconbuffer*2);

	offsetx		= -iconbuffer;
	iconsize	*= 1.5f;

	for(int i = 0; i < (int)_statusicons.size(); ++i)
	{
		MapIcon& icon = _statusicons[i];
		ctx->SetTexture(icon.id);
		phui::box_verts_with_pos(box, offsetx, offsety, iconsize, iconsize);
		phui::draw_gl_quad(box.verts, icon.uv);
		offsetx += iconsize;
	}

	// draw the brigade icon
	if(_bestuniticon > -1 && !TUT_MGR->IsActive())
	{
		bigiconsize	= (64.0f/iconscale)/1.5f;

		float ugx = -iconbuffer*2.5f;
		float ugy = 0;
		float ugw = bigiconsize;
		float ugh = bigiconsize;

		float translatedx = 0.0f;
		float translatedy = 0.0f;

		float frames[10][8] = {
			{0,.75,.25,.75,.25,1,0,1},
			{.25,.75,.5,.75,.5,1,.25,1},
			{.5,.75,.75,.75,.75,1,.5,1},
			{.75,.75,1,.75,1,1,.75,1},
			{0,.5,.25,.5,.25,.75,0,.75},
			{.25,.5,.5,.5,.5,.75,.25,.75},
			{.5,.5,.75,.5,.75,.75,.5,.75},
			{.75,.5,1,.5,1,.75,.75,.75},
			{0,.25,.25,.25,.25,.5,0,.5},
			{.25,.25,.5,.25,.5,.5,.25,.5}
		};

		//int frametime = mv->ParentLoop()->FrameTime();

		if(frametime >= _lastflagtime)
		{
			_lastflagtime = frametime + _flaginterval;

			if(++_lastflagframe > 9)
				_lastflagframe = 0;
		}

		ctx->SetColor(1,1,1,1);

		ctx->SetTexture(_bestuniticon);
		phui::box_verts_with_pos(box, ugx, ugy, ugw, ugh);
		phui::draw_gl_quad(box.verts, frames[_lastflagframe]);

		if(_altuniticon)
		{
			float auiOffset = 1.0f/iconscale;
			glTranslatef(-auiOffset*2, auiOffset*33, 0.0f);
			glScalef(.5, .5, 0.0f);
			ctx->SetTexture(_altuniticon);
			phui::box_verts_with_pos(box, ugx, ugy, ugw, ugh);
			phui::draw_gl_quad(box.verts, frames[_lastflagframe]);
			glScalef(2.0f, 2.0f, 0.0f);
			glTranslatef(auiOffset*2, -auiOffset*33, 0.0f);
		}

		if(_cpside == 1)
			ctx->SetColor(0.0f,0.0f,1.0f,.8f);
		else
			ctx->SetColor(1.0f,0.0f,0.0f,.8f);

		ctx->SetColor(_namecolor[0],_namecolor[1],_namecolor[2],1.0f);
		ctx->DisableTexture();

		ugx = -iconbuffer * 4.0f;
		ugy = bigiconsize - iconbuffer * 2.0f;

		translatedx += ugx;
		translatedy += ugy;

		glTranslatef(ugx, ugy, 0.0f);

		unsigned int count = _cp.GetUnitCount();
		unsigned int limit = std::min(5U, count);
		for(unsigned int s = 0; s < limit; ++s)
		{
			ctx->DrawCircle(iconbuffer, true);
			ugy -= iconbuffer;
			glTranslatef(0.0f, -iconbuffer*3.0f, 0.0f);
			translatedy += -iconbuffer*3.0f;
		}

		glTranslatef(-translatedx, -translatedy, 0.0f);
	}

	// draw the objective box
	if(_objective)
	{
		//int frametime = mv->ParentLoop()->FrameTime();

		if(frametime >= _lastpulsetime + _renderer->alphainterval)
		{
			_lastpulsetime = frametime + _renderer->alphainterval;
			_objectivealpha += (_objectivealphaup) ? _renderer->alphajump: -_renderer->alphajump;

			if(_objectivealpha >= 1)
			{
				_objectivealpha = 1;
				_objectivealphaup = false;
			}
			else if(_objectivealpha <= 0)
			{
				_objectivealpha = _renderer->alphajump;
				_objectivealphaup = true;
			}
		}

		glScalef(scaled, scaled, 1);
		phui::box_verts_with_pos(box, bgx - 5, bgy, bgw + 34, bgh);

		if(_objective == 1)
			ctx->SetColor(1.0f,.3f,.3f,_objectivealpha);
		else
			ctx->SetColor(.3f,1.0f,.3f,_objectivealpha);

		ctx->DisableTexture();
		ctx->DrawLineStrip(4, box.verts, true, 2);
	}
	ctx->SetColor(1,1,1,1);

	glPopMatrix();
}


void StratMapObject::UpdateDetails(StratDB::StratObject const* cp)
{
	_cp = *cp;

	unsigned int cpflags = _cp.StratFlags(true);

	// init cache variables
	_mycountry		= _renderer->client ? _renderer->client->PersonaSelected().m_countryID: 0;
	_myside			= _mycountry ? (_mycountry == 4) ? 2: 1: 0;
	_cpside			= (_cp.Country() == 4) ? 2: 1;
	_namecolor		= (_cpside == 1) ? _renderer->alliedcolor: _renderer->axiscolor;
	_cutoff			= 0;
	_objective		= 0;
	_bestunit		= 0;
	_bestuniticon	= -1;
	_unitcount		= 0;
	_contended		= false;
	_mybase			= false; // is my division represented here?

	_statusicons.clear();

	// cp and bridge objectives
	if(_cp.StratClass() == SDM_CLASS_CP)
	{
		if((_myside == ALLIED && cpflags & CP_STAT_ALLIED_ATTACK_OBJ) ||
			(_myside == AXIS && cpflags & CP_STAT_AXIS_ATTACK_OBJ)) _objective = 1;
		else if((_myside == ALLIED && cpflags & CP_STAT_ALLIED_DEFEND_OBJ) ||
			(_myside == AXIS && cpflags & CP_STAT_AXIS_DEFEND_OBJ)) _objective = 2;
	}

	if(_cp.StratClass() == SDM_CLASS_CP && _cp.StratType() != SDM_CP_BRIDGE)
	{
		// get the cp's units list
		const StratDB::StratObject::UnitList& units = _cp.GetUnits();

		// cp marker
		_straticon = _renderer->countryMarkers[_cp.Country()];

		// brigade count
		_unitcount = (unsigned int)units.size();

		// contention
		_contended = cpflags & CP_STAT_CONTENTION;

		// best brigade
		_bestunit = _cp.FindBestUnit(_myside, _mycountry, 0);

		// other unit here?
		_altuniticon = 0;

		if(_bestunit)
		{
			HCUnit* unit = HCUnit::Lookup(_bestunit);

			if(unit)
			{
				UINT32 bestAlliedUnit = _cp.FindBestAlliedUnit(unit->side(), unit->country());
				HCUnit* alliedUnit = HCUnit::Lookup(bestAlliedUnit);

				switch(unit->country())
				{
					case WWII_ENGLAND:
					case WWII_USA:
					case WWII_FRANCE:
					{
						_bestuniticon = _renderer->animFlagIcons[unit->country()];

						if(alliedUnit)
							_altuniticon = _renderer->animFlagIcons[alliedUnit->country()];
					}
					break;
					case WWII_GERMANY:
						_bestuniticon = _renderer->animFlagIcons[WWII_GERMANY];
					break;
				}
			}
		}

		// see if my division is represented here
		if(_unitcount && _renderer->client)
		{
			const BrigadeSelection& current = _renderer->client->BrigadeSelected();

			if(current.m_detail.brigadeID())
			{
				HCUnit* myBrigade = HCUnit::Lookup(current.m_detail.brigadeID());
				HCUnit* myDivision = myBrigade->parent();

				if(myDivision)
				{
					for(int i = 0; i < _unitcount; ++i)
					{
						HCUnit* deployed = HCUnit::Lookup(units[i]);

						if(deployed && deployed->unitID() == myDivision->unitID() ||
									deployed->parentID() == myDivision->unitID())
						{
							_mybase = true;
							break;
						}
					}
				}
			}
		}

		bool isDeepWaterPort = _cp.IsDeepWaterPort();

		// status icons in order
		if(isDeepWaterPort || cpflags & (CP_STAT_HAS_NAVYBASE | CP_STAT_HAS_RARE_NAVY))
		{
			_statusicons.push_back((isDeepWaterPort || (cpflags & CP_STAT_HAS_RARE_NAVY)) ? _renderer->hasRareNavyIcon : _renderer->hasPortIcon);
		}

		if(cpflags & (CP_STAT_HAS_AIRBASE | CP_STAT_HAS_RARE_AIR))
		{
			_statusicons.push_back((cpflags & CP_STAT_HAS_RARE_AIR) ? _renderer->hasRareAirIcon : _renderer->hasAFIcon);
		}

		if(cpflags & CP_STAT_HAS_RARE_INF)
		{
			_statusicons.push_back(_renderer->hasRareInfIcon);
		}

		if(cpflags & CP_STAT_OWN_HAAC)
		{
			_statusicons.push_back(_renderer->haacIcon);
		}
		else if(cpflags & CP_STAT_FALLBACK)
		{
			_statusicons.push_back(_renderer->fallbackIcon);
		}
		else if(cpflags & CP_STAT_OVERRUN)
		{
			_statusicons.push_back(_renderer->overrunIcon);
		}
		if(_contended)
		{
			_statusicons.push_back(_renderer->contentionIcon);
		}

		if(cpflags & CP_STAT_HEAVY_ENEMY_INF)
		{
			_statusicons.push_back(_renderer->infAttackIconOn);
		}
		else if(cpflags & CP_STAT_ENEMY_INFANTRY)
		{
			_statusicons.push_back(_renderer->infAttackIconOff);
		}

		if(cpflags & CP_STAT_HEAVY_ENEMY_ARMOR)
		{
			_statusicons.push_back(_renderer->groundAttackIconOn);
		}
		else if(cpflags & CP_STAT_ENEMY_ARMOR)
		{
			_statusicons.push_back(_renderer->groundAttackIconOff);
		}

		if(cpflags & CP_STAT_HEAVY_ENEMY_SEA)
		{
			_statusicons.push_back(_renderer->seaAttackIconOn);
		}
		else if(cpflags & CP_STAT_ENEMY_SEACRAFT)
		{
			_statusicons.push_back(_renderer->seaAttackIconOff);
		}

		/*
		if(cpflags & CP_STAT_HEAVY_ENEMY_AIR)
		{
			_statusicons.push_back(_renderer->airAttackIconOn);
		}
		else if(cpflags & CP_STAT_ENEMY_AIRCRAFT)
		{
			_statusicons.push_back(_renderer->airAttackIconOff);
		}
		*/

		if((cpflags & CP_STAT_SURRENDER_CANDIDATE) && _renderer->animFlagIcons[WWII_NO_COUNTRY] >= 0)
		{
			if(_bestuniticon >= 0)
				_altuniticon = _renderer->animFlagIcons[WWII_NO_COUNTRY];
			else
				_bestuniticon = _renderer->animFlagIcons[WWII_NO_COUNTRY];
		}
	}
	else
	{
		// facility marker
		if(_straticon.id == -2)
			_renderer->_straticon(&_cp, _selected,_straticon);

		if(_cp.StratClass() == SDM_CLASS_FACILITY)
			_cutoff	= 1.3f; //(_cp.ParentStratObject() && _cp.ParentStratObject()->StratType() > SDM_CP_SMALL) ? 1.3f: .4f;
	}
}

// this code is ugly and needs cleaning up - ramp
void CPMapBridgeObject::DrawSelf(SuperCellMapView* mv)
{
	if(_straticon.id < 0) return;
	if(!ObjectVisible()) return;

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	ctx->EnableTexture();
	ctx->SetTexture(_straticon.id);
	glPushMatrix();
	ctx->SetColor(1.0f,1.0f,1.0f,1.0f);

	float iconscale = mv->Width() / mv->Scale();
	float iconsize = _renderer->iconsize / iconscale;
	phui::BoxVerts box;

	if(mv->Scale() > 8)
		iconscale = mv->Width()/8.0f;

	float iconx = (float)_cp.PosX()/SDM_SUPERCELL_SIZE - iconsize/2.0f;
	float icony = (float)_cp.PosY()/SDM_SUPERCELL_SIZE - iconsize/2.0f;

	// bridge
	phui::box_verts_with_pos(box, iconx, icony, iconsize, iconsize);
	phui::draw_gl_quad(box.verts, _straticon.uv);

	glTranslatef(iconx, icony, 0);
	glScalef(1/iconscale, 1/iconscale, 1);

	if(_selected || _mouseover)
	{
//		float* fontcolor = (_selected) ? _renderer->selectedcolor: _renderer->color;

		glTranslatef(0, -(iconsize/4),0);
		_cpDrawName.DrawString(ctx, mv->GetFont(), 0,1,0,1, _cp.DisplayName());

		glTranslatef(0, iconsize/4,0);
	}
	ctx->DisableTexture();
	ctx->SetColor(1,1,1,1);

	if(_objective)
	{
		//int frametime = mv->ParentLoop()->FrameTime();
		if(frametime >= _lastpulsetime + _renderer->alphainterval)
		{
			_lastpulsetime = frametime + _renderer->alphainterval;
			_objectivealpha += (_objectivealphaup) ? _renderer->alphajump: -_renderer->alphajump;

			if(_objectivealpha >= 1)
			{
				_objectivealpha = 1;
				_objectivealphaup = false;
			}
			else if(_objectivealpha <= 0)
			{
				_objectivealpha = _renderer->alphajump;
				_objectivealphaup = true;
			}
		}

		float obox = iconsize*iconscale;

		phui::box_verts_with_pos(box, 0, 0, obox - obox/4, obox - obox/2);

		if(_objective == 1)
			ctx->SetColor(1.0f,.3f,.3f,_objectivealpha);
		else
			ctx->SetColor(.3f,1.0f,.3f,_objectivealpha);

		glTranslatef(5, 9, 0);

		ctx->DisableTexture();
		ctx->DrawLineStrip(4, box.verts, true, 2);
	}
	glPopMatrix();
}

bool CPMapBridgeObject::IsHit(SuperCellMapView* mv, float x, float y) const
{
	if(!ObjectVisible()) return false;

	float iconscale = mv->Width() / mv->Scale();
	float bottom	= 0.0f;
	float left		= 0.0f;
	float top		= 0.0f;
	float right		= 0.0f;

	if(mv->Scale() > 10)
		iconscale = mv->Width()/10.0f;

	bottom	= _cp.PosY() / SDM_SUPERCELL_SIZE - _renderer->iconsize/iconscale/2.0f;
	left	= _cp.PosX() / SDM_SUPERCELL_SIZE - _renderer->iconsize/iconscale/2.0f;
	top		= bottom + _renderer->iconsize/iconscale;
	right	= left + _renderer->iconsize/iconscale;

	return (x >= left && x <= right && y >= bottom && y <= top);
}

bool CPMapObject::ObjectVisible() const
{
	unsigned int cpflags = _cp.StratFlags(true);
	return _mouseover || _selected || _childselected || _renderer->showcps ||
		   (_renderer->showews && cpflags & (CP_STAT_CONTENTION | CP_EWS_FLAG_MASK)) ||
		   (_renderer->showfront && cpflags & CP_STAT_ENEMY_LINKED) ||
		   (_renderer->showrear && !(cpflags & CP_STAT_ENEMY_LINKED)) ||
		   (_renderer->showobjectives && cpflags & CP_OBJECTIVES) ||
		   (_renderer->showbrigades && _bestunit) ||
		   (_renderer->showairfields && cpflags & CP_STAT_HAS_AIRBASE) ||
		   (_renderer->showports && cpflags & CP_STAT_HAS_NAVYBASE) ||
		   (_renderer->showdeployments && cpflags & (CP_STAT_HAS_RARE_NAVY | CP_STAT_HAS_RARE_AIR | CP_STAT_HAS_RARE_INF)) ||
		   (_renderer->showorders && cpflags & (CP_STAT_OWN_HAAC | CP_STAT_FALLBACK | CP_STAT_OVERRUN)) ||
		   (_renderer->showmydivision && _mybase);
}

bool CPMapBridgeObject::ObjectVisible() const
{
	return  _renderer->showbridges ||
			(_renderer->showobjectives && _cp.StratFlags(true) & CP_OBJECTIVES);
}

void CPMapObject::DrawSelf(SuperCellMapView* mv)
{
	SuperCellMapView::MapObject::DrawSelf(mv);

	if(ObjectVisible())
		DrawCP(mv);
	else
		DrawCPMarker(mv);
}

bool FacilityMapObject::ObjectVisible(float scale) const
{
	if(_cp.StratType() == SDM_FACILITY_FIREBASE)
	{
		return _cp.StratFlags(true) & CP_STAT_OPEN;
	}else
	{
		return _renderer->showclutter && (!_cutoff || scale <= _cutoff);
	}
}

bool FacilityMapObject::IsHit(SuperCellMapView* mv, float x, float y) const
{
	if(!ObjectVisible(mv->Scale())) return false;

	float iconscale = mv->Width() / mv->Scale();
	float bottom	= 0.0f;
	float left		= 0.0f;
	float top		= 0.0f;
	float right		= 0.0f;

	if(mv->Scale() > 10)
		iconscale = mv->Width()/10.0f;

	bottom	= _cp.PosY() / SDM_SUPERCELL_SIZE - _renderer->iconsize/iconscale/2.0f;
	left	= _cp.PosX() / SDM_SUPERCELL_SIZE - _renderer->iconsize/iconscale/2.0f;
	top		= bottom + _renderer->iconsize/iconscale;
	right	= left + _renderer->iconsize/iconscale;

	return (x >= left && x <= right && y >= bottom && y <= top);
}


bool CPMapObject::IsHit(SuperCellMapView* mv, float x, float y) const
{
	if(!ObjectVisible()) return false;

	float iconscale = mv->Width() / mv->Scale();
	float bottom	= 0.0f;
	float left		= 0.0f;
	float top		= 0.0f;
	float right		= 0.0f;

	if(mv->Scale() > 10)
		iconscale = mv->Width()/10.0f;

	phui::GraphicsContext* ctx	= mv->GetGraphicsContext();
	int font					= mv->GetFont();
	float fontheight			= ctx->FontHeight(font);
	float fontwidth				= (float)_fontwidth;//ctx->FontAdvance(font, _cp.Name());
	float iconsize				= (fontheight*.6f) / iconscale;
	float iconbuffer			= iconsize * .25f;

	bottom	= _cp.PosY() / SDM_SUPERCELL_SIZE - (iconsize+5)/iconscale/2.0f;
	left	= _cp.PosX() / SDM_SUPERCELL_SIZE - (iconsize + iconbuffer + 10)/iconscale/2.0f;
	top		= bottom + (fontheight + (iconbuffer * 4) + iconsize)/iconscale;
	right	= left + (fontwidth + iconbuffer*2 + 34)/iconscale;

	return (x >= left && x <= right && y >= bottom && y <= top);
}

void StratMapObject::Over(SuperCellMapView* mv, int state)
{
	_mouseover = (state != phui::MOUSE_EXIT);
}

void CPMapObject::Over(SuperCellMapView* mv, int state)
{
	bool wasover = _mouseover;

	_mouseover = (state != phui::MOUSE_EXIT);

	if(wasover != _mouseover)
	{
		if(_mouseover && _renderer->tooltip)
		{
			std::wstring tip;

			int objective		= 0;
			int cpflags			= _cp.StratFlags(true);
			int side			= _renderer->client ? (_renderer->client->PersonaSelected().m_countryID == 4) ? 2:1 : 0;
			UINT32 myBrigadeID	= _renderer->client ? _renderer->client->BrigadeSelected().m_detail.brigadeID() : 0;

			if((side == ALLIED && cpflags & CP_STAT_ALLIED_ATTACK_OBJ) ||
			   (side == AXIS && cpflags & CP_STAT_AXIS_ATTACK_OBJ)) objective = 1;
			else if((side == ALLIED && cpflags & CP_STAT_ALLIED_DEFEND_OBJ) ||
			    (side == AXIS && cpflags & CP_STAT_AXIS_DEFEND_OBJ)) objective = 2;

			switch(_cp.Country())
			{
				case WWII_ENGLAND:
					tip += _renderer->tooltip("widgets.map.tooltip.town.british");
					break;
				case WWII_USA:
					tip += _renderer->tooltip("widgets.map.tooltip.town.american");
					break;
				case WWII_FRANCE:
					tip += _renderer->tooltip("widgets.map.tooltip.town.french");
					break;
				case WWII_GERMANY:
					tip += _renderer->tooltip("widgets.map.tooltip.town.german");
					break;
				default:
					tip += _renderer->tooltip("widgets.map.tooltip.town.neutral");
					break;
			}

			tip += L"\n";

			if(objective)
			{
				switch(objective)
				{
					case 1:
						tip += _renderer->tooltip("widgets.map.tooltip.objective.attack");
						break;
					case 2:
						tip += _renderer->tooltip("widgets.map.tooltip.objective.defend");
						break;
				}

				tip += L"\n";
			}

			if(cpflags & CP_STAT_OWN_HAAC)
			{
				tip += _renderer->tooltip("widgets.map.tooltip.orders.haac");
				tip += L"\n";
			}
			else if(cpflags & CP_STAT_FALLBACK)
			{
				tip += _renderer->tooltip("widgets.map.tooltip.orders.fallback");
				tip += L"\n";
			}
			else if(cpflags & CP_STAT_OVERRUN)
			{
				tip += _renderer->tooltip("widgets.map.tooltip.orders.overrun");
				tip += L"\n";
			}

			if(cpflags & CP_STAT_CONTENTION)
			{
				tip += _renderer->tooltip("widgets.map.tooltip.contested");
				tip += L"\n";
			}

			if(cpflags & (CP_STAT_ENEMY_ARMOR |
						CP_STAT_HEAVY_ENEMY_ARMOR |
						CP_STAT_ENEMY_INFANTRY |
						CP_STAT_HEAVY_ENEMY_INF |
						CP_STAT_ENEMY_SEACRAFT |
						CP_STAT_HEAVY_ENEMY_SEA |
						CP_STAT_ENEMY_AIRCRAFT |
						CP_STAT_HEAVY_ENEMY_AIR))
			{
				tip += _renderer->tooltip("widgets.map.tooltip.enemy");
				tip += L"\n";
			}

			const StratDB::StratObject::UnitList& units = _cp.GetUnits();

			if(units.size() > 0 && !TUT_ACTIVE)
			{
				tip += L"\n";
				tip += _renderer->tooltip("widgets.map.tooltip.units");
				tip += L"\n";

				HCUnit* unit			= nullptr;
				BrigadeControlList* oic	= nullptr;
				unsigned int current	= 0;
				bool foundMine			= false;

				for(int i = 0; i < (int)units.size(); ++i)
				{
					unit = HCUnit::Lookup(units[i]);

					if(unit && !unit->isTrainingUnit())
					{
						HCUnit* division = (unit->isBrigade()) ? unit->parent(): unit;

						if(division && division->unitID() != current)
						{
							current = division->unitID();
							tip += uiUTF8toUTF16(division->m_name);
							switch(division->country())
							{
								case WWII_ENGLAND:
									tip += L" - UK";
								break;
								case WWII_USA:
									tip += L" - US";
								break;
								case WWII_FRANCE:
									tip += L" - FR";
								break;
								case WWII_GERMANY:
									tip += L" - DE";
								break;
							}
							tip += L"\n";
						}

						if(unit->unitID() == myBrigadeID)
						{
							foundMine = true;
							tip += L"*";
						}

						if(unit->isBrigade())
						{
							tip += L"     ";
							tip += uiUTF8toUTF16(unit->m_name);
						}
						else
						{
							tip += L"     ";
							tip += _renderer->tooltip("widgets.map.tooltip.divhq");
						}

						oic = _renderer->client ? _renderer->client->LookupBrigadeOIC(unit->unitID()) : 0;
						if(oic && oic->m_officer)
						{
							tip += L" - OIC: ";
							tip += chatGetWideCallsign(oic->m_officer);
						}

						tip += L"\n";
					}
				}

				if(foundMine)
				{
					tip += L"\n";
					tip += _renderer->tooltip("widgets.map.tooltip.yours");
					tip += L"\n";
				}
			}

			mv->SetToolTip(tip.c_str());
		}
		else
		{
			mv->SetToolTip(L"");
			mv->ParentViewport()->HideToolTip();
		}
	}
}

void CPRenderer::MessageReceived(phui::Message* msg, phui::MessageHandler* reply)
{
	if(msg->what == CPUPDATE_MSG)
	{
		sdm_full_obj_id fullid;
		StratDB::StratObject* stratobj;

		for(int i = 0;; i++)
		{
			long sx,sy,id;

			if(msg->FindLong(CPUPDATE_ID, &id,i) != 0) break;
			msg->FindLong(CPUPDATE_SX, &sx,i);
			msg->FindLong(CPUPDATE_SY, &sy,i);

			fullid.cell.ox = (SINT16)(sx << 4);
			fullid.cell.oy = (SINT16)(sy << 4);
			fullid.id = (UINT16)id;

			stratobj = StratDB::FindStratObject(fullid);
			if(!stratobj) continue;

			StratMapObject* mapobj = FindObject(fullid);
			if(mapobj)
				mapobj->UpdateDetails(stratobj);
		}
	}

	phui::MessageHandler::MessageReceived(msg, reply);
}

StratMapObject* CPRenderer::FindObject(sdm_full_obj_id const& id)
{
	ObjectMap::iterator it = _objectmap.find(id.lookup_id());
	if(it == _objectmap.end()) return 0;

	return it->second.ptr();
}

