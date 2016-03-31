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

#include "../global.h"
#include "../mem/physmem.h"
#include "../mem/virtmem.h"

#include "socket.h"
#include "eth.h"
#include "arp.h"
#include "ip.h"
#include "udp.h"
#include "tcp.h"

int32_t NetSocket::Open() {
  _dev = reinterpret_cast<DevEthernet*>(netdev_ctrl->GetDevice());
  if(!_dev) {
    return -1;
  } else {
    return 0;
  }
}

/*
 * (TCP/IP) Socket
 */

uint32_t Socket::L2HeaderLength() { return sizeof(EthHeader); }
uint32_t Socket::L3HeaderLength() { return sizeof(IPv4Header); }
uint32_t Socket::L4HeaderLength() { return sizeof(TCPHeader); }

uint16_t Socket::L4Protocol() { return IPCtrl::kProtocolTCP; }

int32_t Socket::GetEthAddr(uint32_t ipaddr, uint8_t *macaddr) {
  while(arp_table->Find(ipaddr, macaddr) < 0) {
    ARPSocket socket;
    if(socket.Open() < 0) {
      return -1;
    } else {
      socket.TransmitPacket(ARPSocket::kOpARPRequest, ipaddr);
    }
  }
  return 0;
}

int32_t Socket::L2Tx(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type) {
  return eth_ctrl->GenerateHeader(buffer, saddr, daddr, type);
}

bool Socket::L2Rx(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type) {
  return eth_ctrl->FilterPacket(buffer, saddr, daddr, type);
}

int32_t Socket::L3Tx(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr) {
  return ip_ctrl->GenerateHeader(buffer, length, type, saddr, daddr);
}

bool Socket::L3Rx(uint8_t *buffer, uint8_t type, uint32_t saddr, uint32_t daddr) {
  return ip_ctrl->FilterPacket(buffer, type, saddr, daddr);
}

int32_t Socket::L4Tx(uint8_t *buffer, uint32_t length, uint16_t sport, uint16_t dport) {
  return tcp_ctrl->GenerateHeader(buffer, length, sport, dport, _type, _seq, _ack);
}

bool Socket::L4Rx(uint8_t *buffer, uint16_t sport, uint16_t dport) {
  return tcp_ctrl->FilterPacket(buffer, sport, dport, _type, _seq, _ack);
}

int32_t Socket::Transmit(const uint8_t *data, uint32_t length, bool isRawPacket) {
  // alloc buffer
  NetDev::Packet *packet;
  if(!_dev->GetTxPacket(packet)) {
    return -1;
  }

  if(isRawPacket) {
    packet->len = length;
    memcpy(packet->buf, data, length);
  } else {
    packet->len = L2HeaderLength() + L3HeaderLength() + L4HeaderLength() + length;

    // packet body
    uint32_t offsetBody = L2HeaderLength() + L3HeaderLength() + L4HeaderLength();
    memcpy(packet->buf + offsetBody, data, length);

    // TCP header
    uint32_t offsetL4 = L2HeaderLength() + L3HeaderLength();
    L4Tx(packet->buf + offsetL4, L4HeaderLength() + length, _sport, _dport);

    // IP header
    uint32_t offsetL3 = L2HeaderLength();
    uint32_t saddr = _ipaddr;
    L3Tx(packet->buf + offsetL3, L4HeaderLength() + length, L4Protocol(), saddr, _daddr);

    // Ethernet header
    uint8_t ethSaddr[6];
    uint8_t ethDaddr[6] = {0x08, 0x00, 0x27, 0xc1, 0x5b, 0x93}; // TODO:
    _dev->GetEthAddr(ethSaddr);
//    GetEthAddr(_daddr, ethDaddr);
    L2Tx(packet->buf, ethSaddr, ethDaddr, EthCtrl::kProtocolIPv4);
  }

  // transmit
  _dev->TransmitPacket(packet);
  int32_t sentLength = packet->len;

  return sentLength < 0 ? sentLength : sentLength - (sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader));
}

int32_t Socket::TransmitPacket(const uint8_t *data, uint32_t length) {
  int32_t rval = Transmit(data, length, false);
  if(_type == kFlagACK) {
    // TCP acknowledgement
    if(rval >= 0) {
	  SetSequenceNumber(_seq + rval);
    }
  }
  return rval;
}

int32_t Socket::TransmitRawPacket(const uint8_t *data, uint32_t length) {
  return Transmit(data, length, true);
}

int32_t Socket::Receive(uint8_t *data, uint32_t length, bool isRawPacket) {
  // alloc buffer
  int32_t receivedLength;
  DevEthernet::Packet *packet;
  uint8_t ethDaddr[6];
  uint32_t ipDaddr = _ipaddr;
  _dev->GetEthAddr(ethDaddr);

  do {
    // receive
    if(!_dev->ReceivePacket(packet)) continue;

    // filter Ethernet address
	if(!L2Rx(packet->buf, nullptr, ethDaddr, EthCtrl::kProtocolIPv4)) continue;

    // filter IP address
    uint32_t offsetL3 = L2HeaderLength();
    if(!L3Rx(packet->buf + offsetL3 , L4Protocol(), _daddr, ipDaddr)) continue;

    // filter TCP port
    uint32_t offsetL4 = L2HeaderLength() + L3HeaderLength();
    if(!L4Rx(packet->buf + offsetL4, _sport, _dport)) continue;

    break;
  } while(1);

  receivedLength = packet->len;

  if(!isRawPacket) {
    // copy data
    uint32_t offset = L2HeaderLength() + L3HeaderLength() + L4HeaderLength();
    memcpy(data, packet->buf + offset, length);

    // finalization
    _dev->ReuseRxBuffer(packet);
  }

  return receivedLength < 0 ? receivedLength : receivedLength - (sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader));
}

int32_t Socket::ReceivePacket(uint8_t *data, uint32_t length) {
  if(_type == kFlagACK) {
    // TCP acknowledgement
    uint32_t pktSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader) + length;
    int32_t rval;
    uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(pktSize));
    uint8_t *tcp = packet + sizeof(EthHeader) + sizeof(IPv4Header);

    if((rval = Receive(packet, length, true)) >= 0) {
      uint8_t type = tcp_ctrl->GetSessionType(tcp);
      uint32_t seq = tcp_ctrl->GetSequenceNumber(tcp);
      uint32_t ack = tcp_ctrl->GetAcknowledgeNumber(tcp);
      if(type & kFlagFIN) {
        SetSequenceNumber(ack);
        SetAcknowledgeNumber(seq + 1);
        CloseAck(type);
        rval = kConnectionClosed;
      } else if(_ack == seq || (_seq == seq && _ack == ack)) {
        // acknowledge number = the expected next sequence number
        // (but the packet receiving right after 3-way handshake is not the case)
        SetSequenceNumber(ack);
        SetAcknowledgeNumber(seq + rval);
        memcpy(data, packet + pktSize - length, length);
      } else {
        // something is wrong with the received sequence number
        rval = -1;
      }
    }
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
    return rval;
  } else {
    return Receive(data, length, false);
  }
}

int32_t Socket::ReceiveRawPacket(uint8_t *data, uint32_t length) {
  return Receive(data, length, true);
}

int32_t Socket::Listen() {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);
  uint32_t s, t;

  while(1) {
    // receive SYN packet
    SetSessionType(kFlagSYN);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;
    t = tcp_ctrl->GetSequenceNumber(tcp);

    // transmit SYN+ACK packet
    SetSessionType(kFlagSYN | kFlagACK);
    s = rand();
    SetSequenceNumber(s);
    SetAcknowledgeNumber(++t);
    if(TransmitPacket(buffer, 0) < 0) continue;

    // receive ACK packet
    SetSessionType(kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check sequence number
    if(tcp_ctrl->GetSequenceNumber(tcp) != t + 1) continue;

    // check acknowledge number
    if(tcp_ctrl->GetAcknowledgeNumber(tcp) != s + 1) continue;

    break;
  }

  // connection established
  SetSequenceNumber(t + 1);
  SetAcknowledgeNumber(s + 1);

  return 0;
}

int32_t Socket::Connect() {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);
  uint32_t s, t;

  while(1) {
    // transmit SYN packet
    t = rand();
    SetSessionType(kFlagSYN);
    SetSequenceNumber(t);
    SetAcknowledgeNumber(0);
    if(TransmitPacket(buffer, 0) < 0) continue;

    // receive SYN+ACK packet
    SetSessionType(kFlagSYN | kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check acknowledge number
    s = tcp_ctrl->GetAcknowledgeNumber(tcp);
    if(s != t + 1) continue;

    // transmit ACK packet
    t = tcp_ctrl->GetSequenceNumber(tcp);
    SetSessionType(kFlagACK);
    SetSequenceNumber(s + 1);
    SetAcknowledgeNumber(t + 1);
    if(TransmitPacket(buffer, 0) < 0) continue;

    break;
  }

  // connection established
  SetSequenceNumber(s + 1);
  SetAcknowledgeNumber(t + 1);

  return 0;
}

int32_t Socket::Close() {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);
  uint32_t savedSeq = _seq;
  uint32_t savedAck = _ack;
  uint32_t s = _seq;
  uint32_t t = _ack;

  while(1) {
    SetSequenceNumber(savedSeq);
    SetAcknowledgeNumber(savedAck);

    // transmit FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    if(TransmitPacket(buffer, 0) < 0) continue;

    // receive ACK packet
    SetSessionType(kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check sequence number
    if(tcp_ctrl->GetSequenceNumber(tcp) != t) continue;

    // check acknowledge number
    if(tcp_ctrl->GetAcknowledgeNumber(tcp) != s + 1) continue;

    // receive FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check sequence number
    if(tcp_ctrl->GetSequenceNumber(tcp) != t) continue;

    // check acknowledge number
    if(tcp_ctrl->GetAcknowledgeNumber(tcp) != s + 1) continue;

    // transmit ACK packet
    SetSessionType(kFlagACK);
    SetSequenceNumber(s + 1);
    SetAcknowledgeNumber(t + 1);
    if(TransmitPacket(buffer, 0) < 0) continue;

    break;
  }
 
  return 0;
}

int32_t Socket::CloseAck(uint8_t flag) {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);

  if(flag == (kFlagFIN | kFlagACK)) {
    while(1) {
      // transmit ACK packet
      SetSessionType(kFlagACK);
      if(TransmitPacket(buffer, 0) < 0) continue;

      // transmit FIN+ACK packet
      SetSessionType(kFlagFIN | kFlagACK);
      if(TransmitPacket(buffer, 0) < 0) continue;

      // receive ACK packet
      SetSessionType(kFlagACK);
      if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

      // check sequence number
      if(tcp_ctrl->GetSequenceNumber(tcp) != _ack) continue;

      // check acknowledge number
      if(tcp_ctrl->GetAcknowledgeNumber(tcp) != _seq + 1) continue;

      break;
    }
  }
 
  return 0;
}
/*
 * UDPSocket
 */

uint32_t UDPSocket::L4HeaderLength() { return sizeof(UDPHeader); }

uint16_t UDPSocket::L4Protocol() { return IPCtrl::kProtocolUDP; }

int32_t UDPSocket::L4Tx(uint8_t *buffer, uint32_t length, uint16_t sport, uint16_t dport) {
  return udp_ctrl->GenerateHeader(buffer, length, sport, dport);
}

bool UDPSocket::L4Rx(uint8_t *buffer, uint16_t sport, uint16_t dport) {
  return udp_ctrl->FilterPacket(buffer, sport, dport);
}

int32_t UDPSocket::ReceivePacket(uint8_t *data, uint32_t length) {
  return Receive(data, length, false);
}

int32_t UDPSocket::TransmitPacket(const uint8_t *data, uint32_t length) {
  return Transmit(data, length, false);
}

/*
 * ARPSocket
 */

void ARPSocket::SetIPAddr(uint32_t ipaddr) {
  _ipaddr = ipaddr;
}

int32_t ARPSocket::TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha) {
  uint32_t ipDaddr = tpa;
  uint8_t ethSaddr[6];
  uint8_t ethDaddr[6];
  _dev->GetEthAddr(ethSaddr);

  switch(type) {
    case kOpARPRequest:
      memset(ethDaddr, 0xff, 6); // broadcast
      break;
    case kOpARPReply:
      memcpy(ethDaddr, tha, 6);
      break;
    default:
      // unknown ARP operation
      return -1;
  }

  // alloc buffer
  DevEthernet::Packet *packet;
  if(!_dev->GetTxPacket(packet)) {
    return -1;
  }
  uint32_t len = sizeof(EthHeader) + sizeof(ARPPacket);
  packet->len = len;

  // ARP header
  uint32_t offsetARP = sizeof(EthHeader);
  arp_ctrl->GeneratePacket(packet->buf + offsetARP, type, ethSaddr, _ipaddr, ethDaddr, ipDaddr);

  // Ethernet header
  eth_ctrl->GenerateHeader(packet->buf, ethSaddr, ethDaddr, EthCtrl::kProtocolARP);

  // transmit
  _dev->TransmitPacket(packet);

  return len;
}

int32_t ARPSocket::ReceivePacket(uint16_t type, uint32_t *spa, uint8_t *sha) {
  // alloc buffer
  DevEthernet::Packet *packet;
  int16_t op = 0;

  uint8_t ethDaddr[6];
  _dev->GetEthAddr(ethDaddr);

  do {
    // receive
    if(!_dev->ReceivePacket(packet)) continue;

    // filter Ethernet address
    if(!eth_ctrl->FilterPacket(packet->buf, nullptr, ethDaddr, EthCtrl::kProtocolARP)) continue;

    // filter IP address
    if(!arp_ctrl->FilterPacket(packet->buf + sizeof(EthHeader), type, nullptr, 0, ethDaddr, _ipaddr)) continue;

    break;
  } while(1);

  uint8_t *p = packet->buf + sizeof(EthHeader) + kOperationOffset;
  op = ntohs(p[0] << 8 | p[1]);

  // handle received ARP request/reply
  switch(op) {
    case kOpARPReply:
      arp_ctrl->RegisterAddress(packet->buf + sizeof(EthHeader));
    case kOpARPRequest:
      if(spa) *spa = arp_ctrl->GetSourceIPAddress(packet->buf + sizeof(EthHeader));
      if(sha) arp_ctrl->GetSourceMACAddress(sha, packet->buf + sizeof(EthHeader));
      break;
    default:
      _dev->ReuseRxBuffer(packet);
      return -1;
  }

  // finalization
  _dev->ReuseRxBuffer(packet);

  return op;
}
