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

#ifndef __RAPH_KERNEL_NET_IP_H__
#define __RAPH_KERNEL_NET_IP_H__

#include <stdint.h>
#include "layer.h"
#include "socket.h"

/*
 * IPv4 Header
 */
struct IPv4Header {
  // IP Header Length (4bit) | Version (4bit)
  uint8_t ipHdrLen_ver;
  // TYPE of service
  uint8_t type;
  // Total Length
  uint16_t totalLen;
  // Identification
  uint16_t id;
  // Fragment Offset High (5bit) | MF (1bit) | NF (1bit) | Reserved (1bit)
  uint8_t fragOffsetHi_flag;
  // Fragment Offset Low (8bit)
  uint8_t fragOffsetLo;
  // Time to Live
  uint8_t ttl;
  // Layer 4 Protocol ID
  uint8_t protoId;
  // Header Checksum (NOTE: only on header)
  uint16_t checksum;
  // Source Address
  uint32_t srcAddr;
  // Destination Address
  uint32_t dstAddr;
} __attribute__ ((packed));

class IPCtrl {
  L2Ctrl *_l2Ctrl;
  uint16_t _idAutoIncrement;

  const uint8_t kIPVersion        = 4;
  const uint8_t kPktPriority      = (7 << 5);
  const uint8_t kPktDelay         = (1 << 4);
  const uint8_t kPktThroughput    = (1 << 3);
  const uint8_t kPktReliability   = (1 << 2);
  const uint8_t kFlagNoFragment   = (1 << 6);
  const uint8_t kFlagMoreFragment = (1 << 5);
  const uint8_t kTimeToLive       = 16;
  const uint8_t kProtocolIP       = 0;
  const uint8_t kProtocolICMP     = 1;
  const uint8_t kProtocolTCP      = 6;
  const uint8_t kProtocolUDP      = 17;

  uint16_t checkSum(uint8_t *buf, uint32_t size);

public:
  IPCtrl(L2Ctrl *l2Ctrl) : _l2Ctrl(l2Ctrl), _idAutoIncrement(0) {}
  virtual int32_t ReceiveData(uint8_t *data, uint32_t size);
  virtual int32_t TransmitData(const uint8_t *data, uint32_t length);
};

#endif // __RAPH_KERNEL_NET_IP_H__
