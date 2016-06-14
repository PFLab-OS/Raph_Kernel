/*
 * Fundamental constants relating to ethernet.
 *
 * $FreeBSD$
 *
 */

#ifndef _FREEBSD_NET_ETHERNET_H_
#define _FREEBSD_NET_ETHERNET_H_

#define ETHER_ADDR_LEN    6 /* length of an Ethernet address */
#define ETHER_TYPE_LEN    2 /* length of the Ethernet type field */
#define ETHER_CRC_LEN   4 /* length of the Ethernet CRC */
#define ETHER_HDR_LEN   (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)
#define ETHER_MIN_LEN   64  /* minimum frame len, including CRC */
#define ETHER_MAX_LEN   1518  /* maximum frame len, including CRC */
#define ETHER_MAX_LEN_JUMBO 9018  /* max jumbo frame len, including CRC */

#define ETHERMTU  (ETHER_MAX_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#define ETHERMIN  (ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#define ETHERMTU_JUMBO  (ETHER_MAX_LEN_JUMBO - ETHER_HDR_LEN - ETHER_CRC_LEN)

/*
 * 802.1q Virtual LAN header.
 */
struct ether_vlan_header {
	uint8_t evl_dhost[ETHER_ADDR_LEN];
	uint8_t evl_shost[ETHER_ADDR_LEN];
	uint16_t evl_encap_proto;
	uint16_t evl_tag;
	uint16_t evl_proto;
} __packed;

#endif /* _FREEBSD_NET_ETHERNET_H_ */

