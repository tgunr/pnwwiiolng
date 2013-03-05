#pragma once

#include "wwii/wwiiServerTypeEnums.h"

struct SERVER_SUMMARY
{
	UINT64			id;		// mac address of eth0 + pid
	UINT32			address;
	SERVER_TYPE		type;
} ;

