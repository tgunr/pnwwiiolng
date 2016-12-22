#pragma once

#include <string>
#include "chat/chatHandle.h"
#include "chat/chatBrigadeControl.h"
#include "chat/chatBrigadeControlList.h"
#include "wwii/wwiiHCUnits.h"
#include "sdm/StratDB.h"
#include "client/GameClient.h"
#include "ui/uiMain.h"
#include "map/mapPersonaSummary.h"
#include "weap/weapList.h"
#include "ui/uiChat.h"

struct BrigadeControlDescriptor
{
	BrigadeControlDescriptor(const char* l, bool me = false) :
		label("brigade.control."),
		arg1(L"<undef1>"),
		arg2(L"<undef2>"),
		arg3(L"<undef3>"),
		arg4(L"<undef4>"),
		major_broadcast(false)
	{
		label += l;

		if(me)
			label += ".me";
	}

	void getBrigadeDescriptor(UINT32 brigadeID, std::wstring& into) const
	{
		static std::string brigadeName;
		brigadeName = "";

		const HCUnit* brigade = HCUnit::Lookup(brigadeID);
		if ( brigade )
			brigade->getDescriptor(brigadeName);

		into = uiUTF8toUTF16(brigadeName.c_str());
	}

	std::string label;
	std::wstring arg1;
	std::wstring arg2;
	std::wstring arg3;
	std::wstring arg4;

	bool major_broadcast;
};

struct BCD_OfficerBrigade : public BrigadeControlDescriptor
{
	BCD_OfficerBrigade(const BrigadeControlNotice* const bcn, const char* l, bool me = false) : BrigadeControlDescriptor(l, me)
	{
		if(me)
		{
			getBrigadeDescriptor(bcn->m_brigadeID, arg1);
		}
		else
		{
			arg1 = chatGetWideCallsign(bcn->m_officer);
			getBrigadeDescriptor(bcn->m_brigadeID, arg2);
		}
	}
};

struct BCD_OfficerSponsorBrigade : public BrigadeControlDescriptor
{
	BCD_OfficerSponsorBrigade(const BrigadeControlNotice* const bcn, const char* l, bool me = false) : BrigadeControlDescriptor(l, me)
	{
		if(me)
		{
			arg1 = chatGetWideCallsign(bcn->m_sponsor);
			getBrigadeDescriptor(bcn->m_brigadeID, arg2);

		}
		else
		{
			arg1 = chatGetWideCallsign(bcn->m_officer);
			arg2 = chatGetWideCallsign(bcn->m_sponsor);
			getBrigadeDescriptor(bcn->m_brigadeID, arg3);

		}
	}
};

struct BCD_SponsorOfficerBrigade : public BrigadeControlDescriptor
{
	BCD_SponsorOfficerBrigade(const BrigadeControlNotice* const bcn, const char* l, bool me = false) : BrigadeControlDescriptor(l, me)
	{
		if(me)
		{
			arg1 = chatGetWideCallsign(bcn->m_sponsor);
			getBrigadeDescriptor(bcn->m_brigadeID, arg2);
		}
		else
		{
			arg1 = chatGetWideCallsign(bcn->m_sponsor);
			arg2 = chatGetWideCallsign(bcn->m_officer);
			getBrigadeDescriptor(bcn->m_brigadeID, arg3);
		}
	}
};

struct BCD_SponsorBrigadeOriginTarget : public BrigadeControlDescriptor
{
	BCD_SponsorBrigadeOriginTarget(const BrigadeControlNotice* const bcn, const char* l, bool me = false) : BrigadeControlDescriptor(l, me)
	{
		StratDB::StratObject* origin = StratDB::FindStratObject(bcn->m_origin);
		StratDB::StratObject* target = StratDB::FindStratObject(bcn->m_target);

		arg1 = chatGetWideCallsign(bcn->m_sponsor);

		getBrigadeDescriptor(bcn->m_brigadeID, arg2);
		if(origin) arg3		= origin->DisplayName();
		if(target) arg4		= target->DisplayName();
	}
};

struct BCD_BrigadeOriginTarget : public BrigadeControlDescriptor
{
	BCD_BrigadeOriginTarget(const BrigadeControlNotice* const bcn, const char* l, bool me = false) : BrigadeControlDescriptor(l, me)
	{
		StratDB::StratObject* origin = StratDB::FindStratObject(bcn->m_origin);
		StratDB::StratObject* target = StratDB::FindStratObject(bcn->m_target);

		getBrigadeDescriptor(bcn->m_brigadeID, arg1);
		if(origin) arg2		= origin->DisplayName();
		if(target) arg3		= target->DisplayName();
	}
};

struct BCD_Objective : public BrigadeControlDescriptor
{
	BCD_Objective(const BrigadeControlNotice* const bcn, const char* l, bool me = false) : BrigadeControlDescriptor(l, me)
	{
		WWII_SIDE mySide				= wwiiSide(PlayerClient()->PersonaSelected().m_countryID);
		BrigadeControlList* oic			= PlayerClient()->LookupBrigadeOIC(bcn->m_brigadeID);
		StratDB::StratObject* target	= StratDB::FindStratObject(bcn->m_target);

		arg1 = (mySide == ALLIED) ? uiLocString("brigade.control.objective.allied"):
									 uiLocString("brigade.control.objective.axis");

		switch(bcn->m_action)
		{
			case BrigadeControlNotice::REQ_ATTACK:
			case BrigadeControlNotice::REQ_DEFEND:
			{
				label += (bcn->m_action == BrigadeControlNotice::REQ_ATTACK) ? ".attack": ".defend";
				if ( target && target->StratType() == SDM_CP_BRIDGE ) {
					label += ".bridge";
				} else {
					label += ".cp";
					if ( strcmp(l, "objective") != 0 ) // HACK: Proposal/veto messages aren't major.
						major_broadcast = true;
				}
			}
			break;
			case BrigadeControlNotice::REQ_CLEAR:
			{
				label += ".clear";
			}
			break;
		}

		if(target) arg2 = target->DisplayName();
		getBrigadeDescriptor(bcn->m_brigadeID, arg3);
		if(bcn->m_sponsor) arg4 = chatGetWideCallsign(bcn->m_sponsor);
		else if(oic && oic->m_officer) arg4 = chatGetWideCallsign(oic->m_officer);
	}
};

struct BCD_ControlError : public BrigadeControlDescriptor
{
	BCD_ControlError(const BrigadeControlNotice* const bcn, const char* l)
		: BrigadeControlDescriptor(l, false)
	{
		if ( bcn->isTargettedAction() )
		{
			StratDB::StratObject* object;
			object = StratDB::FindStratObject(bcn->m_target);
			if ( object )
				arg1 = object->DisplayName();
			object = StratDB::FindStratObject(bcn->m_origin);
			if ( object )
				arg3 = object->DisplayName();
		}
		else
		{
			arg1 = (bcn->m_officer) ? chatGetWideCallsign(bcn->m_officer) : L"<null>";
		}
		getBrigadeDescriptor(bcn->m_brigadeID, arg2);

	}
};
