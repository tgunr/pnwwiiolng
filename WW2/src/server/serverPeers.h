#pragma once
#include "wwii/wwiiServerTypeEnums.h"

// Functions for accessing the server-peer indexes.

//! Get the teulIndex of a peer if it has an established connection, otherwise -1.
SINT16 serverGetPeerIndex(const UINT32 ip, const SERVER_TYPE serverType_) ;

//! Send a packet to ALL servers of a given type.
namespace Teul { class Packet ; }
bool serverSendPacketToType(Teul::Packet& outgoing, const SERVER_TYPE serverType_) ;

//! Send a packet to ALL peering servers.
bool serverSendPacketToPeers(Teul::Packet& outgoing) ;

//! Return true if we're peering with the specified IP address.
bool serverPeeringWithIP(const UINT32 ip_, const SERVER_TYPE serverType_) ;

//! Return true if we're peering with a host of the specified type.
bool serverPeeringWithType(const SERVER_TYPE serverType_) ;

//! Returns an endpoint pointer for a single connection to a type.
ENDPOINT* serverPeerEndpointByType(const SERVER_TYPE serverType_) ;

//! Send data to a teulcallback if we are connected to the corresponding host.
bool serverSendCallbackToType(const SERVER_TYPE serverType, TEUL_CALLBACK* const tcb, void* data) ;

