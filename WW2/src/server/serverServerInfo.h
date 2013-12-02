#pragma once

#include "server/serverFlagsEnum.h"
#include "server/serverHeartBeat.h"
#include "wwii/wwiiServerTypeEnums.h"

class PlayerStatusNode;

namespace Server
{
class PersonaMirror;

class ServerInfo
{
public:
	ServerInfo()
		: stats(), flags((Server::Flags)0)
		, m_lastRecv(0)
		, availPlayers(0)
		, m_players(), localInfo(NULL), exitDestType(0)
		{
		}
	ServerInfo(const HeartBeat& beat)
		: stats(beat), flags((Server::Flags)0)
		, m_lastRecv(0)
		, availPlayers(0)
		, m_players(), localInfo(NULL), exitDestType(0)
		{
		}

	~ServerInfo();

	SINT32 mirrorPlayer(Server::PersonaMirror* mirrorData);

public:
	HeartBeat			stats;
	Server::Flags		flags;
	TeulTimeMS			m_lastRecv;
	SINT32				availPlayers;	// max players allowed on server
	std::set<PlayerStatusNode*>	m_players;
	void*				localInfo;		// application can store local info here
										// it is freed with the server info.
										// never keep a local pointer to
										// this info since it may be freed in the next
										// call to teulUpdateTransport
	UINT32				exitDestType;	// default exit destination type

// Member functions.
public:
	//! force the sync of server information to other servers.
	//! should be called whenever significant changes are made to the state
	//! of the server.
	SINT32 sync();

	//! Check if there is an open server available for this type.
	static bool IsAvailable(const UINT16 serverType_);

	//! Look up info about a server by its base ip address.
	static Server::ServerInfo* GetInfo(UINT32 baseIP_, UINT16 serverType_);

	//! Same as ServerInfo::GetInfo except by id
	static Server::ServerInfo* GetByID(const UINT64& id);

	//! Get the best server to send to by type.
	//! This function culls out all servers that are not
	//! accepting and returns the one with the most available players.
	static Server::ServerInfo* GetBest(UINT16 serverType_, bool allowFull=false);
};

 extern Server::ServerInfo myServerInfo;

}

