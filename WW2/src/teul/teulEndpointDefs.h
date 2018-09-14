#pragma once
// Created: Sat Mar 11 18:29:36 CST 2006
//
// Definitions for teulEndpoint.h extracted so that they can
// be accessed without including half of teulKit
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

namespace Teul
{
	enum { TeulHeaderBytes = 3 + 1 + 2 + 1 + 1 };	// Sizeof Teul::PacketHeader.
	enum { IPHeaderBytes = 20, TCPHeaderBytes = 20, UDPHeaderBytes = 10 };
	enum { OverheadUDP = IPHeaderBytes + UDPHeaderBytes + TeulHeaderBytes };
	enum { OverheadTCP = IPHeaderBytes + TCPHeaderBytes + TeulHeaderBytes };

	//! The absolute maximum size of a packet frame (using Teul::Packet).
	enum { MaxAllowedPacketSize_bytes = (1 << 20) - 1 };
	//! The maximum size of a Teul::Packet payload.
	enum { MaxAllowedPayload_bytes = MaxAllowedPacketSize_bytes - 11 };

	enum { MaxPacket = 1500 };		//!< Maximum allowable static datagram buffer size.

	enum { MaxMTU = 1400 };		//!< Maximum size of packets including headers.
	enum { InternetMTU = 1254 };	//!< Standard MTU for internet connections.

	enum { TCPMaxPayload = MaxMTU - OverheadTCP };
	enum { TCPStdPayload = InternetMTU - OverheadTCP };
	enum { UDPMaxPayload = MaxMTU - OverheadUDP };
	enum { UDPStdPayload = InternetMTU - OverheadUDP };
	enum { UDBMaxPayload = MaxMTU - OverheadUDP };
	enum { MaxPackBytes = UDBMaxPayload };
	enum { MaxPackBits = MaxPackBytes * 8 };

	enum { MaxEndpointTimeout = 60000 };
	enum { KlivesPerTimeout = 3 };

	enum { MinKliveNeeded = 1000 };
	enum { MaxKliveNeeded = 10000 };
	enum { EndpointUpdateInterval = 1000 };

	enum { MinEndpointTimeout = MinKliveNeeded * KlivesPerTimeout };
	enum { AnyIndex = -1001 };

	enum { MaxEndpointSendErrors = 128 };

	enum { BufferSize = 16384 };

	namespace Tokens
	{
		//! 4-bit markers marker used to indicate start of a teul frame.
		enum
		{
			Encrypted			=	9		// 1001
		,	Unencrypted			=	6		// 0110
		};
	}
}

