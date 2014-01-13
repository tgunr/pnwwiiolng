#pragma once

#include "server/serverReleaseTypeEnum.h"
typedef SINT32 (PLAYER_RELEASE_FUNCTION) ( SINT32 teulIndex, UINT32 customerID, UINT32 otp,  Server::ReleaseType releaseType );

SINT32 authInitAccept(UINT32 appVersion
					, PLAYER_ACCEPT_FUNCTION* reloginAcceptFn
					, PLAYER_RELEASE_FUNCTION* releaseFn
					, ERROR_FUNCTION* externErrorFn);

bool authRecordDispatch(Auth::Dispatch* dispatch, bool fromAuth);

SINT32 authDeleteDispatch(Auth::Dispatch* dispatch);

void authExpirePendingOtps();

