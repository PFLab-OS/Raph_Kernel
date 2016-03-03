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
#include "socket.h"
#include "arp.h"
#include "../raph.h"
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../global.h"

const uint8_t ARPCtrl::kBcastMACAddr[] = {0xff};

int32_t ARPCtrl::GeneratePacket(uint8_t *buffer, uint16_t op, uint8_t *smacaddr, uint32_t sipaddr, uint8_t *dmacaddr, uint32_t dipaddr) {
  ARPPacket * volatile packet = reinterpret_cast<ARPPacket*>(buffer);
  packet->hwtype = htons(kHWEthernet);
  packet->protocol = htons(kProtocolIPv4);
  packet->hlen = 6;
  packet->plen = 4;
  packet->op = htons(op);
  memcpy(packet->hwSaddr, smacaddr, 6);
  packet->protoSaddr = htonl(sipaddr);
  packet->protoDaddr = htonl(dipaddr);
  switch(op) {
    case ARPSocket::kOpARPRequest:
      memset(packet->hwDaddr, 0, 6);
      break;
	case ARPSocket::kOpARPReply:
      memcpy(packet->hwDaddr, dmacaddr, 6);
      break;
    default:
      // unknown ARP operation
      return -1;
  }

  return sizeof(ARPPacket);
}

bool ARPCtrl::FilterPacket(uint8_t *packet, uint16_t op, uint8_t *smacaddr, uint32_t sipaddr, uint8_t *dmacaddr, uint32_t dipaddr) {
  ARPPacket * volatile data = reinterpret_cast<ARPPacket*>(packet);
  return (ntohs(data->op) == op)
      && (!smacaddr || !memcmp(data->hwSaddr, smacaddr, 6))
      && (!sipaddr  || data->protoSaddr == sipaddr)
      && (!dmacaddr || !memcmp(data->hwDaddr, dmacaddr, 6) || !memcmp(data->hwDaddr, kBcastMACAddr, 6))
      && (!dipaddr  || data->protoDaddr == dipaddr);
}
