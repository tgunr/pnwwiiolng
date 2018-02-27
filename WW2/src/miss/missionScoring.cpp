#include <syscfg.h> // Path is project specific.
// Created: Thu Jul  7 15:19:58 CDT 2005
//
// Algorithms for scoring a mission
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "map/mapPlayer.h"
#include "miss/missHitArrayNode.h"
#include "miss/missionPacket.h"
#include "miss/missionTypes.h"
#include "weap/weapWeapon.h"
#include "wwii/wwiiConfig.h"
#include "wwii/wwiiRTBCodes.h"
#include "score/scoreSortie.h"

//////////////////////////////////////////////////////////////////////
// Calculate the rtbmodifier for a given rtb code

static FLOAT32
_rtbMod(UCHAR rtbCode)
{
	switch ((CODE_RTB)rtbCode)
	{
	case RTB_SUCCESS: 		return 1.00f;
	case RTB_BAIL_FRIENDLY: return 0.90f;
	case RTB_BAIL_ENEMY: 	return 0.75f;
	case RTB_DIED: default: return 0.50f;
	}
}

//////////////////////////////////////////////////////////////////////
// Determine the value of bonuses achieved

static FLOAT32
_bonusScore(const Player* player)
{
	const MissionPacket& mp = player->missionPacket();

	FLOAT32 bonuses = 0.0f;

	const auto& scoring = mp.scoring();

	player->log(_FLD, "m:%u ce:%u, nms:%u, ais:%u, res:%u, bh:%u, fh:%u, br:%u, fb:%u, gkill:%u, ckill:%u, guard:%u, ums:%u"
					, player->missionID()
					, scoring.captures()
					, scoring.spawnees()
					, scoring.aiKills()
					, scoring.resuppliesScored()
					, scoring.bridgeKills()
					, scoring.factoryKills()
					, scoring.bridgeRepairs()
					, scoring.firebaseKills()
					, scoring.guardingKills()
					, scoring.cappingKills()
					, scoring.guardingPoints()
					, scoring.umsKills()
					);

	bonuses += (FLOAT32)scoring.captures() * host_config.score.capture;
	bonuses += (FLOAT32)scoring.spawnees() * host_config.sto.mobileSpawneeValue;

	bonuses += (FLOAT32)scoring.aiKills() * host_config.score.destroyAI;
	bonuses += (FLOAT32)scoring.resuppliesScored() * host_config.score.infResupplier;
	bonuses += (FLOAT32)scoring.umsKills() * host_config.score.destroyFMS;

	bonuses += scoring.cappingKills() * host_config.score.capperKillBonus;
	bonuses += scoring.guardingKills() * host_config.score.guardingKillBonus;
	bonuses += (FLOAT32)(scoring.guardingPoints());

	FLOAT32 heavyDamageBonus = 0;
	UCHAR wCategory = player->weaponSpec().wCategory;

	if ( scoring.bridgeKills() > 0 )
		bonuses += host_config.score.damageBridge;

	if ( scoring.factoryKills() > 0 )
		bonuses += host_config.score.damageFactory;

	if ( scoring.bridgeRepairs() > 0 )
		bonuses += host_config.score.repairBridge;

	if ( scoring.firebaseKills() > 0 )
		bonuses += host_config.score.damageFB;

	// Naval bonus for every 20 minutes if you survive 30+ minutes
	static const int BONUS_TIME = 20 * 60;
	if ( wCategory == _SEA_VEHICLE && mp.successfulDuration() && mp.duration_s() > 30 * 60 )
	{
		const FLOAT32 navalBonus = (FLOAT32)((mp.duration_s()) / BONUS_TIME);
		bonuses += navalBonus;
	}

	if ( mp.m_missionType == RESUPPLY && mp.wasSuccessful == true )
		bonuses += host_config.score.resupply;

	return bonuses;
}

UINT32
Player::calculateSortieScore()
{
	UINT32 uniqueHits = 0;
	UINT32 vehtypesHit = 0;
	if ( m_sortie != nullptr )
	{
		uniqueHits = m_sortie->m_sortiesHit.size();
		vehtypesHit = m_sortie->m_typesHit.size();
	}

	missionPacket().scoring().uniqueHits = uniqueHits;
	missionPacket().scoring().vehtypesHit = vehtypesHit;

	const FLOAT32 bonusScore = _bonusScore(this);
	const FLOAT32 rtbMod = _rtbMod(missionPacket().vehicle().rtbState());
	const UINT32& totalKills = missionPacket().scoring().kills();
	const FLOAT32 killScore = missionPacket().m_killScore;
	const FLOAT32 score = std::max((killScore + bonusScore) * rtbMod, 0.0f);

	dbugLog(_FLD, "kills:%u, killScore:%f, bonusscore:%f, rtbMod:%f, score:%f, uniquehits:%u vehtypesHit:%u"
					, totalKills, killScore, bonusScore, rtbMod, score, uniqueHits, vehtypesHit
					);

	// Round up the scores by this much
	static const FLOAT32 SCORE_ROUNDING_FACTOR = 0.5f;
	return (UINT32)(score + SCORE_ROUNDING_FACTOR);
}

