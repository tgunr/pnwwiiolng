#include "syscfg.h" // Path is project specific.
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "mem/mem.h"
#include "imap/imap.h"
#include "imap/iac.h"
#include "imap/ww2imap.h"
#include "imap/ww2func.h"
#include "fio/fio.h"
#include "fio/fiod.h"
#include "locl/loclLang.h"
#include "locl/loclApp.h"
#include "veh/vehmain.h"
#include "ml/mlhead.h"
#include "pers/pershud.h"
#include "ui/uiMain.h"
#include "weap/weapList.h"
#include "xml/XMLLoader2.h"
#include "cfg/cfgPrefs.h"
#include "dip.h"
#include "tut/TutorialManager.h"
#include "ui/uiDotCommands.h"

/************************************************************************************************
	ww2imap
*************************************************************************************************/

#define USER_CFML_DIR ((WantDataDir) ? "usercfml" : "cfml")
//#define IMAP_PRESETS "cfml" FILE_SEPARATOR "presets.xml"
#define GLOBAL_CALIBRATION_FILENAME ((WantDataDir) ? "usercfml/global.calib" : "cfml/global.calib")
#define IMAP_PRESETS "cfml/presets.xml"

#define MAX_VEHICLES 512
#define ALL_VEHICLES -1
#define FROM_DISK 0
#define FROM_ARCHIVE 1

#if platOS == OS_MACOS
#define DEFAULT_MOUSE_SENS	1.00f
#define DEFAULT_AIM_SENS	1.00f
#else
#define DEFAULT_MOUSE_SENS	1.50f
#define DEFAULT_AIM_SENS	0.90f
#endif

enum
{
	KEYMAP_GENERAL_ARCHIVE = 0,
	KEYMAP_GENERAL_DISK,
	KEYMAP_VIEW_ARCHIVE,
	KEYMAP_VIEW_DISK,
	KEYMAP_VEH_CATEGORY_ARCHIVE,
	KEYMAP_VEH_CATEGORY_DISK,
	KEYMAP_VEHICLE_ARCHIVE,
	KEYMAP_VEHICLE_DISK
};

enum
{
	PRESET_REQ_NONE = 0,
	PRESET_REQ_JOYSTICK,
	PRESET_REQ_MOUSE,
	PRESET_REQ_KEYBOARD,
	PRESET_REQ_NUM
};

struct imap_joy_calib
{
	imap_joy_calib* next;

	unsigned int iac;
	float deadband, x, y, fullband;
};

struct imap_axis_calib
{
	float mouseX;
	float mouseY;
	float aimMouseX, aimMouseY;
	imap_joy_calib* joystick;
};

static const char* categoryName[WW2_FUNC_NUM_CATEGORIES] =
{
	"general", "views", "air", "tank",
	"truck", "atgun", "infantry", "sea", "ez"
};

static const int categoryVehicleType[WW2_FUNC_NUM_CATEGORIES] =
{
	-1, -1, VEH_TYPE_AIR, VEH_TYPE_GROUND, VEH_TYPE_GROUND,
	VEH_TYPE_GROUND,VEH_TYPE_PERSON, VEH_TYPE_SEA, -1
};

// Name in weapon spec, currently used for sub-classing ground categories.
static const char* categoryClassName[WW2_FUNC_NUM_CATEGORIES] =
{
	"", "", "", "tank", "truck", "gun", "", "", ""
};

static imap_presets categoryPresets[WW2_FUNC_NUM_CATEGORIES];
static imapKeymapRef archiveKeymap [WW2_FUNC_NUM_CATEGORIES];
static imapKeymapRef categoryKeymap[WW2_FUNC_NUM_CATEGORIES];
static imapKeymapRef vehicleKeymap [MAX_VEHICLES];
static imapKeymapRef vehicleArchiveKeymap [MAX_VEHICLES];

static int categoryKeymapDirty[WW2_FUNC_NUM_CATEGORIES];
static int vehicleKeymapDirty [MAX_VEHICLES];

static int categoryUserKeymap[WW2_FUNC_NUM_CATEGORIES];
static int vehicleUserKeymap[MAX_VEHICLES];

static imap_axis_calib* axisCalib;

static LANGTABLE* iacLangTable;
static LANGTABLE* funcLangTable;

static int curIacLang = -1;
static int curFuncLang = -1;
static int _defaultcfml;
static int _turretAndMouseLookUseSameKeymap;

using ImapIndexMap = std::map<int, int>;
static std::map<int,ImapIndexMap> _imapIndexMap;

// load imap presets
class ImapPresetLoader : public XMLLoader2<ImapPresetLoader>
{
public:

	ImapPresetLoader(const char* path) : XMLLoader2<ImapPresetLoader>(path)
	{
		AddHandler("category", &ImapPresetLoader::DoCategory, this);
		AddHandler("preset", &ImapPresetLoader::DoPreset, this);
	}

	virtual ~ImapPresetLoader()
	{
	}

	// tag handlers
	XML_TAG_HANDLER(DoCategory)
	{
		if(first)
		{
			const char* cat = xmlGetStringFromNode(node, "name");

			for(int i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
			{
				if(!stricmp(cat, categoryName[i]))
				{
					ProcessChildren(node, &i);
					break;
				}
			}
		}

		return true;
	}

	XML_TAG_HANDLER(DoPreset)
	{
		if(first && userData)
		{
			int category			= *(int*)userData;
			imap_presets& presets	= categoryPresets[category];
			imap_preset preset;

			preset.requires			= PRESET_REQ_NONE;
			preset.category			= category;
			preset.ez				= 0;
			preset.name				= categoryName[category];
			preset.path				= xmlGetStringFromNode(node, "path");

			xmlGetNodeValue(node, XML_DATA_SINT, &preset.ez, "ez");

			const char* name		= xmlGetStringFromNode(node, "name");
			const char* requires	= xmlGetStringFromNode(node, "requires");

			if(name && *name)
			{
				preset.name	+= ".";
				preset.name += name;
			}

			if(requires && *requires)
			{
				if(!stricmp(requires, "joystick"))
					preset.requires = PRESET_REQ_JOYSTICK;
				else if(!stricmp(requires, "mouse"))
					preset.requires = PRESET_REQ_MOUSE;
				else if(!stricmp(requires, "keyboard"))
					preset.requires = PRESET_REQ_KEYBOARD;
			}

			presets.push_back(preset);
		}

		return false;
	}
};

static imapKeymapRef ww2Keymap	(int category, int vehicleIdx, int archive);
static void ww2KeymapFileName 	(int category, int vehicleIdx, int archive, char* path, int maxlen, int defaultcfml);
static int  ww2KeymapvehtypeListIdx	(int category, int vehicleIdx);
static void ww2KeymapSetDirty 	(int category, int vehicleIdx, int dirty);
static int  ww2KeymapLoadVehicleRef(imapKeymapRef ref, int vehtypeListIdx, int archive);
static int  ww2KeymapLoadRef(imapKeymapRef ref, int category, int vehicleIdx, int archive, int defaultcfml=_defaultcfml);
static int  ww2KeymapSaveRef(imapKeymapRef ref, int category, int vehicleIdx);

static imap_axis_calib* ww2KeymapLoadAxisCalibration(const char*);
static void ww2KeymapFreeAxisCalibration(imap_axis_calib*);
static imap_joy_calib* ww2KeymapFindCalibration(imap_axis_calib*, unsigned int);
static void ww2KeymapSaveAxisCalibration(imap_axis_calib*, const char* filename);

static void ww2imapSetAxisCalibration(imap_axis_calib*);

static void iac_localized_string	(const char* language, int locid, char* str, int maxlen);
static void ww2func_localized_string(const char* language, int locid, char* str, int maxlen);
#if(0)
static int lookup_lang_id(const char* language);
#endif
static int imap_read (void* userdata, int numbytes, void* buf);
static int imap_write(void* userdata, int numbytes, const void* buf);

static unsigned int ww2imapCenterAxis (int param, unsigned int functionCode, unsigned int unused);
static void get_mouselook(imapKeymapRef ref, UINT32* mouseLookX, UINT32* mouseLookY);
static void get_torso(imapKeymapRef ref, UINT32* torsoX, UINT32* torsoY);
static void get_turret(imapKeymapRef ref, UINT32* turretX, UINT32* turretY);

/************************************************************************************************
	ww2imapInit
*************************************************************************************************/

int ww2imapInit()
{
	imapLoadPresets();
	imapInit();
	imapInstallCallbacks(imap_read, imap_write, iac_info, ww2func_info);

	ww2func_register();
	iacSetLocalizationCallback(iac_localized_string);
	ww2funcSetLocalizationCallback(ww2func_localized_string);

	imapInstallNonRemovableControlCallback(IMAP_CENTER_AXIS, ww2imapCenterAxis);
	imapInstallNonRemovableControlCallback(IMAP_SHOW_INFO, persHUDToggleGameInfo);
	imapInstallNonRemovableControlCallback(IMAP_MOM_TAKE_SCREEN_SHOT, piCapture);
	imapInstallNonRemovableControlCallback(IMAP_MOM_SHOW_GUI_CURSOR, uiImapCursorCallback);
	imapInstallNonRemovableControlCallback(IMAP_CONTINUE_TUTORIAL, TutorialManager::Input);

	return IMAP_NO_ERR;
}

/************************************************************************************************
	ww2imapKill
*************************************************************************************************/

void ww2imapKill()
{
	imapKill();

	if(iacLangTable)
		loclLangTableFree(iacLangTable);

	if(funcLangTable)
		loclLangTableFree(funcLangTable);

	iacLangTable = nullptr;
	funcLangTable = nullptr;
	curIacLang = -1;
	curFuncLang = -1;
}

/************************************************************************************************
	ww2imapSetPreset
*************************************************************************************************/

bool ww2imapSetPreset(int category, int preset)
{
	if(category >= 0 && category < WW2_FUNC_NUM_CATEGORIES)
	{
		// set the preset
		categoryPresets[category].select(preset);

		// now lets save the selected preset in our config
		// if the select failed, then the previous value is
		// simply re-set in our config
		char cfgKey[32];
		snprintf(cfgKey, 31, "input.presets.%d", category);
		cfgPrefsSetInt(cfgKey, categoryPresets[category].selected);

		// return whether it actually got set or not
		return (categoryPresets[category].selected == preset);
	}

	return false;
}

bool ww2imapSetEZPreset(int category)
{
	if(gEZMode && category >= 0 && category < WW2_FUNC_NUM_CATEGORIES)
	{
		imap_presets& presets = categoryPresets[category];
		int selected = -1;

		for(size_t i = 0; i < presets.size(); i++)
		{
			imap_preset& preset = presets.at(i);

			if(preset.ez >= (int)gEZMode && (selected == -1 || preset.ez < selected))
				selected = (int)i;
		}

		// return true if we changed our selected preset
		if(selected >= 0 && selected != presets.selected)
		{
			presets.select(selected);
			return true;
		}
	}

	return false;
}

/************************************************************************************************
	ww2imapGetPresets
*************************************************************************************************/

imap_presets* ww2imapGetPresets(int category)
{
	if(category >= 0 && category < WW2_FUNC_NUM_CATEGORIES)
		return &categoryPresets[category];

	return nullptr;
}

/************************************************************************************************
	ww2imapSetKeymap
*************************************************************************************************/

int  ww2imapSetKeymapById(const char* id)
{
	vehType const* vehtype = vehTypeGetById(id);
	if(!vehtype) return -1;

	return ww2imapSetKeymap(vehtype->VehicleTypeNum);
}

int ww2imapSetKeymap(int vehtypeListIdx)
{
	int i, category = -1;
	imapKeymapRef ref;

	vehType const* vehtype;
	UINT32 mouseLookX, mouseLookY, torsoX, torsoY, turretX, turretY;
	mouseLookX = mouseLookY = torsoX = torsoY = turretX = turretY = 0;

	vehtype = vehtypeGetByIndex((UINT32)vehtypeListIdx);
	if(vehtype == nullptr) return IMAP_ERR;

	 // clear current keymaps
	for(i = 0; i < IMAP_MAX_KEYMAPS; i++)
		imapSetKeymap(nullptr, i);

	ref = imapKeymapAlloc();
	if(ref == nullptr) return IMAP_MEMORY_ERR;

	// general
	ww2KeymapLoadRef(ref, WW2_FUNC_CATEGORY_GENERAL, ALL_VEHICLES, FROM_ARCHIVE);
	imapSetKeymap(ref, KEYMAP_GENERAL_ARCHIVE);

	ww2KeymapLoadRef(ref, WW2_FUNC_CATEGORY_GENERAL, ALL_VEHICLES, FROM_DISK);
	imapSetKeymap(ref, KEYMAP_GENERAL_DISK);

#if RELEASE_CLIENT
	switch(OverrideSpawner)
	{
	case BENCHANTWERP_OVERRIDE:
	case BENCHRAMAGEN_OVERRIDE:
		{
			return 0;
		}
	}
#endif

	// view
	if(vehtype->Type != VEH_TYPE_PERSON)
	{
		ww2KeymapLoadRef(ref, WW2_FUNC_CATEGORY_VIEW, ALL_VEHICLES, FROM_ARCHIVE);
		imapSetKeymap(ref, KEYMAP_VIEW_ARCHIVE);

		ww2KeymapLoadRef(ref, WW2_FUNC_CATEGORY_VIEW, ALL_VEHICLES, FROM_DISK);
		imapSetKeymap(ref, KEYMAP_VIEW_DISK);
	}

	// category
	switch(vehtype->Type)
	{
		case VEH_TYPE_AIR:
			category = WW2_FUNC_CATEGORY_AIR;
		break;
		case VEH_TYPE_GROUND:
		{
			const std::string& weapClassName = weaponClass[vehtypeWeaponSpec(vehtypeListIdx).wClass].name;
			for(i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
			{
				if(weapClassName ==  categoryClassName[i])
				{
					category = i;
					break;
				}
			}
		}
		break;
		case VEH_TYPE_SEA:
			category = WW2_FUNC_CATEGORY_SEA;
		break;
		case VEH_TYPE_PERSON:
			category = WW2_FUNC_CATEGORY_PERSON;
		break;
	}

	if(category > -1)
	{
		ww2KeymapLoadRef(ref, category, ALL_VEHICLES, FROM_ARCHIVE);
		imapSetKeymap(ref, KEYMAP_VEH_CATEGORY_ARCHIVE);
		get_mouselook(ref, &mouseLookX, &mouseLookX);
		get_torso(ref, &torsoX, &torsoY);
		get_turret(ref, &turretX, &turretY);

		ww2KeymapLoadRef(ref, category, ALL_VEHICLES, FROM_DISK);
		imapSetKeymap(ref, KEYMAP_VEH_CATEGORY_DISK);
		get_mouselook(ref, &mouseLookX, &mouseLookX);
		get_torso(ref, &torsoX, &torsoY);
		get_turret(ref, &turretX, &turretY);

		// vehicle
		ww2KeymapLoadVehicleRef(ref, vehtypeListIdx, FROM_ARCHIVE);
		imapSetKeymap(ref, KEYMAP_VEHICLE_ARCHIVE);
		get_mouselook(ref, &mouseLookX, &mouseLookX);
		get_torso(ref, &torsoX, &torsoY);
		get_turret(ref, &turretX, &turretY);

		ww2KeymapLoadVehicleRef(ref, vehtypeListIdx, FROM_DISK);
		imapSetKeymap(ref, KEYMAP_VEHICLE_DISK);
		get_mouselook(ref, &mouseLookX, &mouseLookX);
		get_torso(ref, &torsoX, &torsoY);
		get_turret(ref, &turretX, &turretY);
	}

	imapKeymapFree(ref);

	if(vehtype->Type == VEH_TYPE_PERSON)
	{
		mlSetMouseLookAndTorsoKeymapFlag(mouseLookX == torsoX || mouseLookY == torsoY || mouseLookX == torsoY || mouseLookY == torsoX);
	}else
	{
		_turretAndMouseLookUseSameKeymap = (mouseLookX == turretX || mouseLookY == turretY || mouseLookX == turretY || mouseLookY == turretX);
	}

	if(axisCalib)
	{
		ww2KeymapSaveAxisCalibration(axisCalib, GLOBAL_CALIBRATION_FILENAME);
		ww2KeymapFreeAxisCalibration(axisCalib);
		axisCalib = nullptr;
	}

	// set up calibration
	{
		imap_axis_calib* calib;
		char vehName[256], catName[256];

		snprintf(vehName, 255, "%s\\%s.calib", USER_CFML_DIR, vehtype->Shape);

		if(category > -1)
			snprintf(catName, 255, "%s\\%s.calib", USER_CFML_DIR,  categoryName[category]);

		if(category > -1 && fiocfgAccess(catName, true))
			calib = ww2KeymapLoadAxisCalibration(catName);
		else if(fiocfgAccess(vehName, true))
			calib = ww2KeymapLoadAxisCalibration(vehName);
		else
			calib = ww2KeymapLoadAxisCalibration(GLOBAL_CALIBRATION_FILENAME);

		if(calib)
		{
			ww2imapSetAxisCalibration(calib);
			ww2KeymapFreeAxisCalibration(calib);
		}
	}

	if(vehtype->Type == VEH_TYPE_FLOATER)
		ww2imapAddBuzzardKeymaps();

	//ww2imapAddBetaKeymaps();

	return 0;
}

void ww2imapAddBuzzardKeymaps()
{
	imapKeymapRef ref;
	imap_keymap keymap;
	unsigned int idx;

	memset(&keymap, 0x0, sizeof(keymap));
	ref = imapKeymapAlloc();

	idx = 0;
	keymap.numKeys = 1;

	keymap.keymapType = IMAP_AXIS_KEYMAP;
	keymap.axis.iac = IAC_MOUSEX;
	imapKeymapAdd(ref, IMAP_MOUSE_LOOK_X, &idx, &keymap);

	keymap.keymapType = IMAP_AXIS_KEYMAP;
	keymap.axis.iac = IAC_MOUSEY;
	imapKeymapAdd(ref, IMAP_MOUSE_LOOK_Y, &idx, &keymap);

	keymap.keymapType = IMAP_BUTTON_KEYMAP;
	keymap.btn.iac[0] = IAC_MOUSE_WHEEL_UP;
	imapKeymapAdd(ref, IMAP_TMP_CHANGE_FOV_UP, &idx, &keymap);

	keymap.keymapType = IMAP_BUTTON_KEYMAP;
	keymap.btn.iac[0] = IAC_MOUSE_WHEEL_DOWN;
	imapKeymapAdd(ref, IMAP_TMP_CHANGE_FOV_DOWN, &idx, &keymap);

	imapSetKeymap(ref, KEYMAP_VEHICLE_ARCHIVE);

	imapKeymapFree(ref);
}

void ww2imapAddBetaKeymaps()
{
#ifdef BETA_RELEASE
	/*
	imapKeymapRef ref;
	imap_keymap keymap;
	unsigned int idx;

	memset(&keymap, 0x0, sizeof(keymap));
	ref = imapKeymapAlloc();

	idx = 0;
	keymap.numKeys = 1;

	keymap.keymapType = IMAP_BUTTON_KEYMAP;
	keymap.btn.iac[0] = IAC_SPACE;
	imapKeymapAdd(ref, IMAP_VSPAWN, &idx, &keymap);

	imapSetKeymap(ref, KEYMAP_VEHICLE_ARCHIVE);
	imapSetKeymap(0, KEYMAP_VEHICLE_DISK);

	imapKeymapFree(ref);
	*/
#endif
}

/************************************************************************************************
	ww2KeymapInit
*************************************************************************************************/

int ww2KeymapInit(int defaultcfml)
{
	int i, j;
	imapKeymapRef ref;

	fiocfgMkDir(USER_CFML_DIR, true);

	for(i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
	{
		// archive
		ref = ww2Keymap(i, ALL_VEHICLES, FROM_ARCHIVE);
		if(ref)
			ww2KeymapLoadRef(ref, i, ALL_VEHICLES, FROM_ARCHIVE, defaultcfml);

		// disk
		ref = ww2Keymap(i, ALL_VEHICLES, FROM_DISK);
		if(ref)
			ww2KeymapLoadRef(ref, i, ALL_VEHICLES, FROM_DISK, defaultcfml);
		ww2KeymapSetDirty(i, ALL_VEHICLES, 0);

		// vehicles
		for(j = 0; j < MAX_VEHICLES; j++)
		{
			ref = ww2Keymap(i, j, FROM_DISK);
			if(ref == nullptr) break;

			ww2KeymapLoadRef(ref, i, j, FROM_DISK,defaultcfml);
			ww2KeymapSetDirty(i, j, 0);

			ref = ww2Keymap(i, j, FROM_ARCHIVE);
			if(ref)
				ww2KeymapLoadRef(ref, i, j, FROM_ARCHIVE,defaultcfml);
		}
	}

	axisCalib = ww2KeymapLoadAxisCalibration(GLOBAL_CALIBRATION_FILENAME);

	return 0;
}

/************************************************************************************************
	ww2KeymapKill
*************************************************************************************************/

void ww2KeymapKill()
{
	int i;

	for(i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
	{
		if(archiveKeymap[i])
			imapKeymapFree(archiveKeymap[i]);

		if(categoryKeymap[i])
			imapKeymapFree(categoryKeymap[i]);


		archiveKeymap[i] = nullptr;
		categoryKeymap[i] = nullptr;
	}

	for(i = 0; i < MAX_VEHICLES; i++)
	{
		if(vehicleKeymap[i])
			imapKeymapFree(vehicleKeymap[i]);

		vehicleKeymap[i] = nullptr;

		if(vehicleArchiveKeymap[i])
			imapKeymapFree(vehicleArchiveKeymap[i]);

		vehicleArchiveKeymap[i] = nullptr;

	}

	if(axisCalib)
	{
		ww2KeymapSaveAxisCalibration(axisCalib, GLOBAL_CALIBRATION_FILENAME);
		ww2KeymapFreeAxisCalibration(axisCalib);
		axisCalib = nullptr;
	}
}

/************************************************************************************************
	ww2KeymapSet
*************************************************************************************************/

int ww2KeymapSet(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, const imap_keymap* keymap)
{
	if(ww2KeymapCmp(category, vehicleIdx, functionCode, idx, keymap) == 0) return 0;

	imapKeymapRef ref = nullptr;
	int err = IMAP_ERR;

	ref = ww2Keymap(category, vehicleIdx, FROM_DISK);

	if(ref)
	{
		// have to hard-code onRelease since there's no GUI for it
		if(keymap->keymapType == IMAP_AXIS_ABS_KEYMAP)
		{
			unsigned int flags = ww2funcFlags(functionCode);
			if(flags & WW2_FUNC_USE_ONRELEASE)
			{
				((imap_keymap*)keymap)->axisAbs.onRelease = 1;
				((imap_keymap*)keymap)->axisAbs.releasePercent = (flags & WW2_FUNC_ASYMMETRIC_AXIS) ? 0.0f : 0.5f;
			}
		}

		err = imapKeymapSet(ref, functionCode, idx,  keymap);
	}

	ww2KeymapSetDirty(category, vehicleIdx, 1);

	return err;
}

/************************************************************************************************
	ww2KeymapCmp
*************************************************************************************************/

int  ww2KeymapCmp(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx,const imap_keymap* keymap)
{
	imap_keymap curkeymap;

	if(ww2KeymapGet(category, vehicleIdx, functionCode, idx, keymap->keymapType, &curkeymap, nullptr) == 0)
	{
		return memcmp(keymap, &curkeymap, sizeof(imap_keymap));
	}

	return 1;
}

/************************************************************************************************
	ww2KeymapGet
*************************************************************************************************/

int ww2KeymapGet(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx,
					int keymapType, imap_keymap* keymap, int* depth)
{
	int err = IMAP_KEYMAP_NOT_FOUND_ERR;
	imapKeymapRef ref;

	// vehicle
	if(depth) *depth = 2;
	if(vehicleIdx > -1)
	{
		ref = ww2Keymap(category, vehicleIdx, FROM_DISK);
		if(ref)
		{
			err = imapKeymapGet(ref, functionCode, idx, keymapType, keymap);
		}

		if(err)
		{
			ref = ww2Keymap(category, vehicleIdx, FROM_ARCHIVE);
			err = imapKeymapGet(ref, functionCode, idx, keymapType, keymap);
		}

	}

	// category
	if(err)
	{
		if(depth) (*depth)--;
		ref = ww2Keymap(category, ALL_VEHICLES, FROM_DISK);

		if(ref)
			err = imapKeymapGet(ref, functionCode, idx, keymapType, keymap);
	}

	// archive
	if(err)
	{
		if(depth) (*depth)--;
		ref = ww2Keymap(category, ALL_VEHICLES, FROM_ARCHIVE);

		if(ref)
			err = imapKeymapGet(ref, functionCode, idx, keymapType, keymap);
	}


	return err;
}

/************************************************************************************************
	ww2KeymapClear
*************************************************************************************************/

void ww2KeymapClear(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, int keymapType)
{
	imapKeymapRef ref = ww2Keymap(category, vehicleIdx, 0);
	if(ref == nullptr) return;

	imap_keymap keymap;
	memset(&keymap, 0, sizeof(keymap));
	keymap.keymapType = keymapType;
	if(keymapType != IMAP_AXIS_KEYMAP)
		keymap.numKeys = 1;
	imapKeymapSet(ref, functionCode, idx, &keymap);

	ww2KeymapSetDirty(category, vehicleIdx, 1);
}

/************************************************************************************************
	ww2KeymapRemove
*************************************************************************************************/

void ww2KeymapRemove(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, int keymapType)
{
	imapKeymapRef ref = ww2Keymap(category, vehicleIdx, 0);

	if(ref)
		imapKeymapRemove(ref, functionCode, idx, keymapType);

	ww2KeymapSetDirty(category, vehicleIdx, 1);
}

/************************************************************************************************
	ww2KeymapHasKeyboardAxis
*************************************************************************************************/

int ww2KeymapHasKeyboardAxis(int category, int vehicleIdx, unsigned int functionCode)
{
	int i;

	for(i = 0; i < 4; i++)
	{
		imap_keymap km;
		if(ww2KeymapGet(category, vehicleIdx, functionCode, i, IMAP_AXIS_REL_KEYMAP, &km, nullptr) == 0)
			return 1;
	}

	for(i = 0; i < 11; i++)
	{
		imap_keymap km;
		if(ww2KeymapGet(category, vehicleIdx, functionCode, i, IMAP_AXIS_ABS_KEYMAP, &km, nullptr) == 0)
			return 1;
	}

	return 0;
}

/************************************************************************************************
	ww2KeymapSave
*************************************************************************************************/

int ww2KeymapSave(int category, int vehicleIdx)
{
	imapKeymapRef ref = ww2Keymap(category, vehicleIdx,0);
	int err = IMAP_ERR;

	if(ref)
		err = ww2KeymapSaveRef(ref, category, vehicleIdx);

	ww2KeymapSetDirty(category, vehicleIdx, 0);

	return err;

}

/************************************************************************************************
	ww2KeymapRestore
*************************************************************************************************/

int ww2KeymapRestore(int category, int vehicleIdx)
{
	imapKeymapRef ref = ww2Keymap(category, vehicleIdx, 0);
	int err = IMAP_ERR;

	if(ref)
		err = ww2KeymapLoadRef(ref, category, vehicleIdx, 0);

	ww2KeymapSetDirty(category, vehicleIdx, 0);

	return IMAP_ERR;
}

/************************************************************************************************
	ww2KeymapRemoveAll
*************************************************************************************************/

void ww2KeymapRemoveAll(int category, int vehicleIdx)
{
	imapKeymapRef ref = ww2Keymap(category, vehicleIdx, 0);

	if(ref)
		imapKeymapRemoveAll(ref);

	ww2KeymapSetDirty(category, vehicleIdx, 1);
}

/************************************************************************************************
	ww2KeymapIsDirty
*************************************************************************************************/

int ww2KeymapIsDirty(int category, int vehicleIdx)
{
	int dirty = -1;

	if(vehicleIdx < 0)
	{
		dirty = categoryKeymapDirty[category];
	}
	else
	{
		int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);
		if(idx > -1)
			dirty = vehicleKeymapDirty[idx];
	}

	return dirty;
}

/************************************************************************************************
	ww2KeymapHasUserKeymap
*************************************************************************************************/

int ww2KeymapHasUserKeymap(int category, int vehicleIdx)
{
	int hasuserkeymap = -1;

	if(vehicleIdx < 0)
	{
		hasuserkeymap = categoryUserKeymap[category];
	}else
	{
		int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);
		if(idx > -1)
			hasuserkeymap = vehicleUserKeymap[idx];
	}

	return hasuserkeymap;
}

/************************************************************************************************
	ww2KeymapSaveAll
*************************************************************************************************/

int  ww2KeymapSaveAll()
{
	int i, j;
	int err = IMAP_ERR;

	for(i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
	{
		// category
		if(ww2KeymapIsDirty(i, -1))
			err = ww2KeymapSave(i, -1);

		// vehicles
		for(j = 0; j < MAX_VEHICLES; j++)
		{
			imapKeymapRef ref = ww2Keymap(i, j, 0);
			if(ref == nullptr) break;

			if(ww2KeymapIsDirty(i,j))
				err = ww2KeymapSaveRef(ref, i, j);
		}
	}

	return err;
}

/************************************************************************************************
	ww2KeymapVehicleName
*************************************************************************************************/

const wchar_t* ww2KeymapVehicleName(int category, int vehicleIdx, int maxlen)
{
	int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);

	if(idx > 0)
	{
		// vehicle names are not currently translated, but need to be;
		// this will be addressed at a later date - Ramp
		const vehType* type = vehtypeGetByIndex((UINT32)idx);

		if(type)
		{
			// as long as we found a type, its not an error
			return vehTypeLoc(3, type);
		}
	}

	return nullptr;
}


/************************************************************************************************
	ww2KeymapSetMouseSensitivity
*************************************************************************************************/

void  ww2KeymapSetMouseSensitivity(float scale)
{
	if(axisCalib)
	{
		axisCalib->mouseX = scale;
		axisCalib->mouseY = scale;
	}
}

/************************************************************************************************
	ww2KeymapSetMouseAimSensitivity
*************************************************************************************************/

void  ww2KeymapSetMouseAimSensitivity(float scale)
{
	if(axisCalib)
	{
		axisCalib->aimMouseX = scale;
		axisCalib->aimMouseY = scale;
	}
}

/************************************************************************************************
	ww2KeymapGetMouseSensitivity
*************************************************************************************************/

float ww2KeymapGetMouseSensitivity()
{
	return axisCalib ? axisCalib->mouseX : DEFAULT_MOUSE_SENS;
}

/************************************************************************************************
	ww2KeymapGetMouseAimSensitivity
*************************************************************************************************/

float ww2KeymapGetMouseAimSensitivity()
{
	return axisCalib ? axisCalib->aimMouseX : DEFAULT_AIM_SENS;
}

/************************************************************************************************
	ww2imapSetJoysickCalibration
*************************************************************************************************/

void ww2KeymapSetJoysickCalibration(unsigned int iac, float  deadband, float  fullband, float  x, float  y)
{
	imap_joy_calib* calib = ww2KeymapFindCalibration(axisCalib, iac);

	if(calib)
	{
		calib->deadband = deadband;
		calib->fullband = fullband;
		calib->x = x;
		calib->y = y;
	}
}

/************************************************************************************************
	ww2KeymapGetJoysickCalibration
*************************************************************************************************/

void ww2KeymapGetJoysickCalibration(unsigned int iac, float* deadband, float* fullband, float* x, float* y)
{
	imap_joy_calib* calib = ww2KeymapFindCalibration(axisCalib, iac);

	if(calib)
	{
		*deadband = calib->deadband;
		*fullband = calib->fullband;
		*x = calib->x;
		*y = calib->y;
	}else
	{
		*deadband = *x = *y = *fullband = 0;
	}
}

/************************************************************************************************
	ww2Keymap
*************************************************************************************************/

void ww2KeymapResetCalibration()
{
	if(axisCalib)
	{
		imap_joy_calib* next, *joyCalib = axisCalib->joystick;

		while(joyCalib)
		{
			next = joyCalib->next;
			memFree(joyCalib);
			joyCalib = next;
		}

		axisCalib->mouseX = DEFAULT_MOUSE_SENS;
		axisCalib->mouseY = DEFAULT_MOUSE_SENS;
		axisCalib->joystick = nullptr;
	}
}

/************************************************************************************************
	ww2Keymap
*************************************************************************************************/

imapKeymapRef ww2Keymap(int category, int vehicleIdx, int archive)
{
	imapKeymapRef* ref = nullptr;
	int idx = 0;

	if(category < 0 || category >= WW2_FUNC_NUM_CATEGORIES) return nullptr;

	if(vehicleIdx < 0)
	{
		if(archive)
		{
			ref = &archiveKeymap[category];
		}else
		{
			ref = &categoryKeymap[category];
		}
	}else
	{
		idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);
		if(idx > -1)
		{
			if(archive)
				ref = &vehicleArchiveKeymap[idx];
			else
				ref = &vehicleKeymap[idx];
		}
	}

	if(ref && *ref == nullptr && idx > -1)
		*ref = imapKeymapAlloc();

	return (ref) ? *ref : nullptr;
}

/************************************************************************************************
	ww2KeymapFileName
*************************************************************************************************/

static void ww2KeymapFileName(int category, int vehicleIdx, int archive, char* path, int maxlen, int defaultcfml)
{
	const char* dir = (archive) ? "cfml": USER_CFML_DIR;

	path[0] = '\0';

	if(vehicleIdx < 0)
	{
		if(archive)
		{
			imap_presets& presets = categoryPresets[category];
			snprintf(path, 255, "%s\\%s", dir, presets.at(presets.selected).path.c_str());
		}
		else
		{
			snprintf(path, 255, "%s\\%s.cfml", dir, categoryName[category]);
		}
	}
	else
	{
		int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);

		if(idx > -1)
		{
			snprintf(path, 255, "%s\\%s.cfml", dir, vehtypeGetByIndex((UINT32)idx)->Shape);
		}
	}
}

/************************************************************************************************
	ww2KeymapvehtypeListIdx
*************************************************************************************************/

static int ww2KeymapvehtypeListIdx(int category, int vehicleIdx)
{
	int i, idx, type;
	vehType const* vehtype;

	type = categoryVehicleType[category];

	if(vehicleIdx >=  (int)vehtypeCount() || type == -1 || vehicleIdx < 0) return -1;

	for(i = 0, idx = 0; i < (int)vehtypeCount(); i++)
	{
		vehtype = vehtypeGetByIndex((UINT32)i);

		if(!vehtype->OnlineOnly)
		{
			// subclass ground vehicles
			if(type == VEH_TYPE_GROUND && vehtype->Type == VEH_TYPE_GROUND)
			{
				if(weaponClass[vehtypeWeaponSpec(i).wClass].name == categoryClassName[category])
					idx++;
			}
			else if(vehtype->Type == (UINT32)type)
			{
				idx++;
			}
		}

		if(idx > vehicleIdx)
			return i;
	}

	return -1;
}

/************************************************************************************************
	ww2KeymapSetDirty
*************************************************************************************************/

static void ww2KeymapSetDirty(int category, int vehicleIdx, int dirty)
{
	if(vehicleIdx < 0)
	{
		categoryKeymapDirty[category] = dirty;
	}else
	{
		int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);
		if(idx > -1)
			vehicleKeymapDirty[idx] = dirty;
	}
}

/************************************************************************************************
	ww2KeymapLoadVehicleRef
*************************************************************************************************/

static int  ww2KeymapLoadVehicleRef(imapKeymapRef ref, int vehtypeListIdx, int archive)
{
	char path[256];
	int fd;
	int err = IMAP_ERR;
	const char* dir = archive ? "cfml" : USER_CFML_DIR;

	if(vehtypeListIdx > -1 && vehtypeListIdx < (int)vehtypeCount())
	{
		snprintf(path, 255, "%s\\%s.cfml", dir,vehtypeGetByIndex((UINT32)vehtypeListIdx)->Shape);
	}

	fd = archive ? fioOpen(path) : fiocfgOpen(path, true);
	if(fd > -1)
	{
		err = imapKeymapLoad(ref, &fd);
		fioClose(fd);
	}


	return err;
}

/************************************************************************************************
	ww2KeymapLoadRef
*************************************************************************************************/

static int ww2KeymapLoadRef(imapKeymapRef ref, int category, int vehicleIdx, int archive, int defaultcfml)
{
	char path[256];
	int fd;
	int err = IMAP_ERR;

	ww2KeymapFileName(category, vehicleIdx, archive, path, 255, defaultcfml);

	fd = archive ? fioOpen(path) : fiocfgOpen(path, true);

	if(fd > -1)
	{
		if(!archive)
		{
			if(vehicleIdx > -1)
			{
				int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);
				if(idx > -1)
					vehicleUserKeymap[idx] = 1;
			}else
			{
				categoryUserKeymap[category] = 1;
			}
		}

		err = imapKeymapLoad(ref, &fd);
		fioClose(fd);
	}else if(!archive)
	{
		if(vehicleIdx > -1)
		{
			int idx = ww2KeymapvehtypeListIdx(category, vehicleIdx);
			if(idx > -1)
				vehicleUserKeymap[idx] = 0;
		}else
		{
			categoryUserKeymap[category] = 0;
		}
	}

	return err;
}

/************************************************************************************************
	ww2KeymapSaveRef
*************************************************************************************************/

static int ww2KeymapSaveRef(imapKeymapRef ref, int category, int vehicleIdx)
{
	char path[256];
	int fd;
	int err = IMAP_ERR;

	ww2KeymapFileName(category, vehicleIdx, 0, path, 255, _defaultcfml);

	fd = fiocfgOpenWrite(path, true);
	if(fd > -1)
	{
		err = imapKeymapSave(ref, &fd);
		fioClose(fd);
	}

	return err;
}

/************************************************************************************************
	ww2KeymapLoadAxisCalibration
*************************************************************************************************/

static imap_axis_calib* ww2KeymapLoadAxisCalibration(const char* filename)
{
	imap_axis_calib* calib = (imap_axis_calib*)memAlloc(sizeof(imap_axis_calib));
	int fd;

	if(calib)
	{
		calib->mouseX = calib->mouseY = DEFAULT_MOUSE_SENS;			// Default regular mouse sensitivity
		calib->aimMouseX = calib->aimMouseY = DEFAULT_AIM_SENS;		// Default aiming mouse sensitivity

		fd = fiocfgOpen(filename, true);
		if(fd > -1)
		{
			char line[256];
			char* s;
			unsigned int iac;
			imap_joy_calib* joyCalib;

			for(;;)
			{
				if(!fioGetString(fd, line, 255)) break;

				if(line[0] == '#') continue;

				 // iac
				s = strtok(line, ","); if(s == nullptr) break;
				iac =  strtoul(s, nullptr, 0);

				// deadband or mouse sensitivity
				s = strtok(nullptr, ","); if(s == nullptr) break;
				if(iac == 0) // all mouse axes
				{
					calib->mouseX = calib->mouseY = (float)atof(s); continue;
				}else if(iac == IAC_MOUSEX)
				{
					calib->mouseX = (float)atof(s);
					s = strtok(nullptr, ",");
					if(s)
						calib->aimMouseX = calib->aimMouseY = (float)atof(s);
					continue;
				}else if(iac == IAC_MOUSEY)
				{
					calib->mouseY = (float)atof(s);
					s = strtok(nullptr, ",");
					if(s)
						calib->aimMouseY = calib->aimMouseY = (float)atof(s);
					continue;
				}

				/*
				s = strtok(nullptr, ",");

				if(s) // Separate scalar for aiming
				{
					if(iac == 0) // all mouse axes
					{
						calib->aimMouseX = calib->aimMouseY = (float)atof(s); continue;
					}else if(iac == IAC_MOUSEX)
					{
						calib->aimMouseX = (float)atof(s); continue;
					}else if(iac == IAC_MOUSEY)
					{
						calib->aimMouseY = (float)atof(s); continue;
					}
				}
				else
				{
					continue;
				}
				*/


				// deadband
				joyCalib = ww2KeymapFindCalibration(calib, iac);
				if(joyCalib == nullptr) break;
				joyCalib->deadband = (float)atof(s);

				// x
				s = strtok(nullptr, ","); if(s == nullptr) break;
				joyCalib->x = (float)atof(s);

				// y
				s = strtok(nullptr, ","); if(s == nullptr) break;
				joyCalib->y = (float)atof(s);

				// fullband
				s = strtok(nullptr, ","); if(s == nullptr) break;
				joyCalib->fullband = (float)atof(s);
			}
			fioClose(fd);
			fd = -1;
		}

		if(fd >= 0)
			fioClose(fd); // make sure we got closed
	}

	return calib;
}

/************************************************************************************************
	ww2KeymapFreeAxisCalibration
*************************************************************************************************/

static void ww2KeymapFreeAxisCalibration(imap_axis_calib* calib)
{
	if(calib)
	{
		imap_joy_calib* next, *joyCalib = calib->joystick;

		while(joyCalib)
		{
			next = joyCalib->next;
			memFree(joyCalib);
			joyCalib = next;
		}

		memFree(calib);
	}
}

/************************************************************************************************
	ww2KeymapFindCalibration
*************************************************************************************************/

static imap_joy_calib* ww2KeymapFindCalibration(imap_axis_calib* calib, unsigned int iac)
{
	if(calib)
	{
		imap_joy_calib* joyCalib = calib->joystick;

		while(joyCalib)
		{
			if(joyCalib->iac == iac) return joyCalib;
			if(joyCalib->next == nullptr) break;

			joyCalib = joyCalib->next;
		}

		joyCalib = (imap_joy_calib*)memAlloc(sizeof(imap_joy_calib));
		if(joyCalib)
		{
			joyCalib->iac = iac;
			joyCalib->deadband = 0.0f;
			joyCalib->fullband = 1.0f;
			joyCalib->x = 0.5f;
			joyCalib->y = 0.5f;
			joyCalib->next = calib->joystick;

			calib->joystick = joyCalib;
		}

		return joyCalib;
	}

	return nullptr;
}

/************************************************************************************************
	ww2KeymapSaveAxisCalibration
*************************************************************************************************/

static void ww2KeymapSaveAxisCalibration(imap_axis_calib* calib, const char* filename)
{
	int fd, len;
	char line[256];
	imap_joy_calib* joyCalib = calib->joystick;

	fd = fiocfgOpenWrite(filename, true);
	if(fd > -1)
	{
//		if((calib->mouseX != DEFAULT_MOUSE_SENS) || (calib->aimMouseX != DEFAULT_AIM_SENS)) doesnt work correctly
		{
			len = snprintf(line, 255, "0x%X,%f,%f\n", (unsigned int)IAC_MOUSEX, calib->mouseX, calib->aimMouseX);
			fioWrite(fd, line, len);
		}

//		if((calib->mouseY != DEFAULT_MOUSE_SENS) || (calib->aimMouseY != DEFAULT_AIM_SENS)) doesnt work correctly
		{
			len = snprintf(line, 255, "0x%X,%f,%f\n", (unsigned int)IAC_MOUSEY, calib->mouseY, calib->aimMouseY);
			fioWrite(fd, line, len);
		}

		while(joyCalib)
		{
			if(joyCalib->deadband != 0.0f || joyCalib->x != 0.5f || joyCalib->y != 0.5f || joyCalib->fullband != 1.0f)
			{
				len = snprintf(line, 255, "0x%X,%f,%f,%f,%f\n", joyCalib->iac,joyCalib->deadband, joyCalib->x,
																joyCalib->y, joyCalib->fullband);
				fioWrite(fd, line, len);
			}

			joyCalib = joyCalib->next;
		}

		fioClose(fd);
	}
}

/************************************************************************************************
	ww2imapSetAxisCalibration
*************************************************************************************************/

static void ww2imapSetAxisCalibration(imap_axis_calib* calib)
{
	imap_joy_calib* joyCalib = calib->joystick;

	imapClearAxisCalibration();

	imapCalibrateAxis(IAC_MOUSEX, IMAP_CALIBRATE_LINEAR, 1, &calib->mouseX);
	imapCalibrateAxis(IAC_MOUSEY, IMAP_CALIBRATE_LINEAR, 1, &calib->mouseY);

	// Save aiming sensitivity scalar in prefs
	aimMouse = calib->aimMouseX = calib->aimMouseY; // These should be the same value anyway.

	while(joyCalib)
	{
		imapCalibrateAxis(joyCalib->iac, IMAP_CALIBRATE_BEZIER, 4, &joyCalib->deadband);
		joyCalib = joyCalib->next;
	}
}

/************************************************************************************************
	iac_localized_string
*************************************************************************************************/

void iac_localized_string(const char* language, int locid, char* str, int maxlen)
{
	if(language && stricmp(language, "english") == 0)
		strncpy(str, uiInputString(IDE_LANG_IAC, locid).c_str(), maxlen);
	else
		strncpy(str, uiLocString2(IDE_LANG_IAC, locid).c_str(), maxlen);

	//strncpy(str, (const char*)loclLangGetString(IDE_LANG_IAC, locid), maxlen);

	/*
	const char* s;
	int langID;

	langID = lookup_lang_id(language);

	if(langID != curIacLang)
	{
		curIacLang = langID;
		if(iacLangTable) loclLangTableFree(iacLangTable);
		iacLangTable = loclLangTableLoad((ENUM_LANGUAGES)curIacLang, IDE_LANG_IAC);
	}

	if(iacLangTable)
	{
		s = (char*)loclGetLangTableString(iacLangTable, locid);
		if(s)
			strncpy(str, s, maxlen);
		else
			str[0] = '\0';
	}
	*/
}

/************************************************************************************************
	ww2func_localized_string
*************************************************************************************************/

void ww2func_localized_string(const char* language, int locid, char* str, int maxlen)
{
	if(language && !stricmp(language, "english"))
		strncpy(str, uiInputString(IDE_LANG_FUNCTION, locid).c_str(), maxlen);
	else
		strncpy(str, uiLocString2(IDE_LANG_FUNCTION, locid).c_str(), maxlen);

	//strncpy(str, (const char*)loclLangGetString(IDE_LANG_FUNCTION, locid), maxlen);

	/*
	const char* s;
	int langID;

	langID = lookup_lang_id(language);

	if(langID != curFuncLang)
	{
		curFuncLang = langID;
		if(funcLangTable) loclLangTableFree(funcLangTable);
		funcLangTable = loclLangTableLoad((ENUM_LANGUAGES)curFuncLang, IDE_LANG_FUNCTION);
	}

	if(funcLangTable)
	{
		s = (char*)loclGetLangTableString(funcLangTable, locid);
		if(s)
			strncpy(str, s, maxlen);
		else
			str[0] = '\0';
	}
	*/
}

const std::wstring& ww2imapGetLoc(int locid)
{
	return uiLocString(IDE_LANG_FUNCTION, locid);
}

/************************************************************************************************
	lookup_lang_id
*************************************************************************************************/
#if(0)
static int lookup_lang_id(const char* language)
{
	int i;

	if(!language) return gLanguage;

	for(i = 0; i < NUMBER_OF_LANGUAGES; i++)
	{
		if(!stricmp(g_szLanguages[i], language))
			return i;
	}

	return IDE_LANGUAGE_ENGLISH;
}
#endif

/************************************************************************************************
	imap_read
*************************************************************************************************/

static int imap_read(void* userdata, int numbytes, void* buf)
{
	return (int)fioRead(*(int*)userdata, buf, numbytes);
}

/************************************************************************************************
	imap_write
*************************************************************************************************/

static int imap_write(void* userdata, int numbytes, const void* buf)
{
	return (int)fioWrite(*(int*)userdata, (void*)buf, numbytes);
}

/************************************************************************************************
	ww2imapCenterAxis
*************************************************************************************************/

static unsigned int ww2imapCenterAxis (int param, unsigned int functionCode, unsigned int unused)
{
	imapCenterAxis(IAC_MOUSEX);
	imapCenterAxis(IAC_MOUSEY);
	imapCenterAxis(IAC_MOUSEZ);

	return 0;
}

static void get_turret(imapKeymapRef ref, UINT32* turretX, UINT32* turretY)
{
	imap_keymap keymap;

	if(imapKeymapGet(ref, IMAP_ASX_TURRET_TRAVERSE, 0, IMAP_AXIS_KEYMAP,&keymap) == IMAP_NO_ERR)
		*turretX = keymap.axis.iac;

	if(imapKeymapGet(ref, IMAP_ASX_TURRET_ELEVATE, 0, IMAP_AXIS_KEYMAP,&keymap) == IMAP_NO_ERR)
		*turretY = keymap.axis.iac;
}

static void get_mouselook(imapKeymapRef ref, UINT32* mouseLookX, UINT32* mouseLookY)
{
	imap_keymap keymap;

	if(imapKeymapGet(ref, IMAP_MOUSE_LOOK_X, 0, IMAP_AXIS_KEYMAP,&keymap) == IMAP_NO_ERR)
		*mouseLookX = keymap.axis.iac;

	if(imapKeymapGet(ref, IMAP_MOUSE_LOOK_Y, 0, IMAP_AXIS_KEYMAP,&keymap) == IMAP_NO_ERR)
		*mouseLookY = keymap.axis.iac;
}

static void get_torso(imapKeymapRef ref, UINT32* torsoX, UINT32* torsoY)
{
	imap_keymap keymap;

	if(imapKeymapGet(ref, IMAP_ASX_FP_TORSO_HORIZONTAL_AXIS, 0, IMAP_AXIS_KEYMAP,&keymap) == IMAP_NO_ERR)
		*torsoX = keymap.axis.iac;

	if(imapKeymapGet(ref, IMAP_ASX_FP_TORSO_VERTICAL_AXIS, 0, IMAP_AXIS_KEYMAP,&keymap) == IMAP_NO_ERR)
		*torsoY = keymap.axis.iac;
}


/************************************************************************************************
	ww2KeymapForIdiotsGetType
*************************************************************************************************/

int ww2KeymapForIdiotsGetType(unsigned int functionCode, int keyidx)
{
	if(!ww2funcIsAxis(functionCode)) return IMAP_BUTTON_KEYMAP;
	if(keyidx == 0) return IMAP_AXIS_KEYMAP;

	unsigned int flags = ww2funcFlags(functionCode);
	if((flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_SEC) || !(flags & WW2_FUNC_KEYMAPFORIDIOTS) || (flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_KP))
		return IMAP_AXIS_REL_KEYMAP;

	return IMAP_AXIS_ABS_KEYMAP;
}

/************************************************************************************************
	ww2KeymapForIdiotsGetIndex
*************************************************************************************************/

int ww2KeymapForIdiotsGetIndex(unsigned int functionCode, int keyidx)
{
	if(!ww2funcIsAxis(functionCode) || keyidx == 0) return keyidx;

	unsigned int flags = ww2funcFlags(functionCode);
	if((flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_SEC) || !(flags & WW2_FUNC_KEYMAPFORIDIOTS))
		return (keyidx == 1) ? 2 : 3;
	else if(flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_REL_KP)
		return (keyidx == 1) ? 0 : 1;

	if(flags & WW2_FUNC_KEYMAPFORIDIOTS_AXIS_ABS_40_60)
		return (keyidx == 1) ? 4 : 6;
	else
		return (keyidx == 1) ? 0 : 10;

	return keyidx;
}

/************************************************************************************************
	ww2KeymapSetDefaultCFML
*************************************************************************************************/

void ww2KeymapSetDefaultCFML(int i)
{
	_defaultcfml = i;
}

int ww2KeymapGetDefaultCFML()
{
	return _defaultcfml;
}

int ww2imapTurretAndMouseLookUseSameKeymap()
{
	return _turretAndMouseLookUseSameKeymap;
}


const char* ww2imapGetFuncKeyname(int cat, int veh, unsigned int func, int key, int type)
{
	static char keyname[128];
	int isaxis = ww2funcIsAxis(func);
	imap_keymap keymap;
	int depth = 0;

	*keyname = 0;
	//key = 0; // override for now

	if(cat < 0)
	{
		const vehType* type = vehtypeGetByIndex((UINT32)veh);
		cat = ww2funcGetBestCat(func, (type) ? type->Type: VEH_TYPE_NONE);
	}

	if(cat >= WW2_FUNC_CATEGORY_GENERAL && cat < WW2_FUNC_NUM_CATEGORIES)
	{
		ImapIndexMap& indexMap = _imapIndexMap[cat];
		int vindex = (veh >= 0 && indexMap.find(veh) != indexMap.end()) ? indexMap[veh]: -1;

		if(isaxis)
		{
			if(type == -1)
				type = IMAP_AXIS_KEYMAP;

			if(ww2KeymapGet(cat, vindex, func, key, type, &keymap, &depth) == 0)
			{
				if(keymap.axis.iac != IAC_NOKEY)
					iacPrettyName(keymap.axis.iac, keyname, 127);
			}
		}
		else
		{
			if(type == -1)
				type = IMAP_BUTTON_KEYMAP;

			if(ww2KeymapGet(cat, vindex, func, key, type, &keymap, &depth) == 0)
			{
				if(keymap.btn.iac[0] != IAC_NOKEY)
				{
					char iacname[128];
					*iacname = 0;

					for(int i = 0; i < keymap.numKeys; i++)
					{
						if(i != 0) strcat(keyname, ", ");
						iacPrettyName(keymap.btn.iac[i], iacname, 127);
						strcat(keyname,iacname);
					}
				}
			}
		}
	}

	return keyname;
}

const vehType* ww2imapGetVehTypeByIndex(int category, int index)
{
	int typeIndex = ww2KeymapvehtypeListIdx(category, index);

	if(typeIndex > -1)
		return vehtypeGetByIndex((UINT32)typeIndex);

	return nullptr;
}

void ww2imapInitIndexMap()
{
	for(int i = WW2_FUNC_CATEGORY_AIR; i < WW2_FUNC_NUM_CATEGORIES; i++)
	{
		ImapIndexMap indexMap;

		for(int j = 0;; j++)
		{
			const vehType* type = ww2imapGetVehTypeByIndex(i, j);
			if(!type) break;

			indexMap[type->VehicleTypeNum] = j;
			indexMap[-j] = type->VehicleTypeNum;
		}

		_imapIndexMap[i] = indexMap;
	}
}

void imapLoadPresets()
{
	// make sure we start from scratch in case of reloads
	for(int i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
		categoryPresets[i].clear();

	ImapPresetLoader loader(IMAP_PRESETS);
	loader.Load();

	char cfgKey[32];

	for(int i = 0; i < WW2_FUNC_NUM_CATEGORIES; i++)
	{
		imap_presets& presets = categoryPresets[i];

		snprintf(cfgKey, 31, "input.presets.%d", i);
		int selected = cfgPrefsGetInt(cfgKey, -1);
		int best = -1;

		// get current preset; might need sanity check on selected here
		// in the case where the xml config file has a bad value
		for(int preset = 0; preset < (int)presets.size(); preset++)
		{
			bool ok = false;

			switch(presets.at(preset).requires)
			{
				case PRESET_REQ_JOYSTICK:
					ok = (dipHasJoystick());
				break;
				case PRESET_REQ_MOUSE:
					ok = (dipHasMouse());
				break;
				case PRESET_REQ_KEYBOARD:
					ok = (dipHasKeyboard());
				break;
				default:
					ok = true; // no requirement, we're ok
				break;
			}

			if(preset == selected)
			{
				best = selected;
				break;
			}
			else if(ok && best == -1)
			{
				best = preset;
			}
		}

		presets.select(best);
	}
}
