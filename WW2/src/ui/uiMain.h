#pragma once

#include "phui/View.h"
#include "phui/Viewport.h"
#include "phui/Util.h"
#include "phui/Javascript.h"
#include "wwii/wwiiHCUnits.h"
#include "veh/vehdecal.h"

#define JS_MAX_CALL_ARGS 10

struct sdm_dtl_data;
struct sdm_replacement_octet_data;
struct sdm_strat_data;
class jsGame;

typedef void (*UIXMLCALLBACK)(JSContext* ctx, UINT32 argc, jsval* argv);

/************************************************************************************************
	ui
*************************************************************************************************/

#define UI_FONT_SIZE 12

extern int uiInit();
extern int uiPostInit(); // called at the end of rnmaininit
extern void locInitStrings(bool force = false);
extern int uiKill();

extern void uiUpdate();
extern void uiDraw();
extern void uiDrawString(const char* text, unsigned int color = 0xffffffff, unsigned int size = 0);

extern bool uiInput(struct imap_queue_entry*, int qcount, struct CHAR_INPUT*, int ccount);
extern bool uiFocused();
extern phui::View* uiFocusView();

extern void uiGameToUI();
extern void uiUIToGameStart();
extern void uiUIToGameEnd();
extern void uiSetDespawnInfo(const char* screen, int tab = 0, bool reset = true); // tolua_export

extern bool uiInGame(); // tolua_export

extern void uiJSCallback(JSContext* ctx, UINT32 argc, jsval* argv);
extern JSBool uiCallJS(const char* function, int argc, jsval* argv, UIXMLCALLBACK callback = nullptr); // arbitrarily typed arguments
extern JSBool uiCallJS(const char* function, UIXMLCALLBACK callback, int argc, ...); // string arguments
extern JSBool uiCallJS(const wchar_t* function, UIXMLCALLBACK callback, int argc, ...); // wide string arguments
extern void uiRunScript(const char* script); // tolua_export
extern void uiRunScript(phui::View* target, const char* script); // tolua_export
extern void uiSetCursor(int); // tolua_export
extern void uiSetAFK(int when); // when = when we went afk in ms
extern void uiSetAutoAFK(bool autoafk); // autoafk indicates this was an autoafk
extern bool uiSetFontSize(unsigned int fontsize, const char* font = nullptr); // set the ui font size; takes current resolution into account
extern void uiSetToolTips(bool on); // set the ui tooltips
extern int uiIsAFK();
extern bool uiIsAutoAFK();
extern int uiCursorState();
extern unsigned int uiImapCursorCallback(int param, unsigned int functionCode, unsigned int unused);

extern bool uiShowBuilderInfo(const wchar_t* strat, bool on);

extern int uiConsoleprintf(const char* text, ...);
extern const wchar_t* uiUTF8toUTF16(const char* text);
extern const wchar_t* uiUTF8toUTF16(const unsigned char* text);
extern const char* uiUTF16toUTF8(const wchar_t* text);

#ifdef _UNICODE
#define UTF8(x) uiUTF16toUTF8(x)
#define UTF16(x) uiUTF8toUTF16(x)
#else
#define UTF8(x) x
#define UTF16(x) x
#endif

extern phui::View* uiLoadView(const char*, bool addtoviewport=true, bool show=true);
extern phui::View* uiFindView(const char* name);
extern phui::Viewport* uiViewport();
extern phui::MessageLoop* uiMessageLoop();
extern jsGame* uiGame();
extern void uiGC();

extern int uiGetTexture(const char*);
extern void uiFreeTexture(const char*);

extern bool uiCheckLocTime(int& check);
extern const std::string& uiInputString(const char*);
extern const std::string& uiInputString(int, int);
extern const std::wstring& uiLocString(const char*); // tolua_export
extern const std::wstring& uiLocString(int, int);
extern const std::string& uiLocString2(const char*); // tolua_export
extern const std::string& uiLocString2(int, int); // tolua_export
extern bool uiHasLocString(const char*); // tolua_export
extern bool uiHasLocFile(const char*); // does the ui file exists; ui paths are localized

extern const wchar_t* uiLocalize(const wchar_t* text);
extern const wchar_t* uiLocalize(const char* text);

extern phui::IOCallbacks fioCallbacks;
extern phui::IOCallbacks cfgioCallbacks;

struct DrawIcons;
// IME
#if platOS == OS_WIN32
extern void uiIMEMessageProc(const MSG& m);
#endif
struct drawIcon;

class DrawIconView : public phui::View
{
public:
	DrawIconView(DrawIcons* r, int i) : phui::View() , drawicon(r), icon(i) {}
	virtual void DrawSelf();
	virtual void DrawSelfColor(float r, float g, float b, float alpha = 1.0f);

	DrawIcons* drawicon;
	int icon;
};

class ImageUVView : public phui::View
{
public:
	ImageUVView(int t, float u1, float v1, float u2, float v2) :
	  phui::View(), _texid(t), _u1(u1), _v1(v1), _u2(u2), _v2(v2) {}

	virtual void DrawSelf();

	int _texid;
	float _u1;
	float _v1;
	float _u2;
	float _v2;
};

class BrigadeDecalView : public ImageUVView
{
public:
	BrigadeDecalView(UINT32 bid) : ImageUVView(-1, 0, 0, 0, 0)
	{
		HCUnit* brigade = HCUnit::Lookup(bid);
		if(brigade)
			{
				vehdecalGetUnitTextureForUI(brigade, &_texid, &_u1, &_v1, &_u2, &_v2);
				_v1 = _v1;
				_v2 = _v2;

				// Swap to un-mirror image
				float tmp = _v1;
				_v1 = _v2;
				_v2 = tmp;

		}
	}
};

class TargetView : public phui::View
{
public:
	TargetView(float ts = 100, double tx = 0, double ty = 0, double tz = 0) :
		phui::View(),
		offscreen(0),
		targetsize(ts),
		targetx(tx),
		targety(ty),
		targetz(tz),
		sx(0),
		sy(0),
		sz(0),
		targettex(),
		arrowtex()
	{
	}

	virtual ~TargetView(){}
	virtual void DrawSelf();
	//virtual void DrawChildren(){}

	void Update(bool calc = true);
	void SetTarget(double x, double y, double z)
	{
		targetx = x;
		targety = y;
		targetz = z;
	}

	void SetTargetTexture(const char* t);
	void SetArrowTexture(const char* a);

	SINT32 offscreen;
	float targetsize;
	double targetx, targety, targetz, sx, sy, sz;
	phui::GraphicsContext::TextureInfo targettex;
	phui::GraphicsContext::TextureInfo arrowtex;
};

extern void uiSpawnLoadingScreen();
extern void uiEnterWorld();
extern void uiGameToUI();
extern void uiExitWorld();
