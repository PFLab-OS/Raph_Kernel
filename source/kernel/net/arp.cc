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
#include <raph.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/socket.h>
#include <net/arp.h>

// hardware type
const uint16_t kHWEthernet = 0x0001;

// protocol
const uint16_t kProtocolIPv4 = 0x0800;

// broadcast MAC address (ff:ff:ff:ff:ff:ff)
const uint8_t kBcastMACAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

int32_t ArpGeneratePacket(uint8_t *buffer, uint16_t op, uint8_t *smacaddr, uint32_t sipaddr, uint8_t *dmacaddr, uint32_t dipaddr) {
  ArpPacket * volatile packet = reinterpret_cast<ArpPacket*>(buffer);
  packet->hwtype = htons(kHWEthernet);
  packet->protocol = htons(kProtocolIPv4);
  packet->hlen = 6;
  packet->plen = 4;
  packet->op = htons(op);
  memcpy(packet->hw_saddr, smacaddr, 6);
  packet->proto_saddr = htonl(sipaddr);
  packet->proto_daddr = htonl(dipaddr);
  switch(op) {
    case ArpSocket::kOpARPRequest:
      memset(packet->hw_daddr, 0, 6);
      break;
	case ArpSocket::kOpARPReply:
      memcpy(packet->hw_daddr, dmacaddr, 6);
      break;
    default:
      // unknown ARP operation
      return -1;
  }

  return sizeof(ArpPacket);
}

bool ArpFilterPacket(uint8_t *packet, uint16_t op, uint8_t *smacaddr, uint32_t sipaddr, uint8_t *dmacaddr, uint32_t dipaddr) {
  ArpPacket * volatile data = reinterpret_cast<ArpPacket*>(packet);
  return (!op || ntohs(data->op) == op)
      && (!smacaddr || !memcmp(data->hw_saddr, smacaddr, 6))
      && (!sipaddr  || ntohl(data->proto_saddr) == sipaddr)
      && (ntohs(data->op) == ArpSocket::kOpARPRequest || !dmacaddr || !memcmp(data->hw_daddr, dmacaddr, 6) || !memcmp(data->hw_daddr, kBcastMACAddr, 6))
      && (!dipaddr  || ntohl(data->proto_daddr) == dipaddr);
}

bool RegisterIpAddress(uint8_t *packet) {
  ArpPacket * volatile arp = reinterpret_cast<ArpPacket*>(packet);
  return arp_table->Add(arp->proto_saddr, arp->hw_saddr);
}

void GetSourceMacAddress(uint8_t *buffer, uint8_t *packet) {
  ArpPacket * volatile arp = reinterpret_cast<ArpPacket*>(packet);
  memcpy(buffer, arp->hw_saddr, 6);
}

uint32_t GetSourceIpAddress(uint8_t *packet) {
  ArpPacket * volatile arp = reinterpret_cast<ArpPacket*>(packet);
  return ntohl(arp->proto_saddr);
}

/*
 * ArpTable
 */

ArpTable::ArpTable() {
  for(uint32_t i = 0; i < kMaxNumberRecords; i++) {
    _table[i].ipaddr = 0;
  }
}

bool ArpTable::Add(uint32_t ipaddr, uint8_t *macaddr) {
  Locker locker(_lock);

  uint32_t index = Hash(ipaddr);
  while(_table[index].ipaddr != 0) {
    // record already exists
    index = Probe(index);
  }
  // new record
  memcpy(_table[index].macaddr, macaddr, 6);
  return true;
}

bool ArpTable::Find(uint32_t ipaddr, uint8_t *macaddr) {
  Locker locker(_lock);

  uint32_t index = Hash(ipaddr);

  // TODO: must set the upper bound of probing
  while(_table[index].ipaddr != ipaddr) {
    if(_table[index].ipaddr == 0) {
      // does not exist
      return false;
	} else {
      // conflict
      index = Probe(index);
    }
  }
  memcpy(macaddr, _table[index].macaddr, 6);
  return true;
}
