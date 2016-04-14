/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)if.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD$
 */

#ifndef _FREEBSD_NET_IF_H_
#define	_FREEBSD_NET_IF_H_

#define	IFCAP_RXCSUM		0x00001  /* can offload checksum on RX */
#define	IFCAP_TXCSUM		0x00002  /* can offload checksum on TX */
#define	IFCAP_NETCONS		0x00004  /* can be a network console */
#define	IFCAP_VLAN_MTU		0x00008	/* VLAN-compatible MTU */
#define	IFCAP_VLAN_HWTAGGING	0x00010	/* hardware VLAN tag support */
#define	IFCAP_JUMBO_MTU		0x00020	/* 9000 byte MTU supported */
#define	IFCAP_POLLING		0x00040	/* driver supports polling */
#define	IFCAP_VLAN_HWCSUM	0x00080	/* can do IFCAP_HWCSUM on VLANs */
#define	IFCAP_TSO4		0x00100	/* can do TCP Segmentation Offload */
#define	IFCAP_TSO6		0x00200	/* can do TCP6 Segmentation Offload */
#define	IFCAP_LRO		0x00400	/* can do Large Receive Offload */
#define	IFCAP_WOL_UCAST		0x00800	/* wake on any unicast frame */
#define	IFCAP_WOL_MCAST		0x01000	/* wake on any multicast frame */
#define	IFCAP_WOL_MAGIC		0x02000	/* wake on any Magic Packet */
#define	IFCAP_TOE4		0x04000	/* interface can offload TCP */
#define	IFCAP_TOE6		0x08000	/* interface can offload TCP6 */
#define	IFCAP_VLAN_HWFILTER	0x10000 /* interface hw can filter vlan tag */
#define	IFCAP_POLLING_NOCOUNT	0x20000 /* polling ticks cannot be fragmented */
#define	IFCAP_VLAN_HWTSO	0x40000 /* can do IFCAP_TSO on VLANs */
#define	IFCAP_LINKSTATE		0x80000 /* the runtime link state is dynamic */
#define	IFCAP_NETMAP		0x100000 /* netmap mode supported/enabled */
#define	IFCAP_RXCSUM_IPV6	0x200000  /* can offload checksum on IPv6 RX */
#define	IFCAP_TXCSUM_IPV6	0x400000  /* can offload checksum on IPv6 TX */
#define	IFCAP_HWSTATS		0x800000 /* manages counters internally */

#define IFCAP_HWCSUM_IPV6	(IFCAP_RXCSUM_IPV6 | IFCAP_TXCSUM_IPV6)

#define IFCAP_HWCSUM	(IFCAP_RXCSUM | IFCAP_TXCSUM)
#define	IFCAP_TSO	(IFCAP_TSO4 | IFCAP_TSO6)
#define	IFCAP_WOL	(IFCAP_WOL_UCAST | IFCAP_WOL_MCAST | IFCAP_WOL_MAGIC)
#define	IFCAP_TOE	(IFCAP_TOE4 | IFCAP_TOE6)

#define	IFCAP_CANTCHANGE	(IFCAP_NETMAP)

#define	IFQ_MAXLEN	50
#define	IFNET_SLOWHZ	1		/* granularity is 1 second */




#define	IFF_UP		0x1		/* (n) interface is up */
#define	IFF_BROADCAST	0x2		/* (i) broadcast address valid */
#define	IFF_DEBUG	0x4		/* (n) turn on debugging */
#define	IFF_LOOPBACK	0x8		/* (i) is a loopback net */
#define	IFF_POINTOPOINT	0x10		/* (i) is a point-to-point link */
/*			0x20		   was IFF_SMART */
#define	IFF_DRV_RUNNING	0x40		/* (d) resources allocated */
#define	IFF_NOARP	0x80		/* (n) no address resolution protocol */
#define	IFF_PROMISC	0x100		/* (n) receive all packets */
#define	IFF_ALLMULTI	0x200		/* (n) receive all multicast packets */
#define	IFF_DRV_OACTIVE	0x400		/* (d) tx hardware queue is full */
#define	IFF_SIMPLEX	0x800		/* (i) can't hear own transmissions */
#define	IFF_LINK0	0x1000		/* per link layer defined bit */
#define	IFF_LINK1	0x2000		/* per link layer defined bit */
#define	IFF_LINK2	0x4000		/* per link layer defined bit */
#define	IFF_ALTPHYS	IFF_LINK2	/* use alternate physical connection */
#define	IFF_MULTICAST	0x8000		/* (i) supports multicast */
#define	IFF_CANTCONFIG	0x10000		/* (i) unconfigurable using ioctl(2) */
#define	IFF_PPROMISC	0x20000		/* (n) user-requested promisc mode */
#define	IFF_MONITOR	0x40000		/* (n) user-requested monitor mode */
#define	IFF_STATICARP	0x80000		/* (n) static ARP */
#define	IFF_DYING	0x200000	/* (n) interface is winding down */
#define	IFF_RENAMING	0x400000	/* (n) interface is being renamed */

#endif /* _FREEBSD_NET_IF_H_ */
