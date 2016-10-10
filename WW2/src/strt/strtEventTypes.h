#pragma once

#include "wwii/wwiiFullObjID.h"
#if defined(_CELL_HOST_)
#include <list>
#endif
#include "teul/teulPackers.h"
#include "wwii/wwiiPack.h"
#include "include/inplace_vector.h"

namespace Strat
{

//////////////////////////////////////////////////////////////////////
/// CP and Facility Event messages.

class Event
{
public:
	// Determine the number of events likely to fit into a packet.
	enum	{ MaxPerPacket = 375 };	// Should fit comfortably inside an average packet.
private:
	FULL_OBJ_ID		m_fid;
	UINT8			m_ownership;
	bool			m_largeFlags;
	UINT32			m_flags;

public:
	const FULL_OBJ_ID& fid() const { return m_fid; }
	UINT8 ownership() const { return m_ownership; }
	UINT32 flags() const { return m_flags; }

public:
	Event() : m_fid(), m_ownership(0), m_largeFlags(false), m_flags(0) {}
	Event(const FULL_OBJ_ID& _fid, WWII_COUNTRY _ownership, UINT32 _flags)
		: m_fid(_fid), m_ownership(_ownership)
		, m_largeFlags(_flags >= (1 << 3))	// More than 3 bits for flags?
		, m_flags(_flags)
		{}
	Event(ACTION_OBJECT* const a) : m_fid(), m_ownership(0), m_largeFlags(false), m_flags(0)
	{
		packer(TEUL_UNPACK, a);
	}

	void packer(PackMode mode, ACTION_OBJECT* const a)
	{
		a->offset = wwiiPackFullObjID(mode, a->packed, a->offset, &m_fid, 0);
		a->packer(mode, m_ownership, 4);
		a->packer(mode, m_largeFlags);
		a->packer(mode, m_flags, (m_largeFlags ? 31 : 3));
	}

	friend class EventPacket;
};

//////////////////////////////////////////////////////////////////////
/// Batch of Strat::Event updates.

class EventPacket
{
private:
	typedef WWII::inplace_vector<Strat::Event, Strat::Event::MaxPerPacket> Events;
	Events m_events;

public:
	/// Standard constructor.
	EventPacket() : m_events() {}
	virtual ~EventPacket() {}

	/// Network constructor.
	EventPacket(ACTION_OBJECT* const a) : m_events() {  packer(TEUL_UNPACK, a); }

	/// Get the number of events in this packet.
	size_t size() const { return m_events.size(); }

	/// Get event number #n.
	const Strat::Event& operator[](size_t n) const { return m_events[n]; }

	/// Get the events vector.
	const Events& events() const { return m_events; }
	Events& events() { return m_events; }

	void packer(PackMode mode, ACTION_OBJECT* const a)
	{
		UINT32 count = m_events.size();
		a->packer(mode, count, 16);
		if ( mode == TEUL_UNPACK )
			m_events.resize(count);

		SINT16 curScx = 0, curScy = 0;

		for ( UINT32 i = 0; i < count; ++i )
		{
			Event& cur = m_events[i];
			FULL_OBJ_ID& curFid = cur.m_fid;

			SINT16 scx = curFid.cell.ox >> 4;
			SINT16 scy = curFid.cell.oy >> 4;
			bool sameCell = (scx == curScx && scy == curScy);
			a->packer(mode, sameCell);

			a->packer(mode, curFid.id, 11);

			if ( sameCell == false )
			{
				if ( mode == TEUL_PACK )
				{
					curScx = scx;
					curScy = scy;
				}
				a->packer(mode, curScx, 10);
				a->packer(mode, curScy, 10);
			}

			if ( mode == TEUL_UNPACK )
			{
				curFid.cell.ox = curScx << 4;
				curFid.cell.oy = curScy << 4;
			}

			a->packer(mode, cur.m_ownership, 4);
			a->packer(mode, cur.m_largeFlags);
			a->packer(mode, cur.m_flags, (cur.m_largeFlags ? 31 : 3));
		}
	}
};

} // namespace Strat

