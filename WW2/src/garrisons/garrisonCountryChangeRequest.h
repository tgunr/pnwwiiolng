#pragma once


#include "teul/teulNetworkMsg.h"
#include "wwii/wwiiFullObjID.h"
#include "wwii/wwiiOwnershipEnums.h"
#include "teul/teulPackers.h"
#include "teul/teulBasic.h"
#include "systypes.h"

enum GARRISON_COUNTRY_CHANGE_RESULT
{
	GCR_APPROVED				// 0
	, GCR_REQUESTING
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
	GarrisonCountryChangeReq(ACTION_OBJECT* const a) { a->offset = GarrisonCountryChangeReq::packer(TEUL_UNPACK, a); }

	// Network exchange functions
	SINT32 send(SINT32 teulIndex, FULL_OBJ_ID cp, WWII_COUNTRY const& country);
	SINT32 sendToClient(SINT32 teulIndex);
	SINT32 sendToMap(SINT32 teulIndex);
	SINT32 sendToStrat(SINT32 teulIndex);
	SINT32 receive(SINT32 teulIndex);

#if platBuild == BUILD_CLIENT
	bool   requestChangeCountry(SINT32 teulIndex, const FULL_OBJ_ID cp, WWII_COUNTRY const& country);
#endif // platBuild == BUILD_CLIENT

SINT32 packer(PackMode mode, ACTION_OBJECT* a);
	void strtProcessRequest();

public:
	GARRISON_COUNTRY_CHANGE_RESULT m_result;
	FULL_OBJ_ID	                   m_cp;
	WWII_COUNTRY                   m_country;
	SINT32                         m_clientsTeulIndex;
};
