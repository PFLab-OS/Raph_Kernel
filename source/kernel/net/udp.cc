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

#include "udp.h"
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../global.h"

#include <stdio.h>

int32_t UDPCtrl::Receive(uint8_t *data, uint32_t size, uint32_t port) {
  // alloc buffer
  int32_t bufsize = sizeof(UDPHeader) + size;
  uint8_t *buffer = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(sizeof(uint8_t) * bufsize));

  int32_t result = -1;
  uint16_t receivedPort = 0;

  while(result == -1 || receivedPort != port) {
    // succeed to receive packet and correspond to the specified port
    result = _ipCtrl->ReceiveData(buffer, bufsize, kProtoUDP);
    receivedPort = (buffer[kDstPortOffset] << 8) | buffer[kDstPortOffset + 1];
  }

  if(result > 0) {
    int32_t length = bufsize < result ? bufsize : result;
    memcpy(data, buffer + sizeof(UDPHeader), length);
  
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(buffer));
    return result - sizeof(UDPHeader);
  } else {
    return result;
  }
}

int32_t UDPCtrl::Transmit(const uint8_t *data, uint32_t length, uint32_t dstPort, uint32_t srcPort) {
  int32_t result = -1;

  // alloc datagram
  uint8_t *datagram = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(sizeof(uint8_t) * (sizeof(UDPHeader) + length)));

  // construct header
  UDPHeader header;
  header.srcPort  = (srcPort >> 8) | ((srcPort & 0xff) << 8);
  header.dstPort  = (dstPort >> 8) | ((dstPort & 0xff) << 8);
  header.len      = sizeof(UDPHeader) + length;
  header.len      = (header.len >> 8) | ((header.len & 0xff) << 8);
  header.checksum = 0;  // TODO: calculate

  // construct datagram
  memcpy(datagram, reinterpret_cast<uint8_t*>(&header), sizeof(UDPHeader));
  memcpy(datagram + sizeof(UDPHeader), data, length);

  // call IPCtrl::TransmitData
  _ipCtrl->TransmitData(datagram, sizeof(UDPHeader) + length, kProtoUDP);

  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(datagram));

  return result;
}
