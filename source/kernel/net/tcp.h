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

#ifndef __RAPH_KERNEL_NET_TCP_H__
#define __RAPH_KERNEL_NET_TCP_H__

#include <stdint.h>
#include <stdlib.h>
#include <net/socket.h>
#include <net/layer.h>

struct TcpHeader {
  // source port
  uint16_t sport;
  // destination port
  uint16_t dport;
  // sequence number
  uint32_t seq_number;
  // acknowledge number
  uint32_t ack_number;
  // TCP header length (upper 4bit) | Reserved (lower 4bit)
  uint8_t header_len;
  // Session flags (lower 6bit) + Reserved (upper 2bit)
  uint8_t flag;
  // window size
  uint16_t window_size;
  // checksum
  uint16_t checksum;
  // urgent pointer
  uint16_t urgent_pointer;
} __attribute__ ((packed));

class TcpCtrl : public L4Ctrl {
public:
  TcpCtrl() {}
  virtual int32_t GenerateHeader(uint8_t *header, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack);
  virtual bool FilterPacket(uint8_t *packet, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack);
  uint8_t GetSessionType(uint8_t *packet);
  uint32_t GetSequenceNumber(uint8_t *packet);
  uint32_t GetAcknowledgeNumber(uint8_t *packet);

private:
  static const uint32_t kBasicBufsize     = 0x10;
  static const uint8_t kSrcPortOffset     = 0;
  static const uint8_t kDstPortOffset     = 2;
  static const uint8_t kSeqOffset         = 4;
  static const uint8_t kAckOffset         = 8;
  static const uint8_t kSessionTypeOffset = 13;
  static const uint8_t kWindowSizeOffset  = 14;

  uint16_t CheckSum(uint8_t *buf, uint32_t size, uint32_t saddr, uint32_t daddr);
};

#endif // __RAPH_KERNEL_NET_TCP_H__
