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

#ifndef __RAPH_KERNEL_NET_L2_H__
#define __RAPH_KERNEL_NET_L2_H__

#include <stdint.h>
#include "socket.h"
#include "../dev/layer.h"

class L2Ctrl {
protected:
  Socket *_socket;
  static const uint32_t kDevNumber = 16;
  uint32_t _devNumber = 0;
  DevNetL2 *_devList[kDevNumber];

public:
  virtual bool RegisterDevice(DevNetL2 *dev);
  virtual bool OpenSocket() = 0;
  virtual int32_t ReceiveData(uint8_t *data, uint32_t size) = 0;
  virtual int32_t TransmitData(const uint8_t *data, uint32_t length) = 0;
};

class L4Ctrl {
protected:
  L4Ctrl() {}

public:
  virtual int32_t Receive(uint8_t *data, uint32_t size) = 0;
  virtual int32_t Transmit(const uint8_t *data, uint32_t length) = 0;
};

#endif // __RAPH_KERNEL_NET_L2_H__
