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
#include <global.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/eth.h>

// field offset in Ethernet header
const uint32_t kDstAddrOffset      = 0;
const uint32_t kSrcAddrOffset      = 6;
const uint32_t kProtocolTypeOffset = 12;

// broadcast MAC address (ff:ff:ff:ff:ff:ff)
const uint8_t kBcastAddress[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

int32_t EthGenerateHeader(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type) {
  EthHeader * volatile header = reinterpret_cast<EthHeader*>(buffer);
  memcpy(header->daddr, daddr, 6);
  memcpy(header->saddr, saddr, 6);
  header->type = htons(type);
  return 0;
}

bool EthFilterPacket(uint8_t *packet, uint8_t *saddr, uint8_t *daddr, uint16_t type) {
  EthHeader * volatile header = reinterpret_cast<EthHeader*>(packet);
  return (!daddr || !memcmp(header->daddr, daddr, 6) || !memcmp(header->daddr, kBcastAddress, 6))
      && (!saddr || !memcmp(header->saddr, saddr, 6))
      && (!type || ntohs(header->type) == type);
}

uint16_t GetL3PtclType(uint8_t *packet) {
  EthHeader * volatile header = reinterpret_cast<EthHeader*>(packet);
  return ntohs(header->type);
}
