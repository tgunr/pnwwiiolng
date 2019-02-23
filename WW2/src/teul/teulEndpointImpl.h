#pragma once
// Copyright (C) PlayNet Inc, 2011 - All Rights Reserved.
// Not for redistribution.
//
// Local Info for an Endpoint.

//#include <queue>
#include "wwii/wwiiCryptographer.h"
#include <queue>

namespace Teul { class Packet; }

//! @struct LI_ENDPOINT
//! @brief Local Info extension to struct ENDPOINT.

struct EndpointImpl
{
	//! Purge any existing values.
	void Reset();

	//! Specify the slot within the poll array.
	void setPollSlot(nfds_t newSlot_) { m_pollSlot = newSlot_; }

	//! Retrieve the current poll slot.
	nfds_t pollSlot() const { return m_pollSlot; }

	//! Return the first packet in the send queue to the free list.
	void freeFirstPacketInSendQueue();

	// Encryption properties.
	WWII::Cryptographer<true>	m_txCrypt;		//!< Sending encryptor.
	WWII::Cryptographer<false>	m_rxCrypt;		//!< Receiving decryptor.
	TeulTimeMS					m_lastTickRx;	//!< When we received the last tick.

	std::deque<struct DATA_GRAM *> m_recvQueue;
	std::deque<Teul::Packet*> m_sendQueue;

	//! Buffer of data currently being read from socket.
	struct
	DATA_GRAM*		m_curDatagram;

//	struct event*	m_readEvt;				//!< libEvent read event.
//	struct event*	m_writeEvt;			//!< libEvent write event.

	Teul::PacketHeader::tag_t recvTag;				//!< Last packet received.
	Teul::PacketHeader::tag_t sentTag;				//!< Last packet sent.

	UINT16			m_version;				//!< Currently negotiated version.
	UINT16			m_cryptVersion;			//!< Version to use for encryption.
	nfds_t			m_pollSlot;				//!< Position within the "poll" array under Linux/MacOS.
	TeulTimeMS		kliveInterval;			//!< Time between keep alives.
	Teul::PacketHeader::tick_t		m_lastPacketTick;		//!< Tick of the previous packet.
};

