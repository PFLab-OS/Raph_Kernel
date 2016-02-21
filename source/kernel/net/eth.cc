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

#ifndef __UNIT_TEST__

bool EthCtrl::OpenSocket() {
  if(_devNumber > 0) {
    // TODO: other protocol socket
    UDPSocket *addr = reinterpret_cast<UDPSocket *>(virtmem_ctrl->Alloc(sizeof(UDPSocket)));
    _socket = new(addr) UDPSocket(_devList[--_devNumber]);
    return true;
  } else {
    return false;
  }
}

int32_t EthCtrl::ReceiveData(uint8_t *data, uint32_t size) {
}

int32_t EthCtrl::TransmitData(const uint8_t *data, uint32_t length) {
  int32_t result = -1;

  if(_socket == nullptr) {
    OpenSocket();
  }

  // alloc datagram
  virt_addr vaddr = virtmem_ctrl->Alloc(sizeof(uint8_t) * (sizeof(EthHeader) + length));
  uint8_t *datagram = reinterpret_cast<uint8_t*>(k2p(vaddr));

  // construct header
  EthHeader header;
  // TODO: fetch MAC address
  header.dstAddrHi = 0x0008;
  header.dstAddrMd = 0x1c27;
  header.dstAddrLo = 0x935b;
  header.srcAddrHi = 0x5452;
  header.srcAddrMd = 0x1200;
  header.srcAddrLo = 0x5634;
  header.type      = kProtocolIPv4;

  // construct datagram
  memcpy(datagram, reinterpret_cast<uint8_t*>(&header), sizeof(EthHeader));
  memcpy(datagram + sizeof(EthHeader), data, length);

  // call device datagram transmitter
  kassert(_socket != nullptr);
  _socket->TransmitPacket(datagram, length);

  virtmem_ctrl->Free(vaddr);

  return result;
}

#endif
