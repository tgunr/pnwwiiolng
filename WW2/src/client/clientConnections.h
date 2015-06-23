#pragma once

#include "teul/teulConnection.h"

namespace ww2 { class Client; };

///! @file client/clientConnections.h
///! @brief Describes the ClientConnection, MapConnection, ChatConnection and CellConnection classes.

class ClientHostConnection : public Teul::Connection
{
	public: ClientHostConnection(HostType _type
								, const Teul::ServerAddr& _serverInfo
								, const Teul::PlayerIdent& _playerInfo
								, UINT32 _timeoutMS
								)
								: Teul::Connection(_type, _serverInfo, _playerInfo, _timeoutMS) {}
	protected: ~ClientHostConnection() {}

   // our callbacks that lookup our client and call its process function
	protected: virtual void onLogin(ww2::Client* client);
	protected: virtual void onLoginFail(ww2::Client* client, SINT32 result);
	protected: virtual void onConnect(ww2::Client* client);
	protected: virtual void onError(ww2::Client* client, SINT32 error);

	public: void SetError(ww2::Client* client, SINT32 error);
};

// Map, Chat and Cell derived classes are in source control :-P
