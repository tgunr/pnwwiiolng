#include <syscfg.h> // Path is project specific.

#include "include/infoMessages.h"
#include "MoreGetRoutines.h"
#include "include/infoPack.h"
#include "wwii/wwiiCallbacks.h"

PLAY_COMPUTER_INFO compInfo;

static TEUL_CALLBACK* infoSend;

extern HINSTANCE hMainInstance;

static SINT32 _SendInfo(ACTION_OBJECT* const a)
{
	return teulPackVoid(TEUL_PACK, a->packed, a->offset, a->localRawData, sizeof(PLAY_COMPUTER_INFO));
}

static void _sendInfo(SINT32 teulIndex,PLAY_COMPUTER_INFO* const info)
{
	// Ensure the version stamp is current.
	info->version = info->CurrentVersion();
	teulSend(teulIndex,infoSend,info);
}


static SINT32 _recvInfoReq(ACTION_OBJECT* const a)
{
	COMPUTER_INFO_REQ req;

	a->offset = packCompInfoReq(TEUL_UNPACK,a->packed,a->offset,&req,0);

	//StatusSetStatus("RX/TX Info Request");

	memset(&compInfo, 0, sizeof(compInfo));
	if ( hMainInstance != nullptr )
		GetComputerInfo(hMainInstance, &compInfo, FALSE);

	_sendInfo(a->teulIndex, &compInfo);

	return a->offset;
}

SINT32 infoInstallCallbacks(void)
{
	BEGIN_CALLBACK_REGISTRATION(PgInfo::CallbackX)
	{
		REGISTER_SENDER(PLAY_INFO_SEND_Y, _SendInfo, infoSend);
		REGISTER_RECEIVER(PLAY_INFO_REQ_RECV_Y, _recvInfoReq);
	}
	END_CALLBACK_REGISTRATION(PgInfo::CallbackX)
}
