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
#include <dev/netdev.h>

struct EthHeader {
  // destination MAC address
  uint8_t daddr[6];
  // source MAC address
  uint8_t saddr[6];
  // protocol type
  uint16_t type;
};

int32_t EthGenerateHeader(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type);
bool EthFilterPacket(uint8_t *packet, uint8_t *saddr, uint8_t *daddr, uint16_t type);

uint16_t GetL3PtclType(uint8_t *packet);

#endif // __RAPH_KERNEL_NET_ETH_H__
