#include <syscfg.h> // Path is project specific.
#include <vector>
#include <map>
#include <math.h>

#include "libxml/xmlIO.h"
#include "libxml/tree.h"
#include "libxml/parser.h"

#include "phui/Viewport.h"
#include "phui/KeyCodes.h"
#include "phui/Periodical.h"
#include "phui/MessageLoop.h"
#include "phui/GLGraphicsContext.h"
#include "phui/CSSViewDrawer.h"
#include "phui/UIXML.h"
#include "phui/ConsoleView.h"
#include "phui/DropDownTask.h"
#include "phui/DOM.h"
#include "phui/IMEControl.h"
#include "phui/ImageView.h"
#include "trn/trnclutter.h"

#include "sysmain.h"
#include "include/vars.h"
#include "gl/glplat.h"
#include "pi/pi.h"
#include "dimain.h"
#include "imap/imapTypes.h"
#include "imap/imap.h"
#include "imap/ww2func.h"
#include "fio/fio.h"
#include "pers/pershud.h"
#include "ui/uiMain.h"
#include "uimap/uiMap.h"
#include "locl/loclApp.h"
#include "ld/ldbmp.h"
#include "js/jsGame.h"
#include "fio/fiod.h"
#include "ui/uiChat.h"
#include "ddp.h"
#include "miss/missionClientMessages.h"
#include "client/GameClient.h"
#include "client/clientConnections.h"
#include "ui/uiKeymap.h"
#include "draw/drawIcon.h"
#include "js/jsPlayer.h"
#include "ui/uiMovieView.h"
#include "ld/lddds.h"
#include "ld/ldpix.h"
#include "pi/piTextureAtlas.h"
#include "fp/fpFPS.h"
#include "ogl/oglGlobalStates.h"
#include "uimap/AWSManager.h"
#include "tut/TutorialManager.h"
#include "ui/uiGameEvents.h"
#include "trn/trnmain.h"
#include "math/Vec3d.h"
#include "pers/persmain.h"
#include "math/math3d.h"
#include "locl/loclLang.h"
#include "trn/trnRadClutter.h"
#include "trn/speedgrass.h"
#include "trn/trnvars.h"
#include "gr2/gr2Physics.h"
#include "ui/uiDotCommands.h"
#include "ogl/States.h"
#include "chat/chatWaypoint.h"
#include "scen/ScenarioManager.h"
#include "wwii/wwiiUTF.h"
#include "ez/EZManager.h"
#include "ld/ldtexture.h"

#ifdef GR2_BLEND_EDITOR
#include "gr2/gr2Anim.h"
#endif

#if USE_STATESET
#include "ogl/States.h"
#endif

#include <regex>
#include <unordered_map>
#define RENS std

#ifdef GR2_BLEND_EDITOR
#include "veh/vehcomms.h"
#endif

#if platOS == OS_WIN32
#include "ime/imeMain.h"
#endif

#include "sys/sysWinISO.h"

//#define UI_FONT_NAME "arialbd.ttf"
//#define UI_CFONT_NAME "simhei.ttf"
#define UI_FONT_NAME "wqy-zenhei.ttf"

#ifdef TESTMAC
//extern "C" void spawningin();
//extern "C" void spawningout();
#endif

/************************************************************************************************
	piGraphicsContext
*************************************************************************************************/

class piGraphicsContext : public phui::GLGraphicsContext
{
public:
	virtual void SetTexture(int texid);
	virtual void EnableTexture();
	virtual void DisableTexture();
#if(0)
	virtual void SetColor(float r, float g, float b, float a);
	virtual void DrawQuad(const float* coords, const float* uv = 0);
	virtual void DrawLineStrip(int count, const float* coords, bool closed = false);
	virtual void DrawLineStrip(int count, float const* coords, bool closed, float width);
#endif

	virtual void PopState();
};

extern phui::View* CreateReadoutView(_xmlNode* xmlnode, phui::View* parent);
extern phui::View* CreateGameInfoView(_xmlNode* xmlnode, phui::View* parent);

static bool map_keyinput(CHAR_INPUT*, int* keycode, unsigned short* unicode);
static void push_gl_state();
static void pop_gl_state();
static std::string munge_filename(const char* name);
static int fioOpenCallback(const char* name);
static int cfgOpenCallback(const char* name);
static int fioCloseCallback(int fd);
static size_t fioFileSizeCallback(const char* name);
static size_t cfgFileSizeCallback(const char* name);
static size_t fioSeekCallback (int fd, size_t offset);
static int uiLoadImageFileToSurface(const char* Filename);
#if defined(BETA_RELEASE)
static int interpreter(const char* line, int argc, const char** argv, int refcon);
#endif
static void _jserror(JSContext* cx, const char* message, JSErrorReport* report);
static phui::View* uiRankIconCreate(_xmlNode*, phui::View* parent);
static phui::View* uiMapKeyIconCreate(_xmlNode*, phui::View* parent);
static phui::View* uiCompassCreate(_xmlNode*, phui::View* parent);
static phui::View* uiImageUVCreate(_xmlNode*, phui::View* parent);
static phui::View* uiScenImageCreate(_xmlNode*, phui::View* parent);
static phui::View* uiTargetViewCreate(_xmlNode*, phui::View* parent);
static phui::View* uiBrigadeDecalCreate(_xmlNode* node, phui::View* parent);
static phui::View* uiContactIconCreate(_xmlNode* node, phui::View* parent);
static phui::View* uiFlagIconCreate(_xmlNode* node, phui::View* parent);
static phui::View* uiBarCompassCreate(_xmlNode* node, phui::View* parent);
phui::DOM::ElementDOM* uiImageUVCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);
phui::DOM::ElementDOM* uiTargetViewCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);
phui::DOM::ElementDOM* uiBarCompassViewCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent);
#if(0)
static int openLocDTD(const char*);
#endif
static int skipWordPadGarbage(int fd);
static void locProcessEntity(xmlEntityPtr entity, void* na);
static int xmlinputmatch(char const* filename){ return 1; }
static int cfgOpenWrite(const char*);
static void _focusModified(bool focus);

phui::IOCallbacks fioCallbacks = { fioOpenCallback, fioOpenWrite, fioRead, fioWrite, fioCloseCallback, fioFileSizeCallback, fioSeekCallback };
#if platOS == OS_WIN32
phui::IOCallbacks cfgioCallbacks = { cfgOpenCallback, cfgOpenWrite, fioRead, fioWrite, fioCloseCallback, cfgFileSizeCallback, fioSeekCallback };
#else
phui::IOCallbacks cfgioCallbacks = { cfgOpenCallback, cfgOpenWrite, fiocfgRead, fioWrite, fioCloseCallback, cfgFileSizeCallback, fioSeekCallback };
#endif

SINT32 g_iMouseX, g_iMouseY;

static phui::MessageLoop* _msgloop;
static phui::Viewport* _viewport;
static piGraphicsContext* _pictx;
static phui::CSSViewDrawerFactory* _drawfactory;
static bool _ingame;
static int _cursorstate;
static bool _holddowncursor;
//static phui::ConsoleView* _terminal;
static phui::View* _terminal;
static jsGame* _jsgame;
static phui::View* _spawnscreen;
typedef std::unordered_map<std::string, std::wstring> LocMap;
typedef std::unordered_map<std::string, std::string> InputMap;
static LocMap _locstrings;	// all translation strings; language with english fallback
static InputMap _inputkeys;	// english only keys for input device names and functions
static void _jsabort(const char*);
static SINT32 _isafk; // 0 = not afk, 1+ = frametime when we went afk
static bool _isautoafk;
static bool _killui;
static drawIconRef _mapkeyicons;
static int _loctime; // last time the loc strings were loaded
static std::string _despawnUI = "offline";
static int _despawnTab = 0;
static bool _despawnReset = true;
static UIXMLCALLBACK _uiJsCallback = nullptr;
static drawIconRef _ContactIcons;
static drawIconRef _FlagIcons;

extern drawIconRef _ColorRankIcons;

/************************************************************************************************
	uiInit
*************************************************************************************************/

static phui::GraphicsContext::TextureInfo uiGetAtlasTexture(const char* texname, bool repeatingTexture)
{
	const char* ptr = texname;

	if(SCEN_MGR->IsActive() && strstr("scenario://", ptr))
	{
		ptr += 11;
		ptr = SCEN_MGR->MapPath(ptr);
	}

	piTextureInfo texInfo;
	piTextureAtlasSet::getInstance()->getTexture(ptr, texInfo, false);//repeatingTexture);

	phui::GraphicsContext::TextureInfo info;
	info.texid = texInfo.texid;
	info.uv[0] = texInfo.uv[0];
	info.uv[1] = texInfo.uv[1];
	info.uv[2] = texInfo.uv[2];
	info.uv[3] = texInfo.uv[1];
	info.uv[4] = texInfo.uv[2];
	info.uv[5] = texInfo.uv[3];
	info.uv[6] = texInfo.uv[0];
	info.uv[7] = texInfo.uv[3];

#ifdef FORCE_DDS_NO_FLIP

	TEXTURE* t = piGetTextureData(info.texid);
	if(t && t->isdds())
	{
		info.uv[1] = texInfo.uv[3];
		info.uv[3] = texInfo.uv[3];
		info.uv[5] = texInfo.uv[1];
		info.uv[7] = texInfo.uv[1];
	}

#endif

	return info;
}

static phui::GraphicsContext::TextureInfo uiCreateTexture(int width, int height, int bpp)
{
	phui::GraphicsContext::TextureInfo info;

	int id = piCreateRawTexture(-1, 0, width, height, bpp, 8, CRT_FLAG_WRITABLE);

	// try to create it
	piGetTextureHandle(id, 0);
	//piSetTextureName(id, name); // pass in the name

	TEXTURE* texture = piGetTextureData(id);
	if(!texture) return info;

	info.texid = id;
	info.uv[0] *= texture->widthfactor[0];
	info.uv[1] *= texture->heightfactor[0];
	info.uv[2] *= texture->widthfactor[0];
	info.uv[3] *= texture->heightfactor[0];
	info.uv[4] *= texture->widthfactor[0];
	info.uv[5] *= texture->heightfactor[0];
	info.uv[6] *= texture->widthfactor[0];
	info.uv[7] *= texture->heightfactor[0];

	return info;
}

static void uiDestroyTexture(int texid)
{
	if(piIsTexture(texid, 0))
		piFreeTexture(texid, 0);
}

int uiInit()
{
	locInitStrings();

	_killui = false;

	phui::SetUIXMLIOCallbacks(&fioCallbacks);

	_pictx = new piGraphicsContext;
	_pictx->SetIO(&fioCallbacks);

	_msgloop = new phui::MessageLoop;
	_viewport = new phui::Viewport(_pictx, 0,0,(float)WantedWidth,(float)WantedHeight);

	// this also sets the default font for _pictx
	if(!uiSetFontSize(cfgPrefsGetInt(cfgPrefsGetGlobalName(cfgFONT_SIZE), UI_FONT_SIZE)))
		piExitProgram(_FL,"default font not found!");

	uiSetToolTips(cfgPrefsGetBool(cfgPrefsGetGlobalName(cfgENABLE_TOOLTIPS), true));

	_msgloop->AddHandler(_viewport);

	phui::js::Init();
	phui::js::SetErrorReporter(_jserror);
	phui::js::SetAbortHandler(_jsabort);
	phui::DOM::AddScriptMessageFilter(_msgloop);
	phui::DOM::RegisterTagWithDOM("mapview", uiMapCreateDOM);
	phui::DOM::RegisterTagWithDOM("movie", uiMovieCreateDOM);
	phui::DOM::RegisterTagWithDOM("chaticon", uiChatIconCreateDOM);
	phui::DOM::RegisterTagWithDOM("imageuv", uiImageUVCreateDOM);
	phui::DOM::RegisterTagWithDOM("target", uiTargetViewCreateDOM);
	phui::DOM::SetTextureCallback(uiGetAtlasTexture);

	_drawfactory = new phui::CSSViewDrawerFactory(_viewport, uiGetAtlasTexture);

	phui::RegisterTagWithUIXML("mapview", uiMapCreate);
	phui::RegisterTagWithUIXML("readout", CreateReadoutView);
	phui::RegisterTagWithUIXML("gameinfo", CreateGameInfoView);
	phui::RegisterTagWithUIXML("chat", uiChatCreate);
	phui::RegisterTagWithUIXML("keymap", uiKeymapCreate);
	phui::RegisterTagWithUIXML("rankicon", uiRankIconCreate);
	phui::RegisterTagWithUIXML("compass", uiCompassCreate);
	phui::RegisterTagWithUIXML("movie", uiMovieViewCreate);
	phui::RegisterTagWithUIXML("chaticon", uiChatIconCreate);
	phui::RegisterTagWithUIXML("chatinput", uiChatInputCreate);
	phui::RegisterTagWithUIXML("joycalib", uiJoystickCalibrationCreate);
	phui::RegisterTagWithUIXML("mapkeyicon", uiMapKeyIconCreate);
	phui::RegisterTagWithUIXML("imageuv", uiImageUVCreate);
	phui::RegisterTagWithUIXML("scenimage", uiScenImageCreate);
	phui::RegisterTagWithUIXML("target", uiTargetViewCreate);
	phui::RegisterTagWithUIXML("brigadedecal", uiBrigadeDecalCreate);
	phui::RegisterTagWithUIXML("contacticon", uiContactIconCreate);
	phui::RegisterTagWithUIXML("flagicon", uiFlagIconCreate);
	phui::RegisterTagWithUIXML("barcompass", uiBarCompassCreate);


#ifdef GR2_BLEND_EDITOR
	phui::RegisterTagWithUIXML("animscale", uiAnimWeightCreate);
#endif

	phui::SetUIXMLTextureCallback(uiGetAtlasTexture);
	phui::SetUIXMLCreateTextureCallback(uiCreateTexture);
	phui::SetUIXMLDestroyTextureCallback(uiDestroyTexture);
	phui::SetUIXMLDrawerFactory(_drawfactory);
	phui::SetUIXMLSetViewportSize(_viewport->Width(), _viewport->Height());
	phui::SetUIXMLClipboardCallback(piinfoClipboard);
	phui::SetUIXMLLocalizationCallback((phui::UIXMLLocalizationCallbackA)uiLocalize);
	phui::SetUIXMLLocalizationCallback((phui::UIXMLLocalizationCallbackW)uiLocalize);

	_jsgame = new jsGame;
	PlayerClient()->jsclient = _jsgame->jsplayer->jsclient.ptr();

	// moved terminal setup to uiPostInit()

	// lets set the default tooltip behavior
	_viewport->SetToolTipPath("widgets/tooltips/tooltip.uixml");
	_viewport->SetToolTipDelay(500);
	_viewport->SetFocusCallback(_focusModified);

	_ContactIcons = drawIconLoad("ui/textures/mapvehicles");
	_FlagIcons = drawIconLoad("ui/uidata/chat/commicons3");

	_isafk = 0;
	_isautoafk = false;

	_despawnUI		= (Online) ? "mission": "offline";
	_despawnTab		= 0;
	_despawnReset	= true;

	// force it to chinese is region is china
	if(gRegion == 5 && gLanguage != gRegion)
		cfgPrefsSetGlobal(cfgLANGUAGE_CHOICE, &gRegion);

	return 0;
}

/************************************************************************************************
	uiPostInit
*************************************************************************************************/

int uiPostInit()
{
	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/mapicon.dds");

	if ( -1 == piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/mapterrain.dds") )
  {
	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/newmapto.dds");
  }
  else
  {
	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/mapto.dds");
  }


//#if GREEN_INSTALL == 1
	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/mapbld_small.dds");
//#else
//	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/mapbld.dds");
//#endif

	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/mapcp.dds");
	piTextureAtlasSet::getInstance()->loadAtlas("ui/textures/atlas/ui.dds");

	int fd = fioOpen("ui/game.js");
	if(fd > -1)
	{
		size_t size = fioFileSize("ui/game.js");
		if(size > 0)
		{
			char* buffer = new char[size+1];
			buffer[size] = '\0';

			if(fioRead(fd, buffer, size) == size)
				_jsgame->RunScript(buffer);

			delete [] buffer;
		}

		fioClose(fd);
	}

#ifdef BETA_RELEASE
	if(OpenConsole && fioQuery() == ISDIRECTORY && HasKeyFile)
	{
		_terminal = uiLoadView("widgets/terminal", true, true);
		_terminal->MoveTo(1,-_terminal->Height());

		phui::ConsoleView* display = dynamic_cast<phui::ConsoleView*>(_terminal->FindView("terminal-display"));
		if(display)
			display->setCmdCallback(interpreter);

		_msgloop->AddCommonFilter(new phui::DropDownTask(_terminal, '~', -1, -1));
	}
#endif // BETA_RELEASE

	if(Online)
		uiChatInitWordFilter();

	return 0;
}

/************************************************************************************************
	uiKill
*************************************************************************************************/

int uiKill()
{
    Write_Debug("[uiKill] Begin");

	_killui = true;

	if(_mapkeyicons)
	{
		drawIconKill(_mapkeyicons);
		_mapkeyicons = nullptr;
	}

    if(_ContactIcons)
    {
        drawIconKill(_ContactIcons);
        _ContactIcons = nullptr;
    }

    if(_FlagIcons)
    {
        drawIconKill(_FlagIcons);
        _FlagIcons = nullptr;
    }

	uiChatKill();
/*
#if RELEASE_CLIENT == 0
//	delete _viewport;
//	delete _msgloop;
//	delete _drawfactory;
//	delete _pictx;

	uiChatKill();
	_jsgame->Release();
	phui::js::Kill();
#endif
*/
    Write_Debug("[uiKill] End");

	return 0;
}

/************************************************************************************************
	uiUpdate
*************************************************************************************************/

void uiUpdate()
{
	TUT_MGR->Update();
	SCEN_MGR->Update();

	if(uiInGame())
	{
		if(EZ_MGR)
			EZ_MGR->FrameUpdate();

		HUD_MGR->Update();
	}

	AWSManager::GetInstance()->Update();
	_msgloop->Update(frametime);
	phui::Periodical::Update(frametime);

	if(Online) // only matters when online
	{
		int left = PlayerClient()->SubscriptionExpires();

		// -2 means unmetered; -1 means don't alert; 0 means expired; > 0 is the number of minutes left
		if(left >= 0)
		{
			wchar_t minutes[10+1];
			swprintf(minutes, 10, L"%d", left);
			int icons[CHATICON_MAX] = {-1,-1,IDE_ICON_CAUTION};
			uiChatPrintUILocLine(icons, colorYELLOWALPHA, "mission.timeleft", minutes);

			if(uiInGame())
				uiGameEvent(GAMEEVENT_TIMELEFT, "%d", left);
		}
	}

#if RELEASE_CLIENT == 0
	if(_killui)
	{
		uiChatKill();
		_jsgame->Release();
		phui::js::Kill();
	}
#endif

#ifdef GR2_BLEND_EDITOR
	static bool firstUpdate = true;

	if(firstUpdate && uiFindView("offline"))
	{
		const vehType* vtype = vehTypeGetById("uk_buzzard");

		if(vtype)
		{
			uiDotCommand("/nospawn");
			uiSetDespawnInfo("offline");
			mlSetSpawnStuff(0,0,-1,1);

			WEAPON_SERIAL blah={0};
			vehcommSendVehicleRequest(vtype->VehicleTypeNum,&blah);
		}

		firstUpdate = false;
	}
#endif

//	if(!_ingame)
//		uiGC();
}

/************************************************************************************************
	uiDraw
*************************************************************************************************/

void uiDraw()
{
    static bool aa = false;

    if(aa)
        return;

	push_gl_state();

	if(!uiInGame())
	{
		// start with a clean slate; some ui screens may be smaller than the viewport in extreme wide screen setups
		glClearColor(0, 0, 0, 1.0f);
		oglClearTargets(oglCLEAR_VIEWPORT);
	}

	_viewport->Draw();

	pop_gl_state();
}

/************************************************************************************************
	uiDrawString - draw a string through the ui system
*************************************************************************************************/

void uiDrawString(const char* text, UINT32 color, UINT32 size)
{
	phui::GraphicsContext* ctx = uiViewport()->GetGraphicsContext();
	if(ctx == nullptr) return;

	//save our current state
	//ctx->PushState();
	ctx->EnableTexture();

	float r = (piRGBA_GETRED(color)/255.0f);
	float b = (piRGBA_GETBLUE(color)/255.0f);
	float g = (piRGBA_GETGREEN(color)/255.0f);
	float a = (piRGBA_GETALPHA(color)/255.0f);

	// set the color
	if(a)
		ctx->SetColor(r, g, b, a);

	//x -= ctx->FontAdvance(ctx->GetDefaultFont(), text)/2.0f;
	//y -= ctx->FontHeight(ctx->GetDefaultFont());

	// move us into position
	//ctx->MovePenPosition(x, y);

	//float scale = 1.0f;
	//glScalef(scale, scale, scale);

	// draw the string
	ctx->DrawString(ctx->GetDefaultFont(), text);

	// move us back
	//ctx->MovePenPosition(-x, -y);

	// return to the previous state
	//ctx->PopState();
}

/************************************************************************************************
	uiInput
*************************************************************************************************/

bool uiInput(imap_queue_entry* iqueue, int qcount, CHAR_INPUT* cqueue, int ccount)
{
static float lastMouseX,lastMouseY;

	float x,y;
	bool handled = false;
	static int mousedown;

	sysGetMousePosition(&g_iMouseX, &g_iMouseY);

	x = (float)g_iMouseX;
	y = (float)(WantedHeight-g_iMouseY);

	if(uiCursorState() != 0)
	{
		for(int i = 0; i < qcount; i++)
		{
			imap_queue_entry* ientry = &iqueue[i];
			unsigned int iac = ientry->iac;

			switch(iac)
			{
				case IAC_MOUSEBUTTON1:
				case IAC_MOUSEBUTTON2:
				case IAC_MOUSEBUTTON3:
					if(ientry->state)
					{
#ifdef BETA_RELEASE
						if(!Online && EnableMouseClickToDamageBuildings)
							trncdToggleDamageWithMouseClick(g_iMouseX, g_iMouseY);
						else if(!Online && SCEN_MGR->IsActive())
							trncdCollideWithMouse(g_iMouseX, g_iMouseY);
#endif
						handled |= _viewport->PostMouseDown(x,y, iac - IAC_MOUSEBUTTON1);
					}
					else
						handled |= _viewport->PostMouseUp(x,y, iac - IAC_MOUSEBUTTON1);
				break;

				case IAC_MOUSE_WHEEL_UP:
				case IAC_MOUSE_WHEEL_DOWN:
					handled |= _viewport->PostMouseScroll(x,y, (iac == IAC_MOUSE_WHEEL_UP) ? 1 : -1);
				break;
			}

			if(iac == IAC_MOUSEBUTTON1)
				mousedown = ientry->state;
		}
	}
	// mouse move
	if(((lastMouseX - x) || (lastMouseY - y)) && uiCursorState() != 0)
	{
		_viewport->PostMouseMove(x,y);
		if(mousedown)
			_viewport->PostMouseDrag(x,y,0);
	}

	// key events
	for(int i = 0; i < ccount; i++)
	{
		CHAR_INPUT* centry = &cqueue[i];
		int keycode;
		unsigned short unicode;
		bool chatinput = false;

		if(centry->bState)
			chatinput = uiChatInput(centry->uIac);

		handled |= chatinput;

		if(!chatinput)
		{
			if(map_keyinput(centry, &keycode, &unicode))
			{
				/*
				// hack in and close our despawn window if we hit escape and its up
				// probably a better way to do this
				if(uiInGame())
				{
					if(centry->uIac == IAC_ESCAPE)
					{
						vehVehicle* player = persGetPersonaVehiclePtr();

						if(player && !player->ModelData->DeclaredKilled)
						{
							// if we hit exit again, kill the popup
							phui::View* despawn = uiViewport()->FindView("despawn");

							if(despawn != nullptr)
								uiRunScript(despawn, "doDespawnAction(false);");
						}
					}
				}
				*/

				if(centry->bState)
					handled |= _viewport->PostKeyDown(keycode, unicode);
				else
					handled |= _viewport->PostKeyUp(keycode, unicode);
			}
		}
	}

	lastMouseX = x;
	lastMouseY = y;

	return handled;
}

/************************************************************************************************
	uiFocused
*************************************************************************************************/

bool uiFocused()
{
	return (uiFocusView() != nullptr);
}

/************************************************************************************************
	uiFocused
*************************************************************************************************/

phui::View* uiFocusView()
{
	return (_viewport) ? _viewport->FindFocusView(): nullptr;
}

/************************************************************************************************
	uiLoadView
*************************************************************************************************/

phui::View* uiLoadView(const char* filename, bool addtoviewport, bool show)
{
	std::string path = filename;
	path += ".uixml";
	//phui::SetUIXMLIOCallbacks(&fioCallbacks);

	phui::View* view = phui::LoadUIXML(path.c_str());
	if(view)
	{
		if(addtoviewport)
			_viewport->AddChild(view);

		phui::DOM::Init(view);

		if(show)
			view->Show();
	}

	#ifdef BETA_RELEASE
	//locInitStrings();
	#endif

	uiGC();
	uiConsoleprintf("------------- Loaded UIXML %s -------------\n", path.c_str());

	return view;
}

phui::View* uiFindView(const char* name)
{
	return uiViewport()->FindView(name);
}

/************************************************************************************************
	uiViewport
*************************************************************************************************/

phui::Viewport* uiViewport()
{
	return _viewport;
}

/************************************************************************************************
	uiInputString
*************************************************************************************************/

bool uiCheckLocTime(int& check)
{
	if(check < _loctime)
	{
		check = _loctime;
		return true;
	}

	return false;
}

/************************************************************************************************
	uiInputString
*************************************************************************************************/

const std::string& uiInputString(const char* n)
{
	static std::string name = "";

	InputMap::const_iterator found = _inputkeys.find(n);

	if(found != _inputkeys.end())
		return found->second;

	// make it clear its not a valid input key
	name = n;
	name.insert(0, "?");
	name.append("?");

	return name;
}

const std::string& uiInputString(int category, int id)
{
	return uiInputString((SCHAR*)loclLangGetString((ENUM_CATEGORIES)category, id));
}

/************************************************************************************************
	uiLocString
*************************************************************************************************/

const std::wstring& uiLocString(const char* n)
{
	static std::wstring name = L"";

	// try any scenario specific localizations
	const wchar_t* scenLoc = SCEN_MGR->GetLoc(n);

	if(scenLoc)
	{
		name = scenLoc;
		return name;
	}

	LocMap::const_iterator found = _locstrings.find(n);

	// lookup and return the loc string
	if(found != _locstrings.end())
		return found->second;

	// loc string not found

	// convert the name to widestring format
	name = uiUTF8toUTF16(n);

	// make it clear its not a valid loc string
	name.insert(0, L"?");
	name.append(L"?");

	// return it
	return name;
}

const std::wstring& uiLocString(int category, int id)
{
	return uiLocString((SCHAR*)loclLangGetString((ENUM_CATEGORIES)category, id));
}

const std::string& uiLocString2(const char* n)
{
	static std::string message = "";
	message = uiUTF16toUTF8(uiLocString(n).c_str());
	return message;
}

const std::string& uiLocString2(int category, int id)
{
	return uiLocString2((SCHAR*)loclLangGetString((ENUM_CATEGORIES)category, id));
}

bool uiHasLocString(const char* n)
{
	if(SCEN_MGR->GetLoc(n))
		return true;

	return (_locstrings.find(n) != _locstrings.end()) ? true: false;
}

// utf16 string version
const wchar_t* uiLocalize(const wchar_t* text)
{
	if(!text) return L"";

	static std::wstring buffer;

	// matches that hit for the current test
	RENS::wcmatch matches;
	RENS::wregex filter;

	// looks for string patterns like: ${some.localization.key}
	RENS::wregex re = RENS::wregex(L"\\$\\{([^\\}]+)\\}", RENS::regex::ECMAScript|RENS::regex_constants::icase);

	// set the current buffer to the incoming value
	buffer = text;
	UINT32 count = 0;

	while(RENS::regex_search(buffer.c_str(), matches, re) && matches.size() >= 2)
	{
		const std::wstring& key		= matches.str(1);
		const std::wstring& value	= uiLocString(UTF8(key.c_str()));
		std::wstring replace		= L"";

		replace = L"\\$\\{";
		replace += key;
		replace += L"\\}";

		filter.assign(replace, RENS::regex_constants::icase);

		try
		{
			// filter it
			buffer = RENS::regex_replace(buffer, filter, value);
		}
		catch (RENS::regex_error& /* e */) // not a valid regular expression
		{
			break;	// go ahead and bail out; we have some bad syntax or something
		}
		catch (std::exception& /* e */) // some other problem
		{
			break;	// go ahead and bail out; not sure why we got here
		}

		// just to make sure we don't get in an infinite loop
		if(count >= 100)
			break;

		++count;
	}

	return buffer.c_str();
}

// utf8 version
const wchar_t* uiLocalize(const char* text)
{
	if(!text) return L"";

	static std::string buffer;

	// matches that hit for the current test
	RENS::cmatch matches;
	RENS::regex filter;

	// looks for string patterns like: ${some.localization.key}
	RENS::regex re = RENS::regex("\\$\\{([^\\}]+)\\}", RENS::regex::ECMAScript|RENS::regex_constants::icase);

	// set the current buffer to the incoming value
	buffer = text;
	UINT32 count = 0;

	while(RENS::regex_search(buffer.c_str(), matches, re) && matches.size() >= 2)
	{
		const std::string& key		= matches.str(1);
		const std::string& value	= uiLocString2(key.c_str());
		std::string replace			= "";

		replace = "\\$\\{";
		replace += key;
		replace += "\\}";

		filter.assign(replace, RENS::regex_constants::icase);

		try
		{
			// filter it
			buffer = RENS::regex_replace(buffer, filter, value);
		}
		catch (RENS::regex_error& /*e*/) // not a valid regular expression
		{
			break;	// go ahead and bail out; we have some bad syntax or something
		}
		catch (std::exception& /*e*/) // some other problem
		{
			break;	// go ahead and bail out; not sure why we got here
		}

		// just to make sure we don't get in an infinite loop
		if(count >= 100)
			break;

		++count;
	}

	return UTF16(buffer.c_str());
}

#if platOS == OS_WIN32

void uiIMEMessageProc(const MSG& m)
{
	if(m.message == WM_KEYDOWN && m.wParam == VK_PROCESSKEY)
	{
		UINT32 vk = imeProcessKey();

		if(vk != VK_PROCESSKEY)
			dimainVKDown(vk, ((m.lParam >> 16) & 0x000000FF));
	}
}
#endif

typedef std::unordered_map<std::string, int> TexMapType;

static TexMapType _texmap;

/************************************************************************************************
	uiGetTexture
*************************************************************************************************/

int uiGetTexture(const char* texname)
{
	int texid;
	std::string path = munge_filename(texname);

	TexMapType::iterator it = _texmap.find(path);
	if(it != _texmap.end())
	{
		texid = it->second;
	}else
	{
		texid = uiLoadImageFileToSurface(path.c_str());
		_texmap[path] = texid;
	}

	return texid;
}

/************************************************************************************************
	uiFreeTexture
*************************************************************************************************/

void uiFreeTexture(const char* texname)
{
	int texid;
	std::string path = munge_filename(texname);

	TexMapType::iterator it = _texmap.find(path);
	if(it != _texmap.end())
	{
		texid = it->second;
		piFreeMipTextures(texid);
		_texmap.erase(it);
	}
}

void uiExitWorld() { uiGameToUI(); }

/************************************************************************************************
	uiGameToUI
*************************************************************************************************/

void uiGameToUI()
{
	HUD_MGR->Reset();

	uiMapSaveMinimapScale();
	uiMapHideIngameMap(true);
	persHUDHideHUD(true);
	//persHUDHideReadout(true);
	//persHUDHideGameInfo(true);

	bool training = false;

	if(SCEN_MGR->IsActive())
	{
		if(!SCEN_MGR->HasFlag(ScenarioManager::RESPAWN_ME))
		{
			// load our despawn UI
			phui::View* despawnUI = uiLoadView(_despawnUI.c_str());

			// look up its body so we can set the tab
			phui::View* body = (despawnUI != nullptr) ? despawnUI->FindView("body"): nullptr;
			if(body) body->SetValue(_despawnTab);

			// let everyone know I'm no longer spawned
			SCEN_MGR->SetPlayerState("spawned", "false");
		}
	}
	else
	{
		training = TUT_ACTIVE;

		// make sure we're back on the campaign persona and brigade
		if(Online && _despawnReset && !PlayerClient()->LoadSavedPersona())
			uiSetDespawnInfo("online");

		// load out despawn UI
		phui::View* despawnUI = uiLoadView(_despawnUI.c_str());

		// look up its body so we can set the tab
		phui::View* body = (despawnUI != nullptr) ? despawnUI->FindView("body"): nullptr;
		if(body) body->SetValue(_despawnTab);

		// reset if we were requested to do so
		if(_despawnReset)
			uiSetDespawnInfo((Online) ? "mission": "offline");
	}

	uiChatShow();

	_ingame = false;

#if RELEASE_CLIENT
	switch(OverrideSpawner)
	{
	case BENCHANTWERP_OVERRIDE:
	case BENCHRAMAGEN_OVERRIDE:
		{
			// Reset to default
			OverrideSpawner = NO_OVERRIDE;
			break;
		}
	}
#endif

	imapAxisInputState(0);
	imapButtonInputState(0);
	uiSetCursor(1);

	uiGC();

	// clear out any queued map events
	uiMapClearPlayerEvents();

	if(Online)
	{
        PlayerClient()->OnDespawn();

		/*
		// if we get to here and we're still connected, disconnect us
		if(PlayerClient()->CellState()->Connected())
			PlayerClient()->CellState()->Disconnect();

		// let the host know we've despawned
		if(PlayerClient()->MapState()->Connected())
		{
			//MapHostConnection* mapConn = dynamic_cast<MapHostConnection*>(PlayerClient()->MapState()->connection);
			if(mapConn) mapConn->doDespawned();
		}
		*/
	}

	radclutterRemoveAll();
	trnClutterReset();

	speedgrassClear();
	gr2PhysicsReset();

	// let the tutorial manager know we've changed states
	TUT_MGR->DoEvent(TUT_EVENT_DESPAWN);

	// run any scenario specific dot commands associated with despawning
	SCEN_MGR->RunCommands(TUT_EVENT_DESPAWN);

#ifdef TESTMAC
//  spawningout();
#endif

	if(SCEN_MGR->IsActive() && SCEN_MGR->HasFlag(ScenarioManager::RESPAWN_ME))
	{
		if(SCEN_MGR->Respawn())
		{
			disableButtonInput = FALSE;
			disableAxisInput = FALSE;
			uiSetCursor(0);
		}
		else
		{
			phui::View* respawn = uiFindView("respawn");
			if(respawn) respawn->Close();

			// load our despawn UI
			phui::View* despawnUI = uiLoadView(_despawnUI.c_str());

			// look up its body so we can set the tab
			phui::View* body = (despawnUI != nullptr) ? despawnUI->FindView("body"): nullptr;
			if(body) body->SetValue(_despawnTab);
		}
	}
}

/************************************************************************************************
	uiUIToGameStart
*************************************************************************************************/

void uiUIToGameStart()
{
	uiViewport()->HideToolTip();

	phui::View* respawnView = uiFindView("respawn");

	// close all views except chat and terminal
	for(unsigned int i = 0; i < uiViewport()->ChildCount();)
	{
		phui::View* child = uiViewport()->ChildAt(i);
		if(child != uiChatWindow() && child != _terminal && child != respawnView)
		{
			uiViewport()->RemoveChild(child);
			delete child;
			continue;
		}
		i++;
	}

	uiChatHide(true);

	imapAxisInputState(1);
	imapButtonInputState(1);
	uiSetCursor(0);

	if(SCEN_MGR->HasFlag(ScenarioManager::RESPAWN_ME) && respawnView)
		_spawnscreen = respawnView;
	else if(!_spawnscreen)
		_spawnscreen = uiLoadView("spawn");

	uiUpdate();
	uiDraw();
	piFlip();
}

/************************************************************************************************
	uiUIToGameEnd
*************************************************************************************************/

void uiUIToGameEnd()
{
	if(!_spawnscreen)
	{
		return;
	}

	// load our custom map if we have one
	//SCEN_MGR->LoadTerrain();

	// Delay game entry by one frame to avoid spawning in before all objects are drawn.
	static int delay = 0;
	static int numframes = 1;

	if(delay++ < numframes)
	{
		return;
	}

	delay = 0;

#ifndef LEAVE_UI_TEXTURES

	std::vector<std::string> removetex;

	TexMapType::iterator it = _texmap.begin();
	for(;it != _texmap.end();++it)
	{
		piFreeMipTextures(it->second);
		if(!piIsTexture(it->second, 0))
			removetex.push_back(it->first);
		else
			piAddTexRef(it->second);
	}

	std::vector<std::string>::iterator rit = removetex.begin();
	for(;rit != removetex.end(); ++rit)
	{
		it = _texmap.find(*rit);
		if(it != _texmap.end())
			_texmap.erase(it);
	}

#endif

	_ingame = true;

	// make sure the chat window gets loaded
	uiChatShow();

	// make sure the hud gets loaded
	persHUDShowHUD();

	if(_spawnscreen)
	{
		uiViewport()->RemoveChild(_spawnscreen);
		delete _spawnscreen;
		_spawnscreen = 0;
	}

#ifdef BETA_RELEASE
	if(SuperTroop)
		fpFPSSetSuperTroop(SuperTroop);
#endif
	uiGC();

	// let the tutorial manager know we've changed states
	TUT_MGR->DoEvent(TUT_EVENT_SPAWN);

	// run any scenario specific dot commands associated with spawning
	if(SCEN_MGR->IsActive())
	{
		SCEN_MGR->RunCommands(TUT_EVENT_SPAWN);
		SCEN_MGR->LoadObjects(ScenarioObject::ON_SPAWN);
		SCEN_MGR->SetFlag(ScenarioManager::RESPAWN_ME, false);
		SCEN_MGR->SetPlayerState("spawned", "true");
	}

	if(EZ_MGR)
		EZ_MGR->OnSpawn();

	/*
	// add some waypoints
	HUD_MGR->AddObject(new PersHUDManager::HUDWaypoint(player->ModelData->VehiclePos, "spawn"));

	const std::vector<Waypoint>& waypoints = PlayerClient()->Waypoints();

	for(int i = 0; i < waypoints.size(); i++)
	{
		const Waypoint& w = waypoints.at(i);

		if(w.type() == WPT_TARGET)
			HUD_MGR->AddObject(new PersHUDManager::HUDWaypoint(w, "target"));
		else if(w.type() == WPT_MOBILESPAWN)
			HUD_MGR->AddObject(new PersHUDManager::HUDWaypoint(w, "ms"));
	}
	*/

#ifdef GR2_BLEND_EDITOR
	vehVehicle* player = persGetPersonaVehiclePtr();

	if(player)
		player->ModelData->VehiclePos.z = 1.0f;

	uiDotCommand("/tspawn clear1");
	uiDotCommand("/blendeditor");
#endif

	// make sure our new hud objects get updated on the first frame once spawned in
	HUD_MGR->Update();
}

/************************************************************************************************
	uiRunScript
*************************************************************************************************/

void uiSetDespawnInfo(const char* screen, int tab, bool reset)
{
	_despawnUI		= screen;
	_despawnTab		= (tab >= 0) ? tab: 0;
	_despawnReset	= reset;
}

void uiJSCallback(JSContext* ctx, UINT32 argc, jsval* argv)
{
	if(_uiJsCallback)
		_uiJsCallback(ctx, argc, argv);
}

JSBool uiCallJS(const char* function, int argc, jsval* argv, UIXMLCALLBACK callback)
{
	if(!_jsgame) return JS_FALSE;

	jsval rval;

	_uiJsCallback = callback;

	return _jsgame->CallFunction(function, argc, argv, &rval);
}

JSBool uiCallJS(const char* function, UIXMLCALLBACK callback, int argc, ...)
{
	if(!_jsgame) return JS_FALSE;

	if(argc > JS_MAX_CALL_ARGS)
		argc = JS_MAX_CALL_ARGS;

	//jsval rval;
	jsval argv[JS_MAX_CALL_ARGS]; // up to JS_MAX_CALL_ARGS argments

	va_list args;
	va_start(args,argc);

	for(int i = 0; i < argc; ++i)
	{
		const char* arg = va_arg(args, const char*);
		argv[i] = phui::js::NewString((arg) ? arg : "");
	}

	va_end(args);

	return uiCallJS(function, argc, argv, callback);
}

JSBool uiCallJS(const wchar_t* function, UIXMLCALLBACK callback, int argc, ...)
{
	if(!_jsgame) return JS_FALSE;

	if(argc > JS_MAX_CALL_ARGS)
		argc = JS_MAX_CALL_ARGS;

//	jsval rval;
	jsval argv[JS_MAX_CALL_ARGS]; // up to JS_MAX_CALL_ARGS argments

	va_list args;
	va_start(args,argc);

	for(int i = 0; i < argc; ++i)
	{
		const wchar_t* arg = va_arg(args, const wchar_t*);
		argv[i] = phui::js::NewString((arg) ? arg : L"");
	}

	va_end(args);

	return uiCallJS(UTF8(function), argc, argv, callback);
}

/************************************************************************************************
	uiRunScript
*************************************************************************************************/

void uiRunScript(const char* script)
{
	if(_jsgame)
		_jsgame->RunScript(script);
}

void uiRunScript(phui::View* target, const char* script)
{
	if(!target || !*script) return;

	phui::DOM::ElementDOM* viewDom = dynamic_cast<phui::DOM::ElementDOM*>(target->UserData(DOM_VIEW_USERDATA));
	if(viewDom) viewDom->RunScript(script);
}

int uiGameEvent(jsval* argv, int argc)
{
	if(!argv || !argc)
		return 0;

	uiCallJS("ongameevent", argc, argv);

	return JSVAL_TO_INT(argv[0]);
}

// assumes variable args are wide strings
int uiGameEvent(int e, int argc, ...)
{
	if(argc > JS_MAX_CALL_ARGS - 1)
		argc = JS_MAX_CALL_ARGS - 1;

//	jsval rval;
	jsval argv[JS_MAX_CALL_ARGS]; // up to JS_MAX_CALL_ARGS argments

	va_list args;
	va_start(args,argc);

	// add game event argument
	argv[0] = INT_TO_JSVAL(e);

	for(int i = 0; i < argc; ++i)
	{
		const wchar_t* arg = va_arg(args, const wchar_t*);
		argv[i + 1] = phui::js::NewString((arg) ? arg : L"");
	}

	va_end(args);

	// 1 for the event itself
	uiCallJS("ongameevent", 1 + argc, argv);

	return e;
}

int uiGameEvent(int e, const char* args, ...)
{
	char script[128+1];

	if(args)
	{
		char arguments[100+1];

		va_list marker;
		va_start(marker, args);
		vsnprintf(arguments, 100, args, marker);
		va_end(marker);

		snprintf(script, 128, "game.ongameevent(%d, %s)", e, arguments);
	}
	else
	{
		snprintf(script, 128, "game.ongameevent(%d)", e);
	}

	uiRunScript(script);

	return e;
}

/************************************************************************************************
	uiInGame
*************************************************************************************************/

bool uiInGame()
{
	return _ingame;
}

/************************************************************************************************
	uiSetCursor
*************************************************************************************************/

void uiSetCursor(int state)
{
	_holddowncursor = false;

	if ( state == _cursorstate )
	{
		return;
	}

	_cursorstate = state;

	if(state == 0)
	{
		ddpHideCursor();

		if(_viewport)
			_viewport->HideToolTip();
	}
	else
	{
		ddpShowCursor();
	}
}

/************************************************************************************************
	uiCursorState
*************************************************************************************************/

int uiCursorState()
{
	return _cursorstate;
}

/************************************************************************************************
	uiSetFontSize
*************************************************************************************************/

bool uiSetFontSize(unsigned int fontsize, const char* font)
{
	const char* setFont = (font) ? font: UI_FONT_NAME;
	return (_viewport) ? _viewport->SetDefaultFont(setFont, (int)(fontsize * WantedHeight / 768.0)): false;
}

/************************************************************************************************
	uiSetToolTips
*************************************************************************************************/

void uiSetToolTips(bool on)
{
	if(_viewport)
		_viewport->SetToolTips(on);
}

/************************************************************************************************
	uiSetAFK
*************************************************************************************************/

void uiSetAFK(SINT32 when)
{
	_isafk = when;

	if(!when)
		_isautoafk = false;
}

/************************************************************************************************
	uiSetAutoAFK - set when this was an autoafk
*************************************************************************************************/

void uiSetAutoAFK(bool autoafk)
{
	_isautoafk = autoafk;
}

/************************************************************************************************
	uiIsAFK
*************************************************************************************************/

SINT32 uiIsAFK()
{
	return _isafk;
}

/************************************************************************************************
	uiIsAutoAFK
*************************************************************************************************/

bool uiIsAutoAFK()
{
	return _isautoafk;
}

/************************************************************************************************
	uiImapCursorCallback
*************************************************************************************************/

unsigned int uiImapCursorCallback(int param, unsigned int functionCode, unsigned int unused)
{
	if(param)
	{
		if(uiCursorState() == 0)
		{
			uiSetCursor(1);
			_holddowncursor = true;
		}
	}else
	{
		if(uiCursorState() != 0 && _holddowncursor)
		{
			uiSetCursor(0);
			_holddowncursor = false;
		}
	}

	return 0;
}

bool uiShowBuilderInfo(const wchar_t* strat, bool on)
{
	static bool _drawBuilderMessage = false;

	// Already displaying
	if(_drawBuilderMessage)
	{
		_drawBuilderMessage = on;

		// hide the popup if not on
		if(!on) uiGameEvent(GAMEEVENT_POPUP_BUILDER);

		return true;
	}

	if(on)
	{
		if(!strat)
		{
			_drawBuilderMessage = false;
			return false;
		}

		// show the popup
		uiGameEvent(GAMEEVENT_POPUP_BUILDER, 1, strat);
	}

	_drawBuilderMessage = on;

	return true;
}
bool uiHasLocFile(const char* name)
{
	std::string path = munge_filename(name);
	return fioAccess(path.c_str());
}

static int fioOpenCallback(const char* name)
{
	int fd = -1;
	std::string path = munge_filename(name);
	std::string ext;
	size_t n = path.rfind('.');

	if(n != std::string::npos)
		ext = path.substr(n);

#ifdef BETA_RELEASE
	if(path.compare(0, 6, "loc://") == 0)
		fd = fiodOpen(path.c_str());
#endif
	if(fd < 0)
		fd = fioOpen(path.c_str());

	if(fd > -1 && (ext == ".xml" || ext == ".uixml"))
		skipWordPadGarbage(fd);

	return fd;
}

// stupid motherfucking Notepad adds garbage to the beginning of a UTF-8
// so we skip past to the first <. stupid motherfucking windoze, fucking suck it.
static int skipWordPadGarbage(int fd)
{
	int i = 0;
	for(;;i++)
	{
		char c;
		if(fioRead(fd, &c, 1) != 1) break;
		if(c == '<')
		{
			fioSeek(fd, -1,  SEEK_CUR);
			return i;
			break;
		}
	}

	return 0;
}

// Don't have to change Util.h in phui with this function
static int cfgOpenWrite(const char* name)
{
	int fd = fiocfgOpenWrite(name, true);
	return fd;
}

static size_t fioFileSizeCallback(const char* name)
{
	std::string path = munge_filename(name);
	return (int)fioFileSize(path.c_str());
}

static int cfgOpenCallback(const char* name)
{
	std::string path = munge_filename(name);
	int fd = fiocfgOpen(path.c_str(), true);
	return fd;
}

static size_t cfgFileSizeCallback(const char* name)
{
	std::string path = munge_filename(name);
	return (size_t)fiocfgFileSize(path.c_str(), true);
}

static size_t fioSeekCallback (int fd, size_t offset)
{
	return fioSeek(fd, offset, SEEK_CUR);
}

static int fioCloseCallback(int fd)
{
	fioClose(fd);
	return 0;
}

static std::string munge_filename(const char* name)
{
	std::string path = name;
	std::string ext;
	size_t n;

	n = path.rfind('.');
	if(n != std::string::npos)
		ext = path.substr(n);

	if(path.compare(0, 6, "loc://") == 0)
	{
		std::string filename = path.substr(6);
		path = "languages/"; path += g_szLanguages[gLanguage];
		path += "/"; path += filename;

		if(gLanguage != IDE_LANGUAGE_ENGLISH && !fioAccess(path.c_str()))
		{
			path = "languages/english/";
			path += filename;
		}

	}
	else if(ext == ".uixml")
	{
		path = "ui/xml/";
		path += name;
	}
	else if(ext == ".ttf")
	{
		path = "fonts/";
		path += name;
	}
	//else if(gRegion != IDE_LANGUAGE_ENGLISH && (ext == ".bmp" || ext == ".dds")) // check for any other language
	else if(gRegion == IDE_LANGUAGE_CHINESE && (ext == ".bmp" || ext == ".dds")) // only check chinese for now
	{
		// lets see if we have any localized images; not done for english
		std::string filename = path.substr(0, path.size() - 4);

		filename += ".dds";

		const char* regional = fioGetRegionalPath(filename.c_str());

		if(fioAccess(regional))
		{
			path = regional;
		}
		else
		{
			filename.erase(filename.end() - 4, filename.end());
			filename += ".bmp";

			regional = fioGetRegionalPath(filename.c_str());

			if(fioAccess(regional))
				path = regional;
		}
	}

	return path;
}

// this function is obsolete and will be deleted pretty soon
// once I make sure the new ui.dtd loading stuff is working properly
#if(0)
static int openLocDTD(const char* name)
{
	return -1;

	// Cache language between calls, so it needs to start
	// as an absolutely-not-zero value.
	static UINT32 language = ~0;

	///OPT: Move the inside of the if to a subroutine,
	///and move the variables with it.
	std::string path, filename;
	size_t filesize;
	char* buffer;
	int fd = -1;
	int fd1 = -1;

	filename = name;
	if(filename.compare(0, 6, "loc://") == 0)
		filename = filename.substr(6);

	if(language != gLanguage)
	{
		language = gLanguage;

		fd = fiocfgOpenWrite(filename.c_str(), true);
		if(fd > -1)
		{
			path = "languages/"; path += g_szLanguages[gLanguage]; path += "/";
			path += filename;

			int fd2 = -1;
#ifdef BETA_RELEASE
			fd2 = fd1 = fiodOpen(path.c_str());
#endif
			if(fd1 < 0)
				fd1 = fioOpen(path.c_str());

			if(fd1 > -1)
			{
				if(fd2 > -1)
					filesize = fiodFileSize(path.c_str()) - skipWordPadGarbage(fd1);
				else
					filesize = fioFileSize(path.c_str()) - skipWordPadGarbage(fd1);

				buffer = new char[filesize];

				fioRead(fd1, buffer, filesize);
				fioWrite(fd, buffer, filesize);

				delete [] buffer;
				fioClose(fd1);
			}
			fioPutc(fd,'\n');

			if(language > 0)
			{
				path = "languages/english/";
				path += filename;
				fd1 = fioOpen(path.c_str());

				if(fd1 > -1)
				{
					filesize = fioFileSize(path.c_str()) - skipWordPadGarbage(fd1);
					buffer = new char[filesize];

					fioRead(fd1, buffer, filesize);
					fioWrite(fd, buffer, filesize);

					delete [] buffer;
					fioClose(fd1);
				}
			}
		}
		fioClose(fd);
	}

	return fiocfgOpen(filename.c_str(), true);
}
#endif

void locInitStrings(bool force)
{
	static unsigned int lang = 0xffffffff;

	if(!force && gLanguage == lang)
		return;

	// save the current language
	lang = gLanguage;

	xmlDtdPtr pDtd;
	std::string path;

	_locstrings.clear();

	xmlRegisterInputCallbacks(xmlinputmatch,
		(xmlInputOpenCallback)fioOpenCallback,
		(xmlInputReadCallback)fioRead,
		(xmlInputCloseCallback)fioClose);

	// lets load the default language first
	path = "loc://ui.dtd";

	// load the doc
	pDtd = xmlParseDTD(nullptr, (xmlChar*)path.c_str());

	if(pDtd)
	{
		xmlHashScan((xmlHashTablePtr)pDtd->entities, (xmlHashScanner)locProcessEntity, &_locstrings);
		xmlFreeDtd(pDtd);
	}

	// load our english input names if needed
	if(!_inputkeys.size())
	{
		path = "languages/english/input.dtd";

		// load the doc
		pDtd = xmlParseDTD(nullptr, (xmlChar*)path.c_str());

		if(pDtd)
		{
			xmlHashScan((xmlHashTablePtr)pDtd->entities, (xmlHashScanner)locProcessEntity, &_inputkeys);
			xmlFreeDtd(pDtd);
		}
	}

	xmlCleanupInputCallbacks();

	_loctime = frametime;
}

static void locProcessEntity(xmlEntityPtr entity, void* locmap)
{
	if(locmap != nullptr)
	{
		if(locmap == &_inputkeys)
		{
			(*(InputMap*)locmap)[(char*)entity->name] = (char*)entity->content;
		}
		else if(locmap == &_locstrings)
		{
			(*(LocMap*)locmap)[(char*)entity->name] = uiUTF8toUTF16(entity->content);
		}
	}
}

void _focusModified(bool focus)
{
#if platOS == OS_WIN32
	HKL locale = GetKeyboardLayout(0);

	//if(focus && locale == (HKL)0xe0030804)
	//	return;

	// this IME causes a CTD, disable it for now
	if(focus && locale == (HKL)0xe0040804)
		return;

	// make sure we have an ime client and ui
	//if(!_imeClient || !_imeClient->Control())
	//	return;

	// alert the client of the change
	imeUpdate();
#endif
	/*
	IMEBase* base = imeClient();

	if(!base)
		return;

	// some IME's don't turn on the first time; mostly XP
	if(base->IsActive() && base->GetState() != IME_STATE_ON)
	{
		base->SetOpen();
	}
	*/

	/*
	// hack so XP ime's activate without manual toggling
	if(false && imeActive())
	{
		for(int i = 0; i < 3; i++)
		{
			if(imeState() == IME_STATE_ON)
				break;

			imeToggle();
		}
	}

	if(imeActive())
	{
		phui::TextContainer* tc = dynamic_cast<phui::TextContainer*>(uiViewport()->FindFocusView());

		if(focus && tc)
			imeUI->SetInput(tc);
		else
			imeUI->Hide();
	}
	else
	{
		if(imeUI->IsVisible())
			imeUI->Hide();
	}
	*/
}

/* for use when we localize proper names
static void locProcessName(xmlEntityPtr entity, void* nametype)
{
	static char buffer[255+1];

	if(*nametype == 1) // strat name
	{
		UINT64 key = 0;

		if(sscanf((const char*)entity->name, "strat.%lu", &key) == 1)
		{
			FULL_OBJ_ID fid;
			fid.FromKey(key);

			StratDB::StratObject* strat = StratDB::FindStratObject(fid);
			if(!strat) return;

			strat->SetName(entity->content);
		}
	}
	else // hc name
	{
		UINT32 unitID = 0;

		if(sscanf((const char*)entity->name, "hc.%u.%255s", &unitID, buffer) == 2)
		{
			HCUnit* unit = HCUnit::Lookup(unitID);
			if(!unit) return;

			if(!strcmp(buffer, "nick"))
				strncpy(unit->m_shortName, entity->content, MAX_BRIGADE_SHORT_LENGTH);
			else if(!strcmp(buffer, "title"))
				strncpy(unit->m_name, entity->content, MAX_HCUNIT_NAME_LENGTH);
		}
	}
}
*/

#if defined(BETA_RELEASE)
static int interpreter(const char* line, int argc, const char** argv, int refcon)
{
	phui::DOM::RunScript(_msgloop, nullptr, line);
	return 0;
}
#endif

static bool map_keyinput(CHAR_INPUT* centry, int* keycode, unsigned short* unicode)
{
	if(centry->uChar > 127)
		*keycode = phui::KEYCODE_UNKNOWN;
	else
		*keycode = (int)centry->uChar;

	*unicode = (unsigned short)centry->uChar;

	unsigned int iac = centry->uIac;

	if(iac == IAC_LEFT)
		*keycode = phui::KEYCODE_LEFT;
	else if(iac == IAC_RIGHT)
		*keycode = phui::KEYCODE_RIGHT;
	else if(iac == IAC_UP)
		*keycode = phui::KEYCODE_UP;
	else if(iac == IAC_DOWN)
		*keycode = phui::KEYCODE_DOWN;
	else if(iac == IAC_HOME)
		*keycode = phui::KEYCODE_HOME;
	else if(iac == IAC_END)
		*keycode = phui::KEYCODE_END;
	else if(iac == IAC_BACKSPACE)
		*keycode = phui::KEYCODE_BACKSPACE;
	else if(iac == IAC_DELETE)
		*keycode = phui::KEYCODE_DELETE;
	else if(iac == IAC_ENTER)
		*keycode = phui::KEYCODE_RETURN;
	else if(iac ==IAC_NUMPADSUBTRACT)
		*keycode = phui::KEYCODE_KP_MINUS;
	else if(iac == IAC_NUMPADADD)
		*keycode = phui::KEYCODE_KP_PLUS;
	else if(iac ==IAC_LEFTCONTROL)
		*keycode = phui::KEYCODE_LCTRL;
	else if(iac == IAC_RIGHTCONTROL)
		*keycode = phui::KEYCODE_RCTRL;
	else if(iac ==IAC_LEFTSHIFT)
		*keycode = phui::KEYCODE_LSHIFT;
	else if(iac == IAC_RIGHTSHIFT)
		*keycode = phui::KEYCODE_RSHIFT;
	else if(iac == IAC_INSERT)
		*keycode = phui::KEYCODE_INSERT;


	return true;
}

#if USE_STATESET
static void push_gl_state()
{
    static ref_ptr<RenderStateSet> stateset;

    if(!stateset.valid())
    {
        stateset = new RenderStateSet;
        stateset->SetMode(GL_LIGHTING, 0);
        stateset->SetMode(GL_FOG, 0);
        stateset->SetMode(GL_BLEND, 1);
        stateset->SetMode(GL_CULL_FACE, 0);
        stateset->SetMode(GL_DEPTH_TEST, 0);
    }

    ogl::Disable();

    stateset->Apply();
    TextureState::SetTextureUnit(0);

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_ALL_ATTRIB_BITS);

	glClientActiveTextureARB(GL_TEXTURE0);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);

	oglSetMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(_viewport->X(), _viewport->Width(), _viewport->Y(), _viewport->Height(),1,-1);

	oglSetMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}
#else
static void push_gl_state()
{
	ProgramState::Bind(0);

	ogl::Disable();
	oglDisableTexturesAboveAndIncluding(0);

	oglSetRenderState(oglRENDERSTATE_ZENABLE, FALSE);
	oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, 0);
	oglSetRenderState(oglRENDERSTATE_LIGHTING, FALSE);
	oglSetRenderState(oglRENDERSTATE_ALPHABLENDENABLE, TRUE);
	oglSetRenderState(oglRENDERSTATE_FOGENABLE, 0);

	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_ALL_ATTRIB_BITS);

	glClientActiveTextureARB(GL_TEXTURE0);

	glEnable(GL_CLIP_PLANE0);
	glEnable(GL_CLIP_PLANE1);
	glEnable(GL_CLIP_PLANE2);
	glEnable(GL_CLIP_PLANE3);

	oglSetMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(_viewport->X(), _viewport->Width(), _viewport->Y(), _viewport->Height(),1,-1);

	oglSetMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
}
#endif

#if USE_STATESET
static void pop_gl_state()
{
	glPopMatrix();
	oglSetMatrixMode(GL_PROJECTION);
	glPopMatrix();
	oglSetMatrixMode(GL_MODELVIEW);

	glPopClientAttrib();
	glPopAttrib();
}
#else
static void pop_gl_state()
{
	glPopMatrix();
	oglSetMatrixMode(GL_PROJECTION);
	glPopMatrix();
	oglSetMatrixMode(GL_MODELVIEW);

	glPopClientAttrib();
	glPopAttrib();

	oglSetRenderState(oglRENDERSTATE_ZENABLE, TRUE);
	oglSetRenderState(oglRENDERSTATE_CULLFACEENABLE, piCULLFACEENABLE_DEFAULT);
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(1, 1, 1, 1));
	oglSetRenderState(oglRENDERSTATE_LIGHTING, piLIGHTING_DEFAULT);
	oglSetRenderState(oglRENDERSTATE_FOGENABLE, bFogEnabled);

	ogl::Disable();
}
#endif

static int uiLoadImageFileToSurface(const char* Filename)
{
	LOGICAL		loadalpha;
	char*		ptr;
	static char basename[256], localname[256], tmpname[256], alphaname[256];
	int			texid;
	TEXTURE*	lptex;
	UINT32 TransparentColor = colorMAGENTA | colorALPHA;

	strcpy(basename, Filename);
	strlwr(basename);

	ptr = (char*)strrchr(basename, '.');
	if (ptr) *ptr = 0;

	texid = piGetNamedTexture(basename);
	if (piIsTexture(texid, 0))
	{
		piAddTexRef(texid);
		return(texid);
	}

	strcpy(localname, basename);
	strcpy(tmpname, localname);
	strcat(tmpname, ".dds");

	if(fioAccess(tmpname))
	{
		texid = ldddsLoadDDSTexture2D(tmpname, TransparentColor, LDPIX_NO_TEXTURE_GENERATE);
		if (!piIsTexture(texid, 0))	return -1;

		lptex = piGetTextureData(texid);
		piSetTextureName(texid, localname);
		return texid;
	}

	strcpy(localname, basename);
	strcpy(tmpname, localname);
	strcat(tmpname, ".bmp");

	if(fioAccess(tmpname))
	{
		loadalpha = FALSE;

		lstrcpy(alphaname, localname);
		lstrcat(alphaname, "_alpha.bmp");
		if (fioAccess(alphaname))
		{
			TransparentColor = piRGBA_SETALPHA(TransparentColor,8);
			loadalpha = TRUE;
		}

		texid = ldbmpLoadBMPFileToSurface(tmpname, TransparentColor, 0);
		if (!piIsTexture(texid, 0))	return -1;

		lptex = piGetTextureData(texid);
		piSetTextureName(texid, localname);

		if (loadalpha)
		{
			if (!ldbmpLoadBMPAlphaFileToSurface(alphaname, texid, 0))
			{
				piFreeMipTextures(texid);
				return(-1);
			}

			lptex->hasalpha = TRUE;
		}
	}

	return(texid);
}

/************************************************************************************************
	piGraphicsContext
*************************************************************************************************/

void piGraphicsContext::SetTexture(int texid) { if(texid > -1) piSetActiveTexture(texid,0); }
void piGraphicsContext::EnableTexture() { piEnableTexture(); }
void piGraphicsContext::DisableTexture() { piDisableTexture(); }
void piGraphicsContext::PopState() { GLGraphicsContext::PopState(); piDisableTexture(); }
#if(0)
void piGraphicsContext::SetColor(float r, float g, float b, float a)
{
	oglSetRenderState(oglRENDERSTATE_COLOR4F, piRGBA(r, g, b, a));
}

void piGraphicsContext::DrawQuad(const float* coords, const float* uv)
{
	if(uv)
		ogl::SetTextureUnitCoordPointer(GL_TEXTURE0, 2, GL_FLOAT, 0, (void*)&uv[0]);

	ogl::SetVertexPointer(2, GL_FLOAT, 0, (void*)&coords[0]);
	ogl::DrawArrays(GL_QUADS, 0, 4);
}

void piGraphicsContext::DrawLineStrip(int count, const float* coords, bool closed)
{
	ogl::SetVertexPointer(2, GL_FLOAT, 0, (void*)&coords[0]);
	ogl::DrawArrays(closed ? GL_LINE_LOOP : GL_LINE_STRIP, 0, count);
}

void piGraphicsContext::DrawLineStrip(int count, float const* coords, bool closed, float width)
{
	glLineWidth((GLfloat)width);
	DrawLineStrip(count, coords, closed);
	glLineWidth(1.0f);
}
#endif

int uiConsoleprintf(const char* text, ...)
{
	phui::ConsoleView* display = 0;

	if(_terminal)
		display = dynamic_cast<phui::ConsoleView*>(_terminal->FindView("terminal-display"));

	static char buf[1024];

	va_list marker;
	va_start(marker, text);
	vsnprintf(buf, sizeof(buf) - 1, text, marker);
	va_end(marker);

	int len = 0;//printf("%s\n", buf);

	std::string out = buf;

	if(!out.empty())
	{
		out.insert(0, "CONSOLE: ");

		if(out[out.size()  - 1] != '\n')
			out += "\n";

		Write_Log(LOG_CONSOLE, LOG_INFO, out.c_str());

		if(display) display->AddText(out.c_str());

		len = out.size();
	}

	return len;
}

const wchar_t* uiUTF8toUTF16(const char* text)
{
	return uiUTF8toUTF16((const unsigned char*)text);
}

const wchar_t* uiUTF8toUTF16(const unsigned char* text)
{
	return phui::utf8_to_utf16(text);
}

const char* uiUTF16toUTF8(const wchar_t* text)
{
	return (const char*)phui::utf16_to_utf8(text);
}

static void _jserror(JSContext* cx, const char* message, JSErrorReport* report)
{
	std::string out = "";
	char line[1024+1];

	snprintf(line, 1024, "JS ERROR: filename = %s, line = %u, error = %u\n", report->filename, report->lineno, report->errorNumber);
	out += line;

	snprintf(line, 1024, "%s\n", message);
	out += line;

	if(report->linebuf)
	{
		snprintf(line, 1024, "%s\n\n", report->linebuf);
		out += line;
	}

	// could be more efficient
	uiConsoleprintf(out.c_str());
	//printf("%s",out.c_str());

	if(report->errorNumber == 137)
	{
		//piExitProgram(_FL, out.c_str());
	}
}

phui::MessageLoop* uiMessageLoop()
{
	return _msgloop;
}

jsGame* uiGame()
{
	return _jsgame;
}

/************************************************************************************************
	uiRankIconCreate
*************************************************************************************************/

phui::View* uiRankIconCreate(_xmlNode* node, phui::View* parent)
{
	int rank = 0;

	xmlChar* rankstr = xmlGetProp(node, (xmlChar*)"rank");
	if(rankstr)
	{
		rank = atoi((char*)rankstr);
		xmlFree(rankstr);
	}

	return new DrawIconView(_ColorRankIcons, rank);
}

/************************************************************************************************
	uiContactIconCreate
*************************************************************************************************/

phui::View* uiContactIconCreate(_xmlNode* node, phui::View* parent)
{
	int index = 0;

	xmlChar* indexstr = xmlGetProp(node, (xmlChar*)"index");
	if(indexstr)
	{
		index = atoi((char*)indexstr);
		xmlFree(indexstr);
	}

	return new DrawIconView(_ContactIcons, index);

	/*
	INDEXES
	0	air
	1	sea
	2	ground (armor)
	3	gun
	4	truck
	5	danger
	6	unknown (question mark)
	7	mortar (target)
	8	inf (troop)
	*/
}

/************************************************************************************************
	uiFlagIconCreate
*************************************************************************************************/

phui::View* uiFlagIconCreate(_xmlNode* node, phui::View* parent)
{
	int index = 0;

	xmlChar* indexstr = xmlGetProp(node, (xmlChar*)"index");
	if(indexstr)
	{
		index = atoi((char*)indexstr);
		xmlFree(indexstr);
	}

	return new DrawIconView(_FlagIcons, index);

	/*
	INDEXES
	0	UK round			32	UK flag
	1	US round			33	US flag
	2	FR round			34	FR flag
	3	DE round			35	DE flag
	4	IT round			36	IT flag
	5	JP round			37	JP flag
	9	Earth				42 <blank>
	*/
}

/************************************************************************************************
	BarCompassView
*************************************************************************************************/

class BarCompassView : public phui::View
{
public:
	BarCompassView() : phui::View() {}
	virtual void DrawSelf();
	void DrawBackground(float x, float y, float width, float height);
	char* GetHeadingString(char* into, const size_t sizeofInto);
	int texid;
	float GetHeading();									// Current heading where North = 0.00, West = 90.00, East = -90.00, South = +- 180.00
	float Theta(float barCompassWidth, float scalar);	// Viewing angle := "Slice" of headings visible on compass
	float XCoordForAngle(float angle, float width, float heading, float pixelsPerDegree, float numTicks); // See definition for details
	void DrawCardinalIndicator(phui::GraphicsContext* ctx, float angle, float width, float height, float heading, float pixelsPerDegree, float numTicks);
	float AffineTransform(float x, float x1, float x2, float y1, float y2); // Performs a mapping of x from the range [x1, x2] to the range [y1, y2]
};


/************************************************************************************************
	uiBarCompassCreate
*************************************************************************************************/

phui::View* uiBarCompassCreate(_xmlNode* node, phui::View* parent)
{
	BarCompassView* v = new BarCompassView();
	v->texid = ldLoadTexture("ui/textures/barcompass.dds");
	return v;
}

/************************************************************************************************
	BarCompassView::DrawSelf
*************************************************************************************************/

void BarCompassView::DrawSelf()
{
#if(0)
	View::DrawSelf();

	float wf = this->Width();		// True width
	float hf = this->Height();		// True height
	float xf = this->X();			// True X
	float yf = this->Y();			// True Y
	float width = wf - 10.0f;		// Adjusted width (side borders are 4 each, + 2 for spacing
	float height = hf - 16.0f;		// Adjusted height (top border is 14, + 2 for spacing)

	const int degreesPerTick = 10;			    // A tick mark every xxx degrees
	const float tickLength = hf/3.0f;			// Vertical length of a tick mark
	//float theta = this->Theta(wf, 1000);		// The "slice" of heading degrees visible on the compass, based on width of compass
	float theta = 90.0f;						// HARD CODED! We'll show a range of 90 degrees from left to right
	float halfTheta = theta/2;
	float heading = this->GetHeading();			// North = 0.00, West = 90.00, East = -90.00, South = +- 180.00
	int numTicks = (theta/degreesPerTick)*2;	// Number of ticks visible on compass
	float pixelsPerDegree = theta/width;		// How many pixels for each degree across compass

	// Cardinal direction headings
	const float NORTH = 0.0f;
	const float NORTHWEST = 45.0f;
	const float WEST = 90.0f;
	const float SOUTHWEST = 135.0f;
	const float SOUTH = 180.0f;
	const float SOUTHEAST = -135.0f;
	const float EAST = -90.0f;
	const float NORTHEAST = -45.0f;

	phui::GraphicsContext* ctx = this->GetGraphicsContext();

	this->DrawBackground(4.0f, 4.0f, width, height);



	ctx->DisableTexture();
	//ctx->SetColor(0.0f, 1.0f, 0.0f, 1.00f);	// Bright green
	ctx->SetColor(0.0f, 0.0f, 0.0f, 0.80f);		// Black

	// Draw the main horizontal line that spans the bar compass
	const int lineThickness = 4;
	float topLine[] = {0.0f+4.0f, height,	wf-4.0f, height};
	ctx->DrawLineStrip(2, topLine, true, lineThickness);

	// Draw the current heading indicator
	float indicatorWidth = 10.0f;
	float indicator[] = { (wf/2 - indicatorWidth), height,	(wf/2 + indicatorWidth), height,	(wf/2), (height - 1.25 * indicatorWidth) };
	ctx->DrawPolygon(3, indicator);

	/*
	// Draw North indicator TEST
	this->DrawCardinalIndicator(ctx, NORTH, width, height, heading, pixelsPerDegree, numTicks);

	// Our current heading
	/*
	ctx->SetColor(0.0f, 1.0f, 0.0f, 1.00f);
	ctx->MovePenPosition(10.0f, (hf - 50.0f));
	char headingString[32];
	this->GetHeadingString(headingString, sizeof(headingString));
	ctx->DrawString(0, headingString);
	*/
#endif
}

/************************************************************************************************
	BarCompassView::DrawCardinalIndicator
*************************************************************************************************/
// Draws a diamond at the heading specified  by angle. The "heading" parameter refers to the PLAYER's heading.
void BarCompassView::DrawCardinalIndicator(phui::GraphicsContext* ctx, float angle, float width, float height, float heading, float pixelsPerDegree, float numTicks)
{
	float x = this->XCoordForAngle(angle, width, heading, pixelsPerDegree, numTicks);
	ctx->SetColor(0.0f, 1.0f, 0.0f, 1.0f);
	float diamond[] = {
		x, (height-10.0f),
		(x-10.0f), (height-20.0f),
		x, (height-30.0f),
		(x+10.0f), (height-20.0f)
	};
	ctx->DrawLineStrip(4, diamond, true, 1);
}

/************************************************************************************************
	BarCompassView::GetHeading
*************************************************************************************************/

float BarCompassView::GetHeading()
{
	return -(float)(atan2(playdir.x, playdir.y) * 180.0f / float(PI));
}

/************************************************************************************************
	BarCompassView::Theta
*************************************************************************************************/
// The larger the scalar, the larger the theta
float BarCompassView::Theta(float barCompassWidth, float scalar)
{
	float theta;

	theta = 2.0f * ( atan2( (barCompassWidth)/2, scalar ) ); // In radians, yuck

	theta = theta * (180.0f / PI);		// Convert theta to degrees, much better

	return theta;
}

/************************************************************************************************
	BarCompassView::GetHeadingString
*************************************************************************************************/

char* BarCompassView::GetHeadingString(char* into, const size_t bytes)
{
	snprintf(into, bytes - 1, "Current Heading: %3.3f", BarCompassView::GetHeading());
	return into;
}

/************************************************************************************************
	BarCompassView::XCoordForAngle
*************************************************************************************************/
// For a given angle in degrees, what is the corresponding X-coordinate on the Bar Compass?
float BarCompassView::XCoordForAngle(float angle, float width, float heading, float pixelsPerDegree, float numTicks)
{
	// angle			:=	The angle of the indicator with respect to North (0 degrees)
	// width			:=	The width, in pixels, of the interior of the bar compass
	// heading			:=	The current heading angle of the player with respect to North
	// pixelsPerDegree	:=	The number of pixels on the bar compass taken up by a single degree
	// numTicks			:=	The number of ticks across the bar compass (not sure why this matters, but it does)
	return (width/2) + (heading * pixelsPerDegree * numTicks * 2);
}

/************************************************************************************************
	BarCompassView::AffineTransform
*************************************************************************************************/
// Performs a mapping of x from the range [x1, x2] to the range [y1, y2]
float BarCompassView::AffineTransform(float x, float x1, float x2, float y1, float y2)
{
	return(x - x1) * (y2 - (y1/(x2 - x1)) + y1);
}

/************************************************************************************************
	BarCompassView::DrawBackground
*************************************************************************************************/
// Draws a texture for the compass background
void BarCompassView::DrawBackground(float x, float y, float width, float height)
{
	oglSetActiveTexture(0, texid);

	mlVehicleData* data = persGetPersonaModelDataPtr();
    float rad = (float)(data->VehicleAtt.Yaw);
    float t = -(rad / PI2);
    float s = t - 0.25f; // subtract 45 degrees
    float e = t + 0.25f; // add 45 degrees

    piLVERTEX v[4];
    float u1, v1, u2, v2;

	/*
    u1 = 0.25f; //s;
    v1 = 1; //0.0f;
    u2 = 0.5f; //e;
    v2 = 0; //0.125f * 0.5f;
	*/

	u1 = s;
    v1 = 1; //0.0f;
    u2 = e;
    v2 = 0; //0.125f * 0.5f;

	v[0].x = x;
	v[0].y =  height;
	v[0].z = zdepthLAYER1;
	v[0].color = colorWHITEALPHA;
	v[0].specular = colorALPHA100;
	v[0].tu = u1;
	v[0].tv = v2;

	v[1].x = width;
	v[1].y = height;
	v[1].z = zdepthLAYER1;
	v[1].color = colorWHITEALPHA;
	v[1].specular = colorALPHA100;
	v[1].tu = u2;
	v[1].tv = v2;

	v[2].x = width;
	v[2].y = y;
	v[2].z = zdepthLAYER1;
	v[2].color = colorWHITEALPHA;
	v[2].specular = colorALPHA100;
	v[2].tu = u2;
	v[2].tv = v1;

	v[3].x = x;
	v[3].y = y;
	v[3].z = zdepthLAYER1;
	v[3].color = colorWHITEALPHA;
	v[3].specular = colorALPHA100;
	v[3].tu = u1;
	v[3].tv = v1;

    piINDICES indices[2];

    indices[0].v1 = 0;
    indices[0].v2 = 1;
    indices[0].v3 = 3;
    indices[1].v1 = 1;
    indices[1].v2 = 2;
    indices[1].v3 = 3;

	oglRenderIndexScene(oglPT_TRIANGLELIST,
                        oglVT_LVERTEX,
                        v,
                        4,
                        (unsigned short*)indices,
                        6);

	ogl::Disable();
}


/************************************************************************************************
	uiScenImageCreate
*************************************************************************************************/

phui::View* uiScenImageCreate(_xmlNode* node, phui::View* parent)
{
	phui::ImageView* iv = new phui::ImageView;

	xmlChar* src = xmlGetProp(node, (xmlChar*)"src");
	if(src)
	{
		iv->SetTexture(uiGetAtlasTexture(SCEN_MGR->MapPath((const char*)src), false));
		xmlFree(src);
	}

	return iv;
}


/************************************************************************************************
	uiImageUVCreate
*************************************************************************************************/

phui::View* uiImageUVCreate(_xmlNode* node, phui::View* parent)
{
	int texid	= -1;
	float u1	= 0;
	float v1	= 0;
	float u2	= 0;
	float v2	= 0;

	xmlChar* src = xmlGetProp(node, (xmlChar*)"src");
	if(src)
	{
		texid = uiGetTexture((const char*)src);
		xmlFree(src);
	}

	if(texid < 0)
	{
		xmlChar* texstr = xmlGetProp(node, (xmlChar*)"texid");
		if(texstr)
		{
			texid = atoi((const char*)texstr);
			xmlFree(texstr);
		}
	}

	xmlChar* u1str = xmlGetProp(node, (xmlChar*)"u1");
	if(u1str)
	{
		u1 = (float)atof((char*)u1str);
		xmlFree(u1str);
	}

	xmlChar* v1str = xmlGetProp(node, (xmlChar*)"v1");
	if(v1str)
	{
		v1 = (float)atof((char*)v1str);
		xmlFree(v1str);
	}

	xmlChar* u2str = xmlGetProp(node, (xmlChar*)"u2");
	if(u2str)
	{
		u2 = (float)atof((char*)u2str);
		xmlFree(u2str);
	}

	xmlChar* v2str = xmlGetProp(node, (xmlChar*)"v2");
	if(v2str)
	{
		v2 = (float)atof((char*)v2str);
		xmlFree(v2str);
	}

	return new ImageUVView(texid, u1, v1, u2, v2);
}

phui::View* uiBrigadeDecalCreate(_xmlNode* node, phui::View* parent)
{
	int brigadeid = -1;

	xmlChar* texstr = xmlGetProp(node, (xmlChar*)"brigadeid");
	if(texstr)
	{
		brigadeid = atoi((const char*)texstr);
		xmlFree(texstr);
	}

	return new BrigadeDecalView(brigadeid);
}


phui::View* uiTargetViewCreate(_xmlNode* node, phui::View* parent)
{
	TargetView* tv = new TargetView;

	xmlChar* str = xmlGetProp(node, (xmlChar*)"tsrc");
	if(str)
	{
		tv->SetTargetTexture((const char*)str);
		xmlFree(str);
	}

	str = xmlGetProp(node, (xmlChar*)"asrc");
	if(str)
	{
		tv->SetArrowTexture((const char*)str);
		xmlFree(str);
	}

	str = xmlGetProp(node, (xmlChar*)"ts");
	if(str)
	{
		tv->targetsize = (float)atof((char*)str);
		xmlFree(str);
	}

	str = xmlGetProp(node, (xmlChar*)"tx");
	if(str)
	{
		tv->targetx = atof((char*)str);
		xmlFree(str);
	}

	str = xmlGetProp(node, (xmlChar*)"ty");
	if(str)
	{
		tv->targety = atof((char*)str);
		xmlFree(str);
	}

	str = xmlGetProp(node, (xmlChar*)"tz");
	if(str)
	{
		tv->targetz = atof((char*)str);
		xmlFree(str);
	}

	return tv;
}



phui::View* uiMapKeyIconCreate(_xmlNode* node, phui::View* parent)
{
	if(!_mapkeyicons)
		_mapkeyicons = drawIconLoad("ui\\uidata\\map\\mapkey");

	int icon = 0;

	xmlChar* iconstr = xmlGetProp(node, (xmlChar*)"icon");
	if(iconstr)
	{
		icon = atoi((char*)iconstr);
		xmlFree(iconstr);
	}

	/*
	float rgba[] = {1.0f, 1.0f, 1.0f, 1.0f};
	xmlChar* color = xmlGetProp(node, (xmlChar*)"color");
	if(color)
	{
		// Convert hex color string to RGBA to draw colored icon
	}
	*/

	return new DrawIconView(_mapkeyicons, icon);
}

class CompassView : public phui::View
{
public:
	CompassView(const char* src) : phui::View() { if(src) texid = uiGetAtlasTexture(src,false); }
	virtual void DrawSelf();

	phui::GraphicsContext::TextureInfo texid;
};


/************************************************************************************************
	uiCompassCreate
*************************************************************************************************/

phui::View* uiCompassCreate(_xmlNode* node, phui::View* parent)
{
	xmlChar* src = xmlGetProp(node, (xmlChar*)"src");
	CompassView* v = new CompassView((const char*)src);

	if(src)
		xmlFree(src);

	return v;
}

/************************************************************************************************
	DrawSelf
 ************************************************************************************************/

void DrawIconView::DrawSelf()
{
	View::DrawSelf();

	phui::GraphicsContext* ctx = GetGraphicsContext();

	int texid = drawIconGetTexId(drawicon);

	if(!piIsTexture(texid, 0))
	{
		return;
	}

	ctx->EnableTexture();
	ctx->SetTexture(texid);
	ctx->SetColor(1,1,1,1);

	float u1,v1,u2,v2;
	drawIconGetUV(drawicon, icon, &u1, &v1, &u2, &v2);

	phui::BoxVerts box;
	float uv[8] = {u1,v1, u2,v1, u2,v2, u1,v2};

	phui::box_verts_with_pos(box, 0, 0, Width(), Height());

	ctx->DrawQuad(box.verts, uv);
}

/************************************************************************************************
	DrawSelf
 ************************************************************************************************/

void DrawIconView::DrawSelfColor(float r, float g, float b, float alpha)
{
	View::DrawSelf();

	phui::GraphicsContext* ctx = GetGraphicsContext();

	int texid = drawIconGetTexId(drawicon);

	if(!piIsTexture(texid, 0))
	{
		return;
	}

	ctx->EnableTexture();
	ctx->SetTexture(texid);
	ctx->SetColor(r,g,b,alpha);

	float u1,v1,u2,v2;
	drawIconGetUV(drawicon, icon, &u1, &v1, &u2, &v2);

	phui::BoxVerts box;
	float uv[8] = {u1,v1, u2,v1, u2,v2, u1,v2};

	phui::box_verts_with_pos(box, 0, 0, Width(), Height());

	ctx->DrawQuad(box.verts, uv);
}

/************************************************************************************************
	DrawSelf
 ************************************************************************************************/

void CompassView::DrawSelf()
{
	float angle = -(float)(atan2(playdir.x, playdir.y) * 180.0f / float(PI));
	glPushMatrix();
	glTranslatef(_width/2, _height/2, 0);
	glRotatef(angle, 0, 0, 1);
	glTranslatef(-(_width/2), -(_height/2), 0);

	phui::GraphicsContext* ctx = GetGraphicsContext();

	ctx->EnableTexture();
	ctx->SetTexture(texid.texid);
	ctx->SetColor(1,1,1,1);
	ctx->DrawTexture(texid, 0, 0, _width, _height);
	glPopMatrix();
}

/************************************************************************************************
	DrawSelf
 ************************************************************************************************/

void ImageUVView::DrawSelf()
{
	phui::GraphicsContext* ctx = GetGraphicsContext();

	if(_texid < 0) return;

	ctx->EnableTexture();
	ctx->SetTexture(_texid);
	ctx->SetColor(1,1,1,1);

	phui::BoxVerts box;
	float uv[8] = {_u1,_v1, _u2,_v1, _u2,_v2, _u1,_v2};

	phui::box_verts_with_pos(box, 0, 0, Width(), Height());

	ctx->DrawQuad(box.verts, uv);
}

/************************************************************************************************
	jsImageUV
*************************************************************************************************/

class jsImageUV : public phui::DOM::ElementDOM
{
public:
	jsImageUV(ImageUVView* dv, phui::DOM::ElementDOM*);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	JS_CLASS_DATA
	enum
	{
		SRC_PROP = 100,
		TEXID_PROP,
		U1_PROP,
		V1_PROP,
		U2_PROP,
		V2_PROP
	};

	ImageUVView* imageView;

protected:
	virtual ~jsImageUV();
};

JSClass jsImageUV::jsclass = JS_CLASS_MACRO("imageuv");
JSFunctionSpec jsImageUV::jsfunctions[] =
{
	{0}
};

JSPropertySpec jsImageUV::jsproperties[] =
{
	{"src",		jsImageUV::SRC_PROP,   	JSPROP_PERMANENT},
	{"texid",	jsImageUV::TEXID_PROP,  JSPROP_PERMANENT},
	{"u1",		jsImageUV::U1_PROP,   	JSPROP_PERMANENT},
	{"v1",		jsImageUV::V1_PROP,   	JSPROP_PERMANENT},
	{"u2",		jsImageUV::U2_PROP,   	JSPROP_PERMANENT},
	{"v2",		jsImageUV::V2_PROP,   	JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	uiImageUVCreateDOM
*************************************************************************************************/

phui::DOM::ElementDOM* uiImageUVCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent)
{
	ImageUVView* iv = dynamic_cast<ImageUVView*>(view);
	if(iv)
		return  new jsImageUV(iv, parent);

	return 0;
}

/************************************************************************************************
	uiBarCompassViewCreateDOM
*************************************************************************************************/

phui::DOM::ElementDOM* uiBarCompassCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent)
{
	// TO DO: ??

	return 0;
}

/************************************************************************************************
	jsImageUV
*************************************************************************************************/

jsImageUV::jsImageUV(ImageUVView* v, phui::DOM::ElementDOM* d) : ElementDOM(v,d, &jsImageUV::jsclass)
{
	imageView = v;
	DefineFunctions(jsfunctions);
	DefineProperties(jsproperties);
}

jsImageUV::~jsImageUV()
{
}

JSBool jsImageUV::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case TEXID_PROP:
			*val = INT_TO_JSVAL(imageView->_texid);
		break;
		case U1_PROP:
			JS_NewDoubleValue(_jsctx, imageView->_u1, val);
		break;
		case V1_PROP:
			JS_NewDoubleValue(_jsctx, imageView->_v1, val);
		break;
		case U2_PROP:
			JS_NewDoubleValue(_jsctx, imageView->_u2, val);
		break;
		case V2_PROP:
			JS_NewDoubleValue(_jsctx, imageView->_v2, val);
		break;
		default:
			phui::DOM::ElementDOM::GetProperty(ctx, propid, val);
		break;
	}

	return JS_TRUE;
}

JSBool jsImageUV::SetProperty(JSContext* ctx, int propid, jsval* val)
{
//	double dval = 0.0f;

	switch(propid)
	{
		case SRC_PROP:
			imageView->_texid = uiGetTexture(JS_GetStringBytes(JS_ValueToString(ctx, *val)));
		break;
		case TEXID_PROP:
		{
			int32 ival = -1;
			JS_ValueToInt32(ctx, *val, &ival);
			imageView->_texid = (int)ival;
		}
		break;
		/*
		case U1_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			imageView->_u1 = (float)dval;
		}
		break;
		case V1_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			imageView->_v1 = (float)dval;
		}
		break;
		case U2_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			imageView->_u2 = (float)dval;
		}
		break;
		case V2_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			imageView->_v2 = (float)dval;
		}
		break;
		*/
		default:
			phui::DOM::ElementDOM::SetProperty(ctx, propid, val);
		break;
	}

	return JS_TRUE;
}

/************************************************************************************************
	TargetView
*************************************************************************************************/

#define SQR(x) ((x)*(x))

void TargetView::DrawSelf()
{
	//View::DrawSelf();

	phui::GraphicsContext* ctx = GetGraphicsContext();

	float cx = Width() * .5f;
	float cy = Height() * .5f;

	//uiChatPrintLine("TargetView screen pos: %.2f, %.2f, %.2f", sx, sy, sz);

	if(sz <= 1 && !ChildAt(0)) // visible
	{
		ctx->DisableTexture();
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->MovePenPosition((float)sx, (float)sy);

		double distance = 0;

		vehVehicle* player = playerGetVehicle();
		if(player)
		{
			Vec3d ppos;
			ppos.set(player->ModelData->VehiclePos);
			Vec3d tpos(targetx, targety, targetz);
			ppos -= tpos;
			distance = ppos.length();
		}

		char label[100+1];
		snprintf(label, 100, "%s: %u m", Name(), (unsigned int)distance);

		int font		= ctx->GetDefaultFont();
		float fbuffer	= 4;
		float fheight	= ctx->FontHeight(font);
		float fwidth	= ctx->FontAdvance(font, label);

		float box[10];

		box[0] = -fwidth/2 + -fbuffer;
		box[1] = targetsize + fheight + fbuffer*2;

		box[2] = box[0];
		box[3] = box[1] - (fheight + fbuffer*2);

		box[4] = 0;
		box[5] = 0;

		box[6] = fwidth/2 + fbuffer;
		box[7] = box[3];

		box[8] = box[6];
		box[9] = box[1];

		// draw the background
		ctx->SetColor(_bgcolor[0],_bgcolor[1],_bgcolor[2],_bgcolor[3]);
		ctx->DrawPolygon(5, box);
		/*
		box[0] = -fwidth/2 + -fbuffer;
		box[1] = targetsize + fheight + fbuffer*2;

		box[2] = box[0];
		box[3] = box[1] - (fheight + fbuffer*2);

		box[4] = fwidth/2 + fbuffer;
		box[5] = box[3];

		box[6] = box[4];
		box[7] = box[1];
		*/

		ctx->SetColor(_bordercolor[0],_bordercolor[1],_bordercolor[2],_bordercolor[2]);
		ctx->DrawLineStrip(5, box, true, 3);

		box[0] = -fwidth/2 + -fbuffer;
		box[1] = targetsize + fheight + fbuffer*2;

		box[2] = box[0];
		box[3] = box[1] - (fheight + fbuffer*2);

		box[4] = fwidth/2 + fbuffer;
		box[5] = box[3];

		box[6] = box[4];
		box[7] = box[1];

		ctx->DrawLineStrip(4, box, true, 4);

		ctx->EnableTexture();
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[2]);
		ctx->MovePenPosition(-fwidth/2, targetsize + fbuffer);
		ctx->DrawString(font, label);
		ctx->MovePenPosition(fwidth/2, -(targetsize + fbuffer));
		ctx->MovePenPosition((float)-sx, (float)-sy);
	}

	float dx = (sz < 1) ? (float)sx - cx: cx - (float)sx;
	float dy = (sz < 1) ? (float)sy - cy: cy - (float)sy;

	if(arrowtex.texid > 0 && (sz >= 1.0f || math3dFastSqrt(SQR(dx) + SQR(dy)) > 100))
	{
		// draw arrow pointing to where the target is
		glPushMatrix();

		phui::BoxVerts box;
		float angle = -(float)(atan2(dx, dy) * 180.0f / PI);

		glTranslatef(cx, cy, 0);
		glRotatef(angle, 0,0,1);

		phui::box_verts_with_pos(box, -15, -15, 30, 30);
		//ctx->SetColor(_bordercolor[0], _bordercolor[1], _bordercolor[2], _bordercolor[3]);
		ctx->SetColor(.74f, .88f, .8f, .66f);
		ctx->SetTexture(arrowtex.texid);
		phui::draw_gl_quad(box.verts, arrowtex.uv);
		ctx->SetColor(1, 1, 1, 1);

		glPopMatrix();
	}

	ctx->SetColor(1,1,1,1);
}

/*
void TargetView::DrawSelf()
{
	View::DrawSelf();

	phui::GraphicsContext* ctx = GetGraphicsContext();

	if(sz <= 1) // visible
	{
		ctx->DisableTexture();
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],_textcolor[3]);
		ctx->MovePenPosition((float)sx, (float)sy);

		float box[8];
		float twidth = targetsize/2.0f;
		float theight = targetsize;

		box[0] = -twidth;  box[1] = 0;
		box[2] = 0; box[3] = -theight;
		box[4] = twidth; box[5] = 0;
		box[6] = 0;  box[7] = theight;

		ctx->DrawQuad(box);
		ctx->SetColor(_textcolor[0],_textcolor[1],_textcolor[2],1.0f);
		ctx->DrawLineStrip(4, box, true);

		double distance = 0;

		vehVehicle* player = playerGetVehicle();
		if(player)
		{
			Vec3d ppos;
			ppos.set(player->ModelData->VehiclePos);
			Vec3d tpos(targetx, targety, targetz);
			ppos -= tpos;
			distance = ppos.length();
		}

		char buffer[25];
		snprintf(buffer, 24, "%u m", (unsigned int)distance);
		int font = ctx->GetDefaultFont();
		float fheight = ctx->FontHeight(font);

		twidth = twidth * 1.5f;

		ctx->EnableTexture();
		ctx->MovePenPosition(twidth, 0);
		ctx->DrawString(font, Name());
		ctx->MovePenPosition(0, -fheight);
		ctx->DrawString(font, buffer);
		ctx->MovePenPosition(-twidth, fheight);
		ctx->MovePenPosition((float)-sx, (float)-sy);
	}

	float cx = Width()/2;
	float cy = Height()/2;
	float dx = (sz < 1) ? (float)sx - cx: cx - (float)sx;
	float dy = (sz < 1) ? (float)sy - cy: cy - (float)sy;

	if(sz >= 1.0f || math3dFastSqrt(SQR(dx) + SQR(dy)) > 100)
	{
		// draw arrow pointing to where the target is
		glPushMatrix();

		BoxVerts box;
		float angle = -(float)(atan2(dx, dy) * 180.0f / PI);

		glTranslatef(cx, cy, 0);
		glRotatef(angle, 0,0,1);

		phui::box_verts_with_pos(box, -15, -15, 30, 30);
		ctx->SetColor(0, 1, 0, .7f);
		ctx->SetTexture(arrowtex.texid);
		phui::draw_gl_quad(box.verts, arrowtex.uv);
		ctx->SetColor(1, 1, 1);

		glPopMatrix();
	}

	ctx->SetColor(1,1,1,1);
}
*/

void TargetView::SetTargetTexture(const char* t)
{
	targettex = uiGetAtlasTexture(t, false);
}

void TargetView::SetArrowTexture(const char* a)
{
	arrowtex = uiGetAtlasTexture(a, false);
}

void TargetView::Update(bool calc)
{
	if(calc)
	{
		// lets calculate our target screen position
		piMATRIX world = *(piGetWorldMatrix());
		piSetWorldMatrix(&IdentityMatrix);

		double x = targetx - WorldOriginOffset.x;
		double y = targety - WorldOriginOffset.y;
		double z = -targetz;

		// screen coordinates
		sx = sy = sz = 0;

		// our screen coords for the position passed in
		piGetWorldToScreen(x, y, z, sx, sy, sz);
		piSetWorldMatrix(&world);
	}

	offscreen = 0;

	phui::View* child = ChildAt(0);

	if(child)
	{
		FLOAT32 myWidth = Width();
		FLOAT32 myHeight = Height();
		FLOAT32 childWidth = child->Width();
		FLOAT32 childHeight = child->Height();

		FLOAT32 cx = myWidth * .5f;
		FLOAT32 cy = myHeight * .5f;

		FLOAT32 mx = (FLOAT32)sx - (childWidth * .5f);
		FLOAT32 my = (FLOAT32)sy - (childHeight * .5f);

		if(true || SCEN_MGR->IsActive())
		{
			if(sz >= 1)
			{
				if(mx < cx)
				{
					offscreen = 1;
				}
				else if(mx > cx)
				{
					offscreen = -1;
				}
			}
		}
		else
		{
			if(sz > 1)
			{
				if(mx < cx)
				{
					mx = myWidth - childWidth;
					offscreen = 1;
				}
				else if(mx > cx)
				{
					mx = 0;
					offscreen = -1;
				}

				my = cy;
			}
			else if(mx < 0.0f || mx > myWidth)
			{
				if(mx > cx)
				{
					mx = myWidth - childWidth;
					offscreen = 1;
				}
				else if(mx < cx)
				{
					mx = 0;
					offscreen = -1;
				}

				my = cy;
			}
			else
			{
				bool centerY = false;

				if(mx < 0)
				{
					centerY = true;
					offscreen = -1;
					mx = 0;
				}
				else if(mx > myWidth - childWidth)
				{
					centerY = true;
					mx = myWidth - childWidth;
					offscreen = 1;
				}

				if(centerY)
				{
					my = cy;
				}
				else
				{
					if(my < 0)
						my = 0;
					else if(my > myHeight - childHeight)
						my = myHeight - childHeight;
				}
			}
		}

		child->MoveTo(mx, my);
	}
}

/************************************************************************************************
	jsTargetView
*************************************************************************************************/

class jsTargetView : public phui::DOM::ElementDOM
{
public:
	jsTargetView(TargetView* tv, phui::DOM::ElementDOM*);

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

	JS_CLASS_DATA
	JS_FUNCTION(update);

	enum
	{
		TARGETTEX_PROP = 100,
		ARROWTEX_PROP,
		TARGETSIZE_PROP,
		TX_PROP,
		TY_PROP,
		TZ_PROP
	};

	TargetView* targetView;

protected:
	virtual ~jsTargetView();
};

JSClass jsTargetView::jsclass = JS_CLASS_MACRO("target");
JSFunctionSpec jsTargetView::jsfunctions[] =
{
	JS_FUNCTION_SPEC(jsTargetView, update, 0),
	{0}
};

JSPropertySpec jsTargetView::jsproperties[] =
{
	{"targettex",	jsTargetView::TARGETTEX_PROP,   JSPROP_PERMANENT},
	{"arrowtex",	jsTargetView::ARROWTEX_PROP,	JSPROP_PERMANENT},
	{"targetsize",	jsTargetView::TARGETSIZE_PROP,	JSPROP_PERMANENT},
	{"tx",			jsTargetView::TX_PROP,			JSPROP_PERMANENT},
	{"ty",			jsTargetView::TY_PROP,   		JSPROP_PERMANENT},
	{"tz",			jsTargetView::TZ_PROP,   		JSPROP_PERMANENT},
	{0}
};

/************************************************************************************************
	uiTargetViewCreateDOM
*************************************************************************************************/

phui::DOM::ElementDOM* uiTargetViewCreateDOM(const char* classname, phui::View* view, phui::DOM::ElementDOM* parent)
{
	TargetView* tv = dynamic_cast<TargetView*>(view);
	if(tv) return new jsTargetView(tv, parent);

	return 0;
}

/************************************************************************************************
	jsTargetView
*************************************************************************************************/

jsTargetView::jsTargetView(TargetView* v, phui::DOM::ElementDOM* d) : ElementDOM(v,d, &jsTargetView::jsclass)
{
	targetView = v;
	DefineFunctions(jsfunctions);
	DefineProperties(jsproperties);
}

jsTargetView::~jsTargetView()
{
}

/************************************************************************************************
	update
*************************************************************************************************/

JS_FUNCTION_DEF(jsTargetView, update)
{
	jsTargetView* tv = phui::js::Cast<jsTargetView*>(ctx,jsobj);
	if(!tv) return JS_FALSE;

	tv->targetView->Update();

	return JS_TRUE;
}

JSBool jsTargetView::GetProperty(JSContext* ctx, int propid, jsval* val)
{
	switch(propid)
	{
		case TX_PROP:
			JS_NewDoubleValue(_jsctx, targetView->targetx, val);
		break;
		case TY_PROP:
			JS_NewDoubleValue(_jsctx, targetView->targety, val);
		break;
		case TZ_PROP:
			JS_NewDoubleValue(_jsctx, targetView->targetz, val);
		break;
		default:
			phui::DOM::ElementDOM::GetProperty(ctx, propid, val);
		break;
	}

	return JS_TRUE;
}

JSBool jsTargetView::SetProperty(JSContext* ctx, int propid, jsval* val)
{
	double dval = 0.0f;

	switch(propid)
	{
		case TARGETTEX_PROP:
			targetView->SetTargetTexture(JS_GetStringBytes(JS_ValueToString(ctx, *val)));
		break;
		case ARROWTEX_PROP:
			targetView->SetArrowTexture(JS_GetStringBytes(JS_ValueToString(ctx, *val)));
		break;
		case TARGETSIZE_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			targetView->targetsize = (float)dval;
		}
		break;
		case TX_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			targetView->targetx = dval;
		}
		break;
		case TY_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			targetView->targety = dval;
		}
		break;
		case TZ_PROP:
		{
			JS_ValueToNumber(ctx, *val, &dval);
			targetView->targetz = dval;
		}
		break;
		default:
			phui::DOM::ElementDOM::SetProperty(ctx, propid, val);
		break;
	}

	return JS_TRUE;
}

static void _jsabort(const char* msg)
{
	uiConsoleprintf("JS ABORT: %s\n", msg);
	piExitProgram(_FL, msg);
}

void uiGC()
{
	JS_GC(_jsgame->jsContext());
}
