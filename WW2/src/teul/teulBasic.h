#pragma once
// Created: Tue Nov 28 14:05:08 CST 2006
//
//! Minimal teulKit include
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#include "teulEnums.h"

struct ENDPOINT;

enum { TEUL_UNPACK, TEUL_PACK };

namespace Teul
{
	enum class Mode { Unpack, Pack };

	static const UINT32 INVALID_INDEX = ~0U	;		//!< Value used for "Invalid Index".

	// Get a textual id for an error (default is "error").
	const char* GetErrorName(SINT32 code);

	//! Set the name of the current application.
	void SetAppName(const char* _name);
	//! Get the current application name.
	const char* GetAppName();

	//! Initialize Winsock networking if present.
	void InitializeWinsock();

	//! Is the transport initialized?
	bool IsTransportInited();
	//! Is the transport closing?
	bool IsTransportClosing();

	//! Look up a single IP address (host order) for a given name.
	//! Returns the first IP from the list returned by getaddrinfo.
	//! \return IP address in host order or zero.
	UINT32 GetSingleIP(const char* const name);
} // namespace Teul

//! Send data through a callback on a given connection.
//! Use after you have opened your connection and installed your callbacks.
//! @param[in]	teulIndex		Connection to send to.
//!	@param[in]	teulCallback	Callback structure (to determine send function and packer).
//!	@param[in]	data			Pointer to the data.
SINT32 teulSend(SINT32 teulIndex, struct TEUL_CALLBACK* const teulCallback, void* const data, const UCHAR protocol=PROTOCOL_UNDEF);
SINT32 teulSend(ENDPOINT& endpoint, struct TEUL_CALLBACK* const teulCallback, void* const data, const UCHAR protocol=PROTOCOL_UNDEF);

//! Close a local connection. will not work until its opened.
void teulClose(const SINT32 teulIndex);
void teulClose(ENDPOINT&);

//! Converts a doted quad string to a UINT32 host address.
//! @param[in]	address			IP address to convert.
//!	@param[out]	host			Pointer to where the address should be stored.
//! @return false if address is invalid.
bool teulGetHost(const char* address, UINT32* host);

//! Converts a binary network address to a string.
//! @deprecated Use thread-safe version instead.
//! @ingroup not-thread-safe
//! @param[in]	host			Binary IP address.
//!	@return pointer to static buffer with address as a string.
const char* teulGetAddress(const UINT32 host);

//! Thread safe conversion of a network address to a string.
//! @ingroup thread-safe
//! @param[in]	host			Binary IP address.
//!	@param[out]	buffer			Pointer to output buffer.
//!	@param[in]	bufferSize		Size of buffer.
//! @return pointer to buffer (for compatibility with old version)
const char* teulGetAddress(const UINT32 host, char* buffer, const size_t bufferSize);

//! Perform core teul operations such as draining reads and writes on
//! sockets, processing incoming callbacks and timing out connections.
//! Since we have to ensure that callbacks happen in step with whatever
//! application is using teul, we have to make a call for that all to happen.
//! should be called once per game loop.
//! @param[in]	duration	Sets a maximum number of milliseconds to continue
//!							processing. A value of 0 indicates no wait
//!							(non-blocking), a negative value specifies
//!							wait forever.
//! @ingroup not-thread-safe
void teulUpdateTransport(const TeulTimeMS duration=0ULL);

// close all connections and the debugger. (teul go bye bye)
void teulCloseTransport();

//! Associate local application data with a connection.
void teulSetPersonaData(const SINT32 teulIndex, void* personaData);

//! Returns the persona data for a connection, may be nullptr.
void* teulGetPersonaData(const SINT32 teulIndex);

//! Return a pointer to a specific connection data type, or null.
template<typename ObjType_>
ObjType_* teulGetConnectionData(SINT32 teulIndex_)
{
	return static_cast<ObjType_*>(teulGetPersonaData(teulIndex_));
}

//! Send an error code to the process at the remote end of a connection.
void teulSendError(const SINT32 teulIndex, const SINT32 errorCode);

ENDPOINT* teulGetEndpoint(const SINT32 index);
#ifndef TEUL_PACKET_NAMES
#if defined(DEBUG_WW2) || (platBuild == BUILD_SERVER && defined(DEBUG))
#define TEUL_PACKET_NAMES 1
#else
#define TEUL_PACKET_NAMES 0
#endif
#endif
