#pragma once
// Created: Fri Jan 15 16:19:05 CST 2010
//
// Describes the capture contribution type and interaces
//
// Copyright (C) PlayNet, Inc 1999-2010 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "strt/strtCaptureScore.h"

class FacilityStruct;
class HCUnit;

//tolua_begin
namespace Strat
{
	class CaptureContribution : public Strat::CaptureContributionEvent
	{
	public:
		CaptureContribution() : CaptureContributionEvent() {}
		CaptureContribution(Strat::CaptureContributionEvent::Status withStatus
							, FacilityStruct& forFacility, const ChatHandle forHandle, UINT32 forPlayerID, UINT32 forPlayerCountry, UINT32 forSortieID, UINT32 forMissionID, const HCUnit& forUnit);

		Strat::CaptureContributionEvent::Status		status() const { return m_status; }
		FacilityStruct&		facility() const { return *m_facility; }
		ChatHandle			handle() const { return m_handle; }
		UINT32				playerID() const { return m_playerID; }
		UINT32				playerCountry() const { return m_playerCountry; }
		UINT32				sortieID() const { return m_sortieID; }
		UINT32				missionID() const { return m_missionID; }
		const HCUnit&		unit() const { return *m_unit; }

	public: static void Queue(Strat::CaptureContributionEvent::Status forStatus, FacilityStruct& forFacility, const ChatHandle forHandle, UINT32 forPlayerID, UINT32 forPlayerCountry, UINT32 forSortieID, UINT32 forMissionID, const HCUnit& forUnit);

	// Check if the queue is empty
	public: static bool IsQueueEmpty();

	// Load the new capture contribution into this buffer.
	public: static Strat::CaptureContribution GetQueueFront();

	// Remove the top entry from the queue
	public: static void QueuePop();

	// Get the next active facility
	public: static FacilityStruct* GetNextActiveFacility(FacilityStruct* last);

//tolua_end
	private:
			FacilityStruct*	m_facility;
			const HCUnit*	m_unit;
//tolua_begin
	};
}

//tolua_end

