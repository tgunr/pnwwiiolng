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
	void pollSlot(SINT16 newSlot_) { m_pollSlot = newSlot_; }

	//! Retrieve the current poll slot.
	SINT16 pollSlot() const { return m_pollSlot; }

	//! Return the first packet in the send queue to the free list.
	void FreeFirstPacketInSendQueue();

	// Encryption properties.
	WWII::Cryptographer<true>	m_txCrypt;		//!< Sending encryptor.
	WWII::Cryptographer<false>	m_rxCrypt;		//!< Receiving decryptor.
	TeulTimeMS					m_lastTickRx;	//!< When we received the last tick.

	using DatagramQueue = std::vector<struct DATA_GRAM*>;
	DatagramQueue	recvQueue;
	std::deque<Teul::Packet*> m_sendQueue;

	//! Buffer of data currently being read from socket.
	struct
	DATA_GRAM*		m_curDatagram;

//	struct event*	m_readEvt;				//!< libEvent read event.
//	struct event*	m_writeEvt;			//!< libEvent write event.

	UCHAR			recvTag;				//!< Last packet received.
	UCHAR			sentTag;				//!< Last packet sent.

	UINT16			m_version;				//!< Currently negotiated version.
	UINT16			m_cryptVersion;			//!< Version to use for encryption.
	SINT16			m_pollSlot;				//!< Position within the "poll" array under Linux/MacOS.
	UINT16			kliveInterval;			//!< Time between keep alives.
	UINT16			m_lastPacketTick;		//!< Tick of the previous packet.
};

