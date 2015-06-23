#include <syscfg.h>	// Path is project specific.

#include "wwii/wwiiNetPorts.h"
#include "teul/teulEndpoint.h"

static void
_onAccept(ENDPOINT& endpoint)
{
	dbugLog(_FLD, "t:%d luaClient connection from %s", endpoint.teulIndex(), teulGetAddress(endpoint.fromAddress()));
}

static void
_onError(SINT32 teulIndex, const UCHAR* const /*data*/, SINT32 /*size*/, const TEUL_CALLBACK* const /*object*/, SINT32 error)
{
	if ( error == ENOTCONN )
	{
		dbugLog(_FLD, "t:%d luaClient disconnected", teulIndex);
		return;
	}
	dbugLog(_FLW, "t:%d[luaClient] error:%d", teulIndex, error);
}

void
serverOpenLuaClientInterface()
{
	const SINT32 result = teulOpenListen(WWII::Net::Ports::LuaInterface, _onAccept, 10000, PROTOCOL_TCP, _onError);
	if ( result < 0 )
	{
		LogAbort("Unable to initialize lua scripting interface [port %u]: %d", WWII::Net::Ports::LuaInterface, result);
		/*ABORT*/
	}
}

