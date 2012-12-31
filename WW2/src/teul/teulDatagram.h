#pragma once
// Copyright (C) PlayNet Inc, 2011 - All Rights Reserved.
// Not for redistribution.
//! @struct DATA_GRAM
//! @brief TeulKit datagram (payload) encapsulation.
//! @details Summarizes a teulKit payload: i.e. the bytes and the size.

#include "teul/teulSockTypes.h"

namespace Teul
{
	struct PacketHeader ;
}

struct DATA_GRAM
{
	struct sockaddr_in	sockAddress;
	Teul::PacketHeader*	contents ;	//!< Points to the beginning of the packet data.
	UINT32				m_size ;	//!< Size of the entire object.
} ;
