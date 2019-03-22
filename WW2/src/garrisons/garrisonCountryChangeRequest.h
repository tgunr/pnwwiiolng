#pragma once


#include "teul/teulNetworkMsg.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "teul/teulPackers.h"
#include "teul/teulBasic.h"
#include "systypes.h"
#include "wwii/wwiiHCUnits.h"

enum GARRISON_COUNTRY_CHANGE_RESULT
{
	GCR_APPROVED				// 0
	, GCR_FAILED_OWNER_ALREADY
	, GCR_FAILED_TOO_MANY
	, GCR_FAILED_SUPPLY
	, GCR_FAILED_WRONG_OWNER
	, GCR_FAILED_HAS_OBJECTIVE
	, GCR_FAILED

};

class GarrisonCountryChangeReq
{
public:
	GarrisonCountryChangeReq() : m_result(GCR_FAILED){}
	GarrisonCountryChangeReq(ACTION_OBJECT* const a) { a->offset = packer(TEUL_UNPACK, a); }

	// Network exchange functions
	SINT32 send(SINT32 teulIndex, FULL_OBJ_ID cp, WWII_COUNTRY const& country);
	SINT32 sendToClient(SINT32 teulIndex);
	SINT32 sendToMap(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

#if platBuild == BUILD_CLIENT
	bool   requestChangeCountry(SINT32 teulIndex, const FULL_OBJ_ID cp, WWII_COUNTRY const& country);
#endif // platBuild == BUILD_CLIENT

SINT32 packer(PackMode mode, ACTION_OBJECT* a);

public:
	GARRISON_COUNTRY_CHANGE_RESULT m_result;
	FULL_OBJ_ID	                   m_cp;
	WWII_COUNTRY                   m_country;
	SINT32                         m_clientsTeulIndex;

public:
	/*
		A garrison should not always change to the requested country value, in some cases
		we'll substitute another based on unit availability.
	*/
	static WWII_COUNTRY getReplacementCountry(WWII_COUNTRY requested, HCUnit * unit)
	{
		WWII_COUNTRY retVal = requested;
		BRANCH_ENUM unitBranch = unit->branch();

		// HACK! I really don't like doing this so we need to (long term) include this as part of
		// a country handling overall which generalizes operations based on rules.  For example
		// in this case we'd want a general way of identifying fallback countries from a provided
		// country in a way that can be configured.  We should also be generally checking for missing
		// brigade definitions as opposed to simply assuming its AIR & SEA.  This is a very quick way
		// of achieving the goal so it stands for 1.36
		if (requested == WWII_USA && (unitBranch == BRANCH_AIR || unitBranch == BRANCH_SEA))
		{
			retVal = WWII_ENGLAND;
		} 

		return retVal;
	}
};
