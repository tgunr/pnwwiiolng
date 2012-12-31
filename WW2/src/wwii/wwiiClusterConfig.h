#include "str/strdupptr.h"

namespace Cluster
{
	class Config
	{
		bool	m_loaded;

	public:
		Config() : m_loaded(false) {}
		static void Load(const char* const name_ = NULL);

		struct ServerAddr
		{
			StrDupPtr	m_ip;
			UINT16		m_port;
		};
		struct ServerCred : public ServerAddr
		{
			StrDupPtr	m_username;
			StrDupPtr	m_password;
		};

		struct General
		{
			StrDupPtr	m_name;
			StrDupPtr	m_path;
			char		m_drive[3];
		} m_general;

		struct Network
		{
			StrDupPtr	m_serverNet;
			StrDupPtr	m_cellNet;
		} m_network;

		struct Database
		{
			struct MySQL
			{
				bool		m_enableCompression;
			} m_mysql;

			ServerCred	m_game;
			ServerCred	m_auth;
		} m_db;

		struct Auth
		{
			StrDupPtr	m_errorUrl;
			UINT32		m_appID;
		} m_auth;

		struct ClusterBosss
		{
			ServerAddr	m_master;
		} m_boss;

		struct StratHost
		{
			bool		m_ignoreErrors;
			char		m_language[3];
			struct Lua 
			{
				StrDupPtr	m_path;
				StrDupPtr	m_rcFile;
			} m_lua;
		} m_strat;

		struct ChatHost
		{
			UINT32		m_maxPlayersPer;
		} m_chat;

		struct MapHost
		{
			UINT32		m_maxPlayersPer;
		} m_map;

		struct CellHost
		{
			UINT32		m_maxPlayersPer;
		} m_cell;

		struct IRC
		{
			bool		m_enabled;
			ServerCred	m_server;
		} m_irc;
	};

}

extern Cluster::Config g_cluscfg;
