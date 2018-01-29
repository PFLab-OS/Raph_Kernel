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
  int Bind(uint16_t port) {
    if (_port != 0) {
      gtty->Printf("Socket already bound to port: %d\n", _port);
      return 1;
    }
    if (port == 0) {
      gtty->Printf("Port: %d is reserved.\n", port);
      return 1;
    }
    _port = port;
    this->_rx_queue.SetFunction(network_cpu,
                                make_uptr(new ClassFunction<UserSocket, void *>(
                                    this, &UserSocket::Handle, nullptr)));
    UdpCtrl::GetCtrl().RegisterSocket(port, this);
    gtty->Printf("sock Listen started. port: %d\n", port);
    return 0;
  }
  int ReceiveSync(uint8_t *buf, size_t buf_size, uint8_t dst_ip_addr[4],
                  uint8_t src_ip_addr[4], uint16_t *dst_port,
                  uint16_t *src_port) {
    // returns size read, -1 if error.
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
            gtty->Printf("cnt: %d\n", cnt);
            gtty->Flush();
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
      return -1;
    }
    uptr<UdpCtrl::RxPacket> upacket = _rx_queue.Pop();
    //
    size_t trans_size =
        buf_size < upacket->data->GetLen() ? buf_size : upacket->data->GetLen();
    memcpy(buf, upacket->data->GetRawPtr(), trans_size);
    memcpy(dst_ip_addr, upacket->dest_ip_addr, 4);
    memcpy(src_ip_addr, upacket->source_ip_addr, 4);
    *dst_port = upacket->dest_port;
    *src_port = upacket->source_port;
    return trans_size;
  }

 private:
  uint16_t _port = 0;

  void Handle(void *) {
    /*
    uptr<UdpCtrl::RxPacket> upacket = _rx_queue.Pop();
    if (upacket.IsNull()) {
      return;
    }
    gtty->Printf("Receive via socket from ");
    gtty->Printf("%d.%d.%d.%d\n", upacket->src_ip_addr[0],
                 upacket->src_ip_addr[1], upacket->src_ip_addr[2],
                 upacket->src_ip_addr[3]);
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
