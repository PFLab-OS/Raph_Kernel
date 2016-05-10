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

#include <raph.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/udp.h>

int32_t UdpGenerateHeader(uint8_t *buffer, uint32_t length, uint16_t sport, uint16_t dport) {
  UdpHeader * volatile header = reinterpret_cast<UdpHeader*>(buffer);
  header->sport    = htons(sport);
  header->dport    = htons(dport);
  header->len      = htons(sizeof(UdpHeader) + length);
  header->checksum = 0;
  return 0;
}

bool UdpFilterPacket(uint8_t *packet, uint16_t sport, uint16_t dport) {
  UdpHeader * volatile header = reinterpret_cast<UdpHeader*>(packet);
  return (!sport || ntohs(header->sport) == sport)
      && (!dport || ntohs(header->dport) == dport);
}
