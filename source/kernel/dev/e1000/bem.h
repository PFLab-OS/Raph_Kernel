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
 * Author: Liva
 *
 */

#ifndef __RAPH_KERNEL_E1000_BEM_H__
#define __RAPH_KERNEL_E1000_BEM_H__

#include <dev/eth.h>

class bE1000 : public DevEthernet {
public:
 bE1000(uint8_t bus, uint8_t device, bool mf) : DevEthernet(bus, device, mf) {}
};

struct adapter;

struct BsdDriver {
  bE1000 *parent;
  struct adapter *adapter;
};

typedef BsdDriver *device_t;

#endif /* __RAPH_KERNEL_E1000_BEM_H__ */

