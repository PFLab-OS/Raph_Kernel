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

#ifndef __RAPH_KERNEL_NET_ARP_H__
#define __RAPH_KERNEL_NET_ARP_H__

#include <stdint.h>

struct ARPPacket {
  // hardware type
  uint16_t hwtype;
  // protocol type
  uint16_t protocol;
  // hardware length
  uint8_t hlen;
  // protocol length
  uint8_t plen;
  // operation
  uint16_t op;
  // source hardware address
  uint8_t hwSaddr[6];
  // source protocol address
  uint32_t protoSaddr;
  // destination hardware address
  uint8_t hwDaddr[6];
  // destination protocol address
  uint32_t protoDaddr;
} __attribute__((packed));

#endif // __RAPH_KERNEL_NET_ARP_H__
