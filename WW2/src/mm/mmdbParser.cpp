#include <syscfg.h> // Path is project specific.
// Date: Thu May 10 11:15:31 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Eric "Meat" Hall

#include "versionNo.h"
#include "dbug/dbug.h"

#include "chat/chatHandle.h"
#include "dbse/dbseConstants.h"
#include "dbse/dbseConnection.h"
#include "dbse/dbseHandle.h"
#include "map/mapBrigadeSummary.h"
#include "map/mapPlayer.h"
#include "miss/missHitArrayNode.h"
#include "mm/mmdbHostMessages.h"
#include "mm/mmdbPack.h"
#include "mm/mmdbParser.h"
#include "mm/mmdbQueries.h"
#include "mm/mmdbTypes.h"
#include "include/mydbQuery.h"
#include "wwii/wwiiHCUnits.h"
#include "wwii/wwiiThreads.h"

extern DBHANDLE dbh ;

void
Player::SetRank(RANK_NO level)
{
	m_realRank = level ;
}

//////////////////////////////////////////////////////////////////////
// Send updated brigade details to the maphost

static const char* QRY_SELECT_BRIGADE_DETAILS =
	"SELECT shu.unit_id"
		", UNIX_TIMESTAMP(shu.modified)"
		", shu.title"
		", shu.motd"
		", shu.motto"
		", shu.nick"
		", home.cp_ox"
		", home.cp_oy"
		", home.cp_id"
		", owner.playerid"
		", IF(owner.isplaynet,UPPER(owner.callsign),owner.callsign)"
		", ownerp.rankid"
	" FROM strat_hc_units shu"
		" LEFT JOIN strat_cp home ON (shu.home_cp_oid = home.cp_oid)"
		" LEFT OUTER JOIN wwii_player owner ON (shu.owner_id = owner.playerid)"
			" LEFT OUTER JOIN wwii_persona ownerp ON (shu.owner_id = ownerp.playerid"
													" and shu.country_id = ownerp.countryid"
													" and shu.branch_id = ownerp.branchid)"
	" WHERE (UNIX_TIMESTAMP(shu.modified) = %u and unit_id > %u) or (UNIX_TIMESTAMP(shu.modified) > %u)"
	" ORDER BY 2, 1"
	" LIMIT 16"
;

SINT32
mmdbBrigadeDetailsRequest(SINT32 teulIndex, void* object)
{
	UINT32* prior = (UINT32*)object ;

	try
	{
		DBQuery q(dbh, QRY_SELECT_BRIGADE_DETAILS, prior[0], prior[1], prior[0]) ;

		if ( q.Rows() == 0 )
			return 0 ;

		BrigadeDetailsUpdate bdu ;

		bdu.m_owner = chatGetHandle("SYSTEM") ;
		bdu.m_ownerRank = 13 ;

		while (q.FetchRow())
		{
			const DBRow& row = q.Row() ;
			char ownerHandle[MAX_HANDLE_LEN+1] ;

			bdu.m_brigadeID = row.NextUInt() ;
			bdu.m_lastModified = row.NextUInt() ;
			row.NextString(bdu.m_title, sizeof(bdu.m_title)) ;
			row.NextString(bdu.m_motd, sizeof(bdu.m_motd)) ;
			row.NextString(bdu.m_motto, sizeof(bdu.m_motto)) ;
			row.NextString(bdu.m_shortName, sizeof(bdu.m_shortName)) ;
			bdu.m_homeCPID.cell.ox = (SINT16)row.NextSInt() ;
			bdu.m_homeCPID.cell.oy = (SINT16)row.NextSInt() ;
			bdu.m_homeCPID.id = (UINT16)row.NextUInt() ;
			bdu.m_ownerID = row.NextUInt() ;
			row.NextString(ownerHandle, sizeof(ownerHandle)) ;
			bdu.m_ownerRank = (UINT8)row.NextUInt() ;

			bdu.m_owner = chatGetHandle(ownerHandle) ;

			mmdbSendBrigadeDetails(teulIndex, bdu) ;
		}
	}
	catch (std::exception& e) { dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ; }

	return 0 ;
}

//////////////////////////////////////////////////////////////////////
// Player function

void
Player::SetCountry(WWII_COUNTRY newCountry, WWII_SIDE newSide)
{
	wwiiPlayer::SetCountry(newCountry, newSide) ;
}

