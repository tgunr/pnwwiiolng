#include <syscfg.h> // Path is project specific.
// Date: Thu May 10 11:16:07 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "teul/teulBasic.h"

#include "dbse/dbseConstants.h"
#include "mm/mmdbHostMessages.h"
#include "map/mapPlayer.h"
#include "mm/mmdbPack.h"
#include "mm/mmPack.h"
#include "mm/mmdbTypes.h"
#include "mm/mmdbParser.h"
#include "miss/missionExtMission.h"
#include "wwii/wwiiHCUnits.h"
#include "map/mapBrigadeSummary.h"
#include "inst/instHostInstance.h"
#include "wwii/wwiiCallbacks.h"

static TEUL_CALLBACK*	_XCHG_BRIGADE_DETAILS ;

//////////////////////////////////////////////////////////////////////
// Private, receive functions

#include "versionNo.h"
#if defined(BETA_RELEASE) && defined(HEAVY_DEBUG)
# define REPORT() dbugLog(_FLD, "%s", __FUNCTION__)
#else
# define REPORT()
#endif

static SINT32
_recvBrigadeDetailsRequest(ACTION_OBJECT* const a)
{
	REPORT() ;

	UINT32 prior[2] ;
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &prior[0], 0) ;
	a->offset = teulPackUINT32(TEUL_UNPACK, a->packed, a->offset, &prior[1], 0) ;
	return mmdbBrigadeDetailsRequest(a->teulIndex, prior) ;
}

//////////////////////////////////////////////////////////////////////
// Private, send functions

static SINT32
_sendBrigadeDetailsResponse(ACTION_OBJECT* const a)
{
	BrigadeDetailsUpdate* bdu = (BrigadeDetailsUpdate*)(a->localRawData) ;
	return bdu->packer(TEUL_PACK, a) ;
}

//////////////////////////////////////////////////////////////////////
// Public functions

Player::~Player()
{
}

void Player::ExitBrigade(unsigned int exitState)
{
	wwiiPlayer::ExitBrigade(exitState) ;
}

void Player::SetBranch(BRANCH_ENUM newBranch)
{
	m_branch = newBranch ;
}

bool Player::SetInstance(const Instance::Descriptor& toInstance)
{
	return wwiiPlayer::SetInstance(toInstance) ;
}

void
HCUnit::setHomeCPID(const FULL_OBJ_ID& newHome)
{
	m_homeCPID = newHome ;
}

SINT32
mmdbSendBrigadeDetails(SINT32 teulIndex, BrigadeDetailsUpdate& bdu)
{
	return teulSend(teulIndex, _XCHG_BRIGADE_DETAILS, &bdu) ;
}

//////////////////////////////////////////////////////////////////////
// Setup the callbacks

SINT32
mmdbInstallHostCallbacks()
{
	BEGIN_CALLBACK_REGISTRATION(MM_DBSE_X)

		REGISTER_SENDRECV(MMDB_XCHG_BRIGADE_DETAILS_Y, _sendBrigadeDetailsResponse, _recvBrigadeDetailsRequest, _XCHG_BRIGADE_DETAILS) ;

	END_CALLBACK_REGISTRATION
}
