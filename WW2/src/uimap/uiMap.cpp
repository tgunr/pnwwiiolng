#include <syscfg.h> // Path is project specific.
#include <string>
#include <math.h>

#include "phui/View.h"
#include "phui/UIXML.h"
#include "phui/Util.h"
#include "phui/GraphicsContext.h"
#include "phui/Message.h"
#include "phui/MessageFilter.h"
#include "phui/MessageLoop.h"
#include "phui/UIMessages.h"
#include "phui/KeyCodes.h"
#include "phui/UIMessages.h"
#include "phui/CSSViewDrawer.h"

#include "uimap/uiMap.h"
#include "ui/uiMain.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "sdm/sdm.h"
#include "trn/trnmain.h"
#include "imap/imap.h"
#include "fio/fio.h"
#include "strt/strtTypes.h"
#include "pers/persmain.h"

#include "uimap/SuperCellMapView.h"
#include "uimap/IconMapRenderer.h"
#include "uimap/TerrainObjectRenderer.h"
#include "uimap/CPRenderer.h"
#include "uimap/TopoMapRenderer.h"
#include "uimap/MapTextureRenderer.h"
#include "uimap/ScenarioMapRenderer.h"
#include "sdm/StratDB.h"
#include "sdm/sdmReplacementOctedData.h"
#include "ui/uiMain.h"
#include "chat/chatClientMessages.h"
#include "trn/trnmain.h"
#include "js/jsMapView.h"
#include "fp/fpFPS.h"
#include "strt/strtClientMessages.h"
#include "client/GameClient.h"
#include "map/mapPersonaSummary.h"
#include "uimap/PlayerLayer.h"
#include "uimap/WaypointLayer.h"
#include "uimap/StrategicLayer.h"
#include "math/math3d.h"
#include "pers/pershud.h"
#include "pi/piTextureAtlas.h"
#include "uimap/AWSLayer.h"
#include "ogl/ogl.h"
#include "chat/chatWaypointExchg.h"
#include "tut/TutorialManager.h"
#include "trn/trncd.h"
#include "cfg/cfgPrefs.h"
#include "uimap/CustomMapLayer.h"
#include "scen/ScenarioManager.h"
#include "wwii/wwiiHash.h"			// For unordered map

#include "include/vars.h"

#include "libxml/tree.h"

#if platOS == OS_WIN32
#include <unordered_map>
#define UNORDERED_MAP std::unordered_map
#else
#define UNORDERED_MAP stdext::hash_map
#endif

bool _ingamemapon	= false;
bool _ingamemaplock = false;

#define SQR(x) ((x)*(x))

// OSG: PI is already defined. Works for debug, but not release. This fixes it for now.
#define _pi 3.1415926535

/************************************************************************************************
	uiMap
*************************************************************************************************/

class PlayerMapObject : public SuperCellMapView::MapObject
{
public:
	PlayerMapObject();
	virtual ~PlayerMapObject() {}

	virtual void DrawSelf(SuperCellMapView*);

	int _navring, _navtarget, _navarrow;
	float _navarrowsize, _arrowscale, _arrowradius;
	bool _shownavring;
};

class ScaleMapObject : public SuperCellMapView::MapObject
{
public:
	ScaleMapObject();
	virtual ~ScaleMapObject() {}

	virtual void DrawSelf(SuperCellMapView*);

	int _scalebar;
};

struct trnobjtextureinfo
{
	trnobjtextureinfo() { texInfo.texid = -2; x = y = width = height = 0; mpp = 1; }
	piTextureInfo texInfo;
	int x, y, width, height;
	float mpp;
};

class MapMessageFilter : public phui::MessageFilter
{
public:
	MapMessageFilter() : phui::MessageFilter(phui::ANY_EVENT)
	{
	}

	~MapMessageFilter()
	{
		chatTargetSelection(nullObjID);
	}

	virtual bool Filter(phui::Message* msg, phui::MessageHandler** target,  phui::MessageHandler** reply);
};

class MapInputFilter : public phui::MessageFilter
{
public:
	MapInputFilter(SuperCellMapView* mv) : phui::MessageFilter(phui::ANY_EVENT), _mv(mv)
	{
	}

	~MapInputFilter()
	{
	}

	virtual bool Filter(phui::Message* msg, phui::MessageHandler** target,  phui::MessageHandler** reply);

private:
	SuperCellMapView* _mv;
};


//static std::map<std::string, trnobjtextureinfo> _trnobjtexturemap;
static UNORDERED_MAP<std::string, trnobjtextureinfo> _trnobjtexturemap;

static FULL_OBJ_ID _facOrigin, _missionTarget;

static int get_rendered_octet_icon(IconMapRenderer*, const char* filename, float uv[4]);
static int get_icon_octet_icon(IconMapRenderer*, const char* filename, float uv[4]);
static int get_terrain_data(int sx, int sy, sdm_replacement_octet_data* data);
static int get_trnobj_texturedata(SDM_ObjInfo const* info, int* texid, int* x, int* y, int* width, int* height, float* uv);
static void get_strat_icon(StratDB::StratObject const* stratobj, bool selected, MapIcon&);
static int get_render_texture(int col, int row);
static int get_sdm_data(int scx, int scy, sdm_dtl_data* dtl, sdm_replacement_octet_data* replace);
static void add_player_map_event(const char* mapname, ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data, int expires);

/************************************************************************************************
	uiMapCreate
*************************************************************************************************/

phui::View* uiMapCreate(_xmlNode* xmlnode, phui::View* parent)
{
	SuperCellMapView* mapview		= new SuperCellMapView;
	MapInputFilter* inputFilter		= new MapInputFilter(mapview);
	MapMessageFilter* stratFilter	= new MapMessageFilter;

	// add out map filters
	mapview->AddMessageFilter(stratFilter);
	mapview->AddMessageFilter(inputFilter);

	// we also add the input filter to the main message loop so it will
	// get handled; its only added to the mapview so the mapview will
	// remove it from the main message loop when the mapview is deleted
	uiMessageLoop()->AddMessageFilter(inputFilter);

	if(Online)
	{ // give reasonable starting position
		FLOAT64VECTOR fvecPlayerPos;
		FLOAT64VECTOR vecWorldOriginOffset;
		trnGetWorldOriginOffset(&vecWorldOriginOffset);
		fvecPlayerPos.x = -playpos.x + vecWorldOriginOffset.x;
		fvecPlayerPos.y = -playpos.y + vecWorldOriginOffset.y;
		fvecPlayerPos.z = -playpos.z + vecWorldOriginOffset.z;

		mapview->SetPositionXY((float)fvecPlayerPos.x, (float)fvecPlayerPos.y);
	}else
	{
		mapview->SetPositionSuperCell(18,235);
	}

	if(xmlnode)
	{
		xmlChar* val;

		float sx=-1000, sy=-1000;
		val = xmlGetProp(xmlnode, (xmlChar*)"sx");
		if(val)
		{
			sx = (float)atof((char*)val);
			xmlFree(val);
		}

		val = xmlGetProp(xmlnode, (xmlChar*)"sy");
		if(val)
		{
			sy = (float)atof((char*)val);
			xmlFree(val);
		}

		if(sx > -1000 && sy > -1000)
			mapview->SetPositionSuperCell(sx, sy);
		val = xmlGetProp(xmlnode, (xmlChar*)"minscale");
		if(val)
		{
			mapview->SetMinScale((float)atof((char*)val));
			xmlFree(val);
		}

		val = xmlGetProp(xmlnode, (xmlChar*)"maxscale");
		if(val)
		{
			mapview->SetMaxScale((float)atof((char*)val));
			xmlFree(val);
		}

		val = xmlGetProp(xmlnode, (xmlChar*)"scale");
		if(val)
		{
			mapview->SetScale((float)atof((char*)val));
			xmlFree(val);
		}
		val = xmlGetProp(xmlnode, (xmlChar*)"navscale");
		if(val)
		{
			mapview->SetNavScale((float)atof((char*)val));
			xmlFree(val);
		}
		val = xmlGetProp(xmlnode, (xmlChar*)"alpha");
		if(val)
		{
			mapview->SetAlpha((float)atof((char*)val));
			xmlFree(val);
		}

		val = xmlGetProp(xmlnode, (xmlChar*)"scalebar");
		if(val)
		{
			if(strcmp((char*)val, "off") != 0)
				mapview->MapObjects()->push_back(new ScaleMapObject);
			xmlFree(val);
		}

		for(xmlNodePtr node = xmlnode->children; node; node = node->next)
		{
			uiMapAddLayer(mapview, node);
		}
	}

 	return mapview;
}

/************************************************************************************************
	uiMapCreateDOM
*************************************************************************************************/

phui::DOM::ElementDOM* uiMapCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent)
{
	SuperCellMapView* mv = dynamic_cast<SuperCellMapView*>(view);
	if(mv)
		return  new jsMapView(mv, parent);

	return 0;
}

/************************************************************************************************
	uiMapToggleIngameMap
*************************************************************************************************/

UINT32 uiMapToggleIngameMap(SINT32 keyState,  UINT32 functionNum, UINT32 data)
{
	if(!keyState || _ingamemaplock) return 0;

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionNum, &keyState);

	_ingamemapon = !_ingamemapon;

	if(persIsCrewDead())
		_ingamemapon = false;

	phui::View* _ingamemap = uiViewport()->FindView("ingamemap");

	if(_ingamemapon)
	{
		if(!_ingamemap)
			_ingamemap = uiLoadView("ingamemap", true, false);

		if(gCustomTerrain)
		{
			// now we find the mapview associated with the ingamemap
			phui::View* mv = _ingamemap->FindView("mapview");

			if(mv)
			{
				SuperCellMapView* ssmv = dynamic_cast<SuperCellMapView*>(mv);
				if(ssmv) ssmv->SetScale(.05f);
			}
		}

		fpFPSMapState(TRUE);
		uiSetCursor(1);

		_ingamemap->Show();

		// js layer hook
		uiRunScript("game.onmapactivate()");
	}
	else
	{
		fpFPSMapState(FALSE);

		if(_ingamemap)
		{
			uiMapHideIngameMap();

			// js layer hook
			uiRunScript("game.onmapdeactivate()");
		}
	}

	int etype = TUT_MAP_TOGGLE;
	TUT_MGR->DoEvent(TUT_EVENT_MAP, &etype);

	uiGC();

	return 0;
}

/************************************************************************************************
	uiMapHideIngameMap
*************************************************************************************************/

void uiMapHideIngameMap(bool close)
{
	phui::View* _ingamemap	= uiViewport()->FindView("ingamemap");
	phui::View* _mapchild	= uiViewport()->FindView("fullmap");

	if(_mapchild)
	{
		_mapchild->Hide();
		_mapchild->Close();
		_mapchild = NULL;
	}

	_mapchild = uiViewport()->FindView("report");
	if(_mapchild)
	{
		_mapchild->Hide();
		_mapchild->Close();
		_mapchild = NULL;
	}

	_mapchild = uiViewport()->FindView("waypoint");
	if(_mapchild)
	{
		_mapchild->Hide();
		_mapchild->Close();
		_mapchild = NULL;
	}

	_mapchild = uiViewport()->FindView("mapcontext");
	if(_mapchild)
	{
		_mapchild->Hide();
		_mapchild->Close();
		_mapchild = NULL;
	}

	if(_ingamemap)
	{
		_ingamemap->Hide();

		uiSetCursor(0);

		if(close || fioQuery() == ISDIRECTORY)
		{
			_ingamemap->Close();
			_ingamemap = NULL;
		}
	}

	chatTargetSelection(nullObjID);
	uiGC();

	_ingamemapon = false;
}

bool uiMapIngameMapUp()
{
	return _ingamemapon;
}

// used by the tutorial system to control map toggles
void uiMapLock(bool l)
{
	_ingamemaplock = l;
}

/************************************************************************************************
	PlayerMapObject
*************************************************************************************************/

PlayerMapObject::PlayerMapObject() : SuperCellMapView::MapObject()
{
	_visible = true;
	_navring = uiGetTexture("ui/textures/navring");
	_navarrow = uiGetTexture("ui/textures/navarrow");
	_navtarget = uiGetTexture("ui/textures/navhome");
	_navarrowsize = .25f;
	_arrowscale = .025f;
	_arrowradius = .10f;
	_shownavring = true;
}

void PlayerMapObject::DrawSelf(SuperCellMapView* mv)
{
	FLOAT64VECTOR fvecPlayerPos;
    FLOAT64VECTOR vecWorldOriginOffset;
   	trnGetWorldOriginOffset(&vecWorldOriginOffset);

    fvecPlayerPos.x = -playpos.x + vecWorldOriginOffset.x;
    fvecPlayerPos.y = -playpos.y + vecWorldOriginOffset.y;
    fvecPlayerPos.z = -playpos.z + vecWorldOriginOffset.z;

   	float _mapscx = (float)(fvecPlayerPos.x / SDM_SUPERCELL_SIZE);
    float _mapscy = (float)(fvecPlayerPos.y / SDM_SUPERCELL_SIZE);

	phui::GraphicsContext* ctx = mv->GetGraphicsContext();
	ctx->EnableTexture();

	phui::BoxVerts box;
	float uv[8] = {0,0,1,0,1,1,0,1};
	float x,y,width, height, angle;

	oglSetMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(_mapscx, _mapscy, 0);

	angle = -(float)(atan2(playdir.x, playdir.y) * 180.0f / _pi);

	glRotatef(angle, 0,0,1);

	if(_shownavring)
	{
		float iconscale = mv->Width() / mv->Scale();
		/*
		width = 1000.0f/SDM_SUPERCELL_SIZE * iconscale;
		float scalescale = pow(10.0f, (float)floor(log10(width/(mv->Width()*.9/2.0)))+1);
		*/

		//if(mv->NavScale())
		//	width = height = (mv->Width() * mv->NavScale()) / iconscale;
		//else
			width = height = 2000.0f/SDM_SUPERCELL_SIZE/mv->MapScale();

		x = -width/2; y = -height/2;

		phui::box_verts_with_pos(box, x, y, width, height);
		ctx->SetColor(1,1,1);
		ctx->SetTexture(_navring);
		phui::draw_gl_quad(box.verts, uv);
	}

	width = height = mv->Scale() * _navarrowsize;
	x = -width/2; y = -height/2;
	phui::box_verts_with_pos(box, x, y, width, height);
	if(gCustomTerrain)
		ctx->SetColor((float)piRGBA_GETRED(gIconFriendlyColor), (float)piRGBA_GETGREEN(gIconFriendlyColor), (float)piRGBA_GETBLUE(gIconFriendlyColor));
	else
		ctx->SetColor(1, 1, 1);
	ctx->SetTexture(_navarrow);
	phui::draw_gl_quad(box.verts, uv);

	glPopMatrix();

	// do we have an active waypoint?
	if(PlayerClient()->ActiveWaypointHandle())
	{
		Waypoint activewaypoint = PlayerClient()->ActiveWaypoint();

		// we check the handle anyway because the waypoint might have gotten deleted
		// this shouldn't happen but I found and fix some places it did so better safe than
		// sorry - worse that would happen is you would get an arrow pointing in some random
		// direction
		if(activewaypoint.handle())
		{
			float mscx, mscy;
			float mindist	= mv->Scale() * _arrowradius;				// min distance we will draw the arrow
			float wscx		= activewaypoint.x()/SDM_SUPERCELL_SIZE;	// scx of waypoint
			float wscy		= activewaypoint.y()/SDM_SUPERCELL_SIZE;	// scy of waypoint

			mv->PositionXY(&mscx, &mscy);

			mscx /= SDM_SUPERCELL_SIZE;
			mscy /= SDM_SUPERCELL_SIZE;

			if(math3dFastSqrt(SQR(wscx - mscx) + SQR(wscy - mscy)) > mindist)
			{
				glPushMatrix();

				angle = -(float)(atan2(activewaypoint.x() - mscx*SDM_SUPERCELL_SIZE, activewaypoint.y() - mscy*SDM_SUPERCELL_SIZE) * 180.0f / _pi);
				glTranslatef(mscx, mscy, 0);
				glRotatef(angle, 0,0,1);

				width = height = mv->Scale() * _arrowscale;
				x = -width/2; y = mv->Scale() * _arrowradius - height/2;

				phui::box_verts_with_pos(box, x, y, width, height);
				ctx->SetColor(0, 1, 0);
				ctx->SetTexture(_navtarget);
				phui::draw_gl_quad(box.verts, uv);
				ctx->SetColor(1, 1, 1);

				glPopMatrix();
			}
		}
	}
}

/************************************************************************************************
	ScaleMapObject
*************************************************************************************************/

ScaleMapObject::ScaleMapObject() : SuperCellMapView::MapObject()
{
	_visible = true;
	_scalebar = uiGetTexture("ui/textures/map-scale");
}

void ScaleMapObject::DrawSelf(SuperCellMapView* mv)
{
	phui::GraphicsContext* ctx = mv->GetGraphicsContext();

	float uv[8] = {0,0,1,0,1,1,0,1};
	float x,y,width, height;
	float iconscale = mv->Width() / mv->Scale();

	oglSetMatrixMode(GL_MODELVIEW);

	/*
	width = 1000.0f/SDM_SUPERCELL_SIZE * iconscale;
	float scalescale = pow(10.0f, (float)floor(log10(width/(mv->Width()*.9/2.0)))+1);
	*/

	float mapscale = mv->MapScale();

	// map scale bar
	glPushMatrix();

	float scx, scy;
	mv->PositionSuperCell(&scx, &scy);

	width = 1000.0f/SDM_SUPERCELL_SIZE/mapscale; height = 16 / iconscale;
	x = 0; //-120/iconscale;
	y = -(mv->Height()/2 - (mv->Height() - 40))/iconscale;
	glTranslatef(scx+x, scy+y, 0);

	ctx->SetColor(1,1,1);
	ctx->SetTexture(_scalebar);

	phui::BoxVerts box;
	phui::box_verts_with_pos(box, 0, 0, width, height);
	phui::draw_gl_quad(box.verts, uv);

	char scalestr[32];
	if(mapscale <= 1)
		snprintf(scalestr, 31, "%.0f km", 1/mapscale);
	else
		snprintf(scalestr, 31, "%.0f m", 1000.0/mapscale);

	//glTranslatef(width-ctx->FontAdvance(0,scalestr)/iconscale/2, 0, 0);
	glTranslatef(((ctx->FontAdvance(0, scalestr) + 2) * -1) / iconscale, 0, 0);
	glScalef(1/iconscale, 1/iconscale,1);
	ctx->DrawString(0, scalestr);

	glPopMatrix();
}

/************************************************************************************************
	get_octet_icon
*************************************************************************************************/

#if(0)
static int get_scenario_octet_icon(ScenarioMapRenderer* r, const char* name, float uv[4])
{
	/*
	std::string path = name;
	size_t n = path.find_last_of('.');
	if(n != std::string::npos) path.erase(n);
	path += "_icon_icon";

	piTextureInfo info;
	piTextureAtlasSet::getInstance()->getTexture(path, info);

	uv[0] = info.uv[0]; uv[1] = info.uv[1]; uv[2] = info.uv[2]; uv[3] = info.uv[3];
	return info.texid;
	*/
	return -1;
}
#endif

static int get_icon_octet_icon(IconMapRenderer* r, const char* name, float uv[4])
{
	std::string path = name;
	size_t n = path.find_last_of('.');
	if(n != std::string::npos) path.erase(n);
	path += "_icon_icon";

	piTextureInfo info;
	piTextureAtlasSet::getInstance()->getTexture(path, info);

	uv[0] = info.uv[0]; uv[1] = info.uv[1]; uv[2] = info.uv[2]; uv[3] = info.uv[3];
	return info.texid;
}

static int get_rendered_octet_icon(IconMapRenderer* r, const char* name, float uv[4])
{
	std::string path = name;
	if(path.find("bld") == 0) return -1;

	size_t n = path.find_last_of('.');
	if(n != std::string::npos) path.erase(n);

	piTextureInfo info;
	piTextureAtlasSet::getInstance()->getTexture(path, info);

	if(info.texid < 0)
	{
		path += "_icon";
		piTextureAtlasSet::getInstance()->getTexture(path, info);
	}

	uv[0] = info.uv[0]; uv[1] = info.uv[1]; uv[2] = info.uv[2]; uv[3] = info.uv[3];
	return info.texid;
}

/************************************************************************************************
	get_terrain_data
*************************************************************************************************/

static int get_terrain_data(int sx, int sy, sdm_replacement_octet_data* data)
{
	return get_sdm_data(sx,sy, NULL, data);
}

/************************************************************************************************
	get_trnobj_texturedata
*************************************************************************************************/

static void loadTextureMap(const char* path)
{
	int fd = fioOpen(path);
	if(fd < 0) return;

	char line[256];
	char name[32];
	int x,y;
	float mpp;
	for(;;)
	{
		if(fioGetLine(fd, line, 255) < 1) break;
		sscanf(line, "%31s %d %d %f", name, &x, &y, &mpp);

		trnobjtextureinfo& objinfo = _trnobjtexturemap[name];
		objinfo.x = x;
		objinfo.y = y;
		objinfo.mpp = mpp;
	}

	fioClose(fd);
};

static std::set<std::string> TerrainObjectSpecials;

static int get_trnobj_texturedata(SDM_ObjInfo const* info, int* texid, int* x, int* y, int* width, int* height, float* uv)
{
	if(_trnobjtexturemap.size() == 0)
	{
//#if GREEN_INSTALL == 1
    loadTextureMap("ui/textures/atlas/mapbld_small.xy");
//#else
//    loadTextureMap("ui/textures/atlas/mapbld.xy");
//#endif
    TerrainObjectSpecials.insert("infantrypit1"); // these overrides shold match GenerateIcons.cpp
    TerrainObjectSpecials.insert("atgunpit1");
    TerrainObjectSpecials.insert("farmruin");
  }

	std::string objname = info->objname;
  std::string directory;

	size_t n;
	n = objname.find_last_of("/\\");

	if(n != std::string::npos)
  {
    directory = objname.substr(0,n);
    objname.erase(0, n+1);

    if ( directory == "to" )
    {
      if ( TerrainObjectSpecials.find(objname) == TerrainObjectSpecials.end() )
      {
        *texid = -1;
        return -1;
      }
    }
  }

	objname += "_icon";

  if ( _trnobjtexturemap.find(objname) == _trnobjtexturemap.end() )
  {
    *texid = -1;
    return -1;
  }

	trnobjtextureinfo& objinfo = _trnobjtexturemap[objname];

	if(objinfo.texInfo.texid == -2)
		piTextureAtlasSet::getInstance()->getTexture(objname, objinfo.texInfo);

	if(objinfo.texInfo.texid > 0 && (objinfo.width == 0 || objinfo.height == 0))
	{
		TEXTURE* texture =  piGetTextureData(objinfo.texInfo.texid);
		if(texture)
		{
			objinfo.width  = (int)(texture->imagewidth[0] * fabs(objinfo.texInfo.uv[2] - objinfo.texInfo.uv[0]));
			objinfo.height = (int)(texture->imageheight[0]* fabs(objinfo.texInfo.uv[3] - objinfo.texInfo.uv[1]));
		}
	}

	*texid = objinfo.texInfo.texid;
	*x = objinfo.x;
	*y = objinfo.y;
	uv[0] = objinfo.texInfo.uv[0];
	uv[1] = objinfo.texInfo.uv[1];
	uv[2] = objinfo.texInfo.uv[2];
	uv[3] = objinfo.texInfo.uv[1];
	uv[4] = objinfo.texInfo.uv[2];
	uv[5] = objinfo.texInfo.uv[3];
	uv[6] = objinfo.texInfo.uv[0];
	uv[7] = objinfo.texInfo.uv[3];
	*width  = (int)(objinfo.width  * objinfo.mpp);
	*height = (int)(objinfo.height * objinfo.mpp);

	return 0;
}

bool uiMapGetSdmData(int scx, int scy, sdm_dtl_data* dtl, sdm_replacement_octet_data* replace)
{
	char filename[64];
	snprintf(filename, 63, "terrain/%04x_%04x.sdm", (scx < 0) ? scx & 0x0000ffff : scx, (scy < 0) ? scy & 0x0000ffff : scy);

	int fd = fioCallbacks.openread(filename);
	if(fd > -1)
	{
		if(dtl)
			sdm_read_dtl_data(fd, fioCallbacks.read, dtl);
		else if(replace)
			sdm_seek_replacement_data(fd, fioCallbacks.read, fioCallbacks.seek);

		if(replace)
			sdm_read_replacement_data(fd, fioRead, replace);

		fioCallbacks.close(fd);
	}else
	{
		if(dtl)
			memset(dtl, 0x0, sizeof(sdm_dtl_data));
		if(replace)
			memset(replace, 0x0, sizeof(sdm_replacement_octet_data));
	}

	return true;
}
/*
bool uiMapGetTrnobjTextureData(const char* name, int* texid, int* x, int* y, int* width, int* height, float uv[4])
{
	std::string objname = name;

	size_t n;
	n = objname.find_last_of("/\\");

	if(n != std::string::npos)
    {
	    objname.erase(0, n+1);

	    std::string dir = name;
	    dir.erase(n);

	    if(dir == "to")
	    {
		    *texid = -1;
		    return false;
	    }
    }

	if(_trnobjtexturemap.size() == 0)
		loadTextureMap("ui/textures/atlas/mapbld.xy");

	objname += "_icon";

    if(_trnobjtexturemap.find(objname) == _trnobjtexturemap.end())
        return false;

	trnobjtextureinfo& objinfo = _trnobjtexturemap[objname];

	if(objinfo.texInfo.texid == -2)
		piTextureAtlasSet::getInstance()->getTexture(objname, objinfo.texInfo);

	if(objinfo.texInfo.texid > 0 && (objinfo.width == 0 || objinfo.height == 0))
	{
		TEXTURE* texture =  piGetTextureData(objinfo.texInfo.texid);
		if(texture)
		{
			objinfo.width  = (int)(texture->imagewidth[0] * fabs(objinfo.texInfo.uv[2] - objinfo.texInfo.uv[0]));
			objinfo.height = (int)(texture->imageheight[0]* fabs(objinfo.texInfo.uv[3] - objinfo.texInfo.uv[1]));
		}
	}

	*texid = objinfo.texInfo.texid;

    *x = objinfo.x;
	*y = objinfo.y;

    uv[0] = objinfo.texInfo.uv[0];
    uv[1] = objinfo.texInfo.uv[1];
    uv[2] = objinfo.texInfo.uv[2];
    uv[3] = objinfo.texInfo.uv[3];

    *width  = (int)(objinfo.width  * objinfo.mpp);
	*height = (int)(objinfo.height * objinfo.mpp);

	return true;
}
*/

static void getMapIcon(const char* path, MapIcon& icon)
{
	piTextureInfo info;
	piTextureAtlasSet::getInstance()->getTexture(path, info);

	icon.id = info.texid;
	icon.uv[0] = info.uv[0];
	icon.uv[1] = info.uv[1];
	icon.uv[2] = info.uv[2];
	icon.uv[3] = info.uv[1];
	icon.uv[4] = info.uv[2];
	icon.uv[5] = info.uv[3];
	icon.uv[6] = info.uv[0];
	icon.uv[7] = info.uv[3];
}

/************************************************************************************************
	get_strat_icon
*************************************************************************************************/

static void get_strat_icon(StratDB::StratObject const* stratobj, bool selected, MapIcon& icon)
{
	int objclass = stratobj->StratClass();
	int objtype = stratobj->StratType();

	switch(objclass)
	{
		case CLASS_CP:
			if(objtype == CP_BRIDGE)
				getMapIcon("bridge1_off",icon);
		break;

		case CLASS_FACILITY:
			switch (objtype)
			{
				case FACILITY_CITY:
					getMapIcon("city1-off", icon);
				break;
				case FACILITY_FACTORY:
					getMapIcon("factory1-off", icon);
				break;
				case FACILITY_BASE:
					getMapIcon("base1-off", icon);
				break;
				case FACILITY_DEPOT:
					getMapIcon("gdepot1-off", icon);
				break;
				case FACILITY_OMT:
				break;
				case FACILITY_SHIPYARD:
				break;
				case FACILITY_FIREBASE:
					getMapIcon("firebase1-off", icon);
				break;
				case FACILITY_AIRBASE:
					getMapIcon("airbase1-off", icon);
				break;
				case FACILITY_ARMYBASE:
					getMapIcon("armybase1-off", icon);
				break;
				case FACILITY_NAVALBASE:
					getMapIcon("navybase1-off", icon);
				break;
			}
		break;
	};
}

/************************************************************************************************
	get_render_texture
*************************************************************************************************/

static int get_render_texture(int col, int row)
{
static int texid = -2;
	if(texid == -2)
		texid = uiGetTexture("ui/textures/blitz-map.bmp");
	return texid;
}

static int get_sdm_data(int scx, int scy, sdm_dtl_data* dtl, sdm_replacement_octet_data* replace)
{
	char filename[64];
	snprintf(filename, 63, "terrain/%04x_%04x.sdm", (scx < 0) ? scx & 0x0000ffff : scx, (scy < 0) ? scy & 0x0000ffff : scy);

	int fd = fioCallbacks.openread(filename);
	if(fd > -1)
	{
		if(dtl)
			sdm_read_dtl_data(fd, fioCallbacks.read, dtl);
		else if(replace)
			sdm_seek_replacement_data(fd, fioCallbacks.read, fioCallbacks.seek);

		if(replace)
			sdm_read_replacement_data(fd, fioRead, replace);

		fioCallbacks.close(fd);
	}else
	{
		if(dtl)
			memset(dtl, 0x0, sizeof(sdm_dtl_data));
		if(replace)
			memset(replace, 0x0, sizeof(sdm_replacement_octet_data));
	}

	return 0;
}

/************************************************************************************************
	MapMessageFilter
*************************************************************************************************/

bool MapMessageFilter::Filter(phui::Message* msg, phui::MessageHandler** target,  phui::MessageHandler** reply)
{
	long sx, sy, id;
	FULL_OBJ_ID fullid = nullObjID;

	switch(msg->what)
	{
		case CPUPDATE_MSG:
			return true;
		break;
		/*
		case CPUPDATE_MSG:
		{
			msg->FindLong(CPUPDATE_SX, &sx);
			msg->FindLong(CPUPDATE_SY, &sy);
			msg->FindLong(CPUPDATE_ID, &id);

			fullid.cell.ox = ((SINT16)sx) << 4;
			fullid.cell.oy = ((SINT16)sy) << 4;
			fullid.id = (UINT16)id;

//			strtRequestCPFacilitiesUpdate(PlayerClient()->MapConnection(), fullid);
			///TODO: strtRequestCPFacilitiesUpdate is no-longer required.
//			assert( "TROY: Fix Me" == NULL ) ;
		}
		break;
		*/
		case CPTARGET_MSG:
		{
			if(msg->FindLong(CPUPDATE_SX, &sx) == phui::Message::OK)
			{
				msg->FindLong(CPUPDATE_SY, &sy);
				msg->FindLong(CPUPDATE_ID, &id);

				fullid.cell.ox = ((SINT16)sx) << 4;
				fullid.cell.oy = ((SINT16)sy) << 4;
				fullid.id = (UINT16)id;
			}

			chatTargetSelection(fullid);
		}
		break;
	}

	return false;
};

/************************************************************************************************
	MapInputFilter
*************************************************************************************************/

bool MapInputFilter::Filter(phui::Message* msg, phui::MessageHandler** target,  phui::MessageHandler** reply)
{
	switch(msg->what)
	{
		case phui::KEY_DOWN:
		case phui::KEY_UP:
		{
			if(_mv && _mv->IsVisible())
				_mv->MessageReceived(msg, *target);
		}
		break;
	}

	return false;
};

/************************************************************************************************
	simple ui waypoint management
*************************************************************************************************/

void uiMapActivateWaypoint(const char* name)
{
	PlayerClient()->SetActiveWaypoint(chatGetHandle(name));
}

void uiMapAddWaypoint(const char* name, int type, int subtype, int x, int y, const char* notes)
{
	const ChatHandle& handle = chatGetHandle(name);
	Waypoint waypoint(true, handle, handle, (WP_TYPE)type, subtype, x, y, 0, notes);
	waypoint.setShared(false);
	PlayerClient()->SetWaypoint(waypoint);
}

void uiMapRemoveWaypoint(const char* name)
{
	PlayerClient()->RemoveWaypoint(chatGetHandle(name));
}

void uiMapClearWaypoints()
{
	Waypoint waypoint;
	PlayerClient()->ProcessWaypoint(waypoint, -1);
}

/************************************************************************************************
	uiMapAddLayer
*************************************************************************************************/

void uiMapAddLayer(SuperCellMapView* mapview, xmlNode* node)
{
	xmlChar* val;

	if(!strcmp((char*)node->name, "layer"))
	{
		bool scenarioActive = (SCEN_MGR->IsActive() && gCustomTerrain);

		xmlChar* type = xmlGetProp(node, (xmlChar*)"type");
		if(!type) return;

		xmlChar* id = xmlGetProp(node, (xmlChar*)"id");

		SuperCellMapView::Renderer* r = NULL;
		if(!strcmp((char*)type, "texture") && !scenarioActive)
		{
			MapTextureRenderer* mtr = new MapTextureRenderer;
			mtr->SetTextureCallback(get_render_texture);
			mtr->SetMapOrigin(-26, 210);
			mtr->SetNumColsRows(1,1);
			mtr->SetCellDimensions(64,64);
			r = mtr;
		}else if(!strcmp((char*)type, "icon") && !scenarioActive)
		{
			IconMapRenderer* ir = new IconMapRenderer;
			ir->GetSDMData = get_sdm_data;
			if(!strcmp((char*)id, "rendered"))
				ir->GetOctetTexture = get_rendered_octet_icon;
			else
				ir->GetOctetTexture = get_icon_octet_icon;
			r = ir;
		}else if(!strcmp((char*)type, "scenario"))
		{
			if(gCustomTerrain)
				r = new ScenarioMapRenderer;

		}else if(!strcmp((char*)type, "topo") && !scenarioActive)
		{
			TopoMapRenderer* tr = new TopoMapRenderer;
			tr->_sdmdata = get_sdm_data;

			float altstart = 0, altend = 10000;

			val = xmlGetProp(node, (xmlChar*)"altstart");
			if(val)
			{
				altstart = (float)atof((char*)val);
				xmlFree(val);
			}

			val = xmlGetProp(node, (xmlChar*)"altend");
			if(val)
			{
				altend = (float)atof((char*)val);
				xmlFree(val);
			}
			tr->SetRange(altstart, altend);

			val = xmlGetProp(node, (xmlChar*)"spacing");
			if(val)
			{
				tr->SetSpacing((float)atof((char*)val));
				xmlFree(val);
			}

			r = tr;
		}else if(!strcmp((char*)type, "terrainobj") && !scenarioActive)
		{
			TerrainObjectRenderer* tr = new TerrainObjectRenderer;
			tr->GetTerrainSDMData = get_terrain_data;
			tr->GetTerrainTextureData = get_trnobj_texturedata;
			r = tr;
		}else if(!strcmp((char*)type, "players"))
		{
			PlayerLayer* pl = new PlayerLayer;
			pl->_icontexid = uiGetTexture("ui/textures/mapvehicles.bmp");

			r = pl;
		}else if(!strcmp((char*)type, "waypoints"))
		{
			WaypointLayer* wl = new WaypointLayer;
			wl->_waypointtexid = uiGetTexture("ui/textures/waypoint.bmp");
			wl->_contacttexid = uiGetTexture("ui/textures/mapvehicles.bmp");
			wl->_markertexid = uiGetTexture("ui/textures/marker.bmp");
			r = wl;
		}else if(!strcmp((char*)type, "hclinks") && !scenarioActive)
		{
			StrategicLayer* sl = new StrategicLayer;
			r = sl;
		}else if(!strcmp((char*)type, "custommap"))
		{
			CustomMapLayer* cl = new CustomMapLayer(mapview);
			r = cl;
		}else if(!strcmp((char*)type, "aws"))
		{
			AWSLayer* al = new AWSLayer;
			r = al;
		}else if(!strcmp((char*)type, "strat") && !scenarioActive)
		{
			CPRenderer * cr = new CPRenderer;

			cr->client = PlayerClient();
			PlayerClient()->RegisterHandler(cr);

			cr->_straticon = get_strat_icon;

			getMapIcon("groundattackicon_on2", cr->groundAttackIconOn);
			getMapIcon("groundattackicon_off2", cr->groundAttackIconOff);
			getMapIcon("groundattackicon_on3", cr->heavyGroundAttackIconOn);
			getMapIcon("groundattackicon_off3", cr->heavyGroundAttackIconOff);

			getMapIcon("airattackicon_on2", cr->airAttackIconOn);
			getMapIcon("airattackicon_off2", cr->airAttackIconOff);
			getMapIcon("airattackicon_on3", cr->heavyAirAttackIconOn);
			getMapIcon("airattackicon_off3", cr->heavyAirAttackIconOff);

			getMapIcon("infattackicon_on2", cr->infAttackIconOn);
			getMapIcon("infattackicon_off2", cr->infAttackIconOff);
			getMapIcon("infattackicon_on3", cr->heavyInfAttackIconOn);
			getMapIcon("infattackicon_off3", cr->heavyInfAttackIconOff);

			getMapIcon("seaattackicon_on2", cr->seaAttackIconOn);
			getMapIcon("seaattackicon_off2",cr->seaAttackIconOff);
			getMapIcon("seaattackicon_on3", cr->heavySeaAttackIconOn);
			getMapIcon("seaattackicon_off3", cr->heavySeaAttackIconOff);

			getMapIcon("portbaseflag", cr->hasRareNavyIcon);
			getMapIcon("navybaseflag", cr->hasPortIcon);
			getMapIcon("airbaseflag", cr->hasAFIcon);
			getMapIcon("bomberbaseflag", cr->hasRareAirIcon);
			getMapIcon("paraflag", cr->hasRareInfIcon);

			getMapIcon("bridge1_off", cr->bridgeIcon);
			getMapIcon("holdall1", cr->haacIcon);
			getMapIcon("fallback1", cr->fallbackIcon);
			getMapIcon("alertcamped1", cr->overrunIcon	);
			getMapIcon("contention1-off", cr->contentionIcon);

			getMapIcon("frenchiconflag1", cr->countryFlags[WWII_FRANCE]);
			getMapIcon("germaniconflag1", cr->countryFlags[WWII_GERMANY]);
			getMapIcon("briticonflag1", cr->countryFlags[WWII_ENGLAND]);
			getMapIcon("ui/uidata/icons/cp/american/americonflag1.bmp", cr->countryFlags[WWII_USA]);

			getMapIcon("french_cp_flag", cr->countryMarkers[WWII_FRANCE]);
			getMapIcon("german_cp_flag", cr->countryMarkers[WWII_GERMANY]);
			getMapIcon("british_cp_flag", cr->countryMarkers[WWII_ENGLAND]);
			getMapIcon("ui/uidata/icons/cp/american/cp_flag.bmp", cr->countryMarkers[WWII_USA]);

			cr->animFlagIcons[WWII_NO_COUNTRY]	= uiGetTexture("ui/uidata/icons/brigade/surrender-anim.bmp");
			cr->animFlagIcons[WWII_ENGLAND]		= uiGetTexture("ui/uidata/icons/brigade/britbat-anim.bmp");
			cr->animFlagIcons[WWII_USA]			= uiGetTexture("ui/uidata/icons/brigade/amerbat-anim.bmp");
			cr->animFlagIcons[WWII_FRANCE]		= uiGetTexture("ui/uidata/icons/brigade/frenbat-anim.bmp");
			cr->animFlagIcons[WWII_GERMANY]		= uiGetTexture("ui/uidata/icons/brigade/germbat-anim.bmp");

			cr->iconsize = 32 * WantedHeight / 768.0f;
			cr->cpupdateonclick = Online;
			cr->tooltip = uiLocString;

			for(xmlNodePtr child = node->children; child; child = child->next)
			{
				if(child->type == XML_ELEMENT_NODE && xmlStrcmp((xmlChar*)"filter", child->name) == 0)
				{
					xmlChar* name = xmlGetProp(child, (xmlChar*)"name");
					xmlChar* value = xmlGetProp(child, (xmlChar*)"value");

					if(name && value)
						cr->FilterStrat((char*)name, (unsigned char*)value);

					xmlFree(name);
					xmlFree(value);
				}
			}

			r = cr;
		}

		if(r)
		{
			if(id)
				r->name = (char*)id;

			val = xmlGetProp(node, (xmlChar*)"minscale");
			if(val)
			{
				r->minscale = (float)atof((char*)val);
				xmlFree(val);
			}

			val = xmlGetProp(node, (xmlChar*)"maxscale");
			if(val)
			{
				r->maxscale = (float)atof((char*)val);
				xmlFree(val);
			}

			val = xmlGetProp(node, (xmlChar*)"enabled");
			if(val)
			{
				r->enabled = phui::UIXMLbool(val);
				xmlFree(val);
			}

			mapview->AddRenderer(r);
		}

		if(id) xmlFree(id);
		xmlFree(type);

	}else if(!strcmp((char*)node->name, "object"))
	{
		xmlChar* type = xmlGetProp(node, (xmlChar*)"type");
		if(type)
		{
			if(!strcmp((char*)type, "player"))
			{
				PlayerMapObject* obj = new PlayerMapObject;

				mapview->MapObjects()->push_back(obj);

				val = xmlGetProp(node, (xmlChar*)"navring");
				if(val)
				{
					obj->_shownavring = (strcmp((char*)val, "off") != 0);
					xmlFree(val);
				}
				val = xmlGetProp(node, (xmlChar*)"size");
				if(val)
				{
					obj->_navarrowsize = (float)atof((char*)val);
					xmlFree(val);
				}
				val = xmlGetProp(node, (xmlChar*)"arrowscale");
				if(val)
				{
					obj->_arrowscale = (float)atof((char*)val);
					xmlFree(val);
				}
				val = xmlGetProp(node, (xmlChar*)"arrowradius");
				if(val)
				{
					obj->_arrowradius = (float)atof((char*)val);
					xmlFree(val);
				}
			}

			xmlFree(type);
		}
	}
}

void uiMapStratUpdate(StratDB::StratObject* strat, int country, int vislevel)
{
	// code is only used for tutorials
	if(strat && TUT_ACTIVE)
	{
		if(country > 0)
			strat->SetCountry(country);

		strat->SetUpdateTime(frametime);

		// make sure the terrain object gets updated
		StratDB::StratObjList children;

		if(StratDB::FindStratObjects(children, -1, -1, -1, (StratDB::StratObject*)strat))
		{
			for(unsigned int i = 0; i < children.size(); i++)
			{
				StratDB::StratObject* child = children.at(i);

				if(country > 0)
					child->SetCountry(country);

				child->SetUpdateTime(frametime);

				// update the in game buildings
				if(country > 0 || vislevel >= 0)
				{
					if(country <= 0)
						country = child->Country();

					trnUpdateObjectStatus(child->SupercellX(), child->SupercellY(),  child->FullObjID()->id, country, vislevel);
				}
			}
		}

		phui::Message msg(CPUPDATE_MSG);

		msg.AddLong(CPUPDATE_SX, strat->SupercellX());
		msg.AddLong(CPUPDATE_SY, strat->SupercellY());
		msg.AddLong(CPUPDATE_ID, strat->FullObjID()->id);
		msg.AddLong(CPUPDATE_OWNER, strat->Country());
		msg.AddLong(CPUPDATE_FLAGS, strat->StratFlags());

		PlayerClient()->PostCPUpdate(msg);
	}
}

SuperCellMapView* uiMapFind(const char* name)
{
	return dynamic_cast<SuperCellMapView*>(uiFindView(name));
}

void uiMapSetMissionOriginAndTarget(FULL_OBJ_ID const& origin, FULL_OBJ_ID const& target)
{
	_facOrigin = origin;
	_missionTarget = target;
}

UINT32 uiMapScaleAllMaps(SINT32 keyState,  UINT32 functionNum, UINT32 data)
{
	const char* maps[3] = { "ingamemap", "minimap", "fullmap" };

	for(int i = 0; i < 3; ++i)
	{
		SuperCellMapView* map = dynamic_cast<SuperCellMapView*>(uiViewport()->FindView(maps[i]));

		if(!map)
		{
			phui::View* parent = uiViewport()->FindView(maps[i]);
			if(parent) map = dynamic_cast<SuperCellMapView*>(parent->FindView("mapview"));
		}

		if(map)
		{
			if(keyState)
				map->SetAutoScale((functionNum == IMAP_MOM_UI_MAP_ZOOMIN) ? 1: -1);
			else
				map->SetAutoScale(0);
		}
	}

	if(!keyState)
		uiMapSaveMinimapScale();

	return TRUE;
}

void uiMapProcessPlayerEventToAllMaps(ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data, SINT32 expires)
{
	// main in game map
	uiMapProcessPlayerEventToMap("ingamemap", handle, event, data, expires);

	// HUD minimap
	uiMapProcessPlayerEventToMap("minimap", handle, event, data, expires);

	// in game full map
	uiMapProcessPlayerEventToMap("fullmap", handle, event, data, expires);
}

void uiMapProcessPlayerEventToMap(const char* mapname, ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data, SINT32 expires)
{
	// always add it in case other maps are loaded
	if(expires)
		add_player_map_event(mapname, handle, event, data, expires);

	SuperCellMapView* map = dynamic_cast<SuperCellMapView*>(uiViewport()->FindView(mapname));

	if(!map)
	{
		phui::View* parent = uiViewport()->FindView(mapname);

		if(parent)
		{
			map = dynamic_cast<SuperCellMapView*>(parent->FindView("mapview"));
			if(!map) return;
		}
		else
		{
			return;
		}
	}

	SuperCellMapView::RendererList list = map->RenderList();
	SuperCellMapView::RendererList::iterator it = list.begin();

	for(; it != list.end(); ++it)
	{
		PlayerLayer* pl = dynamic_cast<PlayerLayer*>(it->ptr());

		if(!pl) continue; //  || !pl->_useMissionMemberUpdate

//		SuperCellMapView::MapObjectList* objects = pl->MapObjects();

		PlayerLayer::VehicleMap& vehicles = pl->_vehicles;
		PlayerLayer::VehicleMap::iterator vit = vehicles.begin();

		for(; vit != vehicles.end(); vit++)
		{
			if(vit->second.ptr()->_handle == handle)
			{
				switch(event)
				{
					case MAP_PLAYER_EVENT_EMOTE:
						vit->second.ptr()->HandleEmoteEvent(map, data);
					break;
					case MAP_PLAYER_EVENT_AMMO:
						vit->second.ptr()->HandleEmoteEvent(map, 4); // ammo emote id from fps stuff
					break;
					default:
						// do nothing
					break;
				}

				break; // vehicles loop
			}
		}

		break; // renderer loop
	}
}

void uiMapClearPlayerEvents()
{
	PlayerLayer::blinkMap.clear();
}

void uiMapSaveMinimapScale()
{
	SuperCellMapView* minimap = uiMapFind("minimap");
	if(minimap) cfgPrefsSetFloat("hud.minimap.scale", minimap->Scale());
}

void add_player_map_event(const char* mapname, ChatHandle handle, MAP_PLAYER_EVENT event, SINT32 data, int expires)
{
	if(!uiInGame() || !expires) return;

	switch(event)
	{
		case MAP_PLAYER_EVENT_AMMO:
			PlayerLayer::blinkMap[handle] = expires;
		break;
		default:
			// do nothing
		break;
	}
}
