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

int32_t UDPCtrl::Receive(uint8_t *data, uint32_t size) {
  int32_t result = -1;
  return result;
}

int32_t UDPCtrl::Transmit(const uint8_t *data, uint32_t length) {
  int32_t result = -1;

  // alloc datagram
  virt_addr vaddr = virtmem_ctrl->Alloc(sizeof(uint8_t) * (sizeof(UDPHeader) + length));
  uint8_t *datagram = reinterpret_cast<uint8_t*>(k2p(vaddr));

  // construct header
  UDPHeader header;
  header.srcPort  = kPortHTTP;
  header.dstPort  = kPortHTTP;
  header.len      = sizeof(UDPHeader) + length;
  header.checksum = 0;  // TODO: calculate

  // construct datagram
  memcpy(datagram, reinterpret_cast<uint8_t*>(&header), sizeof(UDPHeader));
  memcpy(datagram + sizeof(UDPHeader), data, length);

  // call IPCtrl::TransmitData
  _ipCtrl->TransmitData(datagram, sizeof(UDPHeader) + length);

  virtmem_ctrl->Free(vaddr);

  return result;
}
