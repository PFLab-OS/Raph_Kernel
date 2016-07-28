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
#include <global.h>
#include <dev/eth.h>
#include <net/ptcl.h>
#include <net/eth.h>
#include <net/ip.h>
#include <net/arp.h>
#include <net/socket.h>

void DevEthernet::FilterRxPacket(void *p) {
  NetDev::Packet *packet;
  kassert(_rx_buffered.Pop(packet));

  // filter by my MAC addresss
  uint8_t eth_addr[6];
  GetEthAddr(eth_addr);

  if (EthFilterPacket(packet->buf, nullptr, eth_addr, 0)) {
    _rx_filtered.Push(packet);
  } else {
    ReuseRxBuffer(packet);
  }
}

void DevEthernet::PrepareTxPacket(NetDev::Packet *packet) {
  uint16_t ptcl = GetL3PtclType(packet->buf);

  uint8_t eth_saddr[6];
  uint8_t eth_daddr[6];

  GetEthAddr(eth_saddr);

  // embed MAC address to packet
  if (ptcl == kProtocolIpv4) {
    uint32_t ipaddr = IpGetDestIpAddress(packet->buf + sizeof(EthHeader));
    arp_table->Find(ipaddr, eth_daddr);
    EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolIpv4);
  } else if (ptcl == kProtocolArp) {
    uint16_t op = ArpGetOperation(packet->buf + sizeof(EthHeader));

    if (op == ArpSocket::kOpArpRequest) {
      memset(eth_daddr, 0xff, 6);
      ArpGeneratePacket(packet->buf + sizeof(EthHeader), 0, eth_saddr, 0, nullptr, 0);     
      EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolArp);
    } else if (op == ArpSocket::kOpArpReply) {
      ArpGetDestMacAddress(eth_daddr, packet->buf + sizeof(EthHeader));
      ArpGeneratePacket(packet->buf + sizeof(EthHeader), 0, eth_saddr, 0, eth_daddr, 0);     
      EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolArp);
    }
  }
}
