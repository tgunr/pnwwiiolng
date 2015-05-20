#pragma once
// Created: Tue Nov 28 14:05:08 CST 2006
//
//! @file teul/teulBasic.h
//! Minimal teulKit include
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

struct ENDPOINT;

enum { TEUL_UNPACK, TEUL_PACK };

namespace Teul
{
	enum class Mode { Unpack, Pack };
	using LoggingFn = void(*)(const char* const, ...);
	extern LoggingFn g_logFnPtr;

	static const UINT32 INVALID_INDEX = ~0U	;		//!< Value used for "Invalid Index".

	// Get a textual id for an error (default is "error").
	const char* GetErrorName(SINT32 code);
	//! Get the local address/port of an endpoint.
	std::pair<UINT32, UINT16> GetLocal(SINT32 teulIndex);
	//! Get the remote address/port of an endpoint.
	std::pair<UINT32, UINT16> GetRemote(SINT32 teulIndex);

	//! Set the name of the current application.
	void SetAppName(const char* _name);
	//! Get the current application name.
	const char* GetAppName();

	//! Is the transport initialized?
	bool IsTransportInited();
	//! Is the transport closing?
	bool IsTransportClosing();

	//! Get the address of an endpoint, as long as it's connected/open.
	ENDPOINT* GetConnectedEndpoint(const SINT32 teulIndex);

	//! Look up a single IP address (host order) for a given name.
	//! Returns the first IP from the list returned by getaddrinfo.
	//! \return IP address in host order or zero.
	UINT32 GetSingleIP(const char* const name);

	//! Look up all IPs for a given hostname.
	bool GetIPs(const char* const name, std::vector<UINT32>& ips);
} // namespace Teul

//! Send data through a callback on a given connection.
//! Use after you have opened your connection and installed your callbacks.
//! @param[in]	teulIndex		Connection to send to.
//!	@param[in]	teulCallback	Callback structure (to determine send function and packer).
//!	@param[in]	data			Pointer to the data.
SINT32 teulSend(SINT32 teulIndex, struct TEUL_CALLBACK* const teulCallback, void* const data);
SINT32 teulSend(ENDPOINT& endpoint, struct TEUL_CALLBACK* const teulCallback, void* const data);

//! Close a local connection. will not work until its opened.
void teulClose(const SINT32 teulIndex, const UINT32 reason=0);
void teulClose(ENDPOINT&, const UINT32 reason=0);

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
//! @param[in]	waitVal		Sets a maximum number of milliseconds to continue
//!							processing. A value of 0 indicates no wait
//!							(non-blocking), a negative value specifies
//!							wait forever.
//! @ingroup not-thread-safe
void teulUpdateTransport(const TeulTimeMS& duration_=0ULL);

//! Get the local address of a given teul index.
//! @param[in]	teulIndex		Index of the connection to query.
//! @return Teul::INVALID_INDEX if the index is not valid.
UINT32 teulGetEndpointLocalAddress(SINT32 teulIndex);
//! Get the remote address of a given teul index.
//! @param[in]	teulIndex		Index of the connection to query.
//! @return Teul::INVALID_INDEX if the index is not valid.
UINT32 teulGetEndpointRemoteAddress(SINT32 teulIndex);

// close all connections and the debugger. (teul go bye bye)
void teulCloseTransport(void);

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

void teulSetNetworkMask(UINT32 address);

UINT32 teulMaskNetworkAddress(UINT32 address);

// if you detect an error in your data handler use this function to forward it to
// the callbacks error handler. mode == TEUL_PACK or TEUL_UNPACK for whether
// the error is detected in the send or recv CALLBACK.
SINT32 teulPostCallbackError(const SINT32 teulIndex
									, const UCHAR* const data, const SINT32 size
									, const struct TEUL_CALLBACK* const object, const SINT32 error);

//! Send an error code to the process at the remote end of a connection.
void teulSendError(const SINT32 teulIndex, const SINT32 errorCode);

ENDPOINT* teulGetEndpoint(const SINT32 index);
