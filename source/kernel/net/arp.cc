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


#include <net/arp.h>
#include <net/eth.h>
#include <dev/eth.h>

int ArpSocket::Open() {
  NetDevCtrl::NetDevInfo *devinfo = netdev_ctrl->GetDeviceInfo();
  DevEthernet *device = static_cast<DevEthernet *>(devinfo->device);
  ProtocolStack *pstack = devinfo->ptcl_stack;

  uint8_t eth_addr[6];
  device->GetEthAddr(eth_addr);

  // stack construction (BaseLayer > EthernetLayer > ArpLayer > ArpSocket)
  ProtocolStackBaseLayer *base_layer_addr = reinterpret_cast<ProtocolStackBaseLayer *>(virtmem_ctrl->Alloc(sizeof(ProtocolStackBaseLayer)));
  ProtocolStackBaseLayer *base_layer = new(base_layer_addr) ProtocolStackBaseLayer();
  base_layer->Setup(nullptr);
  pstack->SetBaseLayer(base_layer);

  EthernetLayer *eth_layer_addr = reinterpret_cast<EthernetLayer *>(virtmem_ctrl->Alloc(sizeof(EthernetLayer)));
  EthernetLayer *eth_layer = new(eth_layer_addr) EthernetLayer();
  eth_layer->Setup(base_layer);
  eth_layer->SetAddress(eth_addr);
  eth_layer->SetUpperProtocolType(EthernetLayer::kProtocolArp);

  ArpLayer *arp_layer_addr = reinterpret_cast<ArpLayer *>(virtmem_ctrl->Alloc(sizeof(ArpLayer)));
  ArpLayer *arp_layer = new(arp_layer_addr) ArpLayer();
  arp_layer->Setup(eth_layer);
  arp_layer->SetAddress(eth_addr, _ipv4_addr);

  return this->Setup(arp_layer) ? 0 : -1;
}


bool ArpLayer::FilterPacket(NetDev::Packet *packet) {
  ArpLayer::Header *header = reinterpret_cast<ArpLayer::Header *>(packet->buf);

  if (!EthernetLayer::CompareAddress(header->hdaddr, _eth_addr)) {
    return false;
  }

  if (_ipv4_addr != ntohl(header->pdaddr)) {
    return false;
  }

  // ARP destination address and operation specified by ArpSocket
  ArpSocket::Chunk *chunk = reinterpret_cast<ArpSocket::Chunk *>(packet->buf + GetProtocolHeaderLength());
  chunk->operation = ntohs(header->op);
  chunk->ipv4_addr = ntohl(header->psaddr);

  // register to ARP table
  arp_table->Add(ntohl(header->psaddr), header->hsaddr);

  return true;
}


bool ArpLayer::PreparePacket(NetDev::Packet *packet) {
  ArpLayer::Header *header = reinterpret_cast<ArpLayer::Header *>(packet->buf);
  header->htype  = htons(0x0001);  // Ethernet
  header->ptcl   = htons(0x0800);  // IPv4
  header->hlen   = 6;
  header->plen   = 4;
  memcpy(header->hsaddr, _eth_addr, 6);
  header->psaddr = htonl(_ipv4_addr);

  // ARP destination address and operation specified by ArpSocket
  ArpSocket::Chunk *chunk = reinterpret_cast<ArpSocket::Chunk *>(packet->buf + GetProtocolHeaderLength());
  header->op     = htons(chunk->operation);
  header->pdaddr = htonl(chunk->ipv4_addr);

  switch(chunk->operation) {
    case ArpSocket::kOpRequest:
      // ARP request is sent by broadcast mode
      EthernetLayer::SetBroadcastAddress(header->hdaddr);
      break;

    case ArpSocket::kOpReply:
      if (!arp_table->Find(chunk->ipv4_addr, header->hdaddr)) {
        return false;
      }

      break;
  }

  return true;
}

/*
 * ArpTable
 */

void ArpTable::Setup() {
  for(uint32_t i = 0; i < kMaxNumberRecords; i++) {
    _table[i].ipaddr = 0;
  }
}


bool ArpTable::Add(uint32_t ipaddr, uint8_t *macaddr) {
  Locker locker(_lock);

  uint32_t index = Hash(ipaddr);

  for (int i = 0; i < kMaxProbingNumber; i++) {
    if (_table[index].ipaddr == 0) {
      // new record
      memcpy(_table[index].macaddr, macaddr, 6);
      _table[index].ipaddr = ipaddr;
      return true;
    } else if (_table[index].ipaddr == ipaddr) {
      // already added
      return true;
    } else {
      // conflict
      index = Probe(index);
    }
  }

  return false;
}


bool ArpTable::Find(uint32_t ipaddr, uint8_t *macaddr) {
  Locker locker(_lock);

  uint32_t index = Hash(ipaddr);

  for (int i = 0; i < kMaxProbingNumber; i++) {
    if (_table[index].ipaddr == 0) {
      // does not exist
      return false;
    } else if (_table[index].ipaddr == ipaddr) {
      // record found
      memcpy(macaddr, _table[index].macaddr, 6);
      return true;
  	} else {
      // conflict
      index = Probe(index);
    }
  }

  return false;
}


bool ArpTable::Exists(uint32_t ipaddr) {
  Locker locker(_lock);

  uint32_t index = Hash(ipaddr);

  for (int i = 0; i < kMaxProbingNumber; i++) {
    if (_table[index].ipaddr == 0) {
      // does not exists
      return false;
    } else if (_table[index].ipaddr == ipaddr) {
      // record found
      return true;
    } else {
      // conflict
      index = Probe(index);
    }
  }

  return false;
}
