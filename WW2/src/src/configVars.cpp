#include <syscfg.h> // Path is project specific.
// Implement all the various config values.

#include "str/strdupptr.h"
#include "teul/teulConfigVar.h"

////////////////////////////////////////////////////////////////////////////////
// Variables.

#include "sdm/sdmConfigVars.h"

#if platOS != OS_WIN32
CVAR_STRING(dataDir, "/playnet/data", "DATA_DIR");
#else
CVAR_STRING(dataDir, "c:\\playnet\\data", "DATA_DIR");
#endif
CVAR_STRING(importFile, "importlist.txt", "IMPORT_FILE");
CVAR_STRING(terrainSubdir, "terrain", "TERRAIN_SUBDIR");
CVAR_STRING(originFile, "origin", "ORIGIN_FILE");

#include "wwii/wwiidbConfigVars.h"

#if platOS != OS_WIN32
CVAR_STRING(wwiiDBAddress, NULL, "WWII_DB_ADDRESS");
#else
CVAR_STRING(wwiiDBAddress, "127.0.0.1", "WWII_DB_ADDRESS");
#endif
CVAR_STRING(wwiiDBDefaultSchema, "wwiionline", "WWII_DB_SCHEMA");

#include "wwii/wwiiConfigVars.h"

CVAR_STRING(wwiiClusterName, NULL, "WWII_CLUSTER");
CVAR_STRING(wwiiIrcServer, "", "WWII_IRC_SERVER");
CVAR_STRING(thblitzPath, SUB_PATH("th_blitz", "weapon"), "THBLITZ_PATH");
CVAR_STRING(blitzworPath, "blitzWor.csv", "BLITZWOR_PATH");

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
		if ( file.mapFile(OS_ENVIRONMENT_CFG) || file.mapFile(DEF_ENVIRONMENT_CFG) )
		{
			_readConfigEntriesFromFile(file);
		}
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
Teul::ConfigVar<Type_>::ConfigVar(const char* const name_, const char* const default_, const char* const envVarName_)
	: m_name(name_)
	, m_envVarName(envVarName_)
	, m_value(nullptr)
{
	ABORT_IF(name_ == nullptr);

	m_union.um_ptr = nullptr;

	if ( IsNumber() )
		m_value = static_cast<char*>(calloc(1, MaxIntStringSize));

	if ( default_ != nullptr )
		Set(default_);

	if ( envVarName_ )
	{
		const char* envValue = GetValueFromEnvironmentOrConfig(envVarName_);
		if ( envValue != nullptr )
			Set(envValue);
		else if ( default_ == nullptr && !envValue )
			LogAbort("Environment variable '%s' not defined.", envVarName_);
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

template<> void Teul::ConfigVar<CVT_UNSIGNED_INT>::SetUnsigned(const UINT64 newValue_)
{
	if ( newValue_ == GetUnsigned() )
		return;
	m_union.um_uint64 = newValue_;
	snprintf(m_value, MaxIntStringSize, "%llu", newValue_);
}

template<> void Teul::ConfigVar<CVT_SIGNED_INT>::SetSigned(const SINT64 newValue_)
{
	if ( newValue_ == GetSigned() )
		return;
	m_union.um_sint64 = newValue_;
	snprintf(m_value, MaxIntStringSize, "%lld", newValue_);
}
