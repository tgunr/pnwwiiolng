#pragma once

/************************************************************************************************
	ww2imap
*************************************************************************************************/
// World War ][ Online specific imap functions

#include "imap/imapTypes.h"
#include <string>
#include <vector>

struct imap_preset
{
	int category;
	int requires;
	int ez;
	std::string name;
	std::string path;
};

struct imap_presets : public std::vector<imap_preset>
{
	int selected;

	imap_presets() : selected(0)
	{
	}

	void select(int s)
	{
		selected = (s >= 0 && s < (int)size()) ? s: selected;
	}
};

int  ww2imapInit();
void ww2imapKill();

void imapLoadPresets();
bool ww2imapSetPreset(int category, int preset);
bool ww2imapSetEZPreset(int category);
imap_presets* ww2imapGetPresets(int category);

int  ww2imapSetKeymap(int vehListIdx);
int  ww2imapSetKeymapById(const char*);

int  ww2KeymapInit(int defaultcfml); // tolua_export
void ww2KeymapKill(); // tolua_export

int  ww2KeymapSet(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, const imap_keymap* keymap);
int  ww2KeymapCmp(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, const imap_keymap* keymap);
int  ww2KeymapGet(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, int keymapType, imap_keymap* keymap, int* depth);

void ww2KeymapClear (int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, int keymapType);
void ww2KeymapRemove(int category, int vehicleIdx, unsigned int functionCode, unsigned int idx, int keymapType);

int  ww2KeymapHasKeyboardAxis(int category, int vehicleIdx, unsigned int functionCode);

int  ww2KeymapSave     (int category, int vehicleIdx);
int  ww2KeymapRestore  (int category, int vehicleIdx);
void ww2KeymapRemoveAll(int category, int vehicleIdx);
int  ww2KeymapIsDirty  (int category, int vehicleIdx);
int  ww2KeymapHasUserKeymap(int category, int vehicleIdx);

int  ww2KeymapSaveAll();

const wchar_t*  ww2KeymapVehicleName(int category, int vehicleIdx, int maxlen);

void  ww2KeymapSetMouseSensitivity(float scale);
void  ww2KeymapSetMouseAimSensitivity(float scale);
float ww2KeymapGetMouseSensitivity();
float ww2KeymapGetMouseAimSensitivity();

void ww2KeymapSetJoysickCalibration(unsigned int iac, float  deadband, float  fullband, float  x, float  y);
void ww2KeymapGetJoysickCalibration(unsigned int iac, float* deadband, float* fullband, float* x, float* y);

void ww2KeymapResetCalibration();

int ww2KeymapForIdiotsGetType(unsigned int functionCode, int keyidx);
int ww2KeymapForIdiotsGetIndex(unsigned int functionCode, int keyidx);

void ww2KeymapSetDefaultCFML(int);
int ww2KeymapGetDefaultCFML();

void ww2imapAddBuzzardKeymaps();
void ww2imapAddBetaKeymaps();

int ww2imapTurretAndMouseLookUseSameKeymap();

// gets the pretty name for the key associated with this keymap
struct vehType;
const std::wstring& ww2imapGetLoc(int locid);
const char* ww2imapGetFuncKeyname(int cat, int veh, unsigned int func, int key, int type = -1); // tolua_export
const vehType* ww2imapGetVehTypeByIndex(int category, int index);
void ww2imapInitIndexMap();
