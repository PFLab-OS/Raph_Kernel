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

#ifndef __RAPH_KERNEL_NET_PTCL_H__
#define __RAPH_KERNEL_NET_PTCL_H__

#include <buf.h>
#include <polling.h>
#include <spinlock.h>
#include <dev/netdev.h>
#include <net/socket.h>

// Layer 3 protocols
const uint16_t kProtocolIPv4 = 0x0800;
const uint16_t kProtocolARP  = 0x0806;

// Layer 4 protocols
const uint8_t kProtocolTCP         = 0x06;
const uint8_t kProtocolUDP         = 0x11;

class ProtocolStack : public Polling {
public:
  ProtocolStack() {}

  void Setup();

  // register new socket
  bool RegisterSocket(NetSocket *socket, uint16_t l3_ptcl);

  // remove socket
  bool RemoveSocket(NetSocket *socket);

  // fetch packet (NetSocket uses)
  bool ReceivePacket(uint32_t socket_id, NetDev::Packet *&packet);

  // !!N.B.!! YOU MUST use FreeRxBuffer to free packet
  // fetched by ProtocolStack::ReceivePacket
  void FreeRxBuffer(NetDev::Packet *packet);

  // pop packet from main queue, then duplicate it into duplicated queue
  void Handle() override;

  void SetDevice(DevEthernet *dev);

private:
  // packet queue (inserted from network device)
  static const uint32_t kQueueDepth = 300;
  typedef RingBuffer <NetDev::Packet*, kQueueDepth> PacketQueue;
  PacketQueue _main_queue;

  struct SocketInfo {
    PacketQueue dup_queue;
    bool in_use;
    uint16_t l3_ptcl;
  };

  // duplicated queue
  static const uint32_t kMaxSocketNumber = 8;
  uint32_t _current_socket_number = 0;
  SocketInfo socket_table[kMaxSocketNumber];

  // reference to the network device holding this protocol stack
  DevEthernet *_device;
  uint8_t _eth_addr[6];

  SpinLock _lock;

  void InitPacketQueue();

  bool FilterPacket(NetDev::Packet *packet);
};

#endif // __RAPH_KERNEL_NET_PTCL_H__
