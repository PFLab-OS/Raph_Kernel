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
 * Author: Levelfour
 * 
 */

#include <stdlib.h>
#include <raph.h>
#include <global.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/ip.h>
#include <net/tcp.h>
#include <net/ptcl.h>

// offset in header
static const uint8_t kSrcPortOffset     = 0;
static const uint8_t kDstPortOffset     = 2;
static const uint8_t kSeqOffset         = 4;
static const uint8_t kAckOffset         = 8;
static const uint8_t kSessionTypeOffset = 13;
static const uint8_t kWindowSizeOffset  = 14;

uint16_t CheckSum(uint8_t *buf, uint32_t size, uint32_t saddr, uint32_t daddr);

int32_t TcpGenerateHeader(uint8_t *buffer, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(buffer);
  header->sport = htons(sport);
  header->dport = htons(dport);
  header->seq_number = htonl(seq);
  header->ack_number = htonl(ack);
  header->header_len = (sizeof(TcpHeader) >> 2) << 4;
  header->flag = type;
  header->window_size = 0;
  header->checksum = 0;
  header->urgent_pointer = 0;

  header->checksum = CheckSum(reinterpret_cast<uint8_t*>(header), length, saddr, daddr);

  return 0;
}

bool TcpFilterPacket(uint8_t *packet, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  // NB: dport of packet sender == sport of packet receiver
  return (!sport || ntohs(header->dport) == sport)
      && (!dport || ntohs(header->sport) == dport)
      && ((header->flag & Socket::kFlagFIN) || header->flag == type);
}

uint16_t CheckSum(uint8_t *buf, uint32_t size, uint32_t saddr, uint32_t daddr) {
  uint64_t sum = 0;

  // pseudo header
  sum += ntohs((saddr >> 16) & 0xffff);
  if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);
  sum += ntohs((saddr >> 0) & 0xffff);
  if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);
  sum += ntohs((daddr >> 16) & 0xffff);
  if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);
  sum += ntohs((daddr >> 0) & 0xffff);
  if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);
  sum += ntohs(kProtocolTCP);
  if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);
  sum += ntohs(static_cast<uint16_t>(size));
  if(sum & 0x80000000) sum = (sum & 0xffff) + (sum >> 16);

  // (true) TCP header and body
  while(size > 1) {
    sum += *reinterpret_cast<uint16_t*>(buf);
    buf += 2;
    if(sum & 0x80000000) {
      // if high order bit set, fold
      sum = (sum & 0xffff) + (sum >> 16);
    }
    size -= 2;
  }

  if(size) {
    // take care of left over byte
    sum += static_cast<uint16_t>(*buf);
  }
 
  while(sum >> 16) {
    sum = (sum & 0xffff) + (sum >> 16);
  }

  return ~sum;
}

// extract sender port
uint16_t GetSourcePort(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return ntohs(header->sport);
}

// extract sender packet session type
uint8_t GetSessionType(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return header->flag;
}

// extract sender sequence number from packet
uint32_t GetSequenceNumber(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return ntohl(header->seq_number);
}

// extract sender acknowledge number from packet
uint32_t GetAcknowledgeNumber(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return ntohl(header->ack_number);
}
