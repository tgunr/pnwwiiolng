#include <syscfg.h> // Path is project specific.
// Created: Fri Sep 29 16:08:01 CDT 2006
//
// Raknet data backchannel functions
//
// Copyright (C) PlayNet, Inc 1999-2006 - All rights reserved.
// Not for redistribution.
//
// Originating Programmer: Oliver "kfsone" Smith
//

#include "raknetConn.h"

#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "RakPeer.h"
#include "RPC4Plugin.h"
#include "BitStream.h"
#include "dbug/dbug.h"
#include "teul/teulCallbackStruct.h"
#include "teul/teulConstants.h"
#include "SecureHandshake.h" 

extern IP_ENDPOINT_MAP ipToEndPoint;

typedef std::map<UINT32, SystemAddress> IP_SYS_MAP;
static IP_SYS_MAP ipToSysMap;

cat::EasyHandshake m_handshake;

void RakNetChannel::_addHost(UINT32 ip, const SystemAddress address)
{
	dbugLog(_FLD, "adding host %s", teulGetAddress(ip));

	IP_SYS_MAP::iterator it = ipToSysMap.find(ip);
	if (it != ipToSysMap.end() && it->second == address)
		return;
	ipToSysMap[ip] = address;
}

void RakNetChannel::_removeHost(UINT32 ip)
{
	dbugLog(_FLD, "disconnecting cell %s", teulGetAddress(ip));
	IP_SYS_MAP::iterator it = ipToSysMap.find(ip);
	if (it != ipToSysMap.end())
	{
		m_peer->CloseConnection(it->second, true, 0, IMMEDIATE_PRIORITY);
		ipToSysMap.erase(it);
	}
}

bool RakNetChannel::init()
{
	// allocate a raknet peer resource
	m_peer = RakNet::RakPeerInterface::GetInstance();
	if (m_peer != nullptr)
	{
		// for servers only we generate the public/private key
		if (!m_isServer || m_handshake.GenerateServerKey(m_public_key, m_private_key))
		{

			// Initialize with the key pair
			if (!m_isServer || m_peer->InitializeSecurity(m_public_key, m_private_key, false))
			{

				// Match to associated port for teul connection, this will be UDP after all
				m_sd.port = (m_isServer)?m_port:0;

				// TODO: link to cell hosts max connections value
				const int maxConn = (m_isServer) ? 2000 : 1;

				// Instruct peer channel to startup 
				m_peer->Startup(maxConn, &m_sd, 1);

				if (m_isServer)
					m_peer->SetMaximumIncomingConnections(maxConn);

				m_initialized = true;
			}
			else 
			{
				dbugLog(_FLW, "Warning: Public/private keys are invalid!");
			}
		}
		else
		{
			dbugLog(_FLW, "Warning Unable to generate key for client");
		}
	}
	else
	{
		dbugLog(_FLW, "Warning: Unable to allocate RakNet peer resource");

	}

	return m_initialized;
}

void RakNetChannel::release()
{
	if (m_initialized)
	{
		if (m_peer != nullptr)
		{
			if (m_isConnected)
			{
				m_peer->Shutdown(0);
			}

			RakNet::RakPeerInterface::DestroyInstance(m_peer);

			m_peer = nullptr;
		}

		m_initialized = false;
	}
}

// TODO: this method is called on clients when the server passes the public key via teul connection
void RakNetChannel::setPublicKey(const char *publicKey, SINT32 teulIndex)
{
	strncpy(m_public_key, publicKey, cat::EasyHandshake::PUBLIC_KEY_BYTES);

	PublicKey pk;
	pk.remoteServerPublicKey = m_public_key;
	pk.publicKeyMode = PKM_USE_KNOWN_PUBLIC_KEY; // Recommended mode

	ENDPOINT *ep = teulGetEndpoint(teulIndex);
	if (ep != nullptr)
	{
		
		if (CONNECTION_ATTEMPT_STARTED != m_peer->Connect(teulGetAddress(ep->destAddress()), ep->destPort(), 0, 0, &pk))
		{
			m_isConnected = false;
			dbugLog(_FLW, "Warning: Connect() returned false - invalid public key most likely");
		}
		else
		{
			m_isConnected = true;
		}
	}
}

// TODO: Call to m_peer->Receive() will be here
void RakNetChannel::update()
{
	if (m_peer != nullptr && m_peer->IsActive())
	{
		RakNet::Packet *p;
		while ((p = m_peer->Receive()) != nullptr)
		{
			unsigned char packetX = (unsigned char)p->data[0];
			UINT32 ip = ntohl(p->systemAddress.address.addr4.sin_addr.s_addr);

#ifdef DEV_BUILD
			dbugLog(_FLD, "cell:%s px:%u", p->systemAddress.ToString(), packetX);
#endif
			switch (packetX)
			{
			case ID_DISCONNECTION_NOTIFICATION:
				dbugLog(_FLN, "cell:%s disconnected", p->systemAddress.ToString());
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				dbugLog(_FLN, "cell:%s connection attempt failed", p->systemAddress.ToString());
				break;
			case ID_NEW_INCOMING_CONNECTION:
				dbugLog(_FLN, "cell:%s connecting", p->systemAddress.ToString());
				_addHost(ip, p->systemAddress);
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				dbugLog(_FLN, "cell:%s connected", p->systemAddress.ToString());
				_addHost(ip, p->systemAddress);
				break;
			case ID_ALREADY_CONNECTED:
				dbugLog(_FLN, "cell:%s already connected", p->systemAddress.ToString());
				_addHost(ip, p->systemAddress);
				break;
			case ID_CONNECTION_LOST:
				dbugLog(_FLW, "cell:%s connection lost", p->systemAddress.ToString());
				break;
			case ID_USER_PACKET_ENUM:
				processApplicationPacket(p, ip);
				break;

			default:
				dbugLog(_FLI, "Unknown packet id %u", packetX);
				break;
			}

			m_peer->DeallocatePacket(p);

		}
	}
}

void RakNetChannel::processApplicationPacket(RakNet::Packet *p, UINT32 ip)
{
	ENDPOINT *sourceEndpoint = (isServer()) ? ipToEndPoint[ip] : m_endpoint;

	if (sourceEndpoint != nullptr)
	{
		dbugLog(_FLD, "Received incoming RakNet msg");
		Teul::PacketHeader* const hdr = reinterpret_cast<Teul::PacketHeader *>(&p->data[1]);
		TEUL_CALLBACK* const callback = TEUL_CALLBACK::Lookup(hdr->m_callX, hdr->m_callY);
		if (PN_UNLIKELY(callback == nullptr))
		{
			dbugLog(_FLW, "t:%d ip:%s cb:%u,%u callback does not exist", sourceEndpoint->teulIndex(), teulGetAddress(sourceEndpoint->fromAddress()), hdr->m_callX, hdr->m_callY);
			return;
		}

		if (PN_UNLIKELY(hdr->m_callX == Teul::Serv::CallbackX && sourceEndpoint->teulIndex() >= 0 && !(sourceEndpoint->m_peer)))
		{
			dbugLog(_FLA, "t:%d ip:%s cb:%d,%d callback access denied", sourceEndpoint->teulIndex(), teulGetAddress(sourceEndpoint->fromAddress()), hdr->m_callX, hdr->m_callY);
			return;
		}

		UCHAR* const dataStart = reinterpret_cast<UCHAR *>(&hdr[1]);	// Immediately after the header.
		callback->processCallback(*sourceEndpoint, dataStart, p->length - sizeof(*hdr) - 1);  // adjust for header and type field
	}
	else
	{
		dbugLog(_FLW, "No endpoint located for incoming msg");
	}

}

bool RakNetChannel::send(Teul::Packet& packet, ENDPOINT& endpoint)
{
	RakNet::BitStream bitStream;
	UINT8 msgType = static_cast<UINT8>(ID_USER_PACKET_ENUM);
	bitStream.Write(msgType);
	bitStream.Write(static_cast<char *>(packet.packet()), packet.packetSizeBytes());
	SystemAddress addr = ipToSysMap[packet.address()];

	UINT32 result = m_peer->Send(&bitStream, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, 0, addr, false);
	if (result == 0)
	{
		dbugLog(_FLW, "Failed to send data over backchannel to host %s, teulIndex %d", addr.ToString(), endpoint.teulIndex());
	}

	return (result > 0);
}
