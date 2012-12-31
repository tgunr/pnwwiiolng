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

#if platOS == OS_MACOS
#undef PREF_FILE_NAME
#define PREF_FILE_NAME "wwiiolprefs.xml"
#ifdef BETA_RELEASE
#undef PREF_FILE_NAME
#define PREF_FILE_NAME "wwiiolbeta.xml"
#endif

#endif

extern bool cfgPrefsLoad(); // loads prefs from disk
extern void cfgPrefsSave(); // tolua_export // saves all prefs to disk
extern void cfgPrefsKill(); // kills the global prefs object
extern void cfgPrefsInit(); // called very late in the load process to post process globals if needed
extern void cfgPrefsSyncGlobals(); // syncs xml config to globals
extern void cfgPrefsSyncPrefs(); // syncs globals to xml config
extern bool cfgPrefsRestartNeeded(); // returns true if any restart vars are changed
extern void cfgPrefsSendSharedPrefs(); // sends a set of shared prefs to the host
extern void cfgPrefsSendSharedPref(const char* key, int defaultValue = 0); // sends a shared pref to the host

// global preferences; use these calls to manipulate globals whenever possible
extern bool cfgPrefsSetGlobal(_cfgPARAMS key, void* value, bool update = true);	// sets a global value
extern bool cfgPrefsGetGlobal(_cfgPARAMS key, void* value, bool config = true);	// gets a global value

// global details
extern const char* cfgPrefsGetGlobalName(_cfgPARAMS key); // gets a global's name
extern cfgPARAM_DETAILS* cfgPrefsGetGlobalDetails(_cfgPARAMS key); // gets a global's details
extern cfgPARAM_DETAILS* cfgPrefsGetGlobalDetails(const char* name); // gets a global's details
extern int cfgPrefsGetGlobalKey(const char* name); // returns the _cfgPARAMS key for a named global

// set arbitrary preferences
extern bool cfgPrefsSetBool(const char* key, bool value); // sets a bool
extern bool cfgPrefsSetInt(const char* key, int value); // sets a int
extern bool cfgPrefsSetLong(const char* key, long value); // sets a long
extern bool cfgPrefsSetFloat(const char* key, float value); // sets a float
extern bool cfgPrefsSetDouble(const char* key, double value); // sets a double
extern bool cfgPrefsSetString(const char* key, const char* value); // sets a string

extern bool cfgPrefsRemove(const char* key = NULL); // delete a pref; deletes all keys if one wasn't specified

// get arbitrary preferences by reference
extern bool cfgPrefsGetBool(const char* key, bool* value); // gets a bool
extern bool cfgPrefsGetInt(const char* key, int* value); // gets a int
extern bool cfgPrefsGetLong(const char* key, long* value); // gets a long
extern bool cfgPrefsGetFloat(const char* key, float* value); // gets a float
extern bool cfgPrefsGetDouble(const char* key, double* value); // gets a double
extern bool cfgPrefsGetString(const char* key, const char** value); // gets a string

// get arbitrary preferences by value
extern bool cfgPrefsGetBool(const char* key, bool defaultValue); // gets a bool
extern int cfgPrefsGetInt(const char* key, int defaultValue); // gets a int
extern long cfgPrefsGetLong(const char* key, long defaultValue); // gets a long
extern float cfgPrefsGetFloat(const char* key, float defaultValue); // gets a float
extern double cfgPrefsGetDouble(const char* key, double defaultValue); // gets a double
extern const char* cfgPrefsGetString(const char* key, const char* defaultValue); // gets a string

// save/restore view properties; what determines what property(s) to save/restore
// see SAVE_VIEW_* defines above
namespace phui { class View ; class Message ; }

extern void cfgPrefsRestoreToView(phui::View* v, int what = SAVE_VIEW_VALUE);
extern void cfgPrefsSaveFromView(phui::View* v, int what = SAVE_VIEW_VALUE);

extern phui::Message* gAppPrefs;
