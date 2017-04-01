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

extern GUID_ENDPOINT_MAP guidToEndPoint;

bool libCatSecurityEnabled = true;

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
				dbugLog(_FLW, "Raknet: Warning: Public/private keys are invalid!");
			}
		}
		else
		{
			dbugLog(_FLW, "Raknet: Warning Unable to generate key for client");
		}
	}
	else
	{
		dbugLog(_FLW, "Raknet: Warning: Unable to allocate RakNet peer resource");

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
				m_endpoint->doError(RN_SK_FAILED);
				m_peer->Shutdown(0);
				m_isConnected = false;
				m_isConnecting = false;
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
            if (!isConnected() && !isConnecting())
            {
                // basically a keep alive
                m_peer->SetOccasionalPing(true);

                if (CONNECTION_ATTEMPT_STARTED != m_peer->Connect(teulGetAddress(ep->destAddress()), ep->destPort(), 0, 0, &pk))
                {
                    m_isConnecting = false;
                    m_isConnected = false;
                    dbugLog(_FLW, "Raknet: Warning: Connect() returned false - invalid public key most likely");
                }
                else
                {
                    retCode = RN_SK_CONNECTING;
                    dbugLog(_FLN, "Raknet: Connecting to backchannel on teulIndex: %d", teulIndex);
                    m_isConnecting = true;
                }
            }
            else
            {
                if (isConnected())
                    dbugLog(_FLW, "Raknet: Can't connect to already connected channel!");
                else if (isConnecting())
                    dbugLog(_FLW, "Raknet: Can't connect to a channel in the process of connecting!");
            }
		}
        else
        {
            dbugLog(_FLW, "Raknet: Can't connect through invalid endpoint!");
        }
	}
    else
    {
        dbugLog(_FLW, "Raknet: Can't connect when not previously initialized!");
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
			switch (packetX)
			{
                case ID_DISCONNECTION_NOTIFICATION:
                    dbugLog(_FLN, "Raknet: %s disconnected", p->systemAddress.ToString());
                    if (!isServer())
                    {
                        // time to abort netcode3
                        m_peer->DeallocatePacket(p);
                        release();
                        return;
                    }
                    break;
                case ID_CONNECTION_ATTEMPT_FAILED:
                    dbugLog(_FLW, "Raknet: %s connection attempt failed", p->systemAddress.ToString());
                    m_peer->DeallocatePacket(p);
                    release();
                    return;
                case ID_NEW_INCOMING_CONNECTION:
                    dbugLog(_FLN, "Raknet: %s connecting", p->systemAddress.ToString());
                    break;
                case ID_CONNECTION_REQUEST_ACCEPTED:
                    dbugLog(_FLN, "Raknet: %s connected", p->systemAddress.ToString());
                    m_isConnecting = false;
                    m_isConnected = true;
                    break;
                case ID_ALREADY_CONNECTED:
                    dbugLog(_FLW, "Raknet: %s already connected", p->systemAddress.ToString());
                    break;
                case ID_CONNECTION_LOST:
                    dbugLog(_FLW, "Raknet: %s connection lost", p->systemAddress.ToString());
                    if (!isServer())
                    {
                        // time to abort netcode3
                        m_peer->DeallocatePacket(p);
                        release();
                        return;
                    }
                    break;
                case ID_USER_PACKET_ENUM:
				{
					SINT32 teulIndex = ntohl(*(reinterpret_cast<SINT32 *>(p->data + 1)));
					// Have we associated this guid with an endpoint yet?
					if (isServer() && guidToEndPoint[p->guid] == nullptr && teulIndex > 0) {
					    dbugLog(_FLD, "Raknet: teulIndex received from client: %d with guid: %s", teulIndex, p->guid.ToString());
						ENDPOINT *ep = teulGetEndpoint(teulIndex);
						guidToEndPoint[p->guid] = ep;
						ep->m_rakNetGuid = p->guid;
					}
                    else
                    {
                        if (teulIndex < 1)
                        {
                            dbugLog(_FLW, "Raknet: recevied bad index from client: %d", teulIndex);
                        }
                    }
					processApplicationPacket(p);
				}
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
void RakNetChannel::processApplicationPacket(RakNet::Packet *p)
{
	ENDPOINT *sourceEndpoint = (isServer()) ? guidToEndPoint[p->guid] : m_endpoint;

	if (sourceEndpoint != nullptr)
	{
		Teul::PacketHeader* const hdr = reinterpret_cast<Teul::PacketHeader *>(&p->data[1+sizeof(SINT32)]);
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
		dbugLog(_FLW, "Raknet: No endpoint located for incoming msg: guid-%s", p->guid.ToString());
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
        dbugLog(_FLW, "Raknet: Server Could not send to backchannel, not initialized for endpoint %d", endpoint.teulIndex());
        return false;
    }
    else if (!isServer() && !isConnected())
    {
        if (isConnecting())
        {
            dbugLog(_FLI, "Raknet: Still waiting for connect, can't send data yet");
        }
        else
        {
            dbugLog(_FLW, "Raknet: Client could not send through backchannel, not connected to server for endpoint %d", endpoint.teulIndex());
        }
        return false;
    }
    
	RakNet::BitStream bitStream;
	UINT8 msgType = static_cast<UINT8>(ID_USER_PACKET_ENUM);
	bitStream.Write(msgType);
	bitStream.Write(m_peerTeulIndex);
	bitStream.Write(static_cast<char *>(packet.packet()), packet.packetSizeBytes());
    
	UINT32 result = 0;

	RakNet::AddressOrGUID guid;

	// We need to specify the target address
    // NOTE: we might be able to do without this map I think - wait for now
    if (isServer())
    {
		guid = endpoint.m_rakNetGuid;
    }
    else
    {
		SystemAddress addr;
		memset(&addr, 0, sizeof(SystemAddress));
		addr.address.addr4 = endpoint.destSockAddress;
		guid = addr;
	}
    
	result = m_peer->Send(&bitStream, PacketPriority::HIGH_PRIORITY, PacketReliability::RELIABLE_ORDERED, 0, guid, false);
	if (result == 0)
    {
        dbugLog(_FLW, "Raknet: Failed to send data over backchannel to host %s, teulIndex %d", guid.ToString(), endpoint.teulIndex());
    }
    else
    {
        dbugLog(_FLD, "Raknet: Sent data via backchannel to host %s, teulIndex %d", guid.ToString(), endpoint.teulIndex());
    }

	return (result > 0);
}

void RakNetChannel::disconnectPeer(ENDPOINT *pEndpoint)
{
    
    if (pEndpoint != nullptr)
    {
        if (pEndpoint->m_pairdUdpConn != nullptr)
        {
            if  (!pEndpoint->m_pairdUdpConn->isServer())
            {
                pEndpoint->m_pairdUdpConn->release();
                delete pEndpoint->m_pairdUdpConn;
                pEndpoint->m_pairdUdpConn = nullptr;
            }
            else
            {
                // see if this is a client endpoint
                if (pEndpoint->m_parent != nullptr)
                {
                    pEndpoint->m_pairdUdpConn->disconnect(pEndpoint);
                }
                
            }
        }
    }

}

void RakNetChannel::disconnect(ENDPOINT *pEndpoint)
{
	// reset all of the associations
	RakNetGUID defaultGuid;
	RakNetGUID guid = pEndpoint->m_rakNetGuid;
	guidToEndPoint[guid] = nullptr;
	pEndpoint->m_rakNetGuid = defaultGuid;
}
