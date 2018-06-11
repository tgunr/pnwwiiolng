#pragma once

#include "cfg/cfgCodes.h"

/*************************************************************************************************
	cfgPrefs
*************************************************************************************************/

// used when saving/restoring a view's properties
#define SAVE_VIEW_VALUE			0x00000001
#define SAVE_VIEW_POSITION		0x00000002
#define SAVE_VIEW_SIZE			0x00000004
#define SAVE_VIEW_VISIBILITY	0x00000008
#define SAVE_VIEW_METRICS		SAVE_VIEW_POSITION | SAVE_VIEW_SIZE
#define SAVE_VIEW_ALL			SAVE_VIEW_VALUE | SAVE_VIEW_VISIBILITY | SAVE_VIEW_METRICS

// these defines are used by the old config stuff and
// will be removed at some point
#define CFG_MAP_FILTER_CIVILIAN 	"map.civilian"
#define CFG_MAP_FILTER_ARMY 		"map.army"
#define CFG_MAP_FILTER_AIRFORCE 	"map.airforce"
#define CFG_MAP_FILTER_NAVY			"map.navy"
#define CFG_MAP_FILTER_DEPOTS		"map.deopots"
#define CFG_MAP_FILTER_NAMES		"map.names"
#define CFG_MISC_AUTORELOAD			"misc.autoreload"
#define CFG_APP_LOADTIME			"app.loadtime" // long
#define CFG_CFML_DEFAULT			"cfml.default" // long

// New preferences saving mechanism for the game

#define PREF_FILE_NAME "wwiiol.xml"

#ifdef BETA_RELEASE
#undef PREF_FILE_NAME
#define PREF_FILE_NAME "wwiiol_beta.xml"
#endif

bool cfgPrefsLoad(); // loads prefs from disk
void cfgPrefsSave(); // tolua_export // saves all prefs to disk
void cfgPrefsKill(); // kills the global prefs object
void cfgPrefsInit(); // called very late in the load process to post process globals if needed
void cfgPrefsPostInit(void); // Called after we've decided to go online/offline
void cfgPrefsSyncGlobals(); // syncs xml config to globals
void cfgPrefsSyncPrefs(); // syncs globals to xml config
bool cfgPrefsRestartNeeded(); // returns true if any restart vars are changed
void cfgPrefsSendSharedPrefs(); // sends a set of shared prefs to the host
void cfgPrefsSendSharedPref(const char* key, int defaultValue = 0); // sends a shared pref to the host

// global preferences; use these calls to manipulate globals whenever possible
bool cfgPrefsSetGlobal(_cfgPARAMS key, void* value, bool update = true);	// sets a global value
bool cfgPrefsGetGlobal(_cfgPARAMS key, void* value, bool config = true);	// gets a global value

// global details
const char* cfgPrefsGetGlobalName(_cfgPARAMS key); // gets a global's name
cfgPARAM_DETAILS* cfgPrefsGetGlobalDetails(_cfgPARAMS key); // gets a global's details
cfgPARAM_DETAILS* cfgPrefsGetGlobalDetails(const char* name); // gets a global's details
int cfgPrefsGetGlobalKey(const char* name); // returns the _cfgPARAMS key for a named global

// set arbitrary preferences
bool cfgPrefsSetBool(const char* key, bool value); // sets a bool
bool cfgPrefsSetInt(const char* key, int value); // sets a int
bool cfgPrefsSetLong(const char* key, long value); // sets a long
bool cfgPrefsSetFloat(const char* key, float value); // sets a float
bool cfgPrefsSetDouble(const char* key, double value); // sets a double
bool cfgPrefsSetString(const char* key, const char* value); // sets a string

bool cfgPrefsRemove(const char* key = nullptr); // delete a pref; deletes all keys if one wasn't specified

// get arbitrary preferences by reference
bool cfgPrefsGetBool(const char* key, bool* value); // gets a bool
bool cfgPrefsGetInt(const char* key, int* value); // gets a int
bool cfgPrefsGetLong(const char* key, long* value); // gets a long
bool cfgPrefsGetFloat(const char* key, float* value); // gets a float
bool cfgPrefsGetDouble(const char* key, double* value); // gets a double
bool cfgPrefsGetString(const char* key, const char** value); // gets a string

// get arbitrary preferences by value
bool cfgPrefsGetBool(const char* key, bool defaultValue); // gets a bool
int cfgPrefsGetInt(const char* key, int defaultValue); // gets a int
long cfgPrefsGetLong(const char* key, long defaultValue); // gets a long
float cfgPrefsGetFloat(const char* key, float defaultValue); // gets a float
double cfgPrefsGetDouble(const char* key, double defaultValue); // gets a double
const char* cfgPrefsGetString(const char* key, const char* defaultValue); // gets a string

// save/restore view properties; what determines what property(s) to save/restore
// see SAVE_VIEW_* defines above
namespace phui { class View; class Message; }

void cfgPrefsRestoreToView(phui::View* v, int what = SAVE_VIEW_VALUE);
void cfgPrefsSaveFromView(phui::View* v, int what = SAVE_VIEW_VALUE);

extern phui::Message* gAppPrefs;
