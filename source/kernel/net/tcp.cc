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
#include "tcp.h"
#include "../raph.h"
#include "../mem/physmem.h"
#include "../mem/virtmem.h"
#include "../global.h"

int32_t TCPCtrl::GenerateHeader(uint8_t *buffer, uint32_t length, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack) {
  TCPHeader * volatile header = reinterpret_cast<TCPHeader*>(buffer);
  header->sport = htons(sport);
  header->dport = htons(dport);
  header->seqNumber = htonl(seq);
  header->ackNumber = htonl(ack);
  header->headerLen = (sizeof(TCPHeader) >> 2) << 4;
  header->flag = type;
  header->windowSize = 0;
  header->checksum = 0;  // TODO: calculate
  header->urgentPointer = 0;
  return 0;
}

bool TCPCtrl::FilterPacket(uint8_t *packet, uint16_t sport, uint16_t dport, uint8_t type, uint32_t seq, uint32_t ack) {
  TCPHeader * volatile header = reinterpret_cast<TCPHeader*>(packet);
  return (!sport || ntohs(header->sport) == sport)
      && (!dport || ntohs(header->dport) == dport)
      && (header->flag == type);
}

uint8_t TCPCtrl::GetSessionType(uint8_t *packet) {
  TCPHeader * volatile header = reinterpret_cast<TCPHeader*>(packet);
  return header->flag;
}

uint32_t TCPCtrl::GetSequenceNumber(uint8_t *packet) {
  TCPHeader * volatile header = reinterpret_cast<TCPHeader*>(packet);
  return ntohl(header->seqNumber);
}

uint32_t TCPCtrl::GetAcknowledgeNumber(uint8_t *packet) {
  TCPHeader * volatile header = reinterpret_cast<TCPHeader*>(packet);
  return ntohl(header->ackNumber);
}
