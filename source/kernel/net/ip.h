/*
 *
 * Copyright (c) 2016 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: levelfour
 * 
 */

#ifndef __RAPH_KERNEL_NET_IP_H__
#define __RAPH_KERNEL_NET_IP_H__


#include <net/pstack.h>


class Ipv4Layer : public ProtocolStackLayer {
public:
  /** version */
  static const uint8_t kIpVersion = 4;

  /** priority */
  static const uint8_t kPktPriority    = 7 << 5;
  static const uint8_t kPktDelay       = 7 << 4;
  static const uint8_t kPktThroughput  = 7 << 3;
  static const uint8_t kPktReliability = 7 << 2;

  /** packet flag */
  static const uint16_t kFlagNoFragment   = 1 << 14;
  static const uint16_t kFlagMoreFragment = 1 << 13;

  /** layer 4 protocols */
  static const uint8_t kProtocolTcp = 0x06;
  static const uint8_t kProtocolUdp = 0x11;

  Ipv4Layer() {}

  /**
   * IPv4 header
   */
  struct Header {
    uint8_t hlen: 4;       /** IP header length */
    uint8_t ver: 4;        /** version */
    uint8_t type;          /** type of service */
    uint16_t total_len;    /** total length of IPv4 packet */
    uint16_t id;           /** identification */
    uint16_t frag_offset;  /** Rsv | DF | MF | fragment offset (13bit) */
    uint8_t ttl;           /** time to live */
    uint8_t pid;           /** layer 4 protocol ID */
    uint16_t checksum;     /** header checksum */
    uint32_t saddr;        /** source address */
    uint32_t daddr;        /** destination address */
  } __attribute__((packed));

  /**
   * Set my IPv4 address.
   *
   * @param ipv4_addr
   */
  void SetAddress(uint32_t ipv4_addr) {
    _ipv4_addr = ipv4_addr;
  }

  /**
   * Set peer IPv4 address.
   *
   * @param ipv4_addr
   */
  void SetPeerAddress(uint32_t ipv4_addr) {
    _peer_addr = ipv4_addr;
  }

  /**
   * Set upper layer protocol type.
   *
   * @param protocol
   */
  void SetProtocol(uint8_t protocol) {
    _upper_protocol = protocol;
  }

  /**
   * Calculate IPv4 checksum.
   *
   * @param header IPv4 header.
   * @param size IPv4 header length.
   * @return checksum.
   */
  uint16_t CheckSum(Header *header, uint32_t size) {
    uint8_t *buf = reinterpret_cast<uint8_t *>(header);
    uint64_t sum = 0;
  
    while (size > 1) {
      sum += *reinterpret_cast<uint16_t*>(buf);
      buf += 2;
      if(sum & 0x80000000) {
        // if high order bit set, fold
        sum = (sum & 0xFFFF) + (sum >> 16);
      }
      size -= 2;
    }
  
    if (size) {
      // take care of left over byte
      sum += static_cast<uint16_t>(*buf);
    }
   
    while (sum >> 16) {
      sum = (sum & 0xFFFF) + (sum >> 16);
    }
  
    return ~sum;
  }

  /**
   * Get hardware destination address from IPv4 address embedded in the packet.
   *
   * @param packet IPv4 packet.
   * @param addr found hardware address.
   * @return if hardware address found in ARP table.
   */
  static bool GetHardwareDestinationAddress(NetDev::Packet *packet, uint8_t *addr);

protected:
  /**
   * Return IPv4 header size.
   * 
   * @return protocol header length.
   */
  virtual int GetProtocolHeaderLength() {
    return sizeof(Ipv4Layer::Header);
  }

  /**
   * Filter the received packet by its header content.
   *
   * @param packet
   * @return if the packet is to be received or not.
   */
  virtual bool FilterPacket(NetDev::Packet *packet);

  /**
   * Make contents of the header before transmitting the packet.
   *
   * @param packet
   * @return if succeeds.
   */
  virtual bool PreparePacket(NetDev::Packet *packet);

private:
  /** constant representing no IPv4 address is set */
  static const uint32_t kAddressNotSet = 0;

  /** IPv4 address assigned to the network device under this layer */
  uint32_t _ipv4_addr = kAddressNotSet;

  /** IPv4 address of the peer */
  uint32_t _peer_addr = kAddressNotSet;

  /** layer 4 protocol type stacked on this layer */
  uint8_t _upper_protocol;

  /** value for id in header */
  uint16_t _id = 0;

  /** time to live (number of hops) */
  uint8_t _ttl = 64;

  void ResetPeerAddress() {
    _peer_addr = kAddressNotSet;
  }
};


#endif // __RAPH_KERNEL_NET_IP_H__
