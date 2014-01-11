#pragma once
// This module provides high-level access to the transport
// functions used in the teul API.
//
// Copyright 1999 Playnet Inc, All rights reserved
// Not to be distributed in API form except as part of a compiled
// library
//
// Originating Programmer: Martin Poulin

#include "teul/teulEnums.h"
#include "teul/teulEndpointDefs.h"
#include "teul/teulTimeMS.h"
#include "teul/teulSockTypes.h"		// For SOCKET
#include "teul/teulFuncTypes.h"

namespace Teul
{
	class Packet;
}

struct EndpointImpl;

struct 	ENDPOINT
{
public:
	///! Set the OS "receive buffer" size.
	///! Aligns size to a 4Kb boundary, only allows growth.
	///! Typically, the operating system doubles this to allow for overheads.
	int setRcvBufferSize(UINT32 newSize);
	///! Set the OS "send buffer" size.
	///! Aligns size to a 4Kb boundary, only allows growth.
	///! Typically, the operating system doubles this to allow for overheads.
	int setSndBufferSize(UINT32 newSize);

	enum { MaxRcvBufferSize = 16 * 1024, MaxSndBufferSize = 128 * 1024 };

public:
	//! Query the endpoint's internal identifier (Teul Index).
	SINT32 teulIndex() const { return m_teulIndex; }

public:
	//! Query the current state of the endpoint.
	UINT32 state() const { return m_state; }

	////////// Member variables.
public:
	Teul::TimeMS		lastDataRecv;			// local last time data was sent on this endpoint
	Teul::TimeMS		lastDataSent;			// local last time data was received on this endpoint
	Teul::TimeMS		lastKliveSent;			// local last time keepalive was sent on this endpoint
	Teul::TimeMS		m_created;				// When endpoint was initialized for this connection.
	TeulTimeMS			m_tickAccumulation;	// Delta the increments in ticks.

public:
	SINT16	 			m_teulIndex;			// current teul index (AKA opaque reference)to this endpoint
	UCHAR				m_state;
	UCHAR				protocol;
	UCHAR				m_readAheadBuffer[3];	//! Read-ahead buffer for storing token & size.

	bool				m_inUse : 1;			// check if the endpoint is in use before we remove it DOH!!
	bool				m_connected : 1;
	bool				m_reliable : 1;
	bool				m_doTimeoutCheck : 1;
	bool				m_closed : 1;
	bool				m_writesPending : 1;
	bool				m_haveTicks : 1;		// False until we've started recording ticks.
	bool				m_peer : 1;			// Peering connection.

	//! Implementation entity for stuff we don't really need to expose to all callers.
	EndpointImpl*		m_impl;

	//! Generic pointer to application-specific data associated with
	//! this connection. Typically either a ww2Client (on the client)
	//! or a wwiiPlayer (on the host).
	void*				appConnectionData;

	ENDPOINT* 			m_parent;				///! Pointer to endpoint that spawned this connection, if this is an accepted incoming connection.
												///! Otherwise nullptr if created by teulOpenSend.

	SOCKET				socket;					// the socket handle

	SINT32				timeout;				// timeout of endpoint
	UINT32				m_readBufferUsed;		//! Current usage of read buffer.

	struct sockaddr_in	localSockAddress;		// local socket address from bind.
	struct sockaddr_in	fromSockAddress;		// where a packet came from;
	struct sockaddr_in	destSockAddress;		// socket address of the destination.

#if platOS == OS_WIN32
	WSAEVENT			eventObject;			// really a WSAEVENT/ HANDLE
#endif

	CONNECT_FUNCTION*	connectFn;				// Connect function.
	ERROR_FUNCTION*		errorFn;				// Error function.

public:
	//! Notify our application of a connection accepted.
	void doAccept(struct ENDPOINT& ep);
	//! Notify our application of a connection becoming connected.
	void doConnect();
	//! Report error to our application.
	void doError(const SINT32 error, void* data = nullptr);

	//! Close down the socket.
	void shutdown();

	//! Send an error on a teulIndex for processing on the remote end.
	void sendError(const SINT32 error);

	UINT32 setTimeout(const UINT32 timeout_);

	void releaseDatagram();

	//! Make sure the socket events match the current state.
	void updateSocketEvents();

	void initialize(SINT32 i);

	//! Try to drain the write queue.
	SINT32 pushSendQueue();

	//! Enable or disable nagle's algorithm.
	//! @return Prior state of the nagle algorithm.
	bool setNagleEnabled(bool);

	//! Attempt to send a packet, or queue it if we cannot
	//! send it immediately.
	SINT32 sendPacket(Teul::Packet& packet);

	//! Flag the endpoint as having become connected.
	void setConnected();

	//! Flag whether the endpoint needs to monitor for writes or not.
	void setWritesPending(const bool pending_);

	//! Encrypt an outgoing stream.
	void encrypt(UCHAR* const stream_, const size_t bytes_);

	//! Decrypt an incoming stream.
	void decrypt(UCHAR* const stream_, const size_t bytes_);

	UINT32 localAddress() const;
	UINT16 localPort() const;
	UINT32 fromAddress() const;
	UINT16 fromPort() const;
	UINT32 destAddress() const;
	UINT16 destPort() const;

private:
	//! Write the actual data to the socket.
	SINT32 writeData(void* const data, const size_t sizeInBytes, const UINT32 address, const UINT32 port);
};


/* Basic functions */

/* Endpoint functions */

extern void			teulCleanupEndpoints(void);

extern ENDPOINT* 	teulCreateEndpoint(SINT32 teulIndex, bool isListening, SINT32 timeout, PROTOCOL protocol);

extern void			teulFreeEndpoint(ENDPOINT* theEndpoint);

extern void			teulRemoveEndpoint(ENDPOINT* inEndpoint);
extern void			teulKillDeadEndpoints(void);

// INTERNAL: Get the next free index.
// Note: this value is incredibly transient.
extern SINT32		teulQueryNextFreeIndex();

