#pragma once
// Created: Sun Jan 13 15:53:36 CST 2008
//
// Chat::TuneToChannel structure
//
// Copyright (C) PlayNet, Inc 1999-2008 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "chat/chatChannelTypes.h"

namespace Chat
{
	//Tunes a specific radioIndex to a channel
	struct TuneToChannel
	{
		TuneToChannel() : m_radioIndex(0), m_type(static_cast<CHANNEL_TYPE>(0)), m_handle(0ULL), m_rx(false), m_tx(false) {}
		TuneToChannel(UCHAR radioIndex, CHANNEL_TYPE type, const ChatHandle& handle, bool rx, bool tx)
			: m_radioIndex(radioIndex)
			, m_type(type)
			, m_handle(handle)
			, m_rx(rx)
			, m_tx(tx)
			{}

		UCHAR			m_radioIndex ;		// Which radio is being tuned
		CHANNEL_TYPE	m_type ;			// Type of channel being tuned
		ChatHandle		m_handle ;			// Packed handle of the channel's name
		bool			m_rx ;				// Is receive enabled?
		bool			m_tx ;				// Is transmit enabled?

		// Common members
		SINT32 packer(LOGICAL mode, ACTION_OBJECT* a) ;

		// Host functions
		bool send(SINT32 connection) ;
	} ;
}
