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
#include "ip.h"
#include "../raph.h"
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../global.h"

int32_t IPCtrl::GenerateHeader(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr) {
  IPv4Header * volatile header = reinterpret_cast<IPv4Header*>(buffer);
  header->ipHdrLen_ver = (sizeof(IPv4Header) >> 2) | (kIPVersion << 4);
  header->type = kPktPriority | kPktDelay | kPktThroughput | kPktReliability;
  header->totalLen = htons(sizeof(IPv4Header) + length);
  header->id = _idAutoIncrement++;
  // TODO: fragment on IP layer
  uint16_t frag = 0;
  header->fragOffsetHi_flag = ((frag >> 8) & 0x1f) | kFlagNoFragment;
  header->fragOffsetLo = (frag & 0xff);
  header->ttl = kTimeToLive;
  header->protoId = type;
  header->checksum = 0;
  header->saddr = htonl(saddr);
  header->daddr = htonl(daddr);
  header->checksum = checkSum(reinterpret_cast<uint8_t*>(header), sizeof(IPv4Header));

  return 0;
}

bool IPCtrl::FilterPacket(uint8_t *packet, uint8_t type, uint32_t saddr, uint32_t daddr) {
  IPv4Header * volatile header = reinterpret_cast<IPv4Header*>(packet);
  return (header->protoId == type)
      && (!saddr || ntohl(header->saddr) == saddr)
      && (!daddr || ntohl(header->daddr) == daddr);
}

uint16_t IPCtrl::checkSum(uint8_t *buf, uint32_t size) {
  uint64_t sum = 0;

  while(size > 1) {
    sum += *reinterpret_cast<uint16_t*>(buf);
    buf += 2;
    if(sum & 0x80000000)   /* if high order bit set, fold */
      sum = (sum & 0xFFFF) + (sum >> 16);
    size -= 2;
  }

  if(size)  /* take care of left over byte */
    sum += static_cast<uint16_t>(*buf);
 
  while(sum >> 16)
    sum = (sum & 0xFFFF) + (sum >> 16);

  return ~sum;
}
