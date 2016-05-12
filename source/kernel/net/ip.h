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

int32_t IpGenerateHeader(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr);
bool IpFilterPacket(uint8_t *packet, uint8_t type, uint32_t saddr, uint32_t daddr);

uint32_t IpGetDestIpAddress(uint8_t *packet);

#endif // __RAPH_KERNEL_NET_IP_H__
