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
#include "layer.h"
#include "socket.h"

struct EthHeader {
  // destination MAC address
  uint16_t dstAddrHi;
  uint16_t dstAddrMd;
  uint16_t dstAddrLo;
  // source MAC address
  uint16_t srcAddrHi;
  uint16_t srcAddrMd;
  uint16_t srcAddrLo;
  // protocol type
  uint16_t type;
};

class EthCtrl : public L2Ctrl {
  static const uint16_t kProtocolIPv4 = (0x08) | (0x00 << 8);
  static const uint16_t kProtocolARP  = (0x08) | (0x06 << 8);

  static const uint32_t kDstAddrOffset      = 0;
  static const uint32_t kSrcAddrOffset      = 6;
  static const uint32_t kProtocolTypeOffset = 12;

public:
  EthCtrl() {}
  virtual bool OpenSocket();
  virtual int32_t ReceiveData(uint8_t *data,
                              uint32_t size,
                              uint8_t *protocolType = nullptr,
                              uint8_t *srcAddr = nullptr);
  virtual int32_t TransmitData(const uint8_t *data,
                               uint32_t length,
                               uint8_t *dstAddr);
};

#endif // __RAPH_KERNEL_NET_ETH_H__
