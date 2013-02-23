#include <syscfg.h> // Path is project specific.
// Implement all the various config values.

#include "str/strdupptr.h"
#include "teul/teulConfigVar.h"

// These are here purely to force implementation of the types.
CVAR_STRING(force_string, NULL, NULL, false, NULL);
CVAR_UINT(force_uint, NULL, NULL, NULL);
CVAR_SINT(force_sint, NULL, NULL, NULL);
CVAR_BOOL(force_bool, NULL, NULL, NULL);

////////////////////////////////////////////////////////////////////////////////
// Variables.

#include "sdm/sdmConfigVars.h"

CVAR_STRING(dataDir, "/playnet/data", "DATA_DIR", true
			, "Top level directory for game data.");
CVAR_STRING(importFile, "importlist.txt", "IMPORT_FILE", true
			, "File which will specify sdm files to be used by the server.");
CVAR_STRING(terrainSubdir, "terrain", "TERRAIN_SUBDIR", true
			, "Path from DATA_DIR to the terrain sdm files and origin file.");
CVAR_STRING(originFile, "origin", "ORIGIN_FILE", true
			, "Name of the file which identifies where the sdm files came from.");
CVAR_STRING(stratLanguage, "en", "STRAT_LANGUAGE", true
			, "2-letter language code for server localization.");

#include "wwii/wwiidbConfigVars.h"

CVAR_STRING(wwiiDBAddress, NULL, "WWII_DB_ADDRESS", true
			, "IP address of the MySQL Database.");
CVAR_STRING(wwiiDBDefaultSchema, "wwiionline", "WWII_DB_SCHEMA", true
			, "Default schema for game data.");

#include "wwii/wwiiConfigVars.h"

#include "server/serverConfigVars.h"
CVAR_STRING(clusterName, NULL, "WWII_CLUSTER", true
			, "Short-name for this cluster, e.g. 'dev'");
CVAR_STRING(ircServer, "", "WWII_IRC_SERVER", false
			, "Address of the IRC server. If not defined, disables IRC.");
CVAR_STRING(ircPassword, "blooGit", "WWII_IRC_PASSWD", true
			, "Server-password for the IRC server.");
CVAR_UINT(ircPort, "6667", "WWII_IRC_PORT"
			, "Port to connect to the IRC server on.");
CVAR_UINT(arenaID, NULL, "ARENA_ID"
			, "Numeric ID for this arena.");
CVAR_STRING(teulWebPath, "/playnet/wwiiol/web/debug", "TEUL_UWEB_PATH", true
			, "Path to the top-level directory for the internal web server.");
CVAR_STRING(authErrorUrl, "http://www.wwiionline.com/auth/errors/", "AUTH_ERROR_URL", true
			, "URL to prefix authentication error URLs with.");

////////////////////////////////////////////////////////////////////////////////
// Read values from the environment of configuration file.

#include <vector>

struct ConfigEntry
{
	ConfigEntry() : m_name(NULL), m_value(NULL) {}
	ConfigEntry(const char* name_, const char* equals_, const char* end_)
		: m_name(name_, static_cast<size_t>(equals_ - name_))
		, m_value(equals_ + 1, static_cast<size_t>(end_ - (equals_ + 1)))
	{}
	ConfigEntry& operator = (ConfigEntry&& rhs_) { m_name = std::move(rhs_.m_name); m_value = std::move(rhs_.m_value); return *this; }
	StrDupPtr	m_name;
	StrDupPtr	m_value;
};

typedef std::vector<ConfigEntry> ConfigEntries;
static bool s_configEntriesRead = false;
static ConfigEntries s_configEntries;

#include "include/io_mapped_file.h"

static void _readConfigEntriesFromFile(IO::MappedFile& file)
{
	s_configEntriesRead = true;
	s_configEntries.reserve(32);
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
		if ( equals == NULL )
		{
			file.skipline();
			continue;
		}
		const char* name = cur;
		cur = equals + 1;
		const char* eol = strstr(cur, "\r\n");
		if ( eol != NULL )
			cur = eol;
		else
			cur = eol = file.endPtr();
		s_configEntries.resize(s_configEntries.size() + 1);
		s_configEntries.back() = std::move(ConfigEntry(name, equals, eol));
		file.skipline();
	}
}

#if defined(WIN32) && defined(UNICODE)
#define OS_ENVIRONMENT_CFG (L"environment.win.cfg")
#define DEF_ENVIRONMENT_CFG L"environment.cfg"
#else
#define OS_ENVIRONMENT_CFG ("environment.unix.cfg")
#define DEF_ENVIRONMENT_CFG "environment.cfg"
#define PARENT_DIR ".."
#endif

static const char* GetValueFromEnvironmentOrConfig(const char* const name_)
{
	// Environment beats config.
	const char* const envValue = getenv(name_);
	if ( envValue != nullptr )
		return envValue;

	if ( s_configEntriesRead == false )
	{
		s_configEntriesRead = true;
		IO::MappedFile file;
		// Apply defaults first.
		if ( file.mapFile(DEF_ENVIRONMENT_CFG) )
			_readConfigEntriesFromFile(file);
		if ( file.mapFile(OS_ENVIRONMENT_CFG) )
			_readConfigEntriesFromFile(file);
	}

	for ( auto& it : s_configEntries )
	{
		if ( strcmp(it.m_name, name_) == 0 )
			return it.m_value.str();
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
// Implementations.

template<ConfigVarType Type_>
Teul::ConfigVar<Type_>::ConfigVar(const char* const name_, const char* const default_, const char* const envVarName_, bool required_)
	: m_name(name_)
	, m_envVarName(envVarName_)
	, m_value(nullptr)
	, m_required(required_)
{
	ABORT_IF(name_ == nullptr);
	ABORT_IF(required_ == true && envVarName_ == nullptr);

	m_union.um_ptr = nullptr;

	if ( IsNumber() )
		m_value = static_cast<char*>(calloc(1, MaxIntStringSize));

	if ( envVarName_ )
	{
		const char* envValue = GetValueFromEnvironmentOrConfig(envVarName_);
		if ( envValue != nullptr )
		{
			if ( m_required == true && *envValue == 0 )
				LogAbort("Invalid (empty) value for %s ('%s').", envVarName_);
			Set(envValue);
			return;
		}
	}

	if ( default_ != nullptr )
		Set(default_);

	if ( m_required && (m_value == nullptr || *m_value == 0) )
		LogAbort("Config variable %s (%s) must be defined with a non-empty value.", name_, envVarName_);
}

template<> void Teul::ConfigVar<CVT_BOOL>::Set(const char* const newValue_)
{
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
		m_value = "1";
		break;
	default:
		m_union.um_bool = false;
		m_value = "0";
		break;
	}
}

template<> void Teul::ConfigVar<CVT_STRING>::SetString(const char* const newValue_)
{
	if ( newValue_ == m_value )
		return;
	if ( m_value )
		free(m_value);
	m_value = newValue_ ? strdup(newValue_) : nullptr;
	m_union.um_ptr = m_value;
}

template<> void Teul::ConfigVar<CVT_UNSIGNED_INT>::SetUint32(const UINT32 newValue_)
{
	if ( newValue_ == GetUint32() )
		return;
	m_union.um_uint32 = newValue_;
	snprintf(m_value, MaxIntStringSize, "%u", newValue_);
}

template<> void Teul::ConfigVar<CVT_SIGNED_INT>::SetSint32(const SINT32 newValue_)
{
	if ( newValue_ == GetSint32() )
		return;
	m_union.um_sint32 = newValue_;
	snprintf(m_value, MaxIntStringSize, "%d", newValue_);
}
