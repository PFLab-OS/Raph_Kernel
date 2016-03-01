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

int32_t TCPCtrl::Receive(uint8_t *data, uint32_t size, uint32_t port) {
  // alloc buffer
  int32_t bufsize = sizeof(TCPHeader) + size;
  uint8_t *buffer = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(sizeof(uint8_t) * bufsize));

  int32_t result = -1;
  uint16_t receivedPort = 0;
  uint32_t dstIPAddr = 0;
  uint8_t sessionType = 0;
  uint32_t receivedAck = 0xffffffff;
  uint32_t receivedSeq = 0xffffffff;

  while(result == -1
     || receivedPort != port
     || !(sessionType & _tcpSessionType)) {
    // succeed to receive packet
    // and correspond to the specified port / session type
    result = _ipCtrl->ReceiveData(buffer, bufsize, kProtoTCP, &dstIPAddr);
    receivedPort = ntohs(buffer + kDstPortOffset);
    receivedAck = ntohl(buffer + kAckOffset);
    receivedSeq = ntohl(buffer + kSeqOffset);
    sessionType = buffer[kSessionTypeOffset];
  }

  if(result > 0) {
    int32_t length = bufsize < result ? bufsize : result;
	kassert(length - sizeof(TCPHeader) <= size);
    memcpy(data, buffer + sizeof(TCPHeader), length - sizeof(TCPHeader));

    // for the following transmission
    _dstIPAddr = dstIPAddr;
    _dstPort = ntohs(buffer + kSrcPortOffset);
    _sequence = receivedSeq;
    _ack = receivedAck;
  
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(buffer));
    return result - sizeof(TCPHeader);
  } else {
    return result;
  }
}

int32_t TCPCtrl::Transmit(const uint8_t *data, uint32_t length, uint32_t dstIPAddr, uint32_t dstPort, uint32_t srcPort) {
  int32_t result = -1;

  // alloc datagram
  uint8_t *datagram = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(sizeof(uint8_t) * (sizeof(TCPHeader) + length)));

  // construct header
  TCPHeader header;
  header.srcPort  = htons(srcPort);
  header.dstPort  = htons(dstPort);
  header.seqNumber = htonl(_sequence);
  header.ackNumber = htonl(_ack);
  header.headerLen = (sizeof(TCPHeader) >> 2) << 4;
  header.flag = _tcpSessionType;
  header.windowSize = 0;
  header.checksum = 0;  // TODO: calculate
  header.urgentPointer = 0;

  // construct datagram
  memcpy(datagram, reinterpret_cast<uint8_t*>(&header), sizeof(TCPHeader));
  memcpy(datagram + sizeof(TCPHeader), data, length);

  // call IPCtrl::TransmitData
  _ipCtrl->TransmitData(datagram, sizeof(TCPHeader) + length, kProtoTCP, dstIPAddr);

  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(datagram));

  return result;
}

int32_t TCPCtrl::ReceiveBasic(uint8_t session, uint32_t port) {
  uint8_t buffer[kBasicBufsize];
  _port = port;
  _tcpSessionType = session;
  return Receive(buffer, kBasicBufsize, _port);
}

int32_t TCPCtrl::TransmitBasic(uint8_t session, uint32_t dstIPAddr, uint32_t dstPort) {
  uint8_t buffer[kBasicBufsize];
  _tcpSessionType = session;
  return Transmit(buffer, kBasicBufsize, dstIPAddr, dstPort, _port);
}

int32_t TCPCtrl::Listen(uint32_t port) {
  _port = port;

  ReceiveBasic(kFlagSYN, _port);

  _ack = _sequence + 1;
  _sequence = rand();
  gtty->Printf("s", "seq = ", "x", _sequence, "s", " ack = ", "x", _ack, "s", "\n");
  return TransmitBasic(kFlagSYN | kFlagACK, _dstIPAddr, _dstPort);
}

int32_t TCPCtrl::Connect(uint32_t dstIPAddr, uint32_t dstPort, uint32_t srcPort) {
  _port = srcPort;

  _sequence = rand();
  gtty->Printf("s", "seq = ", "x", _sequence, "s", " ack = ", "x", _ack, "s", "\n");
  TransmitBasic(kFlagSYN, dstIPAddr, dstPort);

  ReceiveBasic(kFlagSYN | kFlagACK, _port);

  uint32_t tmp = _sequence;
  _sequence = _ack + 1;
  _ack = tmp + 1;
  gtty->Printf("s", "seq = ", "x", _sequence, "s", " ack = ", "x", _ack, "s", "\n");
  return TransmitBasic(kFlagACK, dstIPAddr, dstPort);
}

int32_t TCPCtrl::Close() {
  TransmitBasic(kFlagFIN, _dstIPAddr, _dstPort);
  ReceiveBasic(kFlagFIN | kFlagACK, _port);
  return TransmitBasic(kFlagACK, _dstIPAddr, _dstPort);
}
