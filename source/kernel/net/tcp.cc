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

/*
 * TCP header option parameters
 */
struct TcpOptionEol {
  uint8_t number;
} __attribute__((packed));

struct TcpOptionNop {
  uint8_t number;
} __attribute__((packed));

struct TcpOptionMss {
  uint8_t number;
  uint8_t length;
  uint16_t mss;
} __attribute__((packed));

struct TcpOptionWindowScale {
  uint8_t number;
  uint8_t length;
  uint8_t ws;
} __attribute__((packed));

struct TcpOptionSackPermitted {
  uint8_t number;
  uint8_t length;
} __attribute__((packed));

struct TcpOptionSack {
  uint8_t number;
  uint8_t length;
  uint32_t seq;
} __attribute__((packed));

struct TcpOptionTimeStamp {
  uint8_t number;
  uint8_t length;
  uint32_t ts;
  uint32_t ecr;
} __attribute__((packed));

// option number
static const uint8_t kOptionEndOfOptionList = 0;
static const uint8_t kOptionNoOperation     = 1;
static const uint8_t kOptionMaxSegmentSize  = 2;
static const uint8_t kOptionWindowScale     = 3;
static const uint8_t kOptionSackPermitted   = 4;
static const uint8_t kOptionSack            = 5;
static const uint8_t kOptionTimeStamp       = 8;

// option length
static const uint8_t kOptionLength[] = {
  0x01,
  0x01,
  0x04,
  0x03,
  0x02,
  0xff, // variable length
  0x00, // invalid
  0x00, // invalid
  0x0a,
};

uint16_t CheckSum(uint8_t *buf, uint32_t size, uint32_t saddr, uint32_t daddr);

int32_t TcpGenerateHeader(uint8_t *buffer, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack, struct TcpOptionParameters *options) {
  uint32_t option_offset = sizeof(TcpHeader);

  // generate option fields
  if(options) {
    bool option_field_used = false;

    if((type & Socket::kFlagSYN) && options->mss != TcpOptionParameters::kIgnore) {
      option_field_used = true;

      TcpOptionMss * volatile s = reinterpret_cast<TcpOptionMss*>(buffer + option_offset);
      s->number = kOptionMaxSegmentSize;
      s->length = kOptionLength[kOptionMaxSegmentSize];
      s->mss = options->mss;
      option_offset += kOptionLength[kOptionMaxSegmentSize];
    }

    if((type & Socket::kFlagSYN) && options->ws != TcpOptionParameters::kIgnore) {
      option_field_used = true;

      TcpOptionWindowScale * volatile s = reinterpret_cast<TcpOptionWindowScale*>(buffer + option_offset);
      s->number = kOptionWindowScale;
      s->length = kOptionLength[kOptionWindowScale];
      s->ws = options->ws;
      option_offset += kOptionLength[kOptionWindowScale];
    }

    if(option_field_used) {
      TcpOptionEol * volatile s = reinterpret_cast<TcpOptionEol*>(buffer + option_offset);
      s->number = kOptionEndOfOptionList;
      option_offset += kOptionLength[kOptionEndOfOptionList];
    }

    if(option_offset % 4) {
      // 4B-alignment padding
      uint16_t nb_padding = 4 - (option_offset % 4);
      for(uint16_t i = 0; i < nb_padding; i++) {
        buffer[option_offset] = 0;
        option_offset += 1;
      }
    }
  }

  // generate main fields
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(buffer);
  uint16_t header_length = option_offset;
  header->sport = htons(sport);
  header->dport = htons(dport);
  header->seq_number = htonl(seq);
  header->ack_number = htonl(ack);
  header->header_len = (header_length >> 2) << 4;
  header->flag = type;
  header->window_size = 0xffff;
  header->checksum = 0;
  header->urgent_pointer = 0;

  header->checksum = CheckSum(reinterpret_cast<uint8_t*>(header), header_length + length, saddr, daddr);

  return header_length;
}

bool TcpFilterPacket(uint8_t *packet, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack, struct TcpOptionParameters *options) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);

  // TODO: temporary ignore push flag
  uint8_t recv_type = header->flag & (~Socket::kFlagPSH);

  // NB: dport of packet sender == sport of packet receiver
  return (!sport || ntohs(header->dport) == sport)
      && (!dport || ntohs(header->sport) == dport)
      && ((recv_type & Socket::kFlagFIN) || recv_type == type);
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
  sum += ntohs(kProtocolTcp);
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
uint16_t TcpGetSourcePort(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return ntohs(header->sport);
}

// extract sender packet session type
uint8_t TcpGetSessionType(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return header->flag;
}

// extract sender sequence number from packet
uint32_t TcpGetSequenceNumber(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return ntohl(header->seq_number);
}

// extract sender acknowledge number from packet
uint32_t TcpGetAcknowledgeNumber(uint8_t *packet) {
  TcpHeader * volatile header = reinterpret_cast<TcpHeader*>(packet);
  return ntohl(header->ack_number);
}
