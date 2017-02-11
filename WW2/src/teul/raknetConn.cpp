#include <syscfg.h> // Path is project specific.
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
#include "cat/crypt/tunnel/Keys.hpp"
#include "cat/math/BigTwistedEdwards.hpp"
#include "cat/crypt/rand/Fortuna.hpp"

#if platBuild == BUILD_SERVER
#include "configVars.h"
#endif

extern IP_ENDPOINT_MAP ipToEndPoint;

bool libCatSecurityEnabled = false;

typedef std::map<UINT32, SystemAddress> IP_SYS_MAP;
static IP_SYS_MAP ipToSysMap;

/*
 * Private method called for server to add the incoming client connection
 * to a map for later transmission.
 */
void RakNetChannel::_addHost(UINT32 ip, const SystemAddress address)
{
	dbugLog(_FLD, "adding host %s", teulGetAddress(ip));

	IP_SYS_MAP::iterator it = ipToSysMap.find(ip);
	if (it != ipToSysMap.end() && it->second == address)
		return;
	ipToSysMap[ip] = address;
}

/*
 * Ensures the ip to SystemAddress map is limited to only open 
 * connections.
 */
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

/*
 * Must be called after the connection is created in order to be usable.  Will set the 
 * initialized flag if successful and can be tested through isInitialized()
 */
bool RakNetChannel::init()
{
	// allocate a raknet peer resource
	m_peer = RakNet::RakPeerInterface::GetInstance();
	if (m_peer != nullptr)
	{
		// for servers only we generate the public/private key
		if (!m_isServer || m_handshake.GenerateServerKey(m_pair))  // .generateKeys())
		{

			// Initialize with the key pair
			if (!m_isServer || !libCatSecurityEnabled || m_peer->InitializeSecurity( m_pair,
				false))
			{

				// Match to associated port for teul connection, this will be UDP after all
				//    only server needs port to be identified
				m_sd.port = (m_isServer)?m_port:0;

#if platBuild == BUILD_SERVER
                UINT16 maxConn = cv_maxCellPlayers.GetUint32();
#else
                UINT16 maxConn = 1;
#endif
				// Instruct peer channel to startup
				m_peer->Startup(maxConn, &m_sd, 1);

				if (m_isServer)
					m_peer->SetMaximumIncomingConnections(maxConn);

				dbugLog(_FLN, "RakNet: Successfully initialized backchannel on port %d", m_sd.port);

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

/*
 * Should be called when there is no longer a use for this channel:
 *    - Server should call this when shutting down (but cleanup in general is already fubar so this can wait for that)
 *    - Client should call this when it loses its paired TCP link to host
 *    - Client should call this when shutting down (but normal close of TCP endpoint will trigger it)
 */
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

/*
 * this method is called on clients when the server passes the public key via teul connection
 */
int RakNetChannel::setPublicKey( char *publicKey, SINT32 teulIndex)
{
	PublicKey pk;
	pk.remoteServerPublicKey = publicKey;
	pk.publicKeyMode = PKM_USE_KNOWN_PUBLIC_KEY; // Recommended mode
    pk.myPrivateKey = nullptr;
    pk.myPublicKey = nullptr;
	int retCode = RN_SK_FAILED;

	if (isInitialized())
	{
		ENDPOINT *ep = teulGetEndpoint(teulIndex);
		if (ep != nullptr)
		{
			// basically a keep alive
			m_peer->SetOccasionalPing(true);

			if (CONNECTION_ATTEMPT_STARTED != m_peer->Connect(teulGetAddress(ep->destAddress()), ep->destPort(), 0, 0, &pk))
			{
				m_isConnected = false;
				dbugLog(_FLW, "Warning: Connect() returned false - invalid public key most likely");
			}
			else
			{
				retCode = RN_SK_CONNECTED;
				dbugLog(_FLN, "Connected to backchannel on teulIndex: %d", teulIndex);
				m_isConnected = true;
				m_wasConnected = true;
			}
		}
	}

	return retCode;
}

/*
 * Calling this function is the only way to manage incoming notifications from the peer 
 * system and this includes the user data exchanged.  Although it could be called from its own thread
 * it should be serialized though the same thread that ultimately services the TCP channels so we don't
 * run into simultaneous access issues.  NOTE: Raknet already manages its own separate thread for 
 * each channel anyway so there is no fear that the underlying network layers will not be serviced.
 */
void RakNetChannel::update()
{
	if (m_peer != nullptr && m_peer->IsActive())
	{
		RakNet::Packet *p;
		while ((p = m_peer->Receive()) != nullptr)
		{
			unsigned char packetX = (unsigned char)p->data[0];
			UINT32 ip = ntohl(p->systemAddress.address.addr4.sin_addr.s_addr);

			switch (packetX)
			{
			case ID_DISCONNECTION_NOTIFICATION:
				dbugLog(_FLN, "Raknet: %s disconnected", p->systemAddress.ToString());
                _removeHost(ip);
				break;
			case ID_CONNECTION_ATTEMPT_FAILED:
				dbugLog(_FLW, "Raknet: %s connection attempt failed", p->systemAddress.ToString());
				break;
			case ID_NEW_INCOMING_CONNECTION:
				dbugLog(_FLN, "Raknet: %s connecting", p->systemAddress.ToString());
				_addHost(ip, p->systemAddress);
				break;
			case ID_CONNECTION_REQUEST_ACCEPTED:
				dbugLog(_FLN, "Raknet: %s connected", p->systemAddress.ToString());
				_addHost(ip, p->systemAddress);
				break;
			case ID_ALREADY_CONNECTED:
				dbugLog(_FLW, "Raknet: %s already connected", p->systemAddress.ToString());
				_addHost(ip, p->systemAddress);
				break;
			case ID_CONNECTION_LOST:
				dbugLog(_FLW, "Raknet: %s connection lost", p->systemAddress.ToString());
                _removeHost(ip);
				break;
			case ID_USER_PACKET_ENUM:
				processApplicationPacket(p, ip);
				break;
            case ID_PUBLIC_KEY_MISMATCH:
                dbugLog(_FLW, "Raknet: %s Public Key mismatch", p->systemAddress.ToString());
                break;
			default:
				dbugLog(_FLN, "Raknet: Unknown packet id %u", packetX);
				break;
			}

			m_peer->DeallocatePacket(p);

		}
	}
}

/*
 * Incoming user data is handled here and it is passed through to the registered callback just as it would through
 * the normal TCP endpoint.
 */
void RakNetChannel::processApplicationPacket(RakNet::Packet *p, UINT32 ip)
{
	ENDPOINT *sourceEndpoint = (isServer()) ? ipToEndPoint[ip] : m_endpoint;

	if (sourceEndpoint != nullptr)
	{
		Teul::PacketHeader* const hdr = reinterpret_cast<Teul::PacketHeader *>(&p->data[1]);
		TEUL_CALLBACK* const callback = TEUL_CALLBACK::Lookup(hdr->m_callX, hdr->m_callY);
		if (PN_UNLIKELY(callback == nullptr))
		{
			dbugLog(_FLW, "Raknet: t:%d ip:%s cb:%u,%u callback does not exist", sourceEndpoint->teulIndex(), teulGetAddress(sourceEndpoint->fromAddress()), hdr->m_callX, hdr->m_callY);
			return;
		}

		if (PN_UNLIKELY(hdr->m_callX == Teul::Serv::CallbackX && sourceEndpoint->teulIndex() >= 0 && !(sourceEndpoint->m_peer)))
		{
			dbugLog(_FLA, "Raknet: t:%d ip:%s cb:%d,%d callback access denied", sourceEndpoint->teulIndex(), teulGetAddress(sourceEndpoint->fromAddress()), hdr->m_callX, hdr->m_callY);
			return;
		}

        // Immediately after the header.
		UCHAR* const dataStart = reinterpret_cast<UCHAR *>(&hdr[1]);
        
        // adjust for header and type field
		callback->processCallback(*sourceEndpoint, dataStart, static_cast<SINT32>(p->length - sizeof(*hdr) - 1));
	}
	else
	{
		dbugLog(_FLW, "Raknet: No endpoint located for incoming msg");
	}

}

/*
 * This method should be called by either client or server to transmit a user message through the backchannel to 
 * a partnered endpoint.
 */
bool RakNetChannel::send(Teul::Packet& packet, ENDPOINT& endpoint)
{
    if (!isInitialized())
    {
        dbugLog(_FLW, "Server Could not send to backchannel, not initialized for endpoint %d", endpoint.teulIndex());
        return false;
    }
    else if (!isServer() && !isConnected())
    {
        dbugLog(_FLW, "Client could not send through backchannel, not connected to server for endpoint %d", endpoint.teulIndex());
        return false;
    }
    
	RakNet::BitStream bitStream;
	UINT8 msgType = static_cast<UINT8>(ID_USER_PACKET_ENUM);
	bitStream.Write(msgType);
	bitStream.Write(static_cast<char *>(packet.packet()), packet.packetSizeBytes());
    
    // We need to specify the target address
    // NOTE: we might be able to do without this map I think - wait for now
    SystemAddress addr;
    memset(&addr, 0, sizeof(SystemAddress));
    if (isServer())
    {
        addr = ipToSysMap[endpoint.destAddress()];
    }
    else
    {
        addr.address.addr4 = endpoint.destSockAddress;
    }
    
    UINT32 result = 0;
    
    // make sure we are connected to this address
    if (addr.address.addr4.sin_addr.s_addr != 0)
    {
        result = m_peer->Send(&bitStream, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, 0, addr, false);
        if (result == 0)
        {
            dbugLog(_FLW, "Failed to send data over backchannel to host %s, teulIndex %d", addr.ToString(), endpoint.teulIndex());
        }
        else
        {
            dbugLog(_FLI, "Sent data via backchannel to host %s, teulIndex %d", addr.ToString(), endpoint.teulIndex());
        }
    }

	return (result > 0);
}

