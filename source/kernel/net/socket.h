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

#ifndef __RAPH_KERNEL_NET_SOCKET_H__
#define __RAPH_KERNEL_NET_SOCKET_H__

#include <stdint.h>
#include "../dev/netdev.h"

class NetSocket {
protected:
  NetDev *_dev = nullptr;

public:
  int32_t Open();
};

// TCP/IP Socket
class Socket : public NetSocket {
  uint32_t _daddr = 0x0a00020f;
  uint16_t _dport = 80;

public:
  Socket() {}
  void SetAddr(uint32_t addr) { _daddr = addr; }
  void SetPort(uint16_t port) { _dport = port; }
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length);
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length);
};

// ARP Socket
class ARPSocket : public NetSocket {
public:
  virtual int32_t TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha = nullptr);
  virtual int32_t ReceivePacket(uint16_t type);

  static const uint16_t kOpARPRequest = 0x0001;
  static const uint16_t kOpARPReply = 0x0002;
};

#endif // __RAPH_KERNEL_NET_SOCKET_H__
