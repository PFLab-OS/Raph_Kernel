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
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../global.h"

int32_t IPCtrl::ReceiveData(uint8_t *data, uint32_t size) {
}

int32_t IPCtrl::TransmitData(const uint8_t *data, uint32_t length) {
  int32_t result = -1;

  uint32_t totalLen = sizeof(IPv4Header) + length;

  // alloc the buffer with length of (data + IPv4header)
  virt_addr vaddr = virtmem_ctrl->Alloc(sizeof(uint8_t) * (sizeof(IPv4Header) + length));
  uint8_t *datagram = reinterpret_cast<uint8_t*>(k2p(vaddr));

  // construct IPv4 header
  IPv4Header header;
  header.ipHdrLen_ver = (sizeof(IPv4Header) >> 2) | (kIPVersion << 4);
  header.type = kPktPriority | kPktDelay | kPktThroughput | kPktReliability;
  header.totalLen = (totalLen >> 8 & 0xff) | (totalLen << 8 & 0xff00);
  header.id = _idAutoIncrement++;
  // TODO: fragment on IP layer
  uint16_t frag = 0;
  header.fragOffsetHi_flag = ((frag >> 8) & 0x1f) | kFlagNoFragment;
  header.fragOffsetLo = (frag & 0xff);
  header.ttl = kTimeToLive;
  // TODO: switch protocol ID
  header.protoId = kProtocolUDP;
  header.checksum = 0;
  // TODO: how to get IP address?
  header.srcAddr = 0x0f02000a;
  header.dstAddr = 0x0f02000a;

  header.checksum = checkSum(reinterpret_cast<uint8_t*>(&header), sizeof(IPv4Header));

  // construct datagram
  memcpy(datagram, reinterpret_cast<uint8_t*>(&header), sizeof(IPv4Header));
  memcpy(datagram + sizeof(IPv4Header), data, length);
  
  // call EthCtrl::TransmitData
  result = _l2Ctrl->TransmitData(datagram, sizeof(IPv4Header) + length);

  virtmem_ctrl->Free(vaddr);

  return result;
}

uint16_t IPCtrl::checkSum(uint8_t *buf, uint32_t size) {
  uint16_t sum = 0;

  while(size > 1) {
    sum += *buf++;
    size -= 2;
  }
  if(size)
    sum += *buf;

  sum = (sum & 0xffff) + (sum >> 16);	/* add overflow counts */
  sum = (sum & 0xffff) + (sum >> 16);	/* once again */
  
  return ~sum;
}
