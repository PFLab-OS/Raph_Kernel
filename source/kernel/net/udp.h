/*
 *
 * Copyright (c) 2017 Raphine Project
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
 * Author: Liva
 * 
 */

#pragma once

#include <stdint.h>
#include <raph.h>
#include <ptr.h>
#include <function.h>
#include <string.h>
#include <dev/netdev.h>
#include <queue.h>

class UdpCtrl {
public:
  struct Packet {
    uint8_t dest_ip_addr[4];
    uint16_t dest_port;
    uint16_t source_port;
    uptr<Array<uint8_t>> data;
  };
  class RxPacket : public QueueContainer<RxPacket> {
  public:
    uint8_t dest_ip_addr[4];
    uint8_t source_ip_addr[4];
    uint16_t dest_port;
    uint16_t source_port;
    uptr<Array<uint8_t>> data;
    NetDev *dev;
    RxPacket() : QueueContainer<RxPacket>(this) {
    }
  };
  static void Init();
  static UdpCtrl &GetCtrl() {
    return _udp_ctrl;
  }
  void SetupServer();
  // Deprecated
  void Send(uint8_t (*target_addr)[4], uint16_t target_port, const uint8_t *data, size_t len);
  void SendStr(uint8_t (*target_addr)[4], uint16_t target_port, const char *data) {
    Send(target_addr, target_port, reinterpret_cast<const uint8_t *>(data), strlen(data));
  }
  // TODO reimplement to queue based architecture
  void Send(uptr<Packet> packet);
  void SendBroadCast(uptr<Packet> packet, NetDev *dev);
  class ProtocolInterface {
  public:
  protected:
    friend class UdpCtrl;
    virtual FunctionalQueue<uptr<RxPacket>> &GetRxQueue() = 0;
  };
  void RegisterSocket(uint16_t port, ProtocolInterface *protocol) {
    for (int i = 0; i < kSocketNum; i++) {
      if (_socket[i].protocol == nullptr) {
        _socket[i].protocol = protocol;
        _socket[i].port = port;
        return;
      }
    }
    kernel_panic("UdpCtrl", "No software resource");
  }
private:
  struct FullPacket {
    uint8_t dest_mac_addr[6];
    uint8_t source_ip_addr[4];
    uptr<Packet> packet;
    NetDev *dev;
  };
  void Send(uptr<FullPacket> full_packet);
  void DummyServer(NetDev *dev);
    
  static UdpCtrl _udp_ctrl;
  static const int kSocketNum = 10;
  class Socket {
  public:
    uint16_t port = 0;
    ProtocolInterface *protocol = nullptr;
  } _socket[kSocketNum];
};
