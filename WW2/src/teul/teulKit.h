#pragma once
// This module provides high-level access to the transport
// functions used in the teul API.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin
//
//! @namespace Teul
//! @brief TeulKit namespace.
//! @details TeulKit is PlayNet Inc's proprietary networking/callback stack.
namespace Teul { }

#include "teul/teulEnums.h"
#include "teul/teulBasic.h"
#include "teul/teulFuncTypes.h"

//	initializes all variables required by the teulTransport system.
//	flags  indicate the dbug error level (refer to dbug.h) and future options
//	logType is the log type specified from dbug.h, always TYPE_FILE on non Unix
//	name -  filename  logType is a file, process name for Unix syslog
//	confirmFnPtr is called to confirm file inbound file transfers
//	errorFnPtr is for the user error routine, use null if not required.
//	killFnPtr is the function that is called for cleanup when teul encounters
//	an unrecoverable error.
SINT32 teulInitTransport(SINT32 logType
						, const char* name
						, ERROR_FUNCTION* errorFnPtr
						, void (*killFnPtr)(int /*signo*/)
						, Teul::LoggingFn* const logFnPtr = nullptr
						);

//	there are 48 local connections "built in" to the teul system, and each
//	is referred to by number (starting with 0).

// opens a port [portNumber] for listening. Assumes that all callbacks have been properly
// installed.
// [acceptFnPtr] is an optional ptr to callback function which is called when
// a connection is made to the listening socket, and hence a new connection
// is created.  pass nullptr if you do not want this notification
// [timeout] is a timeout for this connection in ms for this connection (0 uses default).
// [protocol] is the protocol for this connection (TCP, UDP, etc).
// returns the teulIndex associated with this connection.
SINT32 teulOpenListen(const UINT16 portNumber
					, CONNECT_FUNCTION* acceptFnPtr
					, const SINT32 timeout, const PROTOCOL protocol
					, ERROR_FUNCTION* errorFnPtr
					);

//! \brief Opens ports for all of the addresses allocated to the host's primary network interface.
//! Assumes that all callbacks have been properly installed.
//! \param[in] portNumber port to list on.
//! \param[in] acceptFnPtr function to call when an incoming connection is accept()d.
//! \param[in] timeout millisecond period after which the connection is dropped if we can't talk to it.
//! \param[in] errorFnPtr function to call when an error occurs on this interface.
//! \return true if all opens were successful, otherwise false. 
bool teulOpenListenInterface(const UINT16 portNumber
						, CONNECT_FUNCTION* acceptFnPtr
						, const SINT32 timeout
						, ERROR_FUNCTION* errorFnPtr
						);

// opens a port for listening on ipAddress. Assumes that all callbacks have been properly
// installed.  returns the number of the teulIndex associated with this
// connection acceptFnPtr is an optional ptr to callback function which is
// called when a connection is made to the listening socket, and hence a new
// connection is created.  pass nullptr if you do not want this notification
SINT32 teulOpenListenHost(const UINT32 ipAddress
						, const UINT16 portNumber
						, CONNECT_FUNCTION* acceptFnPtr
						, const SINT32 timeout
						, const PROTOCOL protocol
						, ERROR_FUNCTION* errorFnPtr
						);

// opens a connection for sending. Assumes that all callbacks have been properly installed.
// [Host] is a dotted quad combined into a single long with tuelGetHost:
//  returned as [A].[B].[C].[D] = (A<<24) + (B<<16) + (C<<8) + D;
// [connectFnPtr] is an optional ptr to callback function which is called when
// a connection is established to the destination socket, and hence a new connection
// is created.  pass nullptr if you do not want this notification.
// [timeout] is a timeout for this connection in ms for this connection (0 uses default).
// [protocol] is the protocol for this connection (TCP, UDP, etc).
// returns the teulIndex associated with this connection.
SINT32 teulOpenSend(UINT32 Host, const UINT16 portNumber
					, CONNECT_FUNCTION* connectFnPtr
					, const SINT32 timeout, const PROTOCOL protocol
					, ERROR_FUNCTION* errorFnPtr);

