#pragma once
// Created: Fri Sep 29 16:08:01 CDT 2006
//
// Raknet support for WWII Online
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith

#define LIBCAT_SECURITY 1

#include "RakPeerInterface.h"
#include "RakPeer.h"
#include "RPC4Plugin.h"
#include "RakNetTypes.h"

using namespace RakNet;
using namespace cat;

class AddressOrGUID;
class RakPeerInterface;
class EasyHandshake;
class TunnelKeyPair;

class RakNetChannel
{
public:
	// Default Constructor
	RakNetChannel(const bool isServer, const UINT16 port, ENDPOINT *endpoint) 
	{
		m_isServer = isServer;
		m_port = port;
		m_endpoint = endpoint;
		m_initialized = false;
		m_isConnected = false;  // client notion only
		init();
	}

	bool init();
	void release();

	const UCHAR *getPackedPublicKey() { return m_packedPublicKey; }
	void setPublicKey(const char *publicKey, SINT32 teulIndex);

	void update();
	bool send(Teul::Packet& packet, ENDPOINT& endpoint);
	bool isInitialized() { return m_initialized; }
	bool isConnected() { return m_isConnected; }
	bool isServer() { return m_isServer; }

private:

	void processApplicationPacket(RakNet::Packet *packet, UINT32 ip);

	void _addHost(UINT32 ip, const SystemAddress address);
	void _removeHost(UINT32 ip);

	class RakNet::RakPeerInterface *m_peer;
	RakNet::RPC4 m_rpc;

	UCHAR m_packedPublicKey[cat::EasyHandshake::PUBLIC_KEY_BYTES];
	cat::TunnelKeyPair m_pair;
	//char m_public_key[cat::EasyHandshake::PUBLIC_KEY_BYTES];
	//char m_private_key[cat::EasyHandshake::PRIVATE_KEY_BYTES];

	SocketDescriptor m_sd;
	UINT16  m_port;
	ENDPOINT *m_endpoint;
	bool m_isServer;

	bool m_initialized;
	bool m_isConnected;
};


