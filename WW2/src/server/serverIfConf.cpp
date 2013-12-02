#include <syscfg.h> // Path is project specific.
// Created: Mon Jul 02 18:32:26 2001
//
// Copyright (c) 2000 Playnet, Inc., All Rights Reserved.
// Not to be distributed in API form except as part of a
// compiled library.
//
// get_ifi_info courtesy of:
//	 "Unix Network Programing" by W. Richard Stevens
//
// Originating Programmer: Marty "Vandal" Poulin
// Maintaining Programmer: Marty "Vandal" Poulin
// Supporting Programmer:  Brain "Stick" Duelm

#if platOS != OS_WIN32
# include <sys/ioctl.h>
# include <sys/socket.h>
# include <netinet/in.h>
#else
# include <WinSock2.h>
# include <WinInet.h>
# include <Winineti.h>
# include <WinIoCtl.h>
#endif

#include "server/serverIfConf.h"

#if platOS != OS_WIN32
struct ifi_info *
get_ifi_info(int family, bool doAliases)
{
	struct ifi_info*		ifi, *ifihead, **ifipnext;
	int				sockfd, len, lastlen, flags, myflags;
	char*				ptr, *buf, lastname[IFNAMSIZ], *cptr;
	struct ifconf		ifc;
	struct ifreq*		ifr, ifrcopy;
	struct sockaddr_in*	sinptr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	// initial buffer size guess
	for (;; )
	{
		buf = (char*)memAlloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
		{
			if (errno != EINVAL || lastlen != 0)
				dbugLog(_FLA,"ioctl error");
		}
		else
		{
			if (ifc.ifc_len == lastlen)
				break;				// success, len has not changed
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	// increment
		memFree(buf);
	}

	ifihead = nullptr;
	ifipnext = &ifihead;
	lastname[0] = 0;

	for (ptr = buf; ptr < buf + ifc.ifc_len; )
	{
		ifr = (struct ifreq*) ptr;

#ifdef	HAVE_SOCKADDR_SA_LEN
		len = max(sizeof(struct sockaddr), ifr->ifr_addr.sa_len);
#else
		switch (ifr->ifr_addr.sa_family)
		{
#ifdef	IPV6
		case AF_INET6:
			len = sizeof(struct sockaddr_in6);
			break;
#endif
		case AF_INET:
		default:
			len = sizeof(struct sockaddr);
			break;
		}
#endif	// HAVE_SOCKADDR_SA_LEN
		ptr += sizeof(ifr->ifr_name) + len;	// for next one in buffer

		if (ifr->ifr_addr.sa_family != family)
			continue;		// ignore if not desired address family

		myflags = 0;
		if ( (cptr = strchr(ifr->ifr_name, ':')) != nullptr)
			*cptr = 0;		// replace colon will null
		if (strncmp(lastname, ifr->ifr_name, IFNAMSIZ) == 0)
		{
			if (doAliases == 0)
				continue;	// already processed this interface
			myflags = IFI_ALIAS;
		}
		memcpy(lastname, ifr->ifr_name, IFNAMSIZ);

		ifrcopy = *ifr;
		ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
		flags = ifrcopy.ifr_flags;
		if ((flags & IFF_UP) == 0)
			continue;				// ignore if interface not up

		ifi = (ifi_info*) memCalloc(1, sizeof(struct ifi_info));
		*ifipnext = ifi;				// prev points to this new one
		ifipnext = &ifi->ifi_next;		// pointer to next one goes here

		ifi->ifi_flags = flags;		// IFF_xxx values
		ifi->ifi_myflags = myflags;	// IFI_xxx values
		memcpy(ifi->ifi_name, ifr->ifr_name, IFI_NAME);
//		memcpy(ifi->ifi_newname, ifr->ifr_newname, IFI_NAME);
		ifi->ifi_name[IFI_NAME-1] = '\0';

		// for some reason Rich didn't copy the mac...
		// so we do it here.
#ifdef	SIOCGIFHWADDR
		ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy);
		ifi->ifi_hlen =IFHWADDRLEN;
		memcpy(ifi->ifi_haddr,  ifrcopy.ifr_addr.sa_data,IFHWADDRLEN);
#endif

		switch (ifr->ifr_addr.sa_family)
		{
		case AF_INET:
			sinptr = (struct sockaddr_in*) &ifr->ifr_addr;
			if (ifi->ifi_addr == nullptr)
			{
				ifi->ifi_addr = (struct sockaddr*)memCalloc(1, sizeof(struct sockaddr_in));
				memcpy(ifi->ifi_addr, sinptr, sizeof(struct sockaddr_in));

#ifdef	SIOCGIFBRDADDR
				if (flags & IFF_BROADCAST)
				{
					ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
					sinptr = (struct sockaddr_in*) &ifrcopy.ifr_broadaddr;
					ifi->ifi_brdaddr = (struct sockaddr*)memCalloc(1, sizeof(struct sockaddr_in));
					memcpy(ifi->ifi_brdaddr, sinptr, sizeof(struct sockaddr_in));
				}
#endif

#ifdef	SIOCGIFDSTADDR
				if (flags & IFF_POINTOPOINT)
				{
					ioctl(sockfd, SIOCGIFDSTADDR, &ifrcopy);
					sinptr = (struct sockaddr_in*) &ifrcopy.ifr_dstaddr;
					ifi->ifi_dstaddr = (struct sockaddr*) memCalloc(1, sizeof(struct sockaddr_in));
					memcpy(ifi->ifi_dstaddr, sinptr, sizeof(struct sockaddr_in));
				}
#endif
			}
			break;

		default:
			break;
		}
	}

	close(sockfd);
	memFree(buf);
	return(ifihead);	// pointer to first structure in linked list
}

// include free_ifi_info
void
free_ifi_info(struct ifi_info* ifihead)
{
	struct ifi_info*	ifi, *ifinext;

	for (ifi = ifihead; ifi != nullptr; ifi = ifinext)
	{
		if (ifi->ifi_addr != nullptr)
			memFree(ifi->ifi_addr);
		if (ifi->ifi_brdaddr != nullptr)
			memFree(ifi->ifi_brdaddr);
		if (ifi->ifi_dstaddr != nullptr)
			memFree(ifi->ifi_dstaddr);
		ifinext = ifi->ifi_next;			// can't fetch ifi_next after free()
		memFree(ifi);					// the ifi_info{} itself
	}
}
#endif
