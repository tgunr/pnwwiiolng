#pragma once
// Miscellaneous utility functions.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

namespace Teul
{
#if platOS != OS_WIN32
	//! Mac and Linux signal handler. Not sure if it exists for windows.
	//! @param[in] signum	Signal to specify handler for.
	//! @param[in] handler	New signal-handler function receiving signum as argument.
	//! @returns 0 on success, otherwise error code.
	int SetSignalHandler(int signum, void (*handler)(int));
#endif

#if platBuild == TEUL_SERVER
	//! Determine if the specified IP address is one of my public addresses or loopback.
	bool IsMyAddress(UINT32 addr_);
#endif

	//! Get the current state of a given teulIndex.
	//! In case of an error, *outReason will be one of:
	//!	 -EINVAL: teulIndex was illegal.
	//!  -ENOENT: teulIndex does not correspond to an endpoint.
	//!  -ECONABORTED: connection has been closed.
	//!  -ENOTCONN: connection is not connected.
	//! @param[in] teulIndex The connection to query.
	//! @param[out] outReason 0 on a successful lookup, otherwise an error code describing the failure.
	//! @param[out] lastRx_ms time in milliseconds since the last packet was received.
	//! @returns true on a successful query, false if there is a problem with the teulIndex.
	bool GetEndpointStatus(const SINT32 teulIndex, SINT32* const outReason, TeulDeltaMS* const outLastRx_ms);

	//! Convert a host address and port number into host name (dotted decimal notation) and port number strings.
	//! Primary purpose is to simplify future transitions to IPv6. bufLen must be large enough
	//! to accommodate the trailing null.
	//! @param[in] host_ value of the address to query.
	//! @param[in] port_ value of the port number to query.
	//! @param[in] hostBuf_ destination for the host name (dotted numeric) or NULL if not required.
	//! @param[in] hostBufLen_ size of the host name buffer (including trailing zero).
	//! @param[in] portBuf_ destination for the port number or NULL if not required.
	//! @param[in] portBufLen_ size of the port buffer (including trailing zero).
	int HostToAddr(const UINT32 host_, const UINT16 port_, char* const hostBuf_, const size_t hostBufLen_, char* const portBuf_, const size_t portBufLen_);

	//! Convert an internet address into host name (dotted decimal notation) and port number strings.
	//! Primary purpose is to simplify future transitions to IPv6. bufLen must be large enough
	//! to accomodate the trailing null.
	//! @param[in] host_ value of the address to query.
	//! @param[in] port_ value of the port number to query.
	//! @param[in] hostBuf_ destination for the host name (dotted numeric) or NULL if not required.
	//! @param[in] hostBufLen_ size of the host name buffer (including trailing zero).
	//! @param[in] portBuf_ destination for the port number or NULL if not required.
	//! @param[in] portBufLen_ size of the port buffer (including trailing zero).
	int	HostToAddr(const struct sockaddr_in* const sa, char* const hostBuf_, size_t hostBufLen_, char* const portBuf_, size_t portBufLen_);
}
