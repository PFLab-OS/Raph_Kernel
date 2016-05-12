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
 * Author: Levelfour
 * 
 */

#include <string.h>
#include <raph.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/ip.h>

// offset in header
const uint32_t kProtocolTypeOffset = 9;
const uint32_t kSrcAddrOffset      = 13;

// IPv4
const uint8_t kIpVersion           = 4;

// packet priority
const uint8_t kPktPriority         = (7 << 5);
const uint8_t kPktDelay            = (1 << 4);
const uint8_t kPktThroughput       = (1 << 3);
const uint8_t kPktReliability      = (1 << 2);

// packet flag
const uint8_t kFlagNoFragment      = (1 << 6);
const uint8_t kFlagMoreFragment    = (1 << 5);

// time to live (number of hops)
const uint8_t kTimeToLive          = 16;

// IP header id
uint16_t _id_auto_increment;

uint16_t CheckSum(uint8_t *buf, uint32_t size);

int32_t IpGenerateHeader(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr) {
  Ipv4Header * volatile header = reinterpret_cast<Ipv4Header*>(buffer);
  header->ip_header_len_version = (sizeof(Ipv4Header) >> 2) | (kIpVersion << 4);
  header->type = kPktPriority | kPktDelay | kPktThroughput | kPktReliability;
  header->total_len = htons(sizeof(Ipv4Header) + length);
  header->id = _id_auto_increment++;
  // TODO: fragment on IP layer
  uint16_t frag = 0;
  header->frag_offset_hi_flag = ((frag >> 8) & 0x1f) | kFlagNoFragment;
  header->frag_offset_lo = (frag & 0xff);
  header->ttl = kTimeToLive;
  header->proto_id = type;
  header->checksum = 0;
  header->saddr = htonl(saddr);
  header->daddr = htonl(daddr);
  header->checksum = CheckSum(reinterpret_cast<uint8_t*>(header), sizeof(Ipv4Header));

  return 0;
}

bool IpFilterPacket(uint8_t *packet, uint8_t type, uint32_t saddr, uint32_t daddr) {
  Ipv4Header * volatile header = reinterpret_cast<Ipv4Header*>(packet);
  return (header->proto_id == type)
      && (!saddr || ntohl(header->saddr) == saddr)
      && (!daddr || ntohl(header->daddr) == daddr);
}

uint32_t IpGetDestIpAddress(uint8_t *packet) {
  Ipv4Header * volatile header = reinterpret_cast<Ipv4Header*>(packet);
  return ntohl(header->daddr);
}

uint16_t CheckSum(uint8_t *buf, uint32_t size) {
  uint64_t sum = 0;

  while(size > 1) {
    sum += *reinterpret_cast<uint16_t*>(buf);
    buf += 2;
    if(sum & 0x80000000) {
      // if high order bit set, fold
      sum = (sum & 0xFFFF) + (sum >> 16);
    }
    size -= 2;
  }

  if(size) {
    // take care of left over byte
    sum += static_cast<uint16_t>(*buf);
  }
 
  while(sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  return ~sum;
}
