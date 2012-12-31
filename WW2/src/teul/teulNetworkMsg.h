#pragma once
// Created: Sun Apr 17 16:33:51 CDT 2005
//
// Copyright (C) PlayNet, Inc 1999-2005 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//
//! @file teul/teulNetworkMsg.h
//! Provides teul_network_msg template for automating
//! simple object-based messages.

#include "wwii/wwiiNetPorts.h"

//! Implements all of the TeulKit overhead for
//! exchanging structs/classes which implement the
//! standard packer() function.
//! Each message includes a 64-bit "key" or "owner" field
//! and a 32-bit flags field.
//!
//! Instances are aware of their own callback and provide both
//! send and receive function.
//!
//! Usage:
//!  typedef teul_network_msg<object type> mytype ;
//!
//! If you want the object to handle incoming messages, you will need
//! to declare
//!  SINT32 mytype::receiver(SINT32 teulIndex, mytype& incoming)
//! @tparam Tp Type of data being encapsulated.
//! @see teul_network_object
//! @see teul_network_array
//! @see teul_empty_obj
//! @see teul_void_obj
template<typename _Type>
class teul_network_msg
{
private:
	//! Short-hand for the type being encapsulated.
	typedef _Type valuetype ;
	//! Short-hand for self-references.
	typedef teul_network_msg<valuetype> selftype ;

public:
	//! ACTION indicates the type of message being sent.
	enum ACTION
	{
		STUB_ONLY	//!< No data - just owner/flags - can be used to request data.
	,	EXCHANGE	//!< Message includes data.
	,	FAILED		//!< Message indicates an error.
	} ;

	//! Returns a singleton reference to a callback for the template instance.
	static TEUL_CALLBACK*& callback() { static TEUL_CALLBACK* s_callback ; return s_callback ; }

	//! Required if you will be receiving arrays of this type.
	static SINT32 receive(SINT32, selftype&) ;

public:
	//! Constructor without data.
	teul_network_msg(UINT64 withKey, UINT32 withFlags, ACTION withAction=STUB_ONLY)
		: m_key(withKey), m_flags(withFlags), m_action(withAction), m_data()
		{}

	//! Constructor with data.
	teul_network_msg(const valuetype& withData, UINT64 withKey=0LL, UINT32 withFlags=0, ACTION withAction=EXCHANGE)
		: m_key(withKey), m_flags(withFlags), m_action(withAction), m_data(withData)
		{}

	//! Default constructor.
	teul_network_msg()
		: m_key(0LL), m_flags(0), m_action(FAILED), m_data()
		{}

	ACTION action() const { return m_action ; }
	UINT64 key() const { return m_key ; }
	UINT32 flags() const { return m_flags ; }
	const valuetype& data() const { return m_data ; }
	valuetype& data() { return m_data ; }

public:
	UINT64		m_key ;				//!< Owner/Key identifier field.
	UINT32		m_flags ;			//!< User-defined flags field.
	ACTION		m_action ;			//!< Action type.
	valuetype	m_data ;			//!< The actual data.

public:
	//! Send a request
	//! @param[in] teulIndex		Connection request is to be sent on.
	SINT32 request(SINT32 teulIndex) { return send(teulIndex, STUB_ONLY) ; }

	//! Request a particular value
	//! @param[in] teulIndex		Connection the request should be sent on.
	//! @param[in] withKey			User-defined key value for the query.
	SINT32 request(SINT32 teulIndex, UINT64 withKey) { m_key = withKey ; return send(teulIndex, STUB_ONLY) ; }

	//! Send the data we currentl have.
	//! @param[in] teulIndex		Connection data should be sent on.
	SINT32 send(SINT32 teulIndex) { return teulSend(teulIndex, callback(), this) ; }

	//! Send the data we currentl have.
	//! @param[in] endpoint		Connection data should be sent on.
	SINT32 send(ENDPOINT& endpoint) { return teulSend(endpoint, callback(), this) ; }

	//! If the caller doesn't know the teulIndex but the send function might
	SINT32 send() ;

	//! Send with a particular action.
	//! @param[in] teulIndex		Connection data should be sent on.
	//! @param[in] withAction		ACTION to assign the message.
	SINT32 send(SINT32 teulIndex, ACTION withAction)
	{
		m_action = withAction ;
		return teulSend(teulIndex, callback(), this) ;
	}

	//! Send with a particular action.
	//! @param[in] endpoint		Connection data should be sent on.
	//! @param[in] withAction		ACTION to assign the message.
	SINT32 send(ENDPOINT& endpoint, ACTION withAction)
	{
		m_action = withAction ;
		return teulSend(endpoint, callback(), this) ;
	}

	//! Host only: Send via a broadcast network
	//! @param[in]	broadcastIndex	TeulIndex of the broadcast network
	//! @param[in]	directAddr		IP of the destination host (or 0)
	SINT32 broadcast(SINT32 broadcastIndex, SINT32 directAddr=0)
	{
#if platBuild == TEUL_SERVER
		if ( directAddr == 0 || directAddr == -1 )
			return teulSend(broadcastIndex, callback(), this) ;
		else
			return teulSendTo(broadcastIndex, directAddr, WWII::Net::Ports::PrivateBcast, callback(), this) ;
#else
//		Write_Log(LOG_COMM, LOG_ERR, "broadcast not supported") ;
		return -EINVAL ;
#endif
	}

	//! Provides a receive function for the Callback registration.
	//! @param[in] a ACTION_OBJECT describing the incoming message.
	static SINT32 callbackRecv(ACTION_OBJECT* const a)
	{
		selftype incoming ;
		try
		{
			if ( incoming.packer(TEUL_UNPACK, a) < 0 ) return -EINVAL ;
			SINT32 retval = receive(a->teulIndex, incoming) ;
			if ( retval < 0 )
				return retval ;
			return a->offset ;
		}
		catch (std::exception& e)
        {
            dbugLog(_FLA, "%s: %s", __FUNCTION__, e.what()) ;
            return -EINVAL ;
        }
	}

	//! Provides a sender function for the Callback registration.
	//! @param[in] a ACTION_OBJECT describing the outgoing message.
	static SINT32 callbackSend(ACTION_OBJECT* const a)
	{
		selftype* const outgoing = (selftype*)a->localRawData ;
		return outgoing->packer(TEUL_PACK, a) ;
	}

private:
	//! Pack/Unpack data.
	SINT32 packer(LOGICAL mode, ACTION_OBJECT* const a)
	{
		// Pack the head information
		a->offset = teulPackUINT32(mode, a->packed, a->offset, (UINT32*)&m_action, 8) ;
		a->offset = teulPackUINT64(mode, a->packed, a->offset, &m_key, 0) ;
		a->offset = teulPackUINT32(mode, a->packed, a->offset, &m_flags, 0) ;
		if ( m_action == EXCHANGE )
			a->offset = m_data.packer(mode, a) ;

		return a->offset ;
	}
} ;

//! Helper: Implements a zero-size message with a dummy packer.
//! @see teul_network_msg
//! @see teul_empty_obj
class teul_empty_msg
{
public:
	teul_empty_msg() {}
	teul_empty_msg(ACTION_OBJECT* const a) { packer(TEUL_UNPACK, a) ; }
	SINT32 packer(LOGICAL /*mode*/, ACTION_OBJECT* const a) { return a->offset ; }
} ;
