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
#include "socket.h"
#include "layer.h"
#include "ip.h"

struct TCPHeader {
  // source port
  uint16_t srcPort;
  // destination port
  uint16_t dstPort;
  // sequence number
  uint32_t seqNumber;
  // acknowledge number
  uint32_t ackNumber;
  // TCP header length (upper 4bit) | Reserved (lower 4bit)
  uint8_t headerLen;
  // Session flags (lower 6bit) + Reserved (upper 2bit)
  uint8_t flag;
  // window size
  uint16_t windowSize;
  // checksum
  uint16_t checksum;
  // urgent pointer
  uint16_t urgentPointer;
} __attribute__ ((packed));

class TCPCtrl : public L4Ctrl {
  IPCtrl *_ipCtrl;
  uint32_t _dstIPAddr;
  uint32_t _dstPort;
  uint8_t _tcpSessionType;
  uint32_t _sequence;
  uint32_t _ack;
  uint32_t _port;  // source port

  static const uint8_t kProtoTCP          = 6;
  static const uint32_t kBasicBufsize     = 0x10;
  static const uint8_t kSrcPortOffset     = 0;
  static const uint8_t kDstPortOffset     = 2;
  static const uint8_t kSeqOffset         = 4;
  static const uint8_t kAckOffset         = 8;
  static const uint8_t kSessionTypeOffset = 13;
  static const uint8_t kWindowSizeOffset  = 14;
  static const uint8_t kFlagFIN           = 1 << 0;
  static const uint8_t kFlagSYN           = 1 << 1;
  static const uint8_t kFlagRST           = 1 << 2;
  static const uint8_t kFlagPSH           = 1 << 3;
  static const uint8_t kFlagACK           = 1 << 4;
  static const uint8_t kFlagURG           = 1 << 5;

  int32_t ReceiveBasic(uint8_t session, uint32_t port);
  int32_t TransmitBasic(uint8_t session,
                        uint32_t dstIPAddr,
                        uint32_t dstPort);
  inline void InitSequence() { _sequence = rand(); }

public:
  TCPCtrl(IPCtrl *ipCtrl)
    : _ipCtrl(ipCtrl), _dstIPAddr(0), _dstPort(0),
      _tcpSessionType(kFlagRST), _sequence(0), _ack(0),
      _port(0) {
    _ipCtrl->RegisterL4Ctrl(kProtoTCP, this);
  }
  virtual int32_t Receive(uint8_t *data, uint32_t size, uint32_t port);
  virtual int32_t Transmit(const uint8_t *data,
                           uint32_t length,
                           uint32_t dstIPAddr,
                           uint32_t dstPort,
                           uint32_t srcPort);
  int32_t Listen(uint32_t port = kPortHTTP);
  int32_t Connect(uint32_t dstIPAddr, uint32_t dstPort, uint32_t srcPort);
  int32_t Close();
};

#endif // __RAPH_KERNEL_NET_TCP_H__
