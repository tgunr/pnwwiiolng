#include <syscfg.h> // Path is project specific.

#include "phui/GraphicsContext.h"
#include "libxml/tree.h"

#include "pi/pi.h"
#include "include/vars.h"
#include "pers/pershud.h"
#include "fio/fio.h"
#include "trn/trntod.h"
#include "veh/vehIcon.h"
#include "ui/uiMain.h"
#include "ob/obmain.h"
#include "ob/obdraw.h"
#include "ob/obcs.h"
#include "trn/trnRadClutter.h"
#include "include/vars.h"
#include "ui/uiChat.h"
#include "trn/trngrass.h"
#include "uimap/uiMap.h"
#include "js/jsClient.h"
#include "cfg/cfgPrefs.h"
#include "pers/persmain.h"
#include "tut/TutorialManager.h"
#include "trn/speedgrass.h"
#include "chat/chatWaypoint.h"
#include "physics/colliderMain.h"
#include "math/Vec2d.h"
#include "fp/fpFPS.h"
#include "wwii/wwiiServerTrackedObject.h"		// The base STO type
#include "phui/CSSViewDrawer.h"
#include "client/GameClient.h"
#include "scen/ScenarioManager.h"
#include "ogl/GLCounter.h"
#include "sdm/StratDB.h"

#if !defined(USE_OSG)
#include "spt/sptmain.h"
#endif

#if defined(USE_OSG)
#include "osgStats/statsMain.h"
#endif

#ifdef DOROTHY
#include "gr2/gr2Main.h"
#endif

#define _UPDATE_METER_TIME 15000
#define HOST_UPDATE_INTERVAL 200
#define _UPDATE_METER_COUNT (_UPDATE_METER_TIME/HOST_UPDATE_INTERVAL)
#define HUD_DEATH_LIFE 50000
#define HUD_DEATH_HEIGHT 25.0f

static SINT32 _ReceivedUpdateDeltas[_UPDATE_METER_COUNT];

static bool _hudlock = false;
static phui::View* _hud;
static LOGICAL _showreadout;

// singleton
static PersHUDManager _hudMgr;

class GameInfoView : public persHUDTextView
{
public:

	GameInfoView();

	virtual void DrawSelf();
	void DisplayInfo();
	void DisplayUpdateMeter();
};

phui::View* CreateGameInfoView(_xmlNode* xmlnode, phui::View* parent);

/************************************************************************************************
	persHUDToggleHUD
*************************************************************************************************/

UINT32 persHUDToggleHUD(SINT32 param, UINT32 functionNum, UINT32 alwaysZero)
{
	if(!param || _hudlock) return 0;

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionNum, &param);

	// cycle the hud visible global
	if(HUDVisible)
		HUDVisible = HUD_CYCLE_OFF;
	else
		HUDVisible = HUD_CYCLE_ON;

	if(1)
	{
		if(HUDVisible == HUD_CYCLE_OFF)
			persHUDHideHUD(true);
		else
			persHUDShowHUD();
	}

	// update the preference system
	cfgPrefsSetGlobal(cfgHUD_VISIBLE, &HUDVisible, false);

	// alert the ui layer
	uiRunScript("game.oncyclehud()");

#ifdef DOROTHY
	// let gr2 vehicles know when the HUD gets toggled
	gr2ProcessControlInputs(param, functionNum, alwaysZero);
#endif

	return 0;
}

/************************************************************************************************
	persHUDToggleGameInfo
*************************************************************************************************/

unsigned int persHUDToggleGameInfo(int param, unsigned int functionCode, unsigned int unused)
{
	if(!param) return 0;

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionCode, &param);

	// invert the global
	bShowInfo = !bShowInfo;

	// update the preference system
	cfgPrefsSetGlobal(cfgSPECIAL_FX_SHOW_INFO, &bShowInfo, false);

	// alert the ui layer
	uiRunScript("game.oncyclegameinfo()");

	return 0;
}

/************************************************************************************************
	persHUDToggleReadout
*************************************************************************************************/

unsigned int persHUDToggleReadout(int param, unsigned int functionCode, unsigned int unused)
{
	if(!param) return 0;

	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionCode, &param);

#if defined(USE_OSG)
	statsToggle(); // this may not work with the new readout setup
#else

	// invert the global
	_showreadout = ! _showreadout;

	// update the preference system
	cfgPrefsSetGlobal(cfgSHOW_READOUT, &_showreadout, false);

	// alert the ui layer
	uiRunScript("game.oncyclereadout()");

#endif

	return 0;
}

/************************************************************************************************
	persHUDResetHUD
*************************************************************************************************/

void persHUDLock(bool l)
{
	_hudlock = l;
}

/************************************************************************************************
	persHUDResetHUD
*************************************************************************************************/

UINT32 persHUDResetHUD(SINT32 param, UINT32 functionNum, UINT32 alwaysZero)
{
	TUT_MGR->DoEvent(TUT_EVENT_INPUT, &functionNum, &param);
	HUDVisible = HUD_CYCLE_RESET;
	cfgPrefsSetGlobal(cfgHUD_VISIBLE, &HUDVisible, false);
	uiRunScript("game.oncyclehud()");
	return 0;
}

/************************************************************************************************
	persHUDShowHUD
*************************************************************************************************/

void persHUDShowHUD()
{
	if(!_hud)
	{
#ifdef DOROTHY
		_hud = uiLoadView("dorothy/hud");
#else
		if(SCEN_MGR->IsActive())
			_hud = uiLoadView("scenarios/hud");
		else
			_hud = uiLoadView("hud");
#endif
	}

	if(_hud && !_hud->IsVisible())
		_hud->Show();

	uiGC();
}

/************************************************************************************************
	persHUDShowGameInfo
*************************************************************************************************/
/*
void persHUDShowGameInfo()
{
#if defined(USE_OSG)
#else
	if(!_gameinfo)
	{
		_gameinfo = uiLoadView("gameinfo");

		// let the ui determine the min width/height
		_gameinfo->SetMinSize(_gameinfo->Width(), _gameinfo->Height());
	}
	else
	{
		_gameinfo->Show();
	}

	bShowInfo = TRUE;
	uiGC();
#endif
}
*/

/************************************************************************************************
	persHUDShowReadout
*************************************************************************************************/
/*
void persHUDShowReadout()
{
#if defined(USE_OSG)
#else
	if(!_readout)
	{
		_readout = uiLoadView("readout");

		// let the ui determine the min width/height
		_readout->SetMinSize(_readout->Width(), _readout->Height());
	}
	else
	{
		_readout->Show();
	}

	_showreadout = TRUE;
	uiGC();
#endif
}
*/

/************************************************************************************************
	persHUDHideHUD
*************************************************************************************************/

void persHUDHideHUD(bool close)
{
	if(_hud)
	{
		_hud->Hide();

		if(close || fioQuery() == ISDIRECTORY)
		{
			_hud->Close();
			_hud = NULL;
		}
	}
	uiGC();
}

/************************************************************************************************
	persHUDSyncToCycleSetting
*************************************************************************************************/
/*
void persHUDSyncToCycleSetting()
{
	if(HUDVisible != HUD_CYCLE_OFF)
		persHUDShowHUD();
	else
		persHUDHideHUD();
	uiGC();
}
*/

/************************************************************************************************
	persHUDHideReadout
*************************************************************************************************/
/*
void persHUDHideReadout(bool close)
{
	if(!_readout) return;

	_readout->Hide();
	if(close || fioQuery() == ISDIRECTORY)
	{
		_readout->Close();
		_readout = NULL;
	}

	if(!close)
		_showreadout = FALSE;
	uiGC();
}
*/

/************************************************************************************************
	persHUDHideGameInfo
*************************************************************************************************/
/*
void persHUDHideGameInfo(bool close)
{
	if(!_gameinfo) return;

	_gameinfo->Hide();
	if(close || fioQuery() == ISDIRECTORY)
	{
		_gameinfo->Close();
		_gameinfo = NULL;
	}

	if(!close)
		bShowInfo = FALSE;
	uiGC();
}
*/

/************************************************************************************************
	persHudReceivedUpdate
*************************************************************************************************/

LOGICAL persHudReceivedUpdate(void)
{
	SINT32 i;
	static SINT32 _LastUpdateTime;

	if (frametime == _LastUpdateTime) return TRUE;

	if(abs(frametime - _LastUpdateTime) < HOST_UPDATE_INTERVAL)
	{
		for (i=_UPDATE_METER_COUNT-2;i >= 0;i--)
			_ReceivedUpdateDeltas[i+1] = _ReceivedUpdateDeltas[i];

		_ReceivedUpdateDeltas[0] = frametime - _LastUpdateTime;
	}

	_LastUpdateTime = frametime;

	return 0;
}

/************************************************************************************************
	ResetTextPos
*************************************************************************************************/

void persHUDTextView::ResetTextPos()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->EnableTexture();

	_lineheight = ctx->FontHeight(_font) + 2;
	_textposx = _lmargin;
	_textposy = Height() - _tmargin - _lineheight;

	ctx->MovePenPosition(_textposx, _textposy);

	_maxx = _textposx;
	_miny = _textposy - _lineheight;
}

/************************************************************************************************
	ResetTextPos
*************************************************************************************************/

void persHUDTextView::InitCache()
{
	std::vector<std::string> keys;

	StringCache::iterator it = strings.begin();
	for(; it != strings.end(); ++it)
		keys.push_back(it->first);

	strings.clear();

	for(int i = 0; i < (int)keys.size(); ++i)
		strings[keys[i]] = uiLocString(keys[i].c_str()).c_str();

	language = gLanguage;
}

/************************************************************************************************
	ResizeToText
*************************************************************************************************/

void persHUDTextView::ResizeToText()
{
	if(!IsVisible())
		return;

	float ow = Width();
	float oh = Height();
	float width = _maxx + _rmargin; // new width
	float height = oh - _miny + _bmargin; // new height

	// resize to fit the text
	ResizeTo(std::max(width,_minwidth), std::max(height,_minheight));

	if(_parent)
	{
		if(_x + ow/2 > _parent->Width()/2) // right justify
			MoveTo(_x + ow - Width(), _y);

		if(_y + oh/2 > _parent->Height()/2) // top justify
			MoveTo(_x, _y + oh - Height());
	}
}

/************************************************************************************************
	DrawString
*************************************************************************************************/

void persHUDTextView::DrawSelf()
{
	if((UINT32)language != gLanguage)
		InitCache();

	View::DrawSelf();
}

/************************************************************************************************
	DrawString
*************************************************************************************************/

void persHUDTextView::DrawString(const char* str, ...)
{
	static char Buffer[256];
	bool newline = false;

	va_list marker;
	va_start(marker, str);
	int n = vsnprintf(Buffer, 255, str, marker);
	va_end(marker);

	if(n > 0 && Buffer[n-1] == '\n')
	{
		newline = true;
		Buffer[n-1] = '\0';
	}

	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->DrawString(_font, Buffer);

	float advance = ctx->FontAdvance(_font, Buffer);
	ctx->MovePenPosition(advance, 0);

	_textposx += advance;
	if(_textposx > _maxx)
		_maxx = _textposx;

	if(newline)
	{
		if(_textposy < _miny)
			_miny = _textposy;

		ctx->MovePenPosition(-_textposx+_lmargin, -_lineheight);

		_textposx = _lmargin;
		_textposy -= _lineheight;
	}
}

void persHUDTextView::DrawString(const wchar_t* str, ...)
{
	static wchar_t Buffer[256];
	bool newline = false;

	va_list marker;
	va_start(marker, str);
	int n = vswprintf(Buffer, 255, str, marker);
	va_end(marker);

	if(n > 0 && Buffer[n-1] == L'\n')
	{
		newline = true;
		Buffer[n-1] = L'\0';
	}

	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->DrawString(_font, Buffer);

	float advance = ctx->FontAdvance(_font, Buffer);
	ctx->MovePenPosition(advance, 0);

	_textposx += advance;
	if(_textposx > _maxx)
		_maxx = _textposx;

	if(newline)
	{
		if(_textposy < _miny)
			_miny = _textposy;

		ctx->MovePenPosition(-_textposx+_lmargin, -_lineheight);

		_textposx = _lmargin;
		_textposy -= _lineheight;
	}
}

GameInfoView::GameInfoView()
{
	strings["hud.gameinfo.fps"]			= L"";
	strings["hud.gameinfo.time"]		= L"";
	strings["hud.gameinfo.fov"]			= L"";
	strings["hud.gameinfo.icon"]		= L"";
	strings["hud.gameinfo.icon.0"]		= L"";
	strings["hud.gameinfo.icon.1"]		= L"";
	strings["hud.gameinfo.icon.2"]		= L"";
	strings["connect.ping.na"]			= L"";
	strings["connect.ping.notactive"]	= L"";
	strings["connect.ping.stats.brief"]	= L"";
	strings["connect.ping.stats.full"]	= L"";
	strings["connect.ping.loss"]		= L"";
	strings["connect.ping.lossbig"]		= L"";
	strings["connect.ping.kbps"]		= L"";
}

/************************************************************************************************
	DrawSelf
*************************************************************************************************/

void GameInfoView::DrawSelf()
{
	persHUDTextView::DrawSelf();

 	ResetTextPos();
 	DisplayInfo();
	if(Online)
		DisplayUpdateMeter();
	ResizeToText();
}

/************************************************************************************************
	DisplayInfo
*************************************************************************************************/
#if platOS == OS_WIN32 && (!defined(RELEASE_CLIENT) || RELEASE_CLIENT == 0)
	extern std::vector<std::wstring> updtGetNvHistory();
#endif

void GameInfoView::DisplayInfo()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();
	ctx->SetColor(0xdb/255.0f,0xc5/255.0f,0x00/255.0f);

#if defined(USE_OSG)
#else
	UINT32 TOD,Hour,Min;
	trnTODGetTimeOfDay(&TOD);
	Hour = trnTODGetHourFromTimeOfDay(TOD);
	Min = trnTODGetMinuteFromTimeOfDay(TOD);
#if !defined(RELEASE_CLIENT) || RELEASE_CLIENT == 0
	DrawString(L"%ls: %.0f rev %u\n", strings["hud.gameinfo.fps"].c_str(), gFPS, WWII::svnRevisionNo);
#else
	DrawString(L"%ls: %.0f\n", strings["hud.gameinfo.fps"].c_str(), gFPS);
#endif
	DrawString(L"%ls: %02d:%02d\n", strings["hud.gameinfo.time"].c_str(), Hour, Min);

	if(persIsPersonaVehicleType(VEH_TYPE_FLOATER))
		DrawString(L"%ls: %d\n", strings["hud.gameinfo.fov"].c_str(), (UINT32)FieldOfView);

	char modeKey[25];
	snprintf(modeKey, 24, "hud.gameinfo.icon.%d", vehIconMode());

	DrawString(L"%ls: %ls\n", strings["hud.gameinfo.icon"].c_str(), strings[modeKey].c_str());

#if defined(BETA_RELEASE)
	DrawString(L"%ls\n", clientGetNetworkState().c_str());
#endif

#ifdef VEHICLES_GAME_INFO
	DrawString("%s\n", vehVehiclesGameInfo().c_str());
#endif

#if RELEASE_CLIENT == 0
 	DrawString("MSPF = %.0f\n", 1000.0f/gFPS);

    UINT32	points, lines, triangles, vertices;

    piGetRenderStats(&points, &lines, &triangles, &vertices);

    SINT32 total, displayed;

    obcsGetObjectCullStats(&total, &displayed);

    DrawString("OBJECTS: total = %d displayed = %d\n", total, displayed);
    DrawString("TRIANGLES: %d\n", triangles);
    DrawString("DRAW CALLS: %.0f\n", GLCounter::instance().getRecentTotal());

	static char txt[150];
#if !defined(USE_OSG)
    sptGetStats(txt);
    DrawString("%s\n", txt);
#endif
    speedgrassGetStats(txt);
    DrawString("%s\n", txt);
#endif
#endif

#if platOS == OS_WIN32 && (!defined(RELEASE_CLIENT) || RELEASE_CLIENT == 0)
	std::vector<std::wstring> nvHistory = updtGetNvHistory();
	for ( auto it = nvHistory.begin(); it != nvHistory.end(); ++it )
	{
		DrawString(L"%ls\n", it->c_str());
	}
#endif
}

phui::View* CreateGameInfoView(_xmlNode* xmlnode, phui::View* parent)
{
	return new GameInfoView;
}

/************************************************************************************************
	DisplayUpdateMeter
*************************************************************************************************/

void GameInfoView::DisplayUpdateMeter()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();

	ctx->DisableTexture();
	ctx->SetColor(1,1,1);

	glBegin(GL_POINTS);
	int t = 0;
	for(int i=0; i < _UPDATE_METER_COUNT; i++)
	{
		float x = t / (float)_UPDATE_METER_TIME * (Width() - (_lmargin+_rmargin));
		if(t > _UPDATE_METER_TIME) break;

		glVertex3f(x,0,0);
		t += _ReceivedUpdateDeltas[i];
	}
	glEnd();

	_textposy -= 5;
	if(_textposy < _miny) _miny = _textposy;
}

void PersHUDManager::AddObject(HUDObject* o)
{
	objects.push_back(o);

	switch(o->type)
	{
		case DEATH:
		{
			HUDDeath* hd = dynamic_cast<HUDDeath*>(o);

			float x = (float)hd->pos.x;
			float y = (float)hd->pos.y;

			std::wstring id = chatGetWideCallsign(hd->handle);
			const std::wstring& loc = uiLocString("hud.death");

			wchar_t notes[MAX_WP_NOTE_LENGTH];
			swprintf(notes, MAX_WP_NOTE_LENGTH-1, loc.c_str(), id.c_str());

			id.insert(0, L"D");
			ChatHandle handle = chatGetHandle(id.c_str());

			Waypoint waypoint(true, handle, hd->handle, WPT_DEATH, 0, (int)x, (int)y, 0, UTF8(notes));
			waypoint.setShared(false);

			PlayerClient()->SetWaypoint(waypoint);
		}
		break;
	}
}

void PersHUDManager::Update()
{
	// loop over and expire object as needed
	if(objects.empty())
		return;

	HUDObjectList::iterator it = objects.begin();

	// used to adjust offscreen waypoints so they don't pile up on each other
	SINT32 left = 0;
	SINT32 right = 0;
	phui::View* child = NULL;
	phui::CSSViewDrawer* drawer = NULL;

	while(it != objects.end())
	{
		HUDObject* ho = *it;

		if(!ho->Update())
		{
			it = objects.erase(it);
			delete ho;
		}
		else
		{
			if(ho->type == WAYPOINT)
			{
				HUDWaypoint* hwp = dynamic_cast<HUDWaypoint*>(ho);

				if(hwp && hwp->ui)
				{
					phui::View* child = hwp->ui->ChildAt(0);

					if(child && hwp->ui->offscreen)
					{
						FLOAT32 height = child->Height();
						FLOAT32 yAdjust = 0.0f;

						if(hwp->ui->offscreen < 0)
						{
							if(left)
								yAdjust = height * 1.25f * left;

							++left;
						}
						else
						{
							if(right)
								yAdjust = height * 1.25f * right;

							++right;
						}

						child->MoveBy(0.0f, yAdjust);
					}

					if(child)
					{
						drawer = dynamic_cast<phui::CSSViewDrawer*>(child->GetDrawer());

						if(drawer)
						{
							FLOAT32 xAdjust = 10.0f;
							FLOAT32 yAdjust	= 0.0f;

							if(hwp->ui->offscreen < 0)
							{
								drawer->SetShadowX(0);
								drawer->SetShadowY(child->Y() + (child->Height() * .5f));
							}
							else if(hwp->ui->offscreen > 0)
							{
								drawer->SetShadowX(hwp->ui->Width());
								drawer->SetShadowY(child->Y() + (child->Height() * .5f));
								xAdjust = -xAdjust;
							}
							else if(!hwp->useTail)
							{
								drawer->SetShadowX(child->X());
								drawer->SetShadowY(child->Y());
								xAdjust = 0.0f;
							}
							else
							{
								drawer->SetShadowX(child->X() + (child->Width() * .5f));
								drawer->SetShadowY(child->Y() - child->Height());
								xAdjust = 0.0f;
							}

							child->MoveBy(xAdjust, yAdjust);
						}
					}
				}
			}

			++it;
		}
	}
}

void PersHUDManager::Reset()
{
	// loop over and expire object as needed
	if(objects.empty())
		return;

	HUDObjectList::iterator it = objects.begin();

	for(; it != objects.end(); ++it)
	{
		HUDObject* ho = *it;
		delete ho;
	}

	objects.clear();
}

PersHUDManager::HUDObjectList& PersHUDManager::GetObjects()
{
	return objects;
}

bool PersHUDManager::IsVisible()
{
	if(!uiInGame())
		return false;

	vehVehicle* player = persGetPersonaVehiclePtr();

	if(!player || player->VehicleDeadTime)
		return false;

	if(player->VehicleType->Type == VEH_TYPE_PERSON)
	{
		if(fpFPSIsAim() || fpFPSIsBinocs())
			return false;
	}

	return true;
}

// get our singleton
PersHUDManager* PersHUDManager::GetInstance()
{
	return &_hudMgr;
}

PersHUDManager::HUDObject::HUDObject(HUD_OBJECT_TYPE t, int e) :
	type(t), expires(0)
{
	if(e)
		expires = frametime + e;
}

PersHUDManager::HUDObject::~HUDObject()
{
}

// if expired remove, otherwise keep by default
bool PersHUDManager::HUDObject::Update()
{
	return (expires && frametime >= expires) ? false: true;
}

bool PersHUDManager::HUDObject::IsVisible()
{
	return _hudMgr.IsVisible();
}

PersHUDManager::HUDDeath::HUDDeath(ChatHandle& h, FLOAT64VECTOR& p) :
	HUDObject(DEATH, HUD_DEATH_LIFE), handle(h), color(gIconFriendlyColor)
{
	pos = p;
}

FLOAT32 PersHUDManager::HUDDeath::GetAlpha()
{
	return FLOAT32(expires - frametime) / FLOAT32(HUD_DEATH_LIFE);
}

FLOAT32 PersHUDManager::HUDDeath::GetPlacement(FLOAT64VECTOR& p)
{
	FLOAT32 alpha = GetAlpha();
	if(alpha <= 0) return 0.0f;

	p.x = pos.x;
	p.y = pos.y;
	p.z = pos.z + (HUD_DEATH_HEIGHT * (1.0f - alpha));

	if(alpha > .9f)
		alpha = .9f;
	//if(alpha <= .1f)
	//	alpha = .1f;

	return alpha;
}

PersHUDManager::HUDWaypoint::HUDWaypoint(FLOAT32 o) :
	HUDObject(WAYPOINT), ui(NULL), file(NULL), voffset(o), minDistance(0.0f), useTail(true)
{
	pos.x = pos.y = pos.z = 0.0f;
}

PersHUDManager::HUDWaypoint::HUDWaypoint(const Waypoint& w, const char* f, FLOAT32 o) :
	HUDObject(WAYPOINT), ui(NULL), file(f), voffset(o), minDistance(0.0f), useTail(true)
{
	pos.x = (FLOAT64)w.x();
	pos.y = (FLOAT64)w.y();
	pos.z = 0.0f; //FindZ();
}

PersHUDManager::HUDWaypoint::HUDWaypoint(FLOAT64VECTOR& p, const char* f, FLOAT32 o) :
	HUDObject(WAYPOINT), ui(NULL), file(f), voffset(o), minDistance(0.0f), useTail(true)
{
	pos.x = p.x;
	pos.y = p.y;
	pos.z = p.z + voffset;
}

PersHUDManager::HUDWaypoint::HUDWaypoint(const StratDB::StratObject* strat, const char* f, FLOAT32 o, bool centerOnChildren) :
	HUDObject(WAYPOINT), ui(NULL), file(f), voffset(o), minDistance(0.0f), useTail(true)
{
	if(strat)
	{
		pos.x = strat->PosX();
		pos.y = strat->PosY();
		pos.z = 0.0f; //FindZ();

		std::vector<StratDB::StratObject*> stratList;

		switch(strat->StratClass())
		{
			case SDM_CLASS_CP:
				StratDB::FindStratObjects(stratList, SDM_CLASS_FACILITY, SDM_FACILITY_ARMYBASE, -1, (StratDB::StratObject*)strat);
			break;
			case SDM_CLASS_FACILITY:
			{
				if(strat->StratType() == SDM_FACILITY_FIREBASE)
					stratList = strat->ChildStratObjects();
				else
					StratDB::FindStratObjects(stratList, SDM_CLASS_BUILDING, SDM_BUILDING_CAPTURE, -1, (StratDB::StratObject*)strat);
			}
			break;
		}

		if(!stratList.empty())
		{
			UINT32 count = stratList.size();

			if(count)
			{
				SINT32 x = 0;
				SINT32 y = 0;

				for(unsigned int i = 0; i < count; ++i)
				{
					StratDB::StratObject* childStrat = stratList[i];

					x += childStrat->PosX();
					y += childStrat->PosY();
				}

				pos.x = FLOAT64(x / count);
				pos.y = FLOAT64(y / count);
			}
		}
	}
}

PersHUDManager::HUDWaypoint::HUDWaypoint(const ServerTrackedObject* sto, const char* f, FLOAT32 o) :
	HUDObject(WAYPOINT), ui(NULL), file(f), voffset(o), minDistance(0.0f), useTail(true)
{
	if(sto)
	{
		const WorldPosition_m& sPos = sto->position();

		pos.x = sPos.x();
		pos.y = sPos.y();
		pos.z = sPos.z() + voffset;
	}
}

PersHUDManager::HUDWaypoint::~HUDWaypoint()
{
	if(ui)
		ui->Close();

	ui = NULL;
}

bool PersHUDManager::HUDWaypoint::CreateUI(const char* file)
{
	if(!file || !uiInGame())
		return false;

	if(ui)
		ui->Close();

	ui = NULL;

	std::string path = "widgets/hud/";
	path += file;

	phui::View* v = uiLoadView(path.c_str());

	if(v)
	{
		ui = dynamic_cast<TargetView*>(v);
		if(!ui) v->Close();
	}

	return (ui) ? true : false;
}

bool PersHUDManager::HUDWaypoint::Update()
{
	if(!pos.z)
	{
		pos.z = FindZ();

		if(pos.z)
			pos.z += voffset;
	}

	vehVehicle* player = persGetPersonaVehiclePtr();
	FLOAT64 z = (pos.z) ? pos.z : (player) ? player->ModelData->VehiclePos.z: 0.0f;

	if(z && !ui)
		CreateUI(file);

	if(ui)
	{
		ui->SetTarget(pos.x, pos.y, z);
		ui->Update();

		FLOAT32 alpha = 1.0f;
		bool alphaCheck = true;

		if(player && player->VehicleType->Type && VEH_TYPE_PERSON)
		{
			if(fpFPSIsBinocs())
				alphaCheck = false;

			if(fpFPSIsDeployed())
			{
				alphaCheck = false;
				alpha = 0.0f;
			}

			if(fpFPSIsAim())
			{
				alphaCheck = false;
				alpha = 0.0f;
			}
		}

		if(ui->sz <= 1.0f && alphaCheck)
		{
			Vec2d uiPos		= Vec2d(ui->sx, ui->sy);
			Vec2d centerPos = Vec2d(ui->Width() * .5f, ui->Height() * .5f);
			Vec2d distance	= uiPos - centerPos;

			distance.x() = (float)ui->NormalizeValue(phui::View::SCALE_WIDTH, (float)distance.x());
			distance.y() = (float)ui->NormalizeValue(phui::View::SCALE_HEIGHT, (float)distance.y());

			static const FLOAT32 maxDist = 300.0f; // pixels

			if(distance.length2() <= maxDist * maxDist)
				alpha = (float)distance.length() / maxDist;

			if(alpha <= .1f)
				alpha = 0;
		}

		SINT32 distance = -1;

		if(player)
		{
			Vec3d pPos = Vec3d(player->ModelData->VehiclePos);
			Vec3d tPos = Vec3d(pos.x, pos.y, z);
			Vec3d diff = tPos - pPos;

			if(SCEN_MGR->IsActive())
			{
				distance = (SINT32)diff.length();
			}
			else
			{
				if(diff.length2() <= 200.0f * 200.0f)
					distance = (SINT32)diff.length();

				if(distance < 0 || distance > 200.0f)
					distance = -1;
			}

			if(minDistance && diff.length2() < (minDistance * minDistance))
				alpha = 0;
		}

		char script[50+1];
		snprintf(script, 50, "updateHUDObject(%d, %d, %.2f)", ui->offscreen, distance, alpha);

		uiRunScript(ui, script);
	}

	return HUDObject::Update();
}

FLOAT64 PersHUDManager::HUDWaypoint::FindZ()
{
	if(!uiInGame())
		return 0.0f;

	const FLOAT64 rayLength = 5000.0f;

	Vec3d start = Vec3d(pos.x, pos.y, rayLength);
	Vec3d dir	= Vec3d(0, 0, -1.0f);

	// put it in physical space
	start = physicsGameToPhysical(start);

	const colliderQuery::CollisionList& hits = collideRayList(COLLIDER_TILE, rayLength * 2.0f, start, dir, true);

	if(!hits.empty())
	{
		FLOAT64 z = hits.at(0).pos.z();

		if(z == 0.0f)
			return 0.01f;

		return z;
	}

	return 0.0f;
}

PersHUDManager::HUDPlayerWaypoint::HUDPlayerWaypoint(const vehVehicle* p, const char* f) :
	HUDWaypoint(2.0f), player(p)
{
	file = f;
	minDistance = 100.0f;
	useTail = false;
}

// requires a 3rd person vehicle
bool PersHUDManager::HUDPlayerWaypoint::Update()
{
	if(!player || !player->InUse || player->IsMe())
		return false;

	pos.x = player->PosData.Pos.x;
	pos.y = player->PosData.Pos.y;
	pos.z = player->PosData.Pos.z + voffset;

	return HUDWaypoint::Update();
}

PersHUDManager::HUDLeaderWaypoint::HUDLeaderWaypoint(const vehVehicle* leader) :
	HUDPlayerWaypoint(leader, "leader")
{
}

bool PersHUDManager::HUDLeaderWaypoint::Update()
{
	if(!player->IsMyMissionLeader)
		return false;

	return HUDPlayerWaypoint::Update();
}
