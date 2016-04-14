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

#ifndef __RAPH_KERNEL_NET_PTCL_H__
#define __RAPH_KERNEL_NET_PTCL_H__

// Layer 3 protocols
const uint16_t kProtocolIPv4 = 0x0800;
const uint16_t kProtocolARP  = 0x0806;

// Layer 4 protocols
const uint8_t kProtocolTCP         = 0x06;
const uint8_t kProtocolUDP         = 0x11;

#endif // __RAPH_KERNEL_NET_PTCL_H__
