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

#ifndef __RAPH_KERNEL_NETDEV_H__
#define __RAPH_KERNEL_NETDEV_H__

#include <stdint.h>
#include <string.h>
#include "../spinlock.h"
#include "../list.h"

class NetDev {
  static const uint32_t kNetworkInterfaceNameLen = 8;
  // network interface name
  char _name[kNetworkInterfaceNameLen];

public:
  virtual int32_t ReceivePacket(uint8_t *buffer, uint32_t size) = 0;
  virtual int32_t TransmitPacket(const uint8_t *packet, uint32_t length) = 0;
  virtual void GetEthAddr(uint8_t *buffer) {
    memcpy(buffer, _ethAddr, 6);
  }
  void SetName(const char *name) {
    strncpy(_name, name, kNetworkInterfaceNameLen);
  }
  const char *GetName() { return _name; }
protected:
  NetDev() {}
  SpinLock _lock;

  // ethernet address
  uint8_t _ethAddr[6];
  // IP address
  uint32_t _ipAddr = 0;
};

class NetDevCtrl {
  static const uint32_t kNetworkInterfaceNameLen = 8;
  static const uint32_t kMaxDevNumber = 32;
  static const char *kDefaultNetworkInterfaceName;
  uint32_t _curDevNumber = 0;
  NetDev *_devTable[kMaxDevNumber] = {nullptr};

public:
  NetDevCtrl() {}
  bool RegisterDevice(NetDev *dev, const char *name = kDefaultNetworkInterfaceName);
  NetDev *GetDevice(const char *name = kDefaultNetworkInterfaceName);
};
#endif /* __RAPH_KERNEL_NETDEV_H__ */
