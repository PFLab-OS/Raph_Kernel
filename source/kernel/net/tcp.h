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

struct TcpOptionParameters {
  // If you do not set some options, substiture kIgnoreParameter to them
  static const uint32_t kIgnore = 0;

  // maximum segment size (only SYN)
  uint16_t mss;
  // window scale (only SYN)
  uint8_t ws;
};

int32_t TcpGenerateHeader(uint8_t *header, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack, struct TcpOptionParameters *options);
bool TcpFilterPacket(uint8_t *packet, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack, struct TcpOptionParameters *options);

// extract sender port
uint16_t GetSourcePort(uint8_t *packet);

// extract sender packet session type
uint8_t GetSessionType(uint8_t *packet);

// extract sender sequence number from packet
uint32_t GetSequenceNumber(uint8_t *packet);

// extract sender acknowledge number from packet
uint32_t GetAcknowledgeNumber(uint8_t *packet);

#endif // __RAPH_KERNEL_NET_TCP_H__
