#include <syscfg.h> // Path is project specific.
// Implement all the various config values.

#include "mem/mem.h"
#include "str/strdupptr.h"
#include "teul/teulConfigVar.h"

#if platOS == OS_WIN32
#include "Shlobj.h"
#endif

extern bool teulGetHost(const SCHAR* address, UINT32* host);

#ifndef PN_PLAYNET_PATH
#error "PN_PLAYNET_PATH not defined."
#endif

////////////////////////////////////////////////////////////////////////////////
// Helper macros.

#define CFG_STRING(Name_, Default_, EnvName_, Required_, Description_)	\
	ConfigStr cv_##Name_ ( CVT_STRING, #Name_, Default_, EnvName_, Required_, Description_ )

#define CFG_UINT(Name_, Default_, EnvName_, Description_)	\
	ConfigUint cv_##Name_ ( CVT_UNSIGNED_INT, #Name_, Default_, EnvName_, true, Description_ )

#define CFG_SINT(Name_, Default_, EnvName_, Description_)	\
	ConfigSint cv_##Name_ ( CVT_SIGNED_INT, #Name_, Default_, EnvName_, false, Description_ )

#define CFG_BOOL(Name_, Default_, EnvName_, Description_)	\
	ConfigBool cv_##Name_ ( CVT_BOOL, #Name_, Default_, EnvName_, false, Description_ )

#define CFG_ADDR(Name_, Default_, EnvName_, Required_, Description_)	\
	ConfigAddr cv_##Name_ ( CVT_IP_ADDRESS, #Name_, Default_, EnvName_, Required_, Description_ )

#define CFG_PATH(Name_, Default_, EnvName_, Required_, Description_)	\
	ConfigStr cv_##Name_ ( CVT_PATH, #Name_, Default_, EnvName_, Required_, Description_ )

static const char* const s_configVarTypes[NUM_CONFIG_VAR_TYPES] =
{
	"string"
,	"unsigned int"
,	"signed int"
,	"boolean"
,	"ip address"
,	"path"
};

#define PATH_SEPSTR "/"

////////////////////////////////////////////////////////////////////////////////
// Variables.

CFG_PATH(dataDir, PN_PLAYNET_PATH PATH_SEPSTR "wwiiol" PATH_SEPSTR "data", "DATA_DIR", true, "Top level directory for game data.");
CFG_PATH(importFile, "importlist.txt", "IMPORT_FILE", true, "File which will specify sdm files to be used by the server.");
CFG_PATH(terrainSubdir, "terrain", "TERRAIN_SUBDIR", true, "Path from DATA_DIR to the terrain sdm files and origin file.");
CFG_PATH(originFile, "origin", "ORIGIN_FILE", true, "Name of the file which identifies where the sdm files came from.");
CFG_STRING(stratLanguage, "en", "STRAT_LANGUAGE", true, "2-letter language code for server localization.");
CFG_PATH(thblitzPath, "th_blitz" PATH_SEPSTR "weapon", "THBLITZ_PATH", true, "Path from DATA_DIR to the thBlitz csv files.");
CFG_BOOL(stratIgnoreErrors, "0", "STRAT_IGNORE_ERRORS", "Disables sdm-loading errors from causing termination.");
CFG_PATH(blitzworPath, "blitzWor.csv", "BLITZWOR_PATH", true, "DATA_DIR-relative path/filename for the blitzWor csv file");
CFG_PATH(luaPath, PN_PLAYNET_PATH PATH_SEPSTR "wwiiol" PATH_SEPSTR "lua", "LUA_BASE_PATH", true, "Path to the Lua scripts.");
CFG_PATH(defaultLogPath, nullptr, "LOG_PATH", false, "Default location for log files.");
CFG_PATH(teulWebPath, PN_PLAYNET_PATH PATH_SEPSTR "wwiiol" PATH_SEPSTR "web" PATH_SEPSTR "debug", "TEUL_UWEB_PATH", true, "Path to the top-level directory for the internal web server.");
CFG_PATH(luaRCFile, "rc.lua", "LUA_RC_FILE", true, "Name of the startup (rc) Lua script");
CFG_PATH(luaStratlibFile, "stratlib.lua", "LUA_STRATLIB_FILE", true, "Name of the Lua strat library.");

CFG_ADDR(wwiiDBAddress, nullptr, "WWII_DB_ADDRESS", true, "IP address of the MySQL Database.");
CFG_STRING(wwiiDBDefaultSchema, "wwiionline", "WWII_DB_SCHEMA", true, "Default schema for game data.");
CFG_STRING(wwiiDBPassword, "", "WWII_DB_PASSWD", true, "Password to use for the game database.");
CFG_STRING(wwiiDBUser, "wwiiol", "WWII_DB_USER", true, "Username when connecting to the game database.");
CFG_STRING(wwiiDBPort, "3306", "WWII_DB_PORT", true, "Port of the game MySQL server if not default.");
CFG_ADDR(dbsePrimaryAddress, nullptr, "DBSE_PRIMARY_ADDRESS", true, "IP address dbd clients will reach the dbd on.");
CFG_UINT(dbseConnectionTimeout_ms, "3000", "DBSE_CONNECTION_TIMEOUT_MS", "Time in miliseconds before database proxy connections timeout.");
CFG_BOOL(mysqlCompression, "0", "MYSQL_COMPRESSION", "Enable compression when communicating with the MySQL server");

CFG_STRING(clusterName, nullptr, "WWII_CLUSTER", true, "Short-name for this cluster, e.g. 'dev'");
CFG_UINT(arenaID, nullptr, "ARENA_ID", "Numeric ID for this arena.");
CFG_UINT(appID, "1", "APP_ID", "App ID for this server - 1 being release, 12 being beta.");
CFG_ADDR(ircServer, "", "WWII_IRC_SERVER", false, "Address of the IRC server. If not defined, disables IRC.");
CFG_STRING(ircPassword, "", "WWII_IRC_PASSWD", false, "Server-password for the IRC server.");
CFG_UINT(ircPort, "6667", "WWII_IRC_PORT", "Port to connect to the IRC server on.");
CFG_STRING(authErrorUrl, "http://www.wwiionline.com/auth/errors/", "AUTH_ERROR_URL", true, "URL to prefix authentication error URLs with.");
CFG_ADDR(baseAddress, nullptr, "BASE_ADDRESS", true, "Network address (/24) for the cluster (server's will map their last octets (x.x.x.N) into this network for inter-host communications) or IP address of controller host.");

#ifndef PN_DISABLE_AUTH
CFG_ADDR(authHost, "auth.playnet.com", "AUTH_ADDRESS", true, "Host name or IP Address of the authentication server.");
CFG_ADDR(authdbAddress, nullptr, "AUTH_DB", true, "Address of the MySQL server hosting the auth database.");
CFG_STRING(authdbPort, "3306", "AUTH_DB_PORT", true, "Port of the Auth MySQL server if not default.");
CFG_STRING(authdbPassword, "", "AUTH_DB_PASSWD", true, "Password to use for the auth database.");
CFG_STRING(authdbSchema, "auth", "AUTH_DB_SCHEMA", true, "Schema to use for auth data.");
CFG_STRING(authdbUser, "authuser", "AUTH_DB_USER", true, "Username when connecting to the auth database.");
#endif

CFG_UINT(maxClusterUsers, "0", "MM_MAX_USERS", "Maxmimum number of players on the mmhost/cluster");
CFG_UINT(maxCellPlayers, "1250", "MAX_PLAYERS", "Maximum number of players on this cell host.");
CFG_ADDR(baseUpdtAddress, "192.168.11.0", "BASE_UPDT_ADDRESS", true, "Network address to use for private cell network or 0.0.0.0 to disable multiple cells (clustering).");
CFG_UINT(peerConnectionTimeout_ms, "20000", "PEER_CONNECTION_TIMEOUT_MS", "Number of miliseconds before a server-peer connection times out.");
#if defined(WIN32)
CFG_STRING(defaultInterface, NULL, "DEFAULT_IFACE", false, "Name of the network interface to treat as primary.");
#else
CFG_STRING(defaultInterface, "eth0", "DEFAULT_IFACE", true, "Name of the network interface to treat as primary.");
#endif

CFG_STRING(manageProcesses, nullptr, "MANAGE_PROCESSES", false, "Semi-colon separated list of processes this machine should run.");

//////////////////////////////////////////////////////////////////////////
// Helper for creating ensuring a string is in native path style and
// doesn't have trailing slashes.

static char* _makeNativePath(const char* const path_)
{
	ABORT_IF( path_ == nullptr );
	if ( *path_ == 0 )
		return nullptr;

	// Trim trailing slashes off of path names.
	char nativePath[1024];
	size_t len = strprintf(nativePath, "%s", path_);	// Get length in first pass.
	if ( len >= sizeof(nativePath) )
		LogAbort("Invalid path: %s", path_);
	while ( len > 0 && (nativePath[len-1] == '/' || nativePath[len-1] == '\\') )
		nativePath[--len] = 0;
#if defined(WIN32)
	// Perform path naturalization for Windows.
	char* p = nativePath;
	while (*p) {
		if (*p == '/')
			*p = '\\';
		++p;
	}
#endif
	return strdup(nativePath);
}

////////////////////////////////////////////////////////////////////////////////
// Read values from the environment or configuration file.

struct ConfigEntry
{
	ConfigEntry() : m_name(nullptr), m_value(nullptr) {}
	ConfigEntry(const char* name_, const char* nameEnd_, const char* value_, const char* valueEnd_)
		: m_name(name_, static_cast<size_t>(nameEnd_ - name_))
		, m_value(value_, static_cast<size_t>(valueEnd_ - value_))
	{
	}
	StrDupPtr	m_name;
	StrDupPtr	m_value;
	void setValue(const char* value_, const char* valueEnd_)
	{
		m_value.copy(value_, static_cast<size_t>(valueEnd_ - value_));
	}
};

typedef std::vector<ConfigEntry> CONFIG_ENTRIES;
struct ConfigEntries : public CONFIG_ENTRIES
{
	ConfigEntries() : CONFIG_ENTRIES(), m_read(false), m_foundGeneralFile(false), m_foundOSFile(false)
	{
		reserve(64);
	}

	bool m_read;
	bool m_foundGeneralFile;
	bool m_foundOSFile;
};

static ConfigEntries& _getConfigEntries()
{
	static ConfigEntries s_configEntries;
	return s_configEntries;
}

#include "include/io_mapped_file.h"

static void _readConfigEntriesFromFile(IO::MappedFile& file)
{
	ConfigEntries& configEntries = _getConfigEntries();
	configEntries.m_read = true;
	const char*& cur = file.m_currentPtr;
	while ( cur < file.endPtr() )
	{
		// Skip leading whitespace.
		if ( isspace(*cur) )
		{
			++cur;
			continue;
		}
		// Skip comment lines.
		if ( *cur == '#' )
		{
			file.skipline();
			continue;
		}

		const char* equals = strchr(cur, '=');
		if ( equals == nullptr )
		{
			file.skipline();
			continue;
		}
		const char* name = cur;
		const char* value = equals + 1;
		if (*value == '"')
			++value;
		const char* eol = strpbrk(value, "\r\n\"");
		if ( eol != nullptr )
			cur = eol;
		else
			cur = eol = file.endPtr();

		// Trim trailing white space.
		while ( eol > value && isspace(*(eol - 1)) )
			--eol;

		auto it = configEntries.begin();
		for ( auto end = configEntries.end(); it != end; ++it )
		{
			if ( strncasecmp(it->m_name, name, static_cast<size_t>(equals - name)) == 0 )
			{
				it->setValue(value, eol);
				break;
			}
		}
		if ( it == configEntries.end() ) {
			configEntries.emplace_back(ConfigEntry(name, equals, value, eol));
		}
		file.skipline();
	}
}

#if defined(WIN32) && defined(UNICODE)
#define OS_ENVIRONMENT_CFG L"environment.win.cfg"
#define DEF_ENVIRONMENT_CFG L"environment.cfg"
#else
#define OS_ENVIRONMENT_CFG "environment.unix.cfg"
#define DEF_ENVIRONMENT_CFG "environment.cfg"
#define PARENT_DIR ".."
#endif

static const char* GetValueFromEnvironmentOrConfig(const char* name_)
{
	// Environment beats config.
	const char* envValue = getenv(name_);
	if ( envValue != nullptr )
		return envValue;

	ConfigEntries& configEntries = _getConfigEntries();
	if ( configEntries.m_read == false )
	{
#if platOS != OS_WIN32
		const char* paths[] = { "./", PN_PLAYNET_PATH "/etc/", "/etc/", nullptr };
#else
		TCHAR homepath[MAX_PATH];
		SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, homepath);
		const TCHAR* paths[] = { _T(".\\"), _T(PN_PLAYNET_PATH) _T("\\etc"), homepath, nullptr };
#endif

		IO::MappedFile file;
		// Work thru path list to try and find a general file followed by an OS file,
		// once we have found an instance of each file, stop looking for that file.
		for ( size_t i = 0; paths[i] != nullptr; ++i )
		{
			if ( file.mapFile(DEF_ENVIRONMENT_CFG, paths[i]) )
			{
				_readConfigEntriesFromFile(file);
				configEntries.m_foundGeneralFile = true;
				break;
			}
		}
		for ( size_t i = 0; paths[i] != nullptr; ++i )
		{
			if ( file.mapFile(OS_ENVIRONMENT_CFG, paths[i]) )
			{
				_readConfigEntriesFromFile(file);
				configEntries.m_foundOSFile = true;
				break;
			}
		}

		if ( !configEntries.m_foundGeneralFile && !configEntries.m_foundOSFile )
		{
#if defined(WIN32) && defined(UNICODE)
			_ftprintf(stderr, _T("NOTE: Could not find '%s' or '%s' config files, using defaults.\n"),
				DEF_ENVIRONMENT_CFG, OS_ENVIRONMENT_CFG);
#else
			fprintf(stderr, "NOTE: Could not find '%s' or '%s' config files, using defaults.\n",
				DEF_ENVIRONMENT_CFG, OS_ENVIRONMENT_CFG);
#endif
		}

		configEntries.m_read = true;
	}

	for ( auto& it : configEntries )
	{
		if ( strcmp(it.m_name, name_) == 0 )
			return it.m_value.str();
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Registry of all the known config vars.

#include <set>
typedef std::set<Teul::ConfigVar*> ConfigVars;

static ConfigVars& _getConfigVars()
{
	static ConfigVars s_configVars;
	return s_configVars;
}

static void _getSortedConfigVars(std::vector<Teul::ConfigVar*>& myList)
{
	myList.reserve(_getConfigVars().size());
	myList.assign(_getConfigVars().begin(), _getConfigVars().end());
	std::sort(myList.begin(), myList.end(), [](const Teul::ConfigVar* lhs, const Teul::ConfigVar* rhs) { return strcasecmp(rhs->GetEnvVarName(), lhs->GetEnvVarName()) > 0; });
}

void Teul::ConfigVar::PrintCurrentVars()
{
	std::vector<Teul::ConfigVar*> myList;
	_getSortedConfigVars(myList);
	for ( auto& it : myList )
	{
		printf("%-32s (%s %s)\n", it->GetEnvVarName(), s_configVarTypes[it->GetType()], it->GetName());
		printf("  %s\n", it->GetDescription());
		printf("  Default: %c%s%c, Current: %c%s%c\n"
					, it->GetDefault() ? '"' : '<'
					, it->GetDefault() ? it->GetDefault() : "none"
					, it->GetDefault() ? '"' : '>'
					, it->GetString() ? '"' : '<'
					, it->GetString() ? it->GetString() : "none"
					, it->GetString() ? '"' : '>'
					);
		printf("\n");
	}
}

void Teul::ConfigVar::Generate(const char* const filename_)
{
	std::vector<Teul::ConfigVar*> myList;
	_getSortedConfigVars(myList);
	FILE* fp = fopen(filename_, "w");
	if ( fp == nullptr )
	{
		fprintf(stderr, "Unable to create config file: %s\n", filename_);
		exit(-1);
	}

	time_t now = time(nullptr);
	fprintf(fp, "# '%s' WWIIOL environment config defaults\n# Generated %s\n", filename_, ctime(&now));
	for ( auto& it : myList )
	{
		fprintf(fp, "# %s (%s %s, default: %c%s%c)\n"
				, it->GetName()
				, it->GetRequired() ? "required" : "optional"
				, s_configVarTypes[it->GetType()]
				, it->GetDefault() ? '"' : '<'
				, it->GetDefault() ? it->GetDefault() : "none"
				, it->GetDefault() ? '"' : '>'
				);
		fprintf(fp, "# %s\n", it->GetDescription());
		fprintf(fp, "#%s=%s\n", it->GetEnvVarName(), (it->GetDefault() ? it->GetDefault() : (it->GetRequired() ? "???" : "")));
		fprintf(fp, "\n");
	}
	fclose(fp);
}

////////////////////////////////////////////////////////////////////////////////
// Implementations.

namespace Teul
{
	void ConfigVar::InitializeConfigVars()
	{
		for (auto& it : _getConfigVars())
		{
			it->Initialize();
		}
	}

	bool ConfigVar::LoadedGeneralConfig()
	{
		return _getConfigEntries().m_foundGeneralFile;
	}

	bool ConfigVar::LoadedOSConfig()
	{
		return _getConfigEntries().m_foundOSFile;
	}

	ConfigVar::ConfigVar(ConfigVarType type_, const char* name_, const char* default_, const char* envVarName_, bool required_, const char* description_)
		: m_name(name_)
		, m_type(type_)
		, m_envVarName(envVarName_)
		, m_description(description_)
		, m_default(default_)
		, m_value(nullptr)
		, m_required(required_)
		, m_initialized(false)
	{
		ABORT_IF(envVarName_ == nullptr);
		ABORT_IF(name_ == nullptr);
		ABORT_IF(required_ == true && envVarName_ == nullptr);

		_getConfigVars().insert(this);

		m_union.um_ptr = nullptr;
	}

	void ConfigVar::Initialize()
	{
		if ( m_initialized )
			return;
		m_initialized = true;

		if ( IsNumber() )
			m_value = static_cast<char*>(calloc(1, MaxIntStringSize));

		const char* envValue = GetValueFromEnvironmentOrConfig(m_envVarName);
		if ( envValue != nullptr )
		{
			if ( m_required == true && *envValue == 0 )
				LogAbort("Invalid (empty) value for %s.", m_envVarName);
			Set(envValue);
			return;
		}

		if ( m_default != nullptr )
			Set(m_default);
	}

	void ConfigVar::CheckConfiguration()
	{
		for (auto& it : _getConfigVars())
		{
			if (it->m_required && (it->m_value == nullptr || (it->m_default == nullptr && *it->m_value == 0)))
				LogAbort("Config variable %s (%s) must be defined with a non-empty value.", it->GetName(), it->GetEnvVarName());
		}
	}

	ConfigVar::~ConfigVar()
	{
		if ( m_type != CVT_BOOL && m_value != nullptr )
			free(m_value);
	}

	void ConfigVar::Set(const char* newValue_)
	{
		switch ( m_type )
		{
		case CVT_STRING: SetStringValue(newValue_); break;
		case CVT_UNSIGNED_INT: SetUint32Value(newValue_); break;
		case CVT_SIGNED_INT: SetSint32Value(newValue_); break;
		case CVT_BOOL: SetBoolValue(newValue_); break;
		case CVT_IP_ADDRESS: SetIPAddressValue(newValue_); break;
		case CVT_PATH: SetPathValue(newValue_); break;
		default: LogAbort("Invalid config var type"); break;
		}
	}

	void ConfigVar::SetString(const char* newValue_)
	{
		if ( m_type == CVT_STRING ) return SetStringValue(newValue_);
		else if ( m_type == CVT_IP_ADDRESS ) return SetIPAddressValue(newValue_);
		else if ( m_type == CVT_PATH ) return SetPathValue(newValue_);
		LogAbort("Incompatible use of %s::SetString", GetName());
	}

	void ConfigVar::SetUint32(UINT32 newValue_)
	{
		if ( m_type == CVT_UNSIGNED_INT ) return SetUint32Value(newValue_);
		else if ( m_type == CVT_IP_ADDRESS ) return SetIPAddressValue(newValue_);
		LogAbort("Incompatible use of %s::SetUint32", GetName());
	}

	void ConfigVar::SetSint32(SINT32 newValue_)
	{
		if ( m_type == CVT_SIGNED_INT ) return SetSint32Value(newValue_);
		LogAbort("Incompatible use of %s::SetSint32", GetName());
	}

	void ConfigVar::SetBool(bool newValue_)
	{
		if ( m_type == CVT_BOOL ) return SetBoolValue(newValue_);
		LogAbort("Incompatible use of %s::SetBool", GetName());
	}

	void ConfigVar::SetStringValue(const char* newValue_)
	{
		if ( m_initialized && newValue_ == m_value )
			return;
		m_initialized = true;
		if ( m_value )
			free(m_value);
		m_value = newValue_ ? strdup(newValue_) : nullptr;
		m_union.um_ptr = m_value;
	}

	void ConfigVar::SetUint32Value(const char* newValue_)
	{
		if ( newValue_ != nullptr && *newValue_ )
			SetUint32Value(strtoul(newValue_, nullptr, 10));
		else
		{
			m_initialized = true;
			m_union.um_uint32 = 0U;
			m_value[0] = '\0';
		}
	}

	void ConfigVar::SetSint32Value(const char* newValue_)
	{
		if ( newValue_ != nullptr && *newValue_ )
			SetSint32Value(strtol(newValue_, nullptr, 10));
		else
		{
			m_initialized = true;
			m_union.um_sint32 = 0;
			m_value[0] = '\0';
		}
	}

	void ConfigVar::SetBoolValue(const char* newValue_)
	{
		m_initialized = true;
		if ( newValue_ == nullptr )
		{
			m_union.um_bool = false;
			m_value = nullptr;
			return;
		}

		switch ( *newValue_ )
		{
		case 1: case '1': case 'y': case 'Y': case 't': case 'T':
			m_union.um_bool = true;
			m_value = const_cast<char*>("1");
			break;
		default:
			m_union.um_bool = false;
			m_value = const_cast<char*>("0");
			break;
		}
	}

	void ConfigVar::SetIPAddressValue(const char* newValue_)
	{
		if ( m_initialized && newValue_ == m_value )
			return;
		m_initialized = true;
		if ( m_value )
		{
			free(m_value);
			m_value = nullptr;
			m_union.um_uint32 = 0;
		}
		if ( newValue_ == nullptr || *newValue_ == 0 )
			return;

		if ( !teulGetHost(newValue_, &m_union.um_uint32) )
		{
			m_union.um_uint32 = Teul::GetSingleIP(newValue_);
			if ( m_union.um_uint32 == 0 )
			{
				LogAbort("%s: invalid ip address: %s", GetName(), newValue_);
				return;
			}
		}

		m_value = strdup(newValue_);
	}

	void ConfigVar::SetPathValue(const char* newValue_)
	{
		if ( m_initialized && newValue_ == m_value )
			return;
		m_initialized = true;
		if ( m_value )
			free(m_value);
		m_value = newValue_ ? _makeNativePath(newValue_) : nullptr;
		m_union.um_ptr = m_value;
	}

	void ConfigVar::SetUint32Value(UINT32 newValue_)
	{
		if ( m_initialized && newValue_ == m_union.um_uint32 )
			return;
		m_initialized = true;
		m_union.um_uint32 = newValue_;
		snprintf(m_value, MaxIntStringSize, "%u", newValue_);
	}

	void ConfigVar::SetSint32Value(SINT32 newValue_)
	{
		if ( m_initialized && newValue_ == m_union.um_sint32 )
			return;
		m_initialized = true;
		m_union.um_sint32 = newValue_;
		snprintf(m_value, MaxIntStringSize, "%d", newValue_);
	}

	void ConfigVar::SetBoolValue(bool newValue_)
	{
		SetBoolValue(newValue_ ? "1" : "0");
	}

	void ConfigVar::SetIPAddressValue(UINT32 newValue_)
	{
		if ( m_initialized && newValue_ == m_union.um_uint32 )
			return;
		m_initialized = true;
		if ( m_value )
		{
			free(m_value);
			m_value = nullptr;
			m_union.um_uint32 = 0;
		}
		if ( !newValue_ )
			return;
		m_union.um_uint32 = newValue_;
		m_value = static_cast<char*>(calloc(1, MaxIPAddressSize));
		teulGetAddress(newValue_, m_value, MaxIPAddressSize);
	}
}
