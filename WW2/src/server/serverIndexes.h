#pragma once

namespace Server
{
	extern SINT32	broadcastIndex ;		// TeulIndex to send broadcast packets.

	extern UINT32	localHost ;				// IP addr of local host
	extern UINT32	broadcastHost ;			// IP address of the broadcast address.
	extern UINT32	broadcastNet ;			// Network addres of the broadcast network.

	extern bool		broadcastConnected ;	// Flagged once the broadcast index has established.

	static inline CONSTEXPR_FN UINT32 BroadcastMaskAddress(const UINT32 ip)
	{
		// Take the low octet of the input address,
		// return that appended to the upper-three
		// octets of the broadcast network address,
		// thus [192.168.10].xx | (xx.xx.xx.[151])
		// makes 192.168.10.151
		return (Server::broadcastNet | (ip & 0xff)) ;
	}

	static inline CONSTEXPR_FN bool IsMulticast(const UINT32 ip) { return (ip & 0xE0000000) == 0xE0000000; }
}

