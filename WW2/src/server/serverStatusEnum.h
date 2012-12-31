#pragma once
namespace Server
{
enum Status : UINT32
{
	SERVER_LOGIN = 0,
	SERVER_INITIALIZING,
	SERVER_ACCEPTING,
	SERVER_FULL
} ;
}

