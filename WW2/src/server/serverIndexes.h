#pragma once

namespace Server
{
	extern SINT32	broadcastIndex ;		// TeulIndex to send broadcast packets.

	extern UINT32	localHost ;				// IP addr of local host
	extern UINT32	broadcastHost ;			// IP address of the broadcast address.
	extern UINT32	broadcastNet ;			// Network addres of the broadcast network.

	extern bool		broadcastConnected ;	// Flagged once the broadcast index has established.

	// If the broadcastnet is not a network (xxxxxx00) or broadcast (xxxxxxff) value,
	// then we're talking via a controller.
	static inline bool IsBroadcastViaController() { return (UINT32)((broadcastNet & 0xff) - 1) < 254; }

	static inline UINT32 BroadcastMaskAddress(const UINT32 ip)
	{
		// Take the low octet of the input address,
		// return that appended to the upper-three
		// octets of the broadcast network address,
		// thus [192.168.10].xx | (xx.xx.xx.[151])
		// makes 192.168.10.151
		return ((Server::broadcastNet & ~0xff) | (ip & 0xff)) ;
	}
}

