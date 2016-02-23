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
#include "../dev/layer.h"

class Socket {
protected:
  DevNetL2 *_dev;

public:
  Socket(DevNetL2 *dev) : _dev(dev) {}
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length) = 0;
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length) = 0;

  virtual void GetEthAddr(uint8_t *addr);
};

class UDPSocket : public Socket {
public:
  UDPSocket(DevNetL2 *dev) : Socket(dev) {}
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length);
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length);
};

#endif // __RAPH_KERNEL_NET_SOCKET_H__
