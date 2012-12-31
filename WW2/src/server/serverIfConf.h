#pragma once
// Created: Tue Jun 13 13:32:25 2000
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// Originating Programmer:  Marty "Vandal" Poulin

#if platOS != OS_WIN32
#include	<net/if.h>
#else
#include <WinSock2.h>
#endif

#if platOS != OS_WIN32
#define	IFI_NAME		IFNAMSIZ	// same as IFNAMSIZ in <net/if.h>
#define	IFI_HADDR	 8		// allow for 64-bit EUI-64 in future

struct ifi_info
{
  char    ifi_name[IFI_NAME];		// interface name, null terminated
  char    ifi_newname[IFI_NAME];		// interface name, null terminated
  u_char  ifi_haddr[IFI_HADDR];	// hardware address
  u_short ifi_hlen;				// #bytes in hardware address: 0, 6, 8
  short   ifi_flags;				// IFF_xxx constants from <net/if.h>
  short   ifi_myflags;			// our own IFI_xxx flags
  struct sockaddr*  ifi_addr;		// primary address
  struct sockaddr*  ifi_brdaddr;	// broadcast address
  struct sockaddr*  ifi_dstaddr;	// destination address
  struct ifi_info*  ifi_next;		// next of these structures
};

#define	IFI_ALIAS	1			// ifi_addr is an alias

extern struct ifi_info*	get_ifi_info(int family, bool doAliases);
extern void			 free_ifi_info(struct ifi_info*);
#endif
