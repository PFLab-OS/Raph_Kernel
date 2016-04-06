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

#ifndef __RAPH_KERNEL_DEV_ETH_H__
#define __RAPH_KERNEL_DEV_ETH_H__

#include "netdev.h"
#include "pci.h"

class DevEthernet : public DevPCI, public NetDev {
public:
  DevEthernet(uint8_t bus, uint8_t device, bool mf) : DevPCI(bus, device, mf) {}
  // allocate 6 byte before call
  virtual void GetEthAddr(uint8_t *buffer) = 0;
  virtual void SetupNetInterface() = 0;
};

class DevEthernetCtrl : public NetDevCtrl {
  DevEthernet *_devTable[kMaxDevNumber] = {nullptr};
};

#endif /* __RAPH_KERNEL_DEV_ETH_H__ */
