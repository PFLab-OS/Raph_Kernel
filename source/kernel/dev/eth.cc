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

#include <dev/eth.h>
#include <net/ptcl.h>
#include <net/eth.h>
#include <net/arp.h>
#include <net/socket.h>

void DevEthernetFilterRxPacket(void *self) {
  DevEthernet *device = reinterpret_cast<DevEthernet*>(self);

  NetDev::Packet *packet;
  kassert(device->_rx_buffered.Pop(packet));

  // filter by my MAC addresss
  uint8_t eth_addr[6];
  device->GetEthAddr(eth_addr);

  if (EthFilterPacket(packet->buf, nullptr, eth_addr, 0)) {
    device->_rx_filtered.Push(packet);
  } else {
    device->ReuseRxBuffer(packet);
  }
}

void DevEthernet::PrepareTxPacket(NetDev::Packet *packet) {
  uint16_t ptcl = GetL3PtclType(packet->buf);

  uint8_t eth_saddr[6];
  uint8_t eth_daddr[6] = {0x08, 0x00, 0x27, 0xc1, 0x5b, 0x93}; // TODO

  GetEthAddr(eth_saddr);

  // embed MAC address to packet
  if (ptcl == kProtocolIpv4) {
    EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolIpv4);
  } else if (ptcl == kProtocolArp) {
    uint16_t op = GetArpOperation(packet->buf);

    if (op == ArpSocket::kOpArpRequest) {
      ArpGeneratePacket(packet->buf + sizeof(EthHeader), 0, eth_saddr, 0, nullptr, 0);     
      EthGenerateHeader(packet->buf, eth_saddr, nullptr, kProtocolArp);
    } else if (op == ArpSocket::kOpArpReply) {
      ArpGeneratePacket(packet->buf + sizeof(EthHeader), 0, eth_saddr, 0, eth_daddr, 0);     
      EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolArp);
    }
  }
}