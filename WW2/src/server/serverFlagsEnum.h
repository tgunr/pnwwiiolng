#pragma once

namespace Server
{
enum Flags : UINT32
{
	SVRFLAG_SEND_PSTAT		= 0x00000001,	// send player status updates
	SVRFLAG_IGNORE_PSTAT	= 0x00000002,	// track player status
	SVRFLAG_RELOGIN			= 0x00000004,	// allow relogin of client
	SVRFLAG_DETATCHED		= 0x00000008	// act as a detatched server that only
											// reacts to player updates fro add and delete
											// never be use w/ SVRFLAG_SEND_PSTAT
} ;
}
