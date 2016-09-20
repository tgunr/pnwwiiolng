#pragma once
// Created: Sun Apr 17 16:33:51 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
//! @file teul/teulNetworkObject.h
//! Provides teul_network_object template for automating
//! simple bare-bones object based message passing.

#include "wwii/wwiiNetPorts.h"

//! Implements all of the TeulKit overhead for
//! exchanging structs/classes which implement the
//! standard packer() function.
//!
//! Instances are aware of their own callback and provide both
//! a send and receive functions.
//!
//!	Usage:
//!	 typedef teul_network_object<object type> mytype;
//!
//!	If you want the object to handle incoming messages, you will need
//!	to declare
//!	 SINT32 mytype::receiver(SINT32 teulIndex, mytype& incoming)
//! @tparam Tp Type of data being encapsulated.
//! @see teul_network_msg
//! @see teul_network_array
//! @see teul_empty_obj
//! @see teul_void_obj
template<typename _Type>
class teul_network_object
{
private:
	//! Short-hand for the type being encapsulated.
	typedef _Type valuetype;
	//! Short-hand for self-references.
	typedef teul_network_object<valuetype> selftype;

public:
	//! ACTION indicates the type of message being sent.
	enum ACTION
	{
		STUB_ONLY	//!< No data - just owner/flags - can be used to request data.
	,	EXCHANGE	//!< Message includes data.
	,	FAILED		//!< Message indicates an error.
	};

	//! Returns a singleton reference to a callback for the template instance.
	//! @return Reference to the TEUL_CALLBACK registered to your instance of this template.
	static TEUL_CALLBACK*& callback() { static TEUL_CALLBACK* s_callback; return s_callback; }

	//! Required if you will be receiving arrays of this type.
	//! @param[in] teulIndex		TeulIndex of the connection message was received from.
	//! @param[in] incoming			Reference to the unpacked message being received.
	//! @return Negative to indicate error otherwise >= 0
	static SINT32 receive(SINT32 teulIndex, selftype&);

public:
	//! Default/action-specific constructor.
	//! Defaults to action=EXCHANGE (sending data)
	//! @param[in] withAction		The action type for this message, defaults to EXCHANGE
	teul_network_object(ACTION withAction=EXCHANGE)
		: m_action(withAction)
		{}

	//! Constructor with data.
	//! @param[in] withData			Data to be used in the message (not copied)
	//! @param[in] withAction 		Action for this message (defaults to EXCHANGE)
	teul_network_object(const valuetype& withData, ACTION withAction=EXCHANGE)
		: m_action(withAction), m_data(withData)
		{}

	ACTION action() const { return m_action; }
	const valuetype& data() const { return m_data; }
	valuetype& data() { return m_data; }

public:
	ACTION		m_action;			//!< Action type.
	valuetype	m_data;			//!< The actual data.

public:
	//! Send a request for data.
	//! @param[in] teulIndex		Connection request is to be sent on.
	SINT32 request(SINT32 teulIndex) { return send(teulIndex, STUB_ONLY); }

	//! Send the data we currently have.
	//! @param[in] teulIndex		Connection data should be sent on.
	SINT32 send(SINT32 teulIndex) { return teulSend(teulIndex, callback(), this); }

	//! Send the data we currently have.
	//! @param[in] endpoint		Connection data should be sent on.
	SINT32 send(ENDPOINT& endpoint) { return teulSend(endpoint, callback(), this); }

	//! If the caller doesn't know the teulIndex but the send function might
	SINT32 send();

	//! Send with a particular action.
	//! @param[in] teulIndex		Connection data should be sent on.
	//! @param[in] withAction		ACTION to assign the message.
	SINT32 send(SINT32 teulIndex, ACTION withAction)
	{
		m_action = withAction;
		return teulSend(teulIndex, callback(), this);
	}

	//! Send with a particular action.
	//! @param[in] endpoint		Connection data should be sent on.
	//! @param[in] withAction		ACTION to assign the message.
	SINT32 send(ENDPOINT& endpoint, ACTION withAction)
	{
		m_action = withAction;
		return teulSend(endpoint, callback(), this);
	}

	//! Host only: Send via a broadcast network
	//! @param[in]	broadcastIndex	TeulIndex of the broadcast network
	//! @param[in]	directAddr		IP of the destination host (or 0)
	SINT32 broadcast(SINT32 broadcastIndex, SINT32 directAddr=0)
	{
#if platBuild == BUILD_SERVER
		if ( directAddr == 0 || directAddr == -1 )
			return teulSend(broadcastIndex, callback(), this);
		else
			return teulSendTo(broadcastIndex, directAddr, WWII::Net::Ports::PrivateBcast, callback(), this);
#else
//		Write_Log(LOG_COMM, LOG_ERR, "broadcast not supported");
		return -EINVAL;
#endif
	}

	//! Provides a receive function for the Callback registration.
	//! @param[in] a ACTION_OBJECT describing the incoming message.
	static SINT32 callbackRecv(ACTION_OBJECT* const a)
	{
		selftype incoming;
		try
		{
			if ( incoming.packer(TEUL_UNPACK, a) < 0 ) return -EINVAL;
			SINT32 retval = receive(a->teulIndex, incoming);
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
		selftype* const outgoing = (selftype*)a->localRawData;
		return outgoing->packer(TEUL_PACK, a);
	}

private:
	//! Pack/Unpack data.
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a)
	{
		// Pack the head information
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_action, 8);
		if ( m_action == EXCHANGE )
			a->offset = m_data.packer(mode, a);

		return a->offset;
	}
};

//! Helper: Implements a zero-size message with a dummy packer.
//! Not sure why this isn't in its own include file?
//! @see teul_network_object
//! @see teul_empty_msg
//! @see teul_void_obj
class teul_empty_obj
{
public:	SINT32 packer(LOGICAL /*mode*/, ACTION_OBJECT* const a) { return a->offset; }
};

//! Helper: Implements a raw-data message with a self-packer.
//! Not sure why this isn't in its own include file?
//! @param Tp Specifies the object being encapsulated.
//! @see teul_network_object
//! @see teul_empty_obj
template<typename _Tp>
class teul_void_obj : public _Tp
{
public:
	teul_void_obj() : _Tp() {}
	teul_void_obj(ACTION_OBJECT* const a) : _Tp() { packer(TEUL_UNPACK, a); }
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a) { return teulPackVoid(mode, a->packed, a->offset, this, sizeof(*this)); }
};
