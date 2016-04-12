/*
 *
 * Copyright (c) 2016 Project Raphine
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
 * Author: Levelfour
 * 
 */

#ifndef __RAPH_KERNEL_NET_IP_H__
#define __RAPH_KERNEL_NET_IP_H__

#include <stdint.h>
#include <spinlock.h>

/*
 * IPv4 Header
 */
struct Ipv4Header {
  // IP Header Length (4bit) | Version (4bit)
  uint8_t ip_header_len_version;
  // TYPE of service
  uint8_t type;
  // Total Length
  uint16_t total_len;
  // Identification
  uint16_t id;
  // Fragment Offset High (5bit) | MF (1bit) | NF (1bit) | Reserved (1bit)
  uint8_t frag_offset_hi_flag;
  // Fragment Offset Low (8bit)
  uint8_t frag_offset_lo;
  // Time to Live
  uint8_t ttl;
  // Layer 4 Protocol ID
  uint8_t proto_id;
  // Header Checksum (NOTE: only on header)
  uint16_t checksum;
  // Source Address
  uint32_t saddr;
  // Destination Address
  uint32_t daddr;
} __attribute__ ((packed));

class IpCtrl {
public:
  IpCtrl() : _id_auto_increment(0) {}

  int32_t GenerateHeader(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr);
  bool FilterPacket(uint8_t *packet, uint8_t type, uint32_t saddr, uint32_t daddr);

  // Layer 4 protocols
  static const uint8_t kProtocolTCP         = 0x06;
  static const uint8_t kProtocolUDP         = 0x11;

private:
  // offset in header
  static const uint32_t kProtocolTypeOffset = 9;
  static const uint32_t kSrcAddrOffset      = 13;

  // IPv4
  static const uint8_t kIPVersion           = 4;

  // packet priority
  static const uint8_t kPktPriority         = (7 << 5);
  static const uint8_t kPktDelay            = (1 << 4);
  static const uint8_t kPktThroughput       = (1 << 3);
  static const uint8_t kPktReliability      = (1 << 2);

  // packet flag
  static const uint8_t kFlagNoFragment      = (1 << 6);
  static const uint8_t kFlagMoreFragment    = (1 << 5);

  // time to live (number of hops)
  static const uint8_t kTimeToLive          = 16;

  SpinLock _lock;

  // IP header id
  uint16_t _id_auto_increment;

  uint16_t CheckSum(uint8_t *buf, uint32_t size);
};

#endif // __RAPH_KERNEL_NET_IP_H__
