#include <syscfg.h> // Path is project specific.
//
// Date: Wed Sep 21 13:56:35 CDT 2005
//
// Functions for manipulation chatWho objects
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "chat/chatData.h"
#include "chat/chatPlayer.h"
#include "chat/chatPreferenceEnum.h"
#include "chat/chatWho.h"

//////////////////////////////////////////////////////////////////////
// Populate a chatWho with the view of <target> from <player>

void
chatWhoPopulate(
		Chat::Who& who
		, const Player* player
		, const Player* target
		, bool forceExtendedInfo
		)
{
	bool isInMySquad = target->inSquad(player);
	bool isOnMyMission = (player->missionID() && player->missionID() == target->missionID());
	bool isRecruiting = (isInMySquad) ? target->getPreference(CPR_IS_RECRUITING) : false;

	who.setBasics(target->chatHandle(), true, isInMySquad, target->squadRank(), target->squadHandle(), target->joinedSquad());

	who.setPrimaryDetails(target->isAFK()
							, target->isActiveCommander()
							, target->country()
							, target->branch()
							, target->effectiveRank()
							, target->brigadeID()
							, target->missionID()
							, target->isMissionLeader()
							, isRecruiting
							);

	if ( forceExtendedInfo || isInMySquad || isOnMyMission )
	{
		FULL_OBJ_ID location = nullObjID;
		if ( target->lastCP() )
			location = target->lastCP()->Fid();

		who.setExtendedDetails(target->isSpawned(), location);
	}
}

