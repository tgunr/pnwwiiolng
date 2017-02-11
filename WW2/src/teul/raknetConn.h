#pragma once
// Created: February 2017
//
// Raknet data backchannel functions
//
// Copyright (C) PlayNet, Inc 1999-2017 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Ian "frisbone" Frisbie
//
// Notes:
//
//    This class is part of the Teul library and is shared between clients and
// servers.  A server is special because it opens a single channel/object
// and awaits incoming connections.  The client awaits authentication through
// the sharing of the public key through a normal TCP teul endpoint and once
// the key is received it opens a channel to the server.  Once the servers
// receive this open request it registers the IP for transmission to client.

// Raknet headers are sensitive to this flag so it needs to be defined when referencing them
#define LIBCAT_SECURITY 1

#include "RakPeerInterface.h"
#include "RakPeer.h"
#include "RPC4Plugin.h"
#include "RakNetTypes.h"

using namespace RakNet;
using namespace cat;

class AddressOrGUID;
class RakPeerInterface;
class TunnelKeyPair;

enum {
	RN_SK_CONNECTED,
	RN_SK_FAILED
};

class RakNetChannel
{
public:
	RakNetChannel(const bool isSrvr, const UINT16 port, ENDPOINT *endpoint)
	{
		m_isServer = isSrvr;
		m_port = port;
		m_endpoint = endpoint;
		m_initialized = false;
		m_isConnected = false;  // client notion only
		m_wasConnected = false; // client notion only
	}

	bool init();
	void release();

	const UCHAR *getPackedPublicKey() { return m_pair.GetPublicKey(); }
	int setPublicKey( char *publicKey, SINT32 teulIndex);

	void update();
	bool send(Teul::Packet& packet, ENDPOINT& endpoint);
	bool isInitialized() { return m_initialized; }
	bool isConnected() { return m_isConnected; }
	bool isServer() { return m_isServer; }

	// samples last state and returns it while clearing it.
	bool wasDisconnected() { bool retVal = m_wasConnected; m_wasConnected = false; return retVal && !m_isConnected; }

private:

	void processApplicationPacket(RakNet::Packet *packet, UINT32 ip);

	void _addHost(UINT32 ip, const SystemAddress address);
	void _removeHost(UINT32 ip);

	class RakNet::RakPeerInterface *m_peer;
	RakNet::RPC4 m_rpc;

	cat::ServerEasyHandshake m_handshake;

	cat::TunnelKeyPair m_pair;

	SocketDescriptor m_sd;
	UINT16  m_port;
	ENDPOINT *m_endpoint;
	bool m_isServer;

	bool m_initialized;
	bool m_isConnected;
	bool m_wasConnected;
};


