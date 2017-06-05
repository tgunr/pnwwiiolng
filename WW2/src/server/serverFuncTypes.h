#pragma once

#include "wwii/wwiiServerTypeEnums.h"
#include "teul/teulFuncTypes.h"

namespace Server
{
	class PersonaLogin;
	class PersonaExit;
	class ServerInfo;
	enum class Code : uint32_t;
	enum class ReleaseType;
}

using CREATE_PERSONA_FUNCTION = SINT32(*)(const Server::PersonaLogin&, class PlayerStatusNode& psn);
using RELEASE_PERSONA_FUNCTION = SINT32(*)(SINT32 teulIndex, class PlayerStatusNode* psn, Server::ReleaseType releaseType);
using ACTIVATE_PLAYER_FUNCTION = SINT32(*)(SINT32 teulIndex , PlayerStatusNode* psn);
using EXIT_PERSONA_FUNCTION = SINT32(*)(SINT32 teulIndex, PlayerStatusNode* psn, Server::PersonaExit& persXfer);
using EXIT_CLUSTER_FUNCTION = SINT32(*)(PlayerStatusNode& psn);

// -retval indicates failure.
using SERVER_UPDATE = SINT32(*)(Server::ServerInfo* server, Server::Code scode);

// initialize when server is ready to process server info
// 	appVersion	: aplication version served
//	serverType	: type of server e.g. CELL_HOST
// 	enterCallback	: called when a server enters
//	leaveCallback	: called when a server leaves
//	statusCallback	: called when  a severs status changes
//	createPersona	: each player in a group is created individually here
//	releasePersona : called when ever a persona exits
//	activatePersona : called wehever the player logs in and associates to the PLAYER_STATUS
// 	xferCompleteFn  : called when the persona transfer is completed by sendPlayerTp or sendGroupTo
//	errorFunxtion	  : used to relay problems with transfers
SINT32 serverInit(UINT32 appVersion
					, SERVER_TYPE serverType, UINT32 exitDestType
					// sever managment callbacks
					, SERVER_UPDATE enterCallback
					, SERVER_UPDATE leaveCallback
					, SERVER_UPDATE statusCallback
					// player managment callbacks
					, CREATE_PERSONA_FUNCTION	createPersona
					, RELEASE_PERSONA_FUNCTION releasePersona
					, ACTIVATE_PLAYER_FUNCTION activatePlayer
					, EXIT_PERSONA_FUNCTION exitPersonaFn
					, EXIT_CLUSTER_FUNCTION exitClusterFn
					// shared error callback
					, ERROR_FUNCTION errorFunction);

