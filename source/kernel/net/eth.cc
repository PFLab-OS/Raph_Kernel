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

#include "eth.h"
#include <string.h>
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../global.h"

int32_t EthCtrl::ReceiveData(uint8_t *data, uint32_t size, uint8_t *protocolType, uint8_t *srcAddr) {
  // alloc buffer
  int32_t bufsize = sizeof(EthHeader) + size;
  uint8_t *buffer = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(sizeof(uint8_t) * bufsize));

  int32_t result = _dev->ReceivePacket(buffer, bufsize);

  if(result != -1) {
    // success
    int32_t length = bufsize < result ? bufsize : result;
    kassert(length - sizeof(EthHeader) <= size);
    memcpy(data, buffer + sizeof(EthHeader), length - sizeof(EthHeader));
    if(protocolType)
      *protocolType = (buffer[kProtocolTypeOffset] << 8) | buffer[kProtocolTypeOffset + 1];
    if(srcAddr)
      memcpy(srcAddr, buffer+kSrcAddrOffset, 6);
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(buffer));
    return result - sizeof(EthHeader);
  } else {
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(buffer));
    return result;
  }
}

int32_t EthCtrl::TransmitData(const uint8_t *data, uint32_t length, uint8_t *destAddr) {
  int32_t result = -1;

  // alloc datagram
  uint8_t *datagram = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(sizeof(uint8_t) * (sizeof(EthHeader) + length))); 

  // construct header
  EthHeader header;
  // destination address
  memcpy(&header, destAddr, 6);
  // source address
  _dev->GetEthAddr(reinterpret_cast<uint8_t*>(&header) + 6);
  // TODO: switch L3 type
  header.type      = htons(kProtocolARP);

  // construct datagram
  memcpy(datagram, reinterpret_cast<uint8_t*>(&header), sizeof(EthHeader));
  memcpy(datagram + sizeof(EthHeader), data, length);

  // call device datagram transmitter
  kassert(_dev != nullptr);
  _dev->TransmitPacket(datagram, sizeof(EthHeader) + length);

  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(datagram));

  return result;
}
