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
 * Author: Liva
 *
 * reference:
 * RFC 2131 Dynamic Host Configuration Protocol
 * RFC 2132 DHCP Options and BOOTP Vendor Extensions
 */

#pragma once

#include <dev/netdev.h>
#include <net/udp.h>
#include <stdint.h>

class DhcpCtrl : public UdpCtrl::ProtocolInterface {
 public:
  static void Init();
  static DhcpCtrl &GetCtrl() { return _dhcp_ctrl; }
  void AssignAddr(NetDev *dev);

 private:
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

  virtual FunctionalQueue<uptr<UdpCtrl::RxPacket>> &GetRxQueue() override {
    return _rx_queue;
  }

  void Handle(void *);
  void HandleOffer(uptr<UdpCtrl::RxPacket> upacket, Packet *packet);
  void HandleAck(uptr<UdpCtrl::RxPacket> upacket, Packet *packet);

  void SendRequest(NetDev *dev, uint32_t siaddr, uint32_t yiaddr);

  static DhcpCtrl _dhcp_ctrl;
  FunctionalQueue<uptr<UdpCtrl::RxPacket>> _rx_queue;

  static const uint32_t kMagicCookie = 0x63825363;

  static const int kMaxInfo = 10;
  struct Info {
    NetDev *dev = nullptr;
    enum class State {
      kNull,
      kSendDiscover,
      kReceiveOffer,
      kReceivePack,
    } state;
    uint32_t siaddr;
  } _info[kMaxInfo];
};
