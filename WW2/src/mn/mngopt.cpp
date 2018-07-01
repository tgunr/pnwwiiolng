#include "syscfg.h" // Path is project specific.

#include <ctype.h>
#include <string.h>
#include "pi/pi.h"
#include "include/vars.h"
#include "mn/mngopt.h"
#if platOS == OS_MACOS
#include "sysmac/sysMacCompat.h"
#endif

enum key_TYPE
{
	ENULL,
	ELOGICAL,
	EINT,
	EUINT,
	ELONG,
	EDWORD,
	EFLOAT,
	EDOUBLE,
	ESTRING,
	EWSTRING
};

struct GVAR
{
	const char* const	key;
	key_TYPE			ktype;
    union {
        void *basicValue;
        LOGICAL *logicalValue;
        int *intValue;
        unsigned int *uintValue;
        long *longValue;
        unsigned long *dwordValue;
        float *floatValue;
        double *doubleValue;
        char *stringValue;
        wchar_t *wstringValue;
    } valuePtr;
    size_t maxLength;
};

fullpath_t DataPath[MAX_PATH + 1];  //  Used within mngopt.cpp.  MJH - April 8, 2018.

template <typename T, size_t N>
static constexpr size_t sizeof_a(T (&)[N]) { return N; }

#define NULL_KEY_DEF(cmd, var) { cmd, ENULL, { &var }, 0 }
#define LOGICAL_KEY_DEF(cmd, var) { cmd, ELOGICAL, { &var }, 0 }
#define INT_KEY_DEF(cmd, var) { cmd, EINT, { &var }, 0 }
#define UINT_KEY_DEF(cmd, var) { cmd, EUINT, { &var }, 0 }
#define LONG_KEY_DEF(cmd, var) { cmd, ELONG, { &var }, 0 }
#define DWORD_KEY_DEF(cmd, var) { cmd, EDWORD, { &var }, 0 }
#define FLOAT_KEY_DEF(cmd, var) { cmd, EFLOAT, { &var }, 0 }
#define DOUBLE_KEY_DEF(cmd, var) { cmd, EDOUBLE, { &var }, 0 }
#define STRING_KEY_DEF(cmd, var) { cmd, ESTRING, { reinterpret_cast<char *>(&var[0]) }, sizeof_a(var) }
#define WSTRING_KEY_DEF(cmd, var) { cmd, EWSTRING, { reinterpret_cast<wchar_t *>(&var[0]) }, array_size(var) }

//  lowercase ONLY!
static GVAR vars[] = {
    INT_KEY_DEF("width", WantedWidth),
    INT_KEY_DEF("height", WantedHeight),
    INT_KEY_DEF("bpp", WantedBpp),
    INT_KEY_DEF("alpha", WantedAlpha),
    LOGICAL_KEY_DEF("hdw", WantedHdw),
    { "stencil", ELOGICAL, { &WantedStencil }, 0 }, // HACK
    LOGICAL_KEY_DEF("fullscreen", WantedFullscreen),
    LOGICAL_KEY_DEF("fullscreen", WantedBorderless),
    LOGICAL_KEY_DEF("window", WantedWindow),
    { "staccato", ELOGICAL, { &UseStaccato }, 0 }, // HACK
    STRING_KEY_DEF("ticket", TicketFile),
    STRING_KEY_DEF("bypass", BypassString),
    STRING_KEY_DEF("serverip", GameServerIP),
    STRING_KEY_DEF("serverport", GameServerPort),
    STRING_KEY_DEF("serverotp", GameServerOTP),
    STRING_KEY_DEF("apid", ServerApID),
    STRING_KEY_DEF("serverprotocol", ServerProtocol),
    NULL_KEY_DEF("steamlogin", UseSteamLogin),
    NULL_KEY_DEF("settings", LaunchSettingsOnly),
    LOGICAL_KEY_DEF("log", WantedUserLogEnabled),
    INT_KEY_DEF("side", WantedSide),
    LOGICAL_KEY_DEF("data", WantDataDir),
    NULL_KEY_DEF("nowor", NoBlitzWorPreload),
    NULL_KEY_DEF("noinf", NoInfPreload),
    NULL_KEY_DEF("build", g_bBuildMDLS),
    NULL_KEY_DEF("logcomm", g_log_comm),
    NULL_KEY_DEF("logfps", g_log_fps),
    NULL_KEY_DEF("logweap", g_log_weap),
    STRING_KEY_DEF("screen", WantedDisplayDevice),
    NULL_KEY_DEF("nospawn", OverrideSpawner),
    NULL_KEY_DEF("console", OpenConsole),
    LOGICAL_KEY_DEF("ragdolls", gRagdolls),
    NULL_KEY_DEF("buildterrain", g_buildTerrain),
    STRING_KEY_DEF("o", HackID),
    NULL_KEY_DEF("altfont", AltFont),
    STRING_KEY_DEF("authserver", gAuthServer),
    STRING_KEY_DEF("authserverport", gAuthServerPort),
    STRING_KEY_DEF("arena", gArena),
    STRING_KEY_DEF("clientv", gClientV),
#if platOS == OS_WIN32
    WSTRING_KEY_DEF("datadir", DataPath),
#else
    STRING_KEY_DEF("datadir", DataPath),
#endif
#ifdef DEBUG_WW2
    NULL_KEY_DEF("buildmdlonly", DebugBuildingMDLFilesOnly),
#endif
    { nullptr, ENULL, { nullptr }, 0 },
};

static char* _skipFilename(const char* ptr)
{
	if ( *ptr == '"' )
	{
		while ( *ptr == '"' )
			++ptr;
		ptr = strchr(ptr, '"');
		while ( ptr && *ptr == '"' )
			++ptr;
	}
	else
		ptr = strchr(ptr, ' ');

	if ( ptr == nullptr )
		return nullptr;

	while ( *ptr == ' ' )
		++ptr;

	return (char*)ptr;
}

LOGICAL mngoptParse()
{
	if (DataPath[0] == 0)
        Wstring2Char(WW2_exeDataPath, DataPath, sizeof(DataPath));
	
	if (WantedDataPath == L"" && WantDataDir == TRUE)
        Char2Wstring(DataPath, WantedDataPath, _MAX_PATH);

	char*	ptr;

	ptr = _skipFilename(GetCommandLineA());

	if ( ptr == nullptr )
		return TRUE;

    std::vector<std::string> args = split_string(ptr, " ");

    for (size_t i = 0, n = args.size(); i < n; i += 2) {
        std::string cmd = strtrim(strtolower(args[i]));
        std::string value = strtrim(strtolower(i < (n - 1) ? args[i + 1] : ""));

        if (!has_prefix(cmd, "-")) {
            break;
        }
        auto foundCmd = std::find_if(&vars[0], &vars[sizeof(vars) / sizeof(GVAR)], [c = cmd.substr(1)] (const GVAR &k) { return k.key && k.key == c; });

        if (foundCmd == &vars[sizeof(vars) / sizeof(GVAR)]) {
			Write_Debug("Unknown Command Line switch:\n '%s', '%s'\n", cmd.c_str(), value.c_str());
            continue;
        }
        switch (foundCmd->ktype) {
            case ENULL:
                *(foundCmd->valuePtr.logicalValue) = true;
                --i;
                break;
            case ELOGICAL:
                if (value == "0" || value == "false") {
                    *(foundCmd->valuePtr.logicalValue) = false;
                } else if (value == "1" || value == "true") {
                    *(foundCmd->valuePtr.logicalValue) = true;
                } else {
                    Write_Debug("Invalid boolean switch '%s', '%s'", cmd.c_str(), value.c_str());
                }
                break;
            case EINT:
                *(foundCmd->valuePtr.intValue) = strtol(value.c_str(), nullptr, 0);
                break;
            case EUINT:
                *(foundCmd->valuePtr.uintValue) = strtoul(value.c_str(), nullptr, 0);
                break;
            case ELONG:
                *(foundCmd->valuePtr.longValue) = strtol(value.c_str(), nullptr, 0);
                break;
            case EDWORD:
                *(foundCmd->valuePtr.dwordValue) = strtoul(value.c_str(), nullptr, 0);
                break;
            case EFLOAT:
                *(foundCmd->valuePtr.floatValue) = strtof(value.c_str(), nullptr);
                break;
            case EDOUBLE:
                *(foundCmd->valuePtr.doubleValue) = strtod(value.c_str(), nullptr);
                break;
            case ESTRING:
                strlcpy(foundCmd->valuePtr.stringValue, value.c_str(), foundCmd->maxLength);
                break;
            case EWSTRING:
                mbstowcs(foundCmd->valuePtr.wstringValue, value.c_str(), foundCmd->maxLength);
                break;
        }
    }

	return(TRUE);
}
