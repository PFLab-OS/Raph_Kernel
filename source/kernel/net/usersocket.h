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
  void ReceiveSync() {
    gtty->Printf("Entering busy wait\n");
    uptr<Thread> thread = ThreadCtrl::GetCtrl(network_cpu)
                              .AllocNewThread(Thread::StackState::kIndependent);
    do {
      auto t_op = thread->CreateOperator();
      t_op.SetFunc(make_uptr(new Function<void *>(
          [](void *arg) {
            static int cnt = 0;
            cnt++;
            FunctionalQueue<uptr<UdpCtrl::RxPacket>> *rxq =
                reinterpret_cast<FunctionalQueue<uptr<UdpCtrl::RxPacket>> *>(
                    arg);
            if (cnt < 1000 && rxq->IsEmpty()) {
              ThreadCtrl::GetCurrentThreadOperator().Schedule(1000);
            }
          },
          &_rx_queue)));
      t_op.Schedule();
    } while (0);
    thread->Join();
    //
    gtty->Printf("Exited busy wait\n");
    if (_rx_queue.IsEmpty()) {
      gtty->Printf("Timeout\n");
    }
    uptr<UdpCtrl::RxPacket> upacket = _rx_queue.Pop();
    gtty->Printf("Receive via socket from ");
    gtty->Printf("%d.%d.%d.%d\n", upacket->source_ip_addr[0],
                 upacket->source_ip_addr[1], upacket->source_ip_addr[2],
                 upacket->source_ip_addr[3]);
  }

 private:
  void Handle(void *) {
    /*
    uptr<UdpCtrl::RxPacket> upacket = _rx_queue.Pop();
    if (upacket.IsNull()) {
      return;
    }
    gtty->Printf("Receive via socket from ");
    gtty->Printf("%d.%d.%d.%d\n", upacket->source_ip_addr[0],
                 upacket->source_ip_addr[1], upacket->source_ip_addr[2],
                 upacket->source_ip_addr[3]);
                 */
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
