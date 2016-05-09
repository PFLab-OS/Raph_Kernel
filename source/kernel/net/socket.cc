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

#include <global.h>
#include <dev/posixtimer.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/socket.h>
#include <net/eth.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/tcp.h>
#include <net/ptcl.h>

int32_t NetSocket::Open() {
  if((_device_info = netdev_ctrl->GetDeviceInfo()) == nullptr) {
    return -1;
  } else {
    _device_info->ptcl_stack->RegisterSocket(this, _l3_ptcl);
    return 0;
  }
}

void NetSocket::SetReceiveCallback(int apicid, const Function &func) {
  _device_info->ptcl_stack->_socket_table[_ptcl_stack_id].dup_queue.SetFunction(apicid, func);
}

/*
 * (TCP/IP) Socket
 */

Socket::Socket() {
  _l3_ptcl = kProtocolIpv4;
}

int32_t Socket::GetEthAddr(uint32_t ipaddr, uint8_t *macaddr) {
  while(arp_table->Find(ipaddr, macaddr) < 0) {
    ArpSocket socket;
    if(socket.Open() < 0) {
      return -1;
    } else {
      socket.TransmitPacket(ArpSocket::kOpArpRequest, ipaddr, nullptr);
    }
  }
  return 0;
}

int32_t Socket::Transmit(const uint8_t *data, uint32_t length, bool is_raw_packet) {
  // alloc buffer
  NetDev::Packet *packet = nullptr;
  if(!_device_info->device->GetTxPacket(packet)) {
    return kErrorInsufficientBuffer;
  }

  int32_t tcp_hlen = 0;

  if(is_raw_packet) {
    packet->len = length;
    memcpy(packet->buf, data, length);
  } else {
    // TCP header
    uint32_t offset_l4 = sizeof(EthHeader) + sizeof(Ipv4Header);
    uint32_t saddr = _ipaddr;
    struct TcpOptionParameters options;
    options.mss = _mss;
    options.ws = _ws;
    tcp_hlen = TcpGenerateHeader(packet->buf + offset_l4, length, saddr, _daddr, _sport, _dport, _type, _seq, _ack, &options);

    // packet body
    uint32_t offset_body = sizeof(EthHeader) + sizeof(Ipv4Header) + tcp_hlen;
    memcpy(packet->buf + offset_body, data, length);

    // IP header
    uint32_t offset_l3 = sizeof(EthHeader);
    IpGenerateHeader(packet->buf + offset_l3, tcp_hlen + length, kProtocolTcp, saddr, _daddr);

    // Ethernet header
    uint8_t eth_saddr[6];
    uint8_t eth_daddr[6] = {0x08, 0x00, 0x27, 0xc1, 0x5b, 0x93}; // TODO:
    _device_info->device->GetEthAddr(eth_saddr);
//    GetEthAddr(_daddr, eth_daddr);
    EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolIpv4);

    packet->len = sizeof(EthHeader) + sizeof(Ipv4Header) + tcp_hlen + length;
  }

  // transmit
  _device_info->device->TransmitPacket(packet);
  int32_t sent_length = packet->len;

  return (sent_length < 0 || is_raw_packet) ? sent_length : sent_length - (sizeof(EthHeader) + sizeof(Ipv4Header) + tcp_hlen);
}

int32_t Socket::TransmitPacket(const uint8_t *packet, uint32_t length) {
  // return value of receiving ack
  int32_t rval_ack = 0;
  // base count of round trip time
  uint64_t t0 = 0;

  int32_t rval = kErrorUnknown;

  // length intended to be sent
  uint32_t send_length = length > kMss ? kMss : length;

  if(rval_ack != kErrorRetransmissionTimeout) t0 = timer->ReadMainCnt();

  if(_state != kStateAckWait) {
    // successfully sent length of packet
    rval = Transmit(packet, send_length, false);

    if(_type & kFlagACK) {
      // transmission mode is ACK
      if(rval >= 0 && _established) {
        _packet_length = rval;
        _state = kStateAckWait;
      } else if(rval < 0) {
        return rval;  // failure
      }
    }
  }

  if(_state == kStateAckWait) {
    uint8_t packet_ack[sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + 40];
    uint8_t *tcp = packet_ack + sizeof(EthHeader) + sizeof(Ipv4Header);

    uint64_t rto = timer->GetCntAfterPeriod(timer->ReadMainCnt(), GetRetransmissionTimeout());

    // receive acknowledgement
    rval_ack = Receive(packet_ack, sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + 40, true, true, rto);

    if(rval_ack >= 0) {
      // acknowledgement packet received
      uint8_t type = GetSessionType(tcp);
      uint32_t seq = GetSequenceNumber(tcp);
      uint32_t ack = GetAcknowledgeNumber(tcp);

      // sequence & acknowledge number validation
      if(type & kFlagACK && seq == _ack && ack == _seq + _packet_length) {

        // calculate round trip time
        uint64_t t1 = timer->ReadMainCnt();
        _rtt_usec = t1 - t0;

        // seqeuence & acknowledge number is valid
        SetSequenceNumber(_seq + _packet_length);

        _state = kStateEstablished;

        return _packet_length;
      }
    } else if(rval_ack == kErrorRetransmissionTimeout) {
      return kErrorRetransmissionTimeout;
    } else {
      return rval_ack;  // failure
    }
  }

  return rval;
}

int32_t Socket::TransmitRawPacket(const uint8_t *data, uint32_t length) {
  return Transmit(data, length, true);
}

int32_t Socket::Receive(uint8_t *data, uint32_t length, bool is_raw_packet, bool wait_timeout, uint64_t rto) {
  // check timeout
  if(wait_timeout && rto <= timer->ReadMainCnt()) {
    return kErrorRetransmissionTimeout;
  }

  // receiving packet buffer
  DevEthernet::Packet *packet = nullptr;
  // my MAC address
  uint8_t eth_daddr[6];
  _device_info->device->GetEthAddr(eth_daddr);

  if(!_device_info->ptcl_stack->ReceivePacket(GetProtocolStackId(), packet)) {
    return kErrorNoPacketOnWire;
  }

  // filter IP address
  uint32_t offset_l3 = sizeof(EthHeader);
  if(!IpFilterPacket(packet->buf + offset_l3, kProtocolTcp, _daddr, _ipaddr)) {
    _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);
    return kErrorInvalidPacketOnWire;
  }

  // filter TCP port
  uint32_t offset_l4 = sizeof(EthHeader) + sizeof(Ipv4Header);
  if(!TcpFilterPacket(packet->buf + offset_l4, _sport, _dport, _type, _seq, _ack, nullptr)) {
    _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);
    return kErrorInvalidPacketOnWire;
  }

  // received "RAW" packet length
  int32_t received_length = packet->len;

  if(!is_raw_packet) {
    // copy data
    uint32_t offset = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader);
    memcpy(data, packet->buf + offset, length);
  } else {
    memcpy(data, packet->buf, packet->len < length ? packet->len : length);
  }

  // remember source port
  // TODO: 適当なので直す(ポートが途中から違ったら？)
  _dport = GetSourcePort(packet->buf + offset_l4);

  // finalization
  _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);

  return (received_length < 0 || is_raw_packet) ? received_length : received_length - (sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader));
}

int32_t Socket::ReceivePacket(uint8_t *data, uint32_t length) {
  if(_state == kStateCloseWait || _state == kStateLastAck) {
    // continue closing connection
    int32_t rval = CloseAck();

    if(rval >= 0) {
      return kResultConnectionClosed;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateEstablished) {
    if(_type & kFlagACK) {
      // TCP acknowledgement
      uint32_t pkt_size = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + length;
      uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(pkt_size));
      uint8_t *tcp = packet + sizeof(EthHeader) + sizeof(Ipv4Header);
  
      int32_t rval = Receive(packet, pkt_size, true, false, 0);
  
      if(rval >= 0) {
        uint8_t type = GetSessionType(tcp);
        uint32_t seq = GetSequenceNumber(tcp);
        uint32_t ack = GetAcknowledgeNumber(tcp);
  
        // TODO: check if ack number is duplicated
  
        if(type & kFlagFIN) {
          // close connection
          SetSequenceNumber(ack);
          SetAcknowledgeNumber(seq + 1);
          rval = CloseAck();
  
          if(rval >= 0) {
            rval = kResultConnectionClosed;
          }
        } else if(_ack == seq || (_seq == seq && _ack == ack)) {
          // acknowledge number = the expected next sequence number
          // (but the packet receiving right after 3-way handshake is not the case)
          rval -= sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader);
          SetSequenceNumber(ack);
          SetAcknowledgeNumber(seq + rval);
  
          // acknowledge
          if(_state == kStateEstablished) {
            Transmit(nullptr, 0, false);
          }
  
          memcpy(data, packet + pkt_size - length, length);
        } else {
          // sequence number or acknowledgement number is wrong
          rval = kErrorTcpAcknowledgement;
        }
      }
  
      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
      return rval;
    } else {
      return Receive(data, length, false, false, 0);
    }
  }

  return kErrorUnknown;
}

int32_t Socket::ReceiveRawPacket(uint8_t *data, uint32_t length) {
  return Receive(data, length, true, false, 0);
}

int32_t Socket::Listen() {
  if(_state != kStateClosed && _state != kStateListen && _state != kStateSynReceived && _state != kStateSynSent) {
    // connection already established
    return kResultAlreadyEstablished;
  }

  uint32_t kBufSize = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + 40;
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(Ipv4Header);

  if(_state == kStateClosed) {
    // receive SYN packet
    SetSessionType(kFlagSYN);
    int32_t rval = ReceiveRawPacket(buffer, kBufSize);

    if(rval >= 0) {
      SetAcknowledgeNumber(GetSequenceNumber(tcp) + 1);
      _state = kStateListen;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateListen) {
    // transmit SYN+ACK packet
    SetSessionType(kFlagSYN | kFlagACK);

    if(_seq == 0) {
      SetSequenceNumber(rand());
    }

    int32_t rval = TransmitPacket(buffer, 0);

    if(rval >= 0) {
      _state = kStateSynSent;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateSynSent) {
    // receive ACK packet
    SetSessionType(kFlagACK);
    int32_t rval = ReceiveRawPacket(buffer, kBufSize);

    if(rval >= 0) {
      // check sequence number
      if(GetSequenceNumber(tcp) != _ack) {
        return kErrorInvalidPacketOnWire;
      }

      // check acknowledge number
      if(GetAcknowledgeNumber(tcp) != _seq + 1) {
        return kErrorInvalidPacketOnWire;
      }

      // connection established
      uint32_t s = _seq;
      SetSequenceNumber(_ack);
      SetAcknowledgeNumber(s + 1);
      _established = true;
      _state = kStateEstablished;
    } else {
      return rval;  // failure
    }
  }

  return 0;
}

int32_t Socket::Connect() {
  if(_state != kStateClosed && _state != kStateListen && _state != kStateSynReceived && _state != kStateSynSent) {
    // connection already established
    return kResultAlreadyEstablished;
  }

  uint32_t kBufSize = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + 40;
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(Ipv4Header);

  if(_state == kStateClosed) {
    // transmit SYN packet
    SetSessionType(kFlagSYN);
    SetSequenceNumber(rand());
    SetAcknowledgeNumber(0);

    int32_t rval = TransmitPacket(buffer, 0);

    if(rval >= 0) {
      _state = kStateSynSent;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateSynSent) {
    // receive SYN+ACK packet
    SetSessionType(kFlagSYN | kFlagACK);
    int rval = ReceiveRawPacket(buffer, kBufSize);

    if(rval >= 0) {
      // check acknowledge number
      if(GetAcknowledgeNumber(tcp) != _seq + 1) return kErrorInvalidPacketOnWire;

      // transmit ACK packet
      uint32_t t = GetSequenceNumber(tcp);
      SetSessionType(kFlagACK);
      SetSequenceNumber(_seq + 1);
      SetAcknowledgeNumber(GetSequenceNumber(tcp) + 1);
      rval = TransmitPacket(buffer, 0);

      if(rval >= 0) {
        _state = kStateEstablished;

        // connection established
        SetAcknowledgeNumber(t + 1);
        _established = true;

      } else {
        return rval;  // failure
      }
    } else {
      return rval;  // failure
    }
  }

  return 0;
}

int32_t Socket::Close() {
  if(_state == kStateSynSent || _state == kStateListen) {
    _state = kStateClosed;
    return 0;
  }

  uint32_t kBufSize = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + 40;
  uint8_t buffer[kBufSize];
  uint8_t *tcp= buffer + sizeof(EthHeader) + sizeof(Ipv4Header);

  if(_state == kStateEstablished) {
    // transmit FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    int32_t rval = Transmit(buffer, 0, false);

    if(rval >= 0) {
      _state = kStateFinWait1;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateFinWait1) {
    // receive ACK packet
    SetSessionType(kFlagACK);
    int32_t rval = ReceiveRawPacket(buffer, kBufSize);

    if(rval >= 0) {
      // check sequence number
      if(GetSequenceNumber(tcp) != _ack) return kErrorInvalidPacketOnWire;

      // check acknowledge number
      if(GetAcknowledgeNumber(tcp) != _seq + 1) return kErrorInvalidPacketOnWire;

      _state = kStateFinWait2;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateFinWait2) {
    // receive FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    int32_t rval = ReceiveRawPacket(buffer, kBufSize);

    if(rval >= 0) {
      // check sequence number
      if(GetSequenceNumber(tcp) != _ack) return kErrorInvalidPacketOnWire;

      // check acknowledge number
      if(GetAcknowledgeNumber(tcp) != _seq + 1) return kErrorInvalidPacketOnWire;

      // transmit ACK packet
      SetSessionType(kFlagACK);
      SetSequenceNumber(_seq + 1);
      SetAcknowledgeNumber(_ack + 1);

      rval = Transmit(buffer, 0, false);
      
      if(rval >= 0) {
        _established = false;
        _state = kStateClosed;
        _seq = 0; _ack = 0;
      } else {
        return rval;  // failure
      }
    } else {
      return rval;  // failure
    }
  }

  return 0;
}

int32_t Socket::CloseAck() {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(TcpHeader) + 40;
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(Ipv4Header);

  if(_state == kStateEstablished) {
    // transmit ACK packet
    SetSessionType(kFlagACK);
    int rval = Transmit(buffer, 0, false);

    if(rval >= 0) {
      _state = kStateCloseWait;
    } else {
      return rval;  // failure
    }
  }
    
  if(_state == kStateCloseWait) {
    // transmit FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    int rval = Transmit(buffer, 0, false);

    if(rval >= 0) {
      _state = kStateLastAck;
    } else {
      return rval;  // failure
    }
  }

  if(_state == kStateLastAck) {
    // receive ACK packet
    SetSessionType(kFlagACK);
    int rval = ReceiveRawPacket(buffer, kBufSize);

    if(rval >= 0) {
      // check sequence number
      if(GetSequenceNumber(tcp) != _ack) return kErrorInvalidPacketOnWire;

      // check acknowledge number
      if(GetAcknowledgeNumber(tcp) != _seq + 1) return kErrorInvalidPacketOnWire;

      _state = kStateClosed;
      _established = false;
      _seq = 0; _ack = 0;
    } 
  }
 
  return 0;
}
/*
 * UdpSocket
 */

UdpSocket::UdpSocket() {
  _l3_ptcl = kProtocolIpv4;
}

int32_t UdpSocket::TransmitPacket(const uint8_t *data, uint32_t length) {
  // alloc buffer
  NetDev::Packet *packet = nullptr;
  if(!_device_info->device->GetTxPacket(packet)) {
    return kErrorInsufficientBuffer;
  }

  packet->len = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(UdpHeader) + length;

  // packet body
  uint32_t offset_body = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(UdpHeader);
  memcpy(packet->buf + offset_body, data, length);

  // UDP header
  uint32_t offset_l4 = sizeof(EthHeader) + sizeof(Ipv4Header);
  uint32_t saddr = _ipaddr;
  UdpGenerateHeader(packet->buf + offset_l4, sizeof(UdpHeader) + length, _sport, _dport);

  // IP header
  uint32_t offset_l3 = sizeof(EthHeader);
  IpGenerateHeader(packet->buf + offset_l3, sizeof(UdpHeader) + length, kProtocolTcp, saddr, _daddr);

  // Ethernet header
  uint8_t eth_saddr[6];
  uint8_t eth_daddr[6] = {0x08, 0x00, 0x27, 0xc1, 0x5b, 0x93}; // TODO:
  _device_info->device->GetEthAddr(eth_saddr);
//  GetEthAddr(_daddr, eth_daddr);
  EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolIpv4);
  
  // transmit
  _device_info->device->TransmitPacket(packet);
  int32_t sent_length = packet->len;

  return (sent_length < 0) ? sent_length : sent_length - (sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(UdpHeader));
}

int32_t UdpSocket::ReceivePacket(uint8_t *data, uint32_t length) {
  // receiving packet buffer
  DevEthernet::Packet *packet = nullptr;
  // my MAC address
  uint8_t eth_daddr[6];
  _device_info->device->GetEthAddr(eth_daddr);

  if(!_device_info->ptcl_stack->ReceivePacket(GetProtocolStackId(), packet)) {
    return kErrorNoPacketOnWire;
  }

  // filter IP address
  uint32_t offset_l3 = sizeof(EthHeader);
  if(!IpFilterPacket(packet->buf + offset_l3, kProtocolUdp, _daddr, _ipaddr)) {
    _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);
    return kErrorInvalidPacketOnWire;
  }

  // filter TCP port
  uint32_t offset_l4 = sizeof(EthHeader) + sizeof(Ipv4Header);
  if(!UdpFilterPacket(packet->buf + offset_l4, _sport, _dport)) {
    _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);
    return kErrorInvalidPacketOnWire;
  }

  // received "RAW" packet length
  int32_t received_length = packet->len;

  // copy data
  uint32_t offset = sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(UdpHeader);
  memcpy(data, packet->buf + offset, length);

  // remember source port
  // TODO: 適当なので直す(ポートが途中から違ったら？)
  _dport = GetSourcePort(packet->buf + offset_l4);

  // finalization
  _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);

  return (received_length < 0) ? received_length : received_length - (sizeof(EthHeader) + sizeof(Ipv4Header) + sizeof(UdpHeader));
}

/*
 * ArpSocket
 */

ArpSocket::ArpSocket() {
  _l3_ptcl = kProtocolArp;
}

int32_t ArpSocket::TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha) {
  uint32_t ip_daddr = tpa;
  uint8_t eth_saddr[6];
  uint8_t eth_daddr[6];
  _device_info->device->GetEthAddr(eth_saddr);

  switch(type) {
    case kOpArpRequest:
      memset(eth_daddr, 0xff, 6); // broadcast
      break;
    case kOpArpReply:
      memcpy(eth_daddr, tha, 6);
      break;
    default:
      // unknown ARP operation
      return kErrorInvalidPacketParameter;
  }

  // alloc buffer
  DevEthernet::Packet *packet = nullptr;
  if(!_device_info->device->GetTxPacket(packet)) {
    return kErrorInsufficientBuffer;
  }

  packet->len = sizeof(EthHeader) + sizeof(ArpPacket);

  // ARP header
  uint32_t offset_arp = sizeof(EthHeader);
  ArpGeneratePacket(packet->buf + offset_arp , type, eth_saddr, _ipaddr, eth_daddr, ip_daddr);

  // Ethernet header
  EthGenerateHeader(packet->buf, eth_saddr, eth_daddr, kProtocolArp);

  // transmit
  if(!_device_info->device->TransmitPacket(packet)) return kErrorDeviceInternal;

  return type;
}

int32_t ArpSocket::ReceivePacket(uint16_t type, uint32_t *spa, uint8_t *sha) {
  // alloc buffer
  DevEthernet::Packet *packet = nullptr;
  int16_t op = 0;

  uint8_t eth_daddr[6];
  _device_info->device->GetEthAddr(eth_daddr);

  // check if there is a new packet on wire (if so, then fetch it)
  if(!_device_info->ptcl_stack->ReceivePacket(GetProtocolStackId(), packet)) {
    return kErrorNoPacketOnWire;
  }

  // filter IP address
  if(!ArpFilterPacket(packet->buf + sizeof(EthHeader), type, nullptr, 0, eth_daddr, _ipaddr)) {
    _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);
    return kErrorInvalidPacketOnWire;
  }

  uint8_t *p = packet->buf + sizeof(EthHeader) + kOperationOffset;
  op = ntohs(*reinterpret_cast<uint16_t*>(p));

  // handle received ARP request/reply
  uint32_t offset_arp = sizeof(EthHeader);

  switch(op) {
    case kOpArpReply:
      RegisterIpAddress(packet->buf + sizeof(EthHeader));
    case kOpArpRequest:
      if(spa) *spa = GetSourceIpAddress(packet->buf + offset_arp);
      if(sha) GetSourceMacAddress(sha, packet->buf + offset_arp);
      break;
    default:
      _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);
      return kErrorInvalidPacketParameter;
  }

  // finalization
  _device_info->ptcl_stack->FreeRxBuffer(GetProtocolStackId(), packet);

  return op;
}
