/*
 *
 * Copyright (c) 2016 Project Raphine
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

#ifndef __RAPH_KERNEL_NET_ETH_H__
#define __RAPH_KERNEL_NET_ETH_H__

#include <stdint.h>
#include <raph.h>
#include <dev/netdev.h>
#include <net/socket.h>

struct EthHeader {
  // destination MAC address
  uint8_t daddr[6];
  // source MAC address
  uint8_t saddr[6];
  // protocol type
  uint16_t type;
};

class EthCtrl {
  NetDev *_dev = nullptr;

  static const uint32_t kDstAddrOffset      = 0;
  static const uint32_t kSrcAddrOffset      = 6;
  static const uint32_t kProtocolTypeOffset = 12;
  static const uint8_t kBcastAddress[6];

public:
  EthCtrl() {}

  static const uint32_t kHeaderSize = sizeof(EthHeader);

  // protocol type
  static const uint16_t kProtocolIPv4 = 0x0800;
  static const uint16_t kProtocolARP  = 0x0806;

  int32_t GenerateHeader(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type);
  bool FilterPacket(uint8_t *packet, uint8_t *saddr, uint8_t *daddr, uint16_t type);
};

#endif // __RAPH_KERNEL_NET_ETH_H__
