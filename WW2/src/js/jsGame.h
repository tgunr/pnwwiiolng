#pragma once

#include <string>

#include "phui/Javascript.h"
#include "cfg/cfgPrefs.h"
#include "pi/piinfo.h"
#include "js/jsStratDB.h"

/************************************************************************************************
	jsGame
*************************************************************************************************/

class jsPlayer;
class jsCursor;
class jsSettings;
class jsColor;
#ifdef GR2_BLEND_EDITOR
class jsBlendEditor;
#endif

namespace StratDB
{
	class StratObject;
};

class jsGame : public phui::js::Object
{
public:
	jsGame();

void getBuildInfo(std::string& display);

JS_CLASS_DATA;
JS_FUNCTION(load);
JS_FUNCTION(savesettings);
JS_FUNCTION(restoresettings);
JS_FUNCTION(close);
JS_FUNCTION(spawn);
JS_FUNCTION(spawnAt);
JS_FUNCTION(quit);
JS_FUNCTION(despawn);
JS_FUNCTION(findview);
JS_FUNCTION(arena);
JS_FUNCTION(consoleprint);
JS_FUNCTION(playsound);
JS_FUNCTION(setsoundvolume);
JS_FUNCTION(stopsound);
JS_FUNCTION(getkeymaps);
JS_FUNCTION(savekeymaps);
JS_FUNCTION(saveconvergence);
JS_FUNCTION(getconvergence);
JS_FUNCTION(initkeymaps);
JS_FUNCTION(killkeymaps);
JS_FUNCTION(getkeymapvehicles);
JS_FUNCTION(keymapisdirty);
JS_FUNCTION(keymapremoveall);
JS_FUNCTION(keymaprestore);
JS_FUNCTION(hasuserkeymap);
JS_FUNCTION(clearkeymap);
JS_FUNCTION(resetkeymap);
JS_FUNCTION(htmlencode);
JS_FUNCTION(getcontent);
JS_FUNCTION(getkeymapcalibration);
JS_FUNCTION(setkeymapcalibration);
JS_FUNCTION(enableimapinput);
JS_FUNCTION(getlocstring);
JS_FUNCTION(consoleclear);
JS_FUNCTION(launchurl);
JS_FUNCTION(getpref);
JS_FUNCTION(setpref);
JS_FUNCTION(getrgbcolor);
JS_FUNCTION(gethexcolor);
JS_FUNCTION(reloadchat);
JS_FUNCTION(okpref);
JS_FUNCTION(tutorialevent);
JS_FUNCTION(starttutorial);
JS_FUNCTION(stoptutorial);
JS_FUNCTION(tutorialcompleted);
JS_FUNCTION(settutorialcompleted);
JS_FUNCTION(setkeymappreset);
JS_FUNCTION(getkeymappresets); // get the list
JS_FUNCTION(getkeymappreset); // get the selection
JS_FUNCTION(getkeymapkey);
JS_FUNCTION(getworldtoscreen);
JS_FUNCTION(resetcontrollers);
JS_FUNCTION(setcamera);
JS_FUNCTION(utf8);
JS_FUNCTION(utf16);
JS_FUNCTION(uicallback);
JS_FUNCTION(chatprint);
JS_FUNCTION(cancelppo);
JS_FUNCTION(bailout);
JS_FUNCTION(startscenario);
JS_FUNCTION(stopscenario);
JS_FUNCTION(setscenariocomplete);
JS_FUNCTION(haspermission);
JS_FUNCTION(setplayerdata); // scenarios
JS_FUNCTION(addplayerdata); // scenarios

#ifdef GR2_BLEND_EDITOR
JS_FUNCTION(addblend);
JS_FUNCTION(validblend);
JS_FUNCTION(blendname);
JS_FUNCTION(saveblend);
JS_FUNCTION(removeblend);
JS_FUNCTION(addanimation);
JS_FUNCTION(addanimevent);
JS_FUNCTION(playanimation);
JS_FUNCTION(removeanimation);
JS_FUNCTION(setblendweapon);
JS_FUNCTION(setblendweights);
JS_FUNCTION(updateblends);
JS_FUNCTION(reorderblend);
JS_FUNCTION(usedblend);
JS_FUNCTION(clearanimcache);
#endif

enum
{
	GAME_PROP, CPS_PROP, ONLINE_PROP, TIME_PROP, INGAME_PROP,
	SPAWNABLEVEHICLES_PROP, SETTINGS_PROP, SCREENWIDTH_PROP,
	SCREENHEIGHT_PROP, TRAINING_PROP, PERSONAVEHICLES_PROP,
	REGION_PROP, LASTENTERED_PROP, CANBAIL_PROP, BAILEDOUT_PROP,
	SCENARIO_PROP, CUSTOMTERRAIN_PROP, SCENARIOCOMPLETE_PROP, SCENARIOSPAWNS_PROP,
	EZMODE_PROP
#ifdef GR2_BLEND_EDITOR
	,BLENDEDITOR_PROP
#endif
};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

	// gets a type specific location for a cp or fac
	static const StratDB::StratObject* GetOfflineSpawn(const StratDB::StratObject* origin, const void* vtype);
	jsval SetWorldToScreen(double x, double y, double z);

	jsPlayer* jsplayer;

protected:
	phui::js::array_root_ptr keymaps;
	phui::js::array_root_ptr cps;
	phui::js::array_root_ptr spawnablevehicles;
	phui::js::array_root_ptr worldtoscreen;
	phui::js::array_root_ptr scenariospawns;
	phui::js::root_ptr<jsSettings> settings;
	phui::js::root_ptr<jsStratObject> lastentered;

#ifdef GR2_BLEND_EDITOR
	phui::js::root_ptr<jsBlendEditor> blendeditor;
#endif

	~jsGame();
};

class jsCursor : public phui::js::Object
{
public:
	jsCursor();

JS_CLASS_DATA;
JS_FUNCTION(update);

enum{ X_PROP, Y_PROP, VISIBLE_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsCursor();
};


class jsSettings : public phui::js::Object
{
public:
	jsSettings();

JS_CLASS_DATA;

enum
{
	RESTARTNEEDED_PROP = cfgNUM_TOTAL_PARAMS,
	VIDEOCARDS_PROP,
	SOUNDCARDS_PROP,
	LANGUAGES_PROP,
	SSE2_SUPPORTED_PROP,
	WATER_SUPPORTED_PROP,
	REFLECTIONS_SUPPORTED_PROP,
	POSTRENDER_SUPPORTED_PROP,
	GRASSSHADER_SUPPORTED_PROP,
	NORMALMAPS_SUPPORTED_PROP,
	REFRACTIONS_SUPPORTED_PROP,
    SHADOWMAPS_SUPPORTED_PROP,
	// add more manual settings here
	TOTAL_SETTINGS
};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	phui::js::array_root_ptr _videocards;
	phui::js::array_root_ptr _soundcards;
	phui::js::array_root_ptr _languages;

	~jsSettings();
};

class jsColor : public phui::js::Object
{
public:
	jsColor(JSContext* ctx, JSObject* parent);
	jsColor(JSContext* ctx, JSObject* parent, int r, int g, int b);
	jsColor(JSContext* ctx, JSObject* parent, const char* hexcolor);

JS_CLASS_DATA;

enum
{
	RED_PROP,
	GREEN_PROP,
	BLUE_PROP,
	REDHEX_PROP,
	GREENHEX_PROP,
	BLUEHEX_PROP,
	COLOR_PROP,
	COLORHEX_PROP,
	COLORHEXCSS_PROP
};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsColor();

	int32 _red, _green, _blue;
};

class jsVideoCard : public phui::js::Object
{
public:
	jsVideoCard(int32 index, PIDeviceInfo& device);

	JS_CLASS_DATA;

	enum{ INDEX_PROP, NAME_PROP, MODES_PROP, ID_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsVideoCard();

	phui::js::array_root_ptr _videomodes;

	PIDeviceInfo _device;
	int32 _index;
};

class jsSoundCard : public phui::js::Object
{
public:
	jsSoundCard(int32 index, PIDeviceInfo& device);

	JS_CLASS_DATA;

	enum{ INDEX_PROP, NAME_PROP, ID_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsSoundCard();

	PIDeviceInfo _device;
	int32 _index;
};

class jsScenarioState : public phui::js::Object
{
public:
	jsScenarioState();
	JS_CLASS_DATA;

	enum
	{
		STATUS_PROP,
		ACTIVE_PROP,
		READY1_PROP,
		READY2_PROP,
		TEAM_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
};

#ifdef GR2_BLEND_EDITOR

class jsWeight : public phui::js::Object
{
public:
	jsWeight(int blend, int animation, int bone);

	JS_CLASS_DATA;

	enum{ INDEX_PROP, VALUE_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsWeight();

	int _bone;
	int _animation;
	int _blend;
};

class jsAnimation : public phui::js::Object
{
public:
	jsAnimation(int blend, int animation);

	JS_CLASS_DATA;

	enum{ INDEX_PROP, FILENAME_PROP, WEIGHTS_PROP, TIME_PROP, LENGTH_PROP, EVENTS_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsAnimation();

	int _animation;
	int _blend;
	phui::js::array_root_ptr _weights;
	phui::js::array_root_ptr _events;
};

class jsAnimEvent : public phui::js::Object
{
public:
	jsAnimEvent(int blend, int animation, int evt);

	JS_CLASS_DATA;

	enum{ INDEX_PROP, TYPE_PROP, TIME_PROP };

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsAnimEvent();

	int _event;
	int _animation;
	int _blend;
};

class jsBlend : public phui::js::Object
{
public:
	jsBlend(int blend);

	JS_CLASS_DATA;

	enum
	{
		INDEX_PROP,
		FILENAME_PROP,
		ANIMATIONS_PROP,
		WEIGHT_PROP,
		MODIFIED_PROP,
		SELECTED_PROP,
		SPEED_PROP,
		TIME_PROP,
		LENGTH_PROP,
		TYPE_PROP,
		PLAY_PROP,
		ACTIVE_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsBlend();

	int _blend;
	phui::js::array_root_ptr _animations;
};

class jsBlendEditor : public phui::js::Object
{
public:
	jsBlendEditor();

	JS_CLASS_DATA;

	enum
	{
		ANIMATIONS_PROP,
		BLENDS_PROP,
		TYPES_PROP,
		STATE1_PROP,
		STATE2_PROP,
		STATE3_PROP,
		STATE4_PROP,
		ROTATEMODEL_PROP,
		OSCILLATION_PROP,
		TYPE_PROP
	};

	virtual JSBool GetProperty(JSContext* ctx, int propid, jsval* val);
	virtual JSBool SetProperty(JSContext* ctx, int propid, jsval* val);

protected:
	~jsBlendEditor();

	phui::js::array_root_ptr _blends;
	phui::js::array_root_ptr _animations;
	phui::js::array_root_ptr _types;
	phui::js::array_root_ptr _state1;
	phui::js::array_root_ptr _state2;
	phui::js::array_root_ptr _state3;
	phui::js::array_root_ptr _state4;

	SINT32 _type;
};

#endif // GR2_BLEND_EDITOR

