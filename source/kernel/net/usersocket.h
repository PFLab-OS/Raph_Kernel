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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: hikalium
 *
 */

#pragma once

#include <dev/netdev.h>
#include <net/udp.h>
#include <stdint.h>

extern CpuId network_cpu;

class UserSocket : public UdpCtrl::ProtocolInterface {
 public:
  void Listen(uint16_t port) {
    this->_rx_queue.SetFunction(network_cpu,
                                make_uptr(new ClassFunction<UserSocket, void *>(
                                    this, &UserSocket::Handle, nullptr)));
    UdpCtrl::GetCtrl().RegisterSocket(port, this);
    gtty->Printf("sock Listen started. port: %d\n", port);
  }

 private:
  void Handle(void *) {
    gtty->Printf("Receive!\n");

    uptr<UdpCtrl::RxPacket> upacket = _rx_queue.Pop();
    if (upacket.IsNull()) {
      return;
    }
  }
  /*
  struct Packet {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t bootp_options[192];
    uint32_t magic_cookie;
    uint8_t options[312 - 4];
  } __attribute__((__packed__));
  static_assert(sizeof(Packet) == 548, "");
  */
  virtual FunctionalQueue<uptr<UdpCtrl::RxPacket>> &GetRxQueue() override {
    return _rx_queue;
  }
  FunctionalQueue<uptr<UdpCtrl::RxPacket>> _rx_queue;
};
