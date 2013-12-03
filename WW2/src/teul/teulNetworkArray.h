#pragma once
// Created: Sun Apr 17 16:33:51 CDT 2005
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
//! @file teul/teulNetworkArray.h
//! Provides teul_network_array template for automating
//! simple array-of-object based message passing.

// PS: Why network_array and not NETWORK_ARRAY or networkArray?
// I thought that "network_array" was more in keeping with
// STL naming conventions, and since this is intended to be a
// Standard Playnet Template...

#include "wwii/wwiiPack.h"

//! Implements all of the TeulKit overhead for sending
//! messages comprising an association ID ("owner"),
//! an optional flags field and a n-sized array of
//! objects which implement the standard teulKit
//! packer() function.
//! Instances are aware of their own callback and provide both
//! a send and receive functions.
//! Message-splitting by MTU size is handled automatically.
//!
//! Key functions:
//!	add(T) -- adds an entry
//!	complete() -- returns true if the list has finished network transfer
//!	send(teulIndex, [max packets]) -- sends the current list
//!	kill(teulIndex) -- kills the list, optionally sending to teulIndex
//!	request(teulIndex) -- request a list
//!
//! Usage:
//!  typedef network_array<object type> mytype;
//!	TEUL_CALLBACK* mtype::s_callback;
//!
//! If you want the object to handle incoming messages, you will need
//! to declare SINT32 mytype::receiver(SINT32 fromTeulIndex, mytype& incoming)
//!
//! @tparam Tp Type of object contained within the array.
//! @see teul_network_object
//! @see teul_network_msg
//! @see teul_empty_obj
//! @see teul_void_obj
template<typename Tp>
class teul_network_array
{
private:
	//! Short-hand for self-references.
	typedef teul_network_array<Tp> __self;

public:
	//! ACTION indicates the type of message being sent.
	enum ACTION
	{
		REQUEST			//!< A short packet requesting data,
	,	START			//!< First of possibly several packets,
	,	APPEND			//!< Continuation packet,
	,	KILL			//!< Tells recipient to ignore previous data.
	};

	//! Data elements are stored using an STL vector, instance::array_type is a short hand for the resulting type.
	typedef std::vector<Tp> array_type;

	//! Returns a singleton reference to a callback for the template instance.
	static TEUL_CALLBACK*& callback() { static TEUL_CALLBACK* s_callback = nullptr; return s_callback; }

	//! Required if you will be receiving arrays of this type.
	static SINT32 receive(SINT32 teulIndex, __self&);

private:
	UINT64				m_owner;		//!< 64-bit field to indicate source (e.g. a ChatHandle)
	UINT32				m_flags;		//!< 32-bit flags
	array_type			m_data;		//!< Actual data as std::vector
	ACTION				m_action;		//!< Action type.
	size_t				m_watermark;	//!< Last transmitted entry
	UINT32				m_mtu;			//!< MTU being used.

public:
	//! Default constructor
	//! @param[in] withOwner		64-bit association field (usually ChatHandle)
	//! @param[in] withFlags		32-bit user-defined flags field
	//! @param[in] withMTU			Use a custom MTU for this message type
	teul_network_array(UINT64 withOwner=0, UINT32 withFlags=0, UINT32 withMTU=522)
		: m_owner(withOwner), m_flags(withFlags)
		, m_action(KILL), m_watermark(0)
		, m_mtu(withMTU)
		{}

	//! Copy constructor
	teul_network_array(teul_network_array const& rhs)
		: m_owner(rhs.m_owner)
		, m_flags(rhs.m_flags)
		, m_data(rhs.m_data)
		, m_action(KILL), m_watermark(0)
		, m_mtu(rhs.m_mtu)
		{}

	//! Assignment operator
	teul_network_array& operator=(teul_network_array const& rhs)
	{
		m_owner = rhs.m_owner;
		m_flags = rhs.m_flags;
		m_data = rhs.m_data;
		m_action = rhs.m_action;
		m_watermark = 0;

		return *this;
	}

public:
	// Query functions
	ACTION action() const { return m_action; }
	SINT32 completed() const { return m_watermark >= size(); }
	const array_type& data() const { return m_data; }
	array_type& data() { return m_data; }
	const Tp& data(size_t i) const { return ( i < size() ) ? m_data[i] : m_data[size() - 1]; }
	bool empty() const { return m_data.empty(); }
	UINT32 flags() const { return m_flags; }
	UINT32 mtu() const { return m_mtu; }
	UINT64 owner() const { return m_owner; }
	size_t size() const { return (empty()) ? 0 : m_data.size(); }

public:
	// Modify functions
	UINT32 setFlags(UINT32 withFlags) { m_flags = withFlags; m_watermark = 0; return m_flags; }
	UINT32 setMTU(UINT32 withMTU) { m_mtu = withMTU; return m_mtu; }

public:
	//! Appends a value of type Tp to the end of the array.
	//! @param[in] value		Value to be added.
	void add(const Tp& value)
	{
		m_data.push_back(value);
	}

	//! Sends the current data to the recipient on the specified teulIndex.
	//! If packetLimit is non-zero, at most that many packets will be sent
	//! leaving the remaining data to be sent later.
	//! @param[in] teulIndex	Connection data should be sent to.
	//! @param[in] packetLimit	Maximum packets to send or 0.
	SINT32 send(SINT32 teulIndex, UINT32 packetLimit=0)
	{
		ENDPOINT* const endpoint = teulGetEndpoint(teulIndex);
		if ( PN_UNLIKELY(endpoint == nullptr) )
			return -ENOTCONN;
		return send(*endpoint, packetLimit);
	}

	//! Sends the current data to the recipient on the specified endpoint.
	//! If packetLimit is non-zero, at most that many packets will be sent
	//! leaving the remaining data to be sent later.
	//! @param[in] teulIndex	Connection data should be sent to.
	//! @param[in] packetLimit	Maximum packets to send or 0.
	SINT32 send(ENDPOINT& endpoint, UINT32 packetLimit=0)
	{
		SINT32 teulError = 0;
		UINT32 packets = 0;

		if ( m_watermark == 0 )
			m_action = START;
		else if ( m_watermark >= size() )
		{
			// Restart
			m_watermark = 0;
			m_action = START;
		}

		do
		{
			teulError = teulSend(endpoint, callback(), this);
			m_action = APPEND;
			packets++;
		}
		while ( teulError >= 0 && !completed() && (packetLimit == 0 || packets < packetLimit) );

		return teulError;
	}

	//! Provides a receive function for the Callback registration.
	//! @param[in] a ACTION_OBJECT describing the incoming message.
	static SINT32 callbackRecv(ACTION_OBJECT* const a)
	{
		__self incoming;
		try
		{
			incoming.packer(TEUL_UNPACK, a);
			SINT32 retval = receive(a->teulIndex, incoming);
			incoming.m_data.clear();
			if ( retval < 0 )
				return retval;
			return a->offset;
		}
		catch (std::exception& e) { dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()); return -EINVAL; }
	}

	//! Provides a sender function for the Callback registration.
	//! @param[in] a ACTION_OBJECT describing the outgoing message.
	static SINT32 callbackSend(ACTION_OBJECT* const a)
	{
		__self* outgoing = (__self*)a->localRawData;
		return outgoing->packer(TEUL_PACK, a);
	}

	//! Reset to default state.
	void clear()
	{
		m_data.clear();
		m_action = KILL;
		m_watermark = 0;
	}

	//! Destroy the list locally and if any data has been sent, tell the recipient to do the same.
	//! @return true if the message was sent, false otherwise.
	bool kill(SINT32 teulIndex)	// Tell both ends to destroy the list
	{
		clear();
		if ( teulIndex >= 0 )
			return (teulSend(teulIndex, callback(), this) >= 0);
		return false;
	}

	//! Request a particular data type over this callback.
	//! Since Callback registration is encapsulated within the
	//! template, it is possible for the template to generate a
	//! request for this type of data.
	//! withOwner and withFlags are optional, dependent on your
	//! specific implementation.
	//! Will require you to implement the receive() function above.
	bool request(SINT32 teulIndex, UINT64 withOwner=0, UINT32 withFlags=0)
	{
		m_action = REQUEST;
		m_owner = withOwner;
		m_flags = withFlags;
		if ( teulIndex >= 0 )
			return (teulSend(teulIndex, callback(), this) >= 0);
		return false;
	}

	//! Pack/Unpack data.
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* a)
	{
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_action, 8);
		if ( mode == TEUL_UNPACK )
		{
			m_data.clear();
			m_watermark = 0;
		}
		a->offset = teulPackUINT64(mode, a->packed, a->offset, &m_owner, 0);
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_flags, 0);

		switch (m_action)
		{
			case REQUEST:
			case KILL:
			break;

			case START:
			case APPEND:
				if ( mode == TEUL_UNPACK )
					a->offset = _unpackBody(a);
				else
					a->offset = _packBody(a);
			break;
		}

		return a->offset;
	}

private:
	//! The packer is a little fiddly. We don't know how many of these
	//! objects we can pack into the space remaining, and if the object
	//! has a 'flexible' packer of its own (e.g. text) then we can't
	//! make informed guesses about how many fit into a packet.
	//! So instead, we just pack until one of the packets goes over
	//! the MTU. Then we roll packet.
	//! Slightly wasteful, but it has advantages that pay for the wate.
	SINT32
	_packBody(ACTION_OBJECT* const a)
	{
		// Remember where we are packing the entry count so we can
		// write the final tally here when we know it
		SINT32 entryOffset = a->offset;

		UINT16 entries = 0;
		a->offset = teulPackUINT16(TEUL_PACK, a->packed, a->offset, &entries, 16);

		// Offset limit in bits (<<3 = *8)
		SINT32 offsetLimit = m_mtu << 3;
		SINT32 lastOffset = a->offset;

		for (; a->offset < offsetLimit && m_watermark < size(); ++m_watermark, lastOffset = a->offset )
		{
			a->offset = m_data[m_watermark].packer(TEUL_PACK, a);
			if ( a->offset >= offsetLimit )
			{
				a->offset = lastOffset;
				break;
			}
			++entries;
		}

		// 0 indicates not completed, 1 indicates end of list
		LOGICAL endOfRun = (m_watermark < size()) ? 0 : 1;

		// 15 bits for count - upto 32768 entries, and 1 bit for 'EOR'
		if ( endOfRun )
			entries |= 0x8000;
		entryOffset = teulPackUINT16(TEUL_PACK, a->packed, entryOffset, &entries, 16);

		return a->offset;
	}

	//! The unpacker, however, is a cakewalk :)
	//! m_watermark is only set when we receive "completed"
	SINT32
	_unpackBody(ACTION_OBJECT* const a)
	{
		UINT16 entries = 0;
		LOGICAL endOfRun = 0;
		a->offset = teulPackUINT16(TEUL_UNPACK, a->packed, a->offset, &entries, 16);
		if ( entries & 0x8000 )
		{
			entries &= 0x7fff;
			endOfRun = 1;
		}

		// Avoid lots of reallocs
		m_data.reserve(entries);

		// For each object we're receiving, push back a network constructor
		for ( UINT32 i = 0; a->offset > 0 && i < entries; ++i )
			m_data.push_back(a);

		// If successful, update the watermark on end of run so that
		// completed() returns true
		if ( a->offset > 0 && endOfRun )
			m_watermark = size();

		return a->offset;
	}
};
