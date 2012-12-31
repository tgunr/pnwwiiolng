#include <syscfg.h> // path is platform specific.

#include <assert.h>
#include <string>
#include "tinyxml/tinyxml.h"

#include "dbug/dbug.h"
#include "str/strdupptr.h"
#include "wwii/wwiiClusterConfig.h"

static const char* const xmlFile = "clusterconfig.xml";
static const char* const xmlVersion = "1.0";

class Cluster::Config g_cluscfg;

#include <OpenThreads/Mutex>
static OpenThreads::Mutex s_cluscfgMutex;

// The loader uses exceptions to propagate errors in a clean manner.
struct ElementException
{
	ElementException(const char* path_, const char* tag_, const char* problem_, StrDupPtr error_)
		: m_path(path_), m_tag(tag_), m_error(error_), m_problem(problem_) {}
	StrDupPtr m_path, m_tag, m_error;
	const char* m_problem;
};
struct MissingContainerException : public ElementException
{
	MissingContainerException(const char* path_, const char* tag_)
		: ElementException(path_, tag_, "Missing section", "Required") {}
};
struct MissingValueException : public ElementException
{
	MissingValueException(const char* path_, const char* tag_)
		: ElementException(path_, tag_, "Missing value", "Required") {}
};
struct InvalidValueException : public ElementException
{
	InvalidValueException(const char* path_, const char* tag_, const char* error_=NULL)
		: ElementException(path_, tag_, "Invalid value", error_) {}
};

//////////////////////////////////////////////////////////////////////////
// Locate a value element in a container at a given path. While this is
// slightly inefficient (we keep searching paths over and over), it is
// only intended for startup.

const char* _findElement(const TiXmlHandle& root, const char* const path_, const char* tag_, bool required_, size_t maxLen_)
{
	StrDupPtr path(path_);	// Copy so we can convert the '/'s to zeros and back.

	// Find the container.
	TiXmlElement* element = root.Element();
	for ( char* cur = path.str() ; cur != NULL ; )
	{
		char* end = strchr(cur, '/');
		if ( end != NULL )
			*end = 0;
		element = element->FirstChildElement(cur);
		if ( element == NULL )
		{
			if ( required_ )
				throw MissingContainerException(path, cur);
			return NULL;
		}
		cur = end;
		if ( cur != NULL )
			*(cur++) = '/';
	}

	// Find the element itself.
	element = element->FirstChildElement(tag_);
	if ( element == NULL )
	{
		if ( required_ )
			throw MissingValueException(path_, tag_);
		return NULL;
	}

	// Basic validation.
	TiXmlNode* textNode = element->FirstChild();
	if ( textNode == NULL || textNode->Type() != TiXmlNode::TEXT )
	{
		if ( required_ )
			throw InvalidValueException(path_, tag_, "Empty value");
		return NULL;
	}

	const char* value = textNode->ToText()->Value();
	if ( value == NULL || *value == 0 )
		throw InvalidValueException(path_, tag_, "Empty value");
	if ( maxLen_ > 0 && strlen(value) > maxLen_)
		throw InvalidValueException(path_, tag_, "value is too long (character count)");

	dbugLog(_FLD, "ccfg: %s/%s = %s", path_, tag_, value);

	return value;
}

// Get a char[n] value.
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, char* into_, bool required_)
{
	const char* value = _findElement(root_, path_, tag_, required_, 2);

	strcpy(into_, value ? value : "");
}

// Get a numeric value.
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, UINT32& into_, bool required_)
{
	const char* value = _findElement(root_, path_, tag_, required_, 10);

	into_ = static_cast<UINT32>(value ? atol(value) : 0);
}

// Get a numeric short value.
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, UINT16& into_, bool required_)
{
	const char* value = _findElement(root_, path_, tag_, required_, 5);

	into_ = static_cast<UINT16>(value ? atoi(value) : 0);
}

// Get a bool value
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, bool& into_, bool required_)
{
	const char* value = _findElement(root_, path_, tag_, required_, 5);

	const bool isTrue = strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 || strcasecmp(value, "on") == 0 || strcasecmp(value, "1") == 0;
	if ( isTrue )
	{
		into_ = true;
		return;
	}
	const bool isFalse = strcasecmp(value, "false") == 0 || strcasecmp(value, "no") == 0 || strcasecmp(value, "off") == 0 || strcasecmp(value, "0") == 0;
	if ( isFalse )
	{
		into_ = false;
		return;
	}

	throw InvalidValueException(path_, tag_, "Invalid boolean, expected one of true|false|yes|no|on|off or 0|1.");
}

// Get a string value
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, StrDupPtr& into_, bool required_)
{
	const char* value = _findElement(root_, path_, tag_, required_, 1024);
	into_ = value;
}

// Get server credential struct.
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, Cluster::Config::ServerCred& into_, bool required_)
{
	std::string container = path_;
	container += "/";
	container += tag_;

	_getValue(root_, container.c_str(), "Address", into_.m_ip, required_);
	_getValue(root_, container.c_str(), "Port", into_.m_port, false);
	_getValue(root_, container.c_str(), "User", into_.m_username, required_);
	_getValue(root_, container.c_str(), "Password", into_.m_password, required_);
}

// Handle server address struct.
void
_getValue(const TiXmlHandle& root_, const char* const path_, const char* tag_, Cluster::Config::ServerAddr& into_, bool required_)
{
	std::string container = path_;
	container += "/";
	container += tag_;

	_getValue(root_, container.c_str(), "Address", into_.m_ip, required_);
	_getValue(root_, container.c_str(), "Port", into_.m_port, false);
}

#include <OpenThreads/ScopedLock>

void
Cluster::Config::Load(const char* filename_)
{
	OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_cluscfgMutex);
	if ( g_cluscfg.m_loaded )
		return;

	if ( filename_ == NULL )
		filename_ = "/playnet/etc/clusterconfig.xml";

	try
	{
		TiXmlDocument doc(filename_) ;
		if ( doc.LoadFile() == false )
			throw std::invalid_argument("Unable to load cluster configuration file.");

		TiXmlElement* rootEl = doc.FirstChildElement("ClusterConfig") ;
		if ( rootEl == NULL )
			throw std::invalid_argument("Unable to find <ClusterConfig> container in config file.");
		const TiXmlHandle root = TiXmlHandle(rootEl);

		dbugLog(_FLI, "ccfg: reading file %s", filename_) ;

		_getValue(root, "General", "Name", g_cluscfg.m_general.m_name, true);
		_getValue(root, "General", "BasePath", g_cluscfg.m_general.m_path, true);
#if platOS == OS_WIN32
		_getValue(root, "General", "BaseDrive", g_cluscfg.m_general.m_drive, true);
#else
		_getValue(root, "General", "BaseDrive", g_cluscfg.m_general.m_drive, true);
#endif

		_getValue(root, "Network", "ServerNet", g_cluscfg.m_network.m_serverNet, true);
		_getValue(root, "Network", "CellNet", g_cluscfg.m_network.m_cellNet, true);

		_getValue(root, "Database/MySQL", "EnableCompression", g_cluscfg.m_db.m_mysql.m_enableCompression, true);

		_getValue(root, "Database", "Game", g_cluscfg.m_db.m_game, true);
		_getValue(root, "Database", "Auth", g_cluscfg.m_db.m_auth, true);

		_getValue(root, "Auth", "ErrorURL", g_cluscfg.m_auth.m_errorUrl, true);
		_getValue(root, "Auth", "AppID", g_cluscfg.m_auth.m_appID, true);

		_getValue(root, "ClusterBoss", "Master", g_cluscfg.m_boss.m_master, true);

		_getValue(root, "StratHost", "IgnoreErrors", g_cluscfg.m_strat.m_ignoreErrors, true);
		_getValue(root, "StratHost", "Language", g_cluscfg.m_strat.m_language, true);
		_getValue(root, "StratHost/Lua", "Path", g_cluscfg.m_strat.m_lua.m_path, true);
		_getValue(root, "StratHost/Lua", "RCFile", g_cluscfg.m_strat.m_lua.m_rcFile, true);

		_getValue(root, "ChatHost", "MaxPlayersPer", g_cluscfg.m_chat.m_maxPlayersPer, false);

		_getValue(root, "MapHost", "MaxPlayersPer", g_cluscfg.m_map.m_maxPlayersPer, false);

		_getValue(root, "CellHost", "MaxPlayersPer", g_cluscfg.m_cell.m_maxPlayersPer, false);

		_getValue(root, "IRC", "Enabled", g_cluscfg.m_irc.m_enabled, true);
		if ( g_cluscfg.m_irc.m_enabled )
			_getValue(root, "IRC", "Server", g_cluscfg.m_irc.m_server, true);

		g_cluscfg.m_loaded = true;
	}
	catch ( ElementException& e )
	{
		if ( e.m_problem)
			LogAbort("ccfg: %s %s/%s: %s", e.m_problem, e.m_path.str(), e.m_tag.str(), e.m_error.str());
	}
	catch ( std::exception& e )
	{
		LogAbort("%s: %s", __FUNCTION__, e.what()) ;
	}
}
