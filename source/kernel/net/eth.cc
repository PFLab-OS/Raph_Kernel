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
 * Author: levelfour
 * 
 */

#include <net/eth.h>
#include <net/arp.h>
#include <net/ip.h>


const uint8_t EthernetLayer::kArpRequestMacAddress[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uint8_t EthernetLayer::kBroadcastMacAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


bool EthernetLayer::FilterPacket(NetDev::Packet *packet) {
  EthernetLayer::Header *header = reinterpret_cast<EthernetLayer::Header *>(packet->buf);

  if (!EthernetLayer::CompareAddress(header->daddr, _mac_address)) {
    return false;
  }

  if (ntohs(header->type) != _protocol) {
    return false;
  }

  return true;
}


bool EthernetLayer::PreparePacket(NetDev::Packet *packet) {
  EthernetLayer::Header *header = reinterpret_cast<EthernetLayer::Header *>(packet->buf);

  switch (_protocol) {
    case kProtocolArp:
      DetachProtocolHeader(packet);
      ArpLayer::GetHardwareDestinationAddress(packet, header->daddr);
      AttachProtocolHeader(packet);
      break;

    case kProtocolIpv4: {
      DetachProtocolHeader(packet);
      bool found = Ipv4Layer::GetHardwareDestinationAddress(packet, header->daddr);
      AttachProtocolHeader(packet);

      if (!found) {
        return false;
      }

      break;
    }

    default:
      return false;
  }

  memcpy(header->saddr, _mac_address, 6);
  header->type = htons(_protocol);

  return true;
}
