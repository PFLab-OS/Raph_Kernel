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

#include "../mem/physmem.h"
#include "../mem/virtmem.h"

#include "socket.h"
#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

#define __NETCTRL__
#include "global.h"

int32_t NetSocket::Open() {
  _dev = netdev_ctrl->GetDevice();
  if(!_dev) {
    return -1;
  } else {
    return 0;
  }
}

int32_t Socket::TransmitPacket(const uint8_t *data, uint32_t length) {
  // alloc buffer
  uint32_t len = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader) + length;
  uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(len));

  // TCP header
  uint32_t offsetTCP = sizeof(EthHeader) + sizeof(IPv4Header);
  uint16_t sport = 80; // TODO:
  tcp_ctrl->GenerateHeader(packet + offsetTCP, len - offsetTCP, sport, _dport);

  // IP header
  uint32_t offsetIP = sizeof(EthHeader);
  uint32_t saddr = 0x0a00020f; // TODO:
  ip_ctrl->GenerateHeader(packet + offsetIP, len - offsetIP, IPCtrl::kProtocolTCP, saddr, _daddr);

  // Ethernet header
  uint8_t ethSaddr[6];
  uint8_t ethDaddr[6] = {0x08, 0x00, 0x27, 0xc1, 0x5b, 0x93}; // TODO:
  _dev->GetEthAddr(ethSaddr);
  eth_ctrl->GenerateHeader(packet, ethSaddr, ethDaddr, EthCtrl::kProtocolIPv4);

  // transmit
  _dev->TransmitPacket(packet, len);

  // finalization
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));

  return length;
}

int32_t Socket::ReceivePacket(uint8_t *data, uint32_t length) {
  // alloc buffer
  uint32_t len = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader) + length;
  uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(len));
  uint8_t ethDaddr[6];
  uint32_t ipDaddr = 0x0a00020f; // TODO:
  uint16_t dport = 80; // TODO:
  _dev->GetEthAddr(ethDaddr);

  do {
    // receive
    if(_dev->ReceivePacket(packet, length) < 0) continue;

    // filter Ethernet address
	if(!eth_ctrl->FilterPacket(packet, nullptr, ethDaddr, EthCtrl::kProtocolIPv4)) continue;

    // filter IP address
    uint32_t offsetIP = sizeof(EthHeader);
    if(!ip_ctrl->FilterPacket(packet + offsetIP, IPCtrl::kProtocolTCP, _daddr, ipDaddr)) continue;

    // filter TCP port
    uint32_t offsetTCP = sizeof(EthHeader) + sizeof(IPv4Header);
    if(!tcp_ctrl->FilterPacket(packet + offsetTCP, 0, dport)) continue;

    break;
  } while(1);

  // copy data
  uint32_t offset = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  memcpy(data, packet + offset, length);

  // finalization
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));

  return len;
}

int32_t ARPSocket::TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha) {
  uint32_t ipSaddr = 0x0a00020f; // TODO:
  uint32_t ipDaddr = tpa;
  uint8_t ethSaddr[6];
  uint8_t ethDaddr[6];
  _dev->GetEthAddr(ethSaddr);

  switch(type) {
    case kOpARPRequest:
      memset(ethDaddr, 0xff, 6); // broadcast
      break;
    case kOpARPReply:
      memcpy(ethDaddr, tha, 6);
      break;
    default:
      // unknown ARP operation
      return -1;
  }

  // alloc buffer
  uint32_t len = sizeof(EthHeader) + sizeof(ARPPacket);
  uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(len));


  // ARP header
  uint32_t offsetARP = sizeof(EthHeader);
  arp_ctrl->GeneratePacket(packet + offsetARP, type, ethSaddr, ipSaddr, ethDaddr, ipDaddr);

  // Ethernet header
  eth_ctrl->GenerateHeader(packet, ethSaddr, ethDaddr, EthCtrl::kProtocolARP);

  // transmit
  _dev->TransmitPacket(packet, len);

  // finalization
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));

  return len;
}

int32_t ARPSocket::ReceivePacket(uint16_t type) {
  // alloc buffer
  uint32_t length = sizeof(EthHeader) + sizeof(ARPPacket);
  uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(length));

  uint32_t offsetARP = sizeof(EthHeader);
  uint8_t ethDaddr[6];
  _dev->GetEthAddr(ethDaddr);

  do {
    // receive
    if(_dev->ReceivePacket(packet, length) < 0) continue;

    // filter Ethernet address
    if(!eth_ctrl->FilterPacket(packet, nullptr, ethDaddr, EthCtrl::kProtocolARP)) continue;

    // filter IP address
    if(!arp_ctrl->FilterPacket(packet + offsetARP, type, nullptr, 0, ethDaddr, 0)) continue;

    break;
  } while(1);

  // handle received ARP request/reply
  switch(type) {
    case kOpARPReply:
      arp_ctrl->RegisterAddress(packet + offsetARP);
      break;
    case kOpARPRequest:
      {
        // auto reply
        ARPPacket * volatile req = reinterpret_cast<ARPPacket*>(packet + offsetARP);
        TransmitPacket(kOpARPReply, req->protoSaddr, req->hwSaddr);
        break;
      }
    default:
      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
      return -1;
  }

  // finalization
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));

  return length;
}
