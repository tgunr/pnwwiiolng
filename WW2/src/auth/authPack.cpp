#include <syscfg.h> // Path is project specific.
// Created: Thu Jun 15 16:22:35 2000
//
// All the code to the pack routines for the authple reside here.
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Shane "Elmo" Wickson
// Supporting Programmer:  Marty "Vandal" Poulin

#include "teul/teulKit.h"
#include "auth/authTypes.h"
#include "auth/authDispatch.h"

SINT32 authPackAPP_FILE_CHECK(PackMode mode, UCHAR* data, SINT32 offset, APP_FILE_CHECK* request, SINT32 /*size*/)
{
	if (PN_UNLIKELY(request == nullptr))
		return 0;

	offset = teulPackSINT32(mode,data,offset,&request->appID,0);
	offset = teulPackUINT32(mode,data,offset,&request->presenterVersion,0);

	offset = teulPackVoid(mode,data,offset,&request->fileKey,MAX_PASSWORD);
	offset = teulPackVoid(mode,data,offset,&request->manifestKey,MAX_PASSWORD);

	request->fileKey[MAX_PASSWORD-1] = '\0';
	request->manifestKey[MAX_PASSWORD-1] = '\0';

	return offset;
}

SINT32
authPackARENA_REQ(PackMode mode, UCHAR* data, SINT32 offset, ARENA_REQ* request, SINT32 /*size*/)
{
	if (PN_UNLIKELY(request == nullptr))
		return 0;

	offset = teulPackSINT32(mode,data,offset,&request->arenaID,0);
	offset = teulPackUINT32(mode,data,offset,&request->sessionID,0);
	offset = teulPackUINT32(mode,data,offset,(UINT32*)&request->infoOnly,8);
	offset = teulPackUINT32(mode,data,offset,&request->presenterVersion,0);

	request->username[0] = '\0';
	request->password[0] = '\0';

	request->oVersion = 1;

	return offset;
}

SINT32 authPackARENA_REQ2(PackMode mode, UCHAR* data, SINT32 offset, ARENA_REQ* request, SINT32 /*size*/)
{
	if (PN_UNLIKELY(request == nullptr))
		return 0;

	offset = teulPackSINT32(mode,data,offset,&request->arenaID,0);
	offset = teulPackUINT32(mode,data,offset,&request->sessionID,0);
	offset = teulPackUINT32(mode,data,offset,(UINT32*)&request->infoOnly,8);
	offset = teulPackUINT32(mode,data,offset,&request->presenterVersion,0);

	offset = teulPackVoid(mode,data,offset,&request->username,MAX_OLD_USERNAME);
	offset = teulPackVoid(mode,data,offset,&request->password,MAX_PASSWORD);

	request->username[MAX_OLD_USERNAME-1]='\0';
	request->password[MAX_PASSWORD-1]='\0';

	request->oVersion = 2;

	return offset;
}

SINT32 authPackAPP_VERIFY(PackMode mode, UCHAR* data, SINT32 offset, APP_VERIFY* object, SINT32 /*size*/)
{
	if (PN_UNLIKELY(object == nullptr))
		return 0;

	offset = teulPackSINT32(mode,data,offset,&object->arenaID,0);
	offset = teulPackSINT32(mode,data,offset,&object->sessionID,0);
	offset = teulPackSINT32(mode,data,offset,&object->status,0);
	offset = teulPackSINT32(mode,data,offset,&object->appID,0);
	offset = teulPackSINT32(mode,data,offset,&object->appVersionMax,0);
	offset = teulPackVoid(mode,data,offset,&object->appleID,4);

	return offset;
}

SINT32 authPackAPP_LAUNCH_REQ(PackMode mode, UCHAR* data, SINT32 offset, APP_LAUNCH_REQ* request, SINT32 /*size*/)
{
	if (PN_UNLIKELY(request == nullptr))
		return -EINVAL;

	offset = teulPackVoid(mode,data,offset,&request->username,MAX_OLD_USERNAME);
	offset = teulPackVoid(mode,data,offset,&request->password,MAX_PASSWORD);

	request->username[MAX_OLD_USERNAME-1]='\0';
	request->password[MAX_PASSWORD-1]='\0';

	offset = teulPackSINT32(mode,data,offset,&request->status,0);
	offset = teulPackSINT32(mode,data,offset,&request->arenaID,0);
	offset = teulPackUINT32(mode,data,offset,&request->sessionID,0);
	offset = teulPackSINT32(mode,data,offset,&request->appID,0);
	offset = teulPackUINT32(mode,data,offset,&request->appVersion,0);

	offset = teulPackSINT32(mode,data,offset,&request->cdKeySize,0);
	
	// Ugly hack: The size sent by the client can't be trusted; we'll get a
	// nasty buffer overflow if we use an invalid size. However, always
	// packing/unpacking MAX_PARAM (as similarly done by the username and
	// password packers above) would break all teul clients using this packet,
	// requiring new versions of Playgate and the Mac client at minimum. By just
	// clamping the size actually read, we maintain compatibility while still
	// avoiding the overflow. Note that if a packet legitimately has a cdKeySize
	// that's greater than MAX_PARAM (rather than it being a crypto error), the
	// remaining fields in the packet will be corrupted, but this happening is
	// already an invalid data condition so we don't care, and doing it
	// "properly" is just more trouble than its worth when this is already a
	// major design flaw. Note also "MAX_PARAM - 1", because also unlike the
	// un/pw fields, cdKey is not defined with +1 bytes as its size in the
	// APP_LAUNCH_REQ structure. We use both min() and max() because the size is
	// signed and therefore may be less than zero (which, when the value reaches
	// memcpy(), will result in an attempt to copy gigabytes of memory!).
	SINT32 safeSize = std::min(MAX_PARAM - 1, std::max(0, request->cdKeySize));
	offset = teulPackVoid(mode,data,offset,&(request->cdKey),safeSize);
	// Truncate both the decided safe size and the end of the buffer, for
	// excessive safety.
	request->cdKey[safeSize] = '\0';
	request->cdKey[MAX_PARAM - 1] = '\0';

	UINT32 ignore = (UINT32)request->ignoreMinorUpdates;
	offset = teulPackUINT32(mode,data,offset,&ignore,0);
	request->ignoreMinorUpdates = (LOGICAL) ignore;

	return offset;
}

SINT32 authPackAPP_LAUNCH_REQ2(PackMode mode, UCHAR* data, SINT32 offset, APP_LAUNCH_REQ* request, SINT32 /*size*/)
{
	if (PN_UNLIKELY(request == nullptr))
		return -EINVAL;

	offset = teulPackVoid(mode,data,offset,&request->username,MAX_OLD_USERNAME);
	offset = teulPackVoid(mode,data,offset,&request->password,MAX_PASSWORD);

	request->username[MAX_OLD_USERNAME-1]='\0';
	request->password[MAX_PASSWORD-1]='\0';

	offset = teulPackSINT32(mode,data,offset,&request->status,0);
	offset = teulPackSINT32(mode,data,offset,&request->arenaID,0);
	offset = teulPackUINT32(mode,data,offset,&request->sessionID,0);
	offset = teulPackSINT32(mode,data,offset,&request->appID,0);
	offset = teulPackUINT32(mode,data,offset,&request->appVersion,0);

	offset = teulPackSINT32(mode,data,offset,&request->cdKeySize,0);
	offset = teulPackVoid(mode,data,offset,&(request->cdKey),request->cdKeySize);

	offset = teulPackUCHAR(mode,data,offset,&request->oVersion,0);

	offset = teulPackVoid(mode,data,offset,&request->fileKey,MAX_PASSWORD);
	offset = teulPackVoid(mode,data,offset,&request->manifestKey,MAX_PASSWORD);

	request->fileKey[MAX_PASSWORD-1]='\0';
	request->manifestKey[MAX_PASSWORD-1]='\0';


	request->cdKey[request->cdKeySize] = '\0';

	UINT32 ignore = (UINT32) request->ignoreMinorUpdates;
	offset = teulPackUINT32(mode,data,offset,&ignore,0);
	request->ignoreMinorUpdates = (LOGICAL) ignore;

	return offset;
}

SINT32 authPackAPP_LAUNCH(PackMode mode, UCHAR* data, SINT32 offset, APP_LAUNCH* object, SINT32 /*size*/)
{
	if (PN_UNLIKELY(object == nullptr))
		return -EINVAL;

	offset = teulPackSINT32(mode,data,offset,&object->appID,0);
	offset = teulPackSINT32(mode,data,offset,&object->appVersion,0);
	offset = teulPackSINT32(mode,data,offset,&object->dataSize,0);

	offset = teulPackVoid(mode,data,offset,object->data,object->dataSize);
	object->data[object->dataSize] = '\0';
	offset = teulPackUINT32(mode,data,offset,&object->serverIP,0);
	offset = teulPackUINT32(mode,data,offset,&object->serverPort,0);
	offset = teulPackUINT32(mode,data,offset,(UINT32*)&object->protocol,0);
	offset = teulPackUINT32(mode,data,offset,&object->otp,0);
	offset = teulPackSINT32(mode,data,offset,&object->status,0);
	offset = teulPackVoid(mode,data,offset,&object->appleID,4);

	offset = teulPackUINT32(mode,data,offset,(UINT32*)&object->action,0);
	offset = teulPackSINT32(mode,data,offset,&object->dlAppID,0);

	return offset;
}

#if platBuild == BUILD_SERVER

SINT32 authPackSERVER_LOAD(PackMode mode, UCHAR* data, SINT32 offset, AUTH_SERVER_LOAD* object, SINT32 /*size*/)
{
	if (PN_UNLIKELY(object == nullptr))
		return -EINVAL;

	offset = teulPackSINT32(mode,data,offset,&object->numPlayers,0);
	offset = teulPackUINT32(mode,data,offset,&object->hostAddress,0);
	offset = teulPackUINT32(mode,data,offset,&object->arenaID,0);

	return offset;
}

void
Auth::Dispatch::packer(const unsigned int mode, ACTION_OBJECT* const a)
{
	a->packer(mode, &m_customerID, 0);
	a->bytePacker(mode, m_callsign, sizeof(m_callsign));
	a->packer(mode, &m_clientIP, 0);
	a->packer(mode, &m_OTP, 0);
	a->packer(mode, &m_appVersion, 0);
	a->packer(mode, &m_platform, 0);
}

SINT32 authPackGAME_INIT(PackMode mode, UCHAR* data, SINT32 offset, AUTH_GAME_INIT*init, SINT32 /*size*/)
{
	if (PN_UNLIKELY(init == nullptr))
		return 0;

	offset = teulPackUINT32(mode,data,offset,&(init->arenaID),0);
	offset = teulPackUINT32(mode,data,offset,&(init->hostVersion),0);
	offset = teulPackUINT32(mode,data,offset,&(init->appID),0);
	offset = teulPackUINT32(mode,data,offset,&(init->appVersion),0);
	offset = teulPackUINT32(mode,data,offset,&(init->maxUsers),0);
	offset = teulPackUINT32(mode,data,offset,&(init->port),0);
	offset = teulPackUINT32(mode,data,offset,&(init->ipAddress),0);

	offset = teulPackVoid(mode, data, offset, init->hello, 16);

	return offset;
}

//This function packs/unpacks the hello acknowledgment
SINT32 authPackGAME_ACK(PackMode mode, UCHAR* data, SINT32 offset, AUTH_GAME_ACK* object, SINT32 /*size*/)
{
	return teulPackUINT32(mode,data,offset,&(object->gotcha),0);
}

//Packs the init login message
SINT32 authPackClearLoginTable(PackMode mode, UCHAR* data, SINT32 offset, CLEAR_LOGIN_TABLE* object, SINT32 /*size*/)
{
	return teulPackUINT32(mode,data,offset,&(object->init),0);
}

#endif
