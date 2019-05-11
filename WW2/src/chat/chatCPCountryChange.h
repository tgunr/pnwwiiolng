#include "syscfg.h" // Path is project specific.
#include "teul/teulBasic.h"
#include "teul/teulPackers.h"
#include "wwii/wwiiFullObjID.h"

#pragma once
enum VOTE_PERMIT { VP_NONE, VP_ALL, VP_OFFICER, VP_COMMANDER };

UINT32 delayTime = 90;

class CPController {
	void voteFor(Vote::Mode mode, BrigadeControl::BRIGADE_ACTION_REQ action, const ChatHandle nominee, const ChatHandle sponsor, UINT32 duration, const FULL_OBJ_ID origin = nullObjID);

};

void
chatCPCountryChangeProposal(FULL_OBJ_ID cp, WWII_COUNTRY toCountry, ChatHandle byHandle);


class CPCountryChangeNotice
{
public:

	enum Responses {
		_ERROR, _PROPOSAL, VETOED, _REMOVED
	};

	CPCountryChangeNotice(UINT32 notice = _ERROR)
		: m_notice(notice), m_error(0), m_time(delayTime), m_votePermit(VP_NONE), m_received(0) {}
	CPCountryChangeNotice(UINT32 notice, UINT32 error, VOTE_PERMIT vote_permit)
		: m_notice(notice), m_error(error), m_time(delayTime), m_votePermit(vote_permit), m_received(0) {}

public:
	//// CLIENT: These functions are essentially entry points
	//// for data coming back from the host
	void processResponse();
	void processNotice();
	//BrigadeControlDescriptor getDescription() const;

	bool isTargetedAction() {
		return true;
	}

	void
	VoteFor(Vote::Mode mode, CPCountryChangeNotice::Responses, const ChatHandle nominee, const ChatHandle sponsor, const FULL_OBJ_ID origin = nullObjID);

	// OICbrigade = id of the brigade you are OICing or 0
	// amCommander = true if you are a high command staffer
	bool isVetoable(UINT32 oicBrigade, bool amCommander) const
	{
		switch (m_votePermit)
		{
		case VP_ALL: return true;
			break;
		case VP_COMMANDER: return amCommander;
			break;
		case VP_OFFICER: return (oicBrigade > 0 || amCommander);
			break;
		default:
			break;
		}
		return false;
	}

public:
	UINT32		m_notice;		// Message code
	UINT32		m_error;		// If m_notice is _ERROR, this is populate with an LStringID
	UINT32		m_time;			// How-long (in sec) before a delayed effect takes place (e.g. nomination)
	UINT32		m_votePermit;	// See VOTE_PERMIT enums
	SINT32		m_received;		// point in time to base m_time off of

	static TEUL_CALLBACK* s_callback;

public:
	SINT32 packer(PackMode mode, ACTION_OBJECT* a);

	bool send(SINT32 teulIndex) { return (teulSend(teulIndex, s_callback, this) >= 0); }

};
