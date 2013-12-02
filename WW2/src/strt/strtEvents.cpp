#include <syscfg.h> // Path is project specific.
// Created: Thu Dec  9 17:40:31 CST 2010
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer: Oliver "kfs1" Smith

#include "host/host.h"
#include "host/hostPlayer.h"
#include "teul/teulTransmission.h"
#include "strt/strtEvents.h"
#include "strt/strtEventTypes.h"

#include "strt/strtCPStruct.h"
#include "strt/strtFacilityStruct.h"
#include "strt/strtObjectStruct.h"

TEUL_CALLBACK* Strat::Events::Callback;

std::vector<CPStruct*> cps;
std::vector<FacilityStruct*> facs;

//////////////////////////////////////////////////////////////////////
// EventPacket-derived class that provides Send and Add functionality.

class UpdatePacket : public Strat::EventPacket
{
public:
	UpdatePacket() : EventPacket() {}

	// Function for sending.
	virtual void Send() = 0;

	// Function for adding a packet with overflow handling.
	void Add(const Strat::Event& newEvent)
	{
		events().push_back(newEvent);
		if ( size() >= Strat::Event::MaxPerPacket )
		{
			Send();
		}
	}
};

//////////////////////////////////////////////////////////////////////
// Derived class that sends broadcasts of packets.

class BroadcastPacket : public UpdatePacket
{
	public: BroadcastPacket(PLAYERS& playerList) : UpdatePacket(), m_playerList(playerList) {}

	private: struct InstanceFilterFn
	{
		bool operator () (wwiiPlayer* const player) const
		{
			return player->instance().isCampaign();
		}
	};

	public: virtual void Send()
	{
		dbugLog(_FLD, "transmitting EventPacket with %" PRSizeT " events to %" PRSizeT " players"
						, size(), m_playerList.size());

		TeulTransmission broadcast(Strat::Events::Callback);

		broadcast.populate(this);
		broadcast.send(m_playerList.begin(), m_playerList.end(), InstanceFilterFn());

		events().clear();
	}

	public: PLAYERS& m_playerList;
};

//////////////////////////////////////////////////////////////////////
// Derived class that sends packets to individual players.

class DirectPacket : public UpdatePacket
{
	public: DirectPacket(SINT32 teulIndex) : UpdatePacket(), m_teulIndex(teulIndex) {}

	public: virtual void Send()
	{
#ifdef HEAVY_DEBUG
		dbugLog(_FLD, "t:%d transmitting EventPacket with %u events", m_teulIndex, events().size());
#endif
		const SINT32 result = teulSend(m_teulIndex, Strat::Events::Callback, this);
#ifdef HEAVY_DEBUG
		if ( g_logDebug )
			dbugLog(_FLDB, " result = %d", result);
#endif
		if ( result < 0 )
			dbugLog(_FLW, "t:%d err:%d while sending strat event packet", m_teulIndex, result);

		events().clear();
	}

	private:	SINT32	m_teulIndex;
};

//////////////////////////////////////////////////////////////////////
// Functions for add a CP/Facility/Object to the event queue.
//
// Updates are batched together and transmitted periodically. Only
// the latest data for any given entity is sent.
//

template<typename _Type>
static void
_addEvent(std::vector<_Type*>& events, _Type* const source)
{
	if ( source->modified )
		return;
	events.push_back(source);
	source->modified = true;
}

void
Strat::Events::Add(CPStruct* const cp)
{
	_addEvent(cps, cp);
}

void
Strat::Events::Add(FacilityStruct* const fac)
{
	_addEvent(facs, fac);
}

extern void mapUpdateBuildingVisLevel(const ObjectStruct* const obj);

void
Strat::Events::Add(ObjectStruct* const obj)
{
//	_addEvent(objs, obj);
	mapUpdateBuildingVisLevel(obj);
}

//////////////////////////////////////////////////////////////////////
// Iterate over the updates for a given event source and build
// EventPackets from the updates. When the EventPacket becomes full,
// broadcast that packet to all players.

template<typename _Type>
static void
_addUpdatesToPacket(BroadcastPacket& packet, std::vector<_Type*>& events)
{
	const size_t numEvents = events.size();
	for ( size_t i = 0; i < numEvents; ++i )
	{
		_Type* const source = events[i];
		packet.Add(Strat::Event(source->fullObjID, source->country, source->flags));

		source->modified = false;
	}

	events.clear();
}

//////////////////////////////////////////////////////////////////////
// Broadcast top-level strat events to all players.

extern void mapSendDirtySupercellData();

void
Strat::Events::Publish()
{
	const size_t numCPs = cps.size();
	const size_t numFacs = facs.size();
	const size_t numUpdates = numCPs + numFacs;

	if ( numUpdates > 0 )
	{
		BroadcastPacket packet(g_players);

		dbugLog(_FLD, "Strat::Events::Publish %" PRSizeT " cps, %" PRSizeT " facs, %" PRSizeT " total events"
						, numCPs, numFacs, numUpdates
						);

		if ( g_players.empty() == false )
		{
			_addUpdatesToPacket(packet, cps);
			_addUpdatesToPacket(packet, facs);

			if ( packet.size() > 0 )
				packet.Send();
		}
		else
		{
			for ( auto it = cps.begin(); it != cps.end(); ++it )
				(*it)->modified = false;
			for ( auto it = facs.begin(); it != facs.end(); ++it )
				(*it)->modified = false;

			cps.clear();
			facs.clear();
		}
	}

	mapSendDirtySupercellData();
}

//////////////////////////////////////////////////////////////////////
// Send a player all CP states.

void
Strat::Events::SendMapState(SINT32 teulIndex, bool isCampaign, const CPStruct* const cpList)
{
	///TODO:INST: Batch these up and send one per second so we don't have to spend
	///TODO:INST: lots of time recreating them when 100 people log in together.
	dbugLog(_FLD, "t:%d SendMapState", teulIndex);
	DirectPacket packet(teulIndex);
	for ( const CPStruct* cp = cpList; cp != NULL; cp = cp->next )
	{
		const WWII_COUNTRY country = isCampaign ? cp->country : cp->originalCountry;
		const UINT32 flags = isCampaign ? cp->flags : ((cp->flags & (CP_STAT_HAS_NAVYBASE|CP_STAT_HAS_AIRBASE|CP_STAT_MAKES_RDP)) | CP_STAT_OPEN | CP_STAT_IN_SUPPLY);

		packet.Add(Strat::Event(cp->fullObjID, country, flags));

		for ( FacilityStruct* fac = cp->facilityList; fac != NULL; fac = fac->next )
		{
			if ( fac->country != cp->country || fac->flags != 0 )
				packet.Add(Strat::Event(fac->fullObjID, fac->country, fac->flags));
		}
	}

	if ( packet.size() > 0 )
		packet.Send();
}

//////////////////////////////////////////////////////////////////////
// Invoked by the callback sender.

SINT32
Strat::Events::SendFunction(ACTION_OBJECT* const a)
{
	EventPacket* const packet = (EventPacket*)(a->localRawData);
	packet->packer(TEUL_PACK, a);
	return a->offset;
}

