#pragma once
// Standard teul enumerations

// The recommended modem MTU is 576 bytes; TCP/IP has a 40 byte packet over head
// and teul incurs a 24 byte overhead, so the maximum modem-friendly data segment
// we can have is 512 bytes. However broadband users have a recommended MTU of 1400 bytes.
// The only problem with this is that if we hit a 576 mtu somewhere en-route, we'll
// fragment into 3 packets. It's therefore a better idea to use a data segment that
// will comfortably sit inside two MTUs.

// connection states..
enum // do not change order!
{
	IS_OPENING,			// no callbacks are allowed in this mode
	IS_LISTENING,		// state of all listening connections
	IS_OPEN,			// connection is ready for callback transmission
	IS_DEAD				// connection is dead and about to be freed
};

// each connection must have a protocol bound to it so the operating system knows how to
// maniplulate packets. The choice of protocol can make a big difference to the performance
// and flexability of the connection.
enum PROTOCOL
{
	PROTOCOL_UDP,			// User Datagram protocol, unreliable, datagram based, most flexible.
	PROTOCOL_TCP,			// Transport Control Protocol, reliable, stream based, extra overhead.
	PROTOCOL_UDB,			// UDB = UDP bare, no rel, also allows broadcast
	PROTOCOL_CTRL,			// CTRL = Via the controller
	PROTOCOL_UNDEF
};

// The base packer modes.
enum PACKER_MODE
{
	TEUL_UNPACK,
	TEUL_PACK
};

// The mask for "send directly to"
// TODO: Do we need this?
#define DIRECT_MASK 0x000000ff
