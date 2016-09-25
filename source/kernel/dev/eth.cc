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
#include <net/pstack.h>

void DevEthernet::FilterRxPacket(void *p) {
  NetDev::Packet *packet;
  assert(_rx_buffered.Pop(packet));
  packet->buf = packet->data;

  // filter by my MAC addresss
  uint8_t eth_addr[6];
  GetEthAddr(eth_addr);

  EthernetHeader *header = reinterpret_cast<EthernetHeader *>(packet->buf);

  if (!memcmp(eth_addr, header->daddr, 6) ||
      !memcmp(eth_addr, kBroadcastMacAddress, 6)) {
    _rx_filtered.Push(packet);
  } else {
    ReuseRxBuffer(packet);
  }
}

void DevEthernet::PrepareTxPacket(NetDev::Packet *packet) {
  // TODO: fetch from ARP table
  uint8_t eth_daddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

  EthernetHeader *header = reinterpret_cast<EthernetHeader *>(packet->buf);
  GetEthAddr(header->saddr);
  memcpy(header->daddr, eth_daddr, 6);
  header->type = htons(0x0806);  // TODO: do not hard-code
}
