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
 */

#include "dhcp.h"
#include "udp.h"
#include <stdlib.h>
#include <dev/eth.h>
#include <tty.h>
#include <global.h>

extern CpuId network_cpu;
DhcpCtrl DhcpCtrl::_dhcp_ctrl;

void DhcpCtrl::Init() {
  new (&_dhcp_ctrl) DhcpCtrl;
  _dhcp_ctrl._rx_queue.SetFunction(
      network_cpu, make_uptr(new ClassFunction<DhcpCtrl, void *>(
                       &_dhcp_ctrl, &DhcpCtrl::Handle, nullptr)));
  UdpCtrl::GetCtrl().RegisterSocket(68, &_dhcp_ctrl);
}

void DhcpCtrl::AssignAddr(NetDev *dev) {
  auto packet = make_uptr(new UdpCtrl::Packet);
  memset(packet->dest_ip_addr, 0xFF, 4);
  packet->dest_port = 67;
  packet->source_port = 68;
  packet->data = make_uptr(new Array<uint8_t>(sizeof(Packet)));

  // DHCP Discover
  Packet *dhcp_packet = reinterpret_cast<Packet *>(packet->data->GetRawPtr());

  memset(dhcp_packet, 0, sizeof(Packet));

  dhcp_packet->op = 1;
  dhcp_packet->htype = 1;
  dhcp_packet->hlen = 6;
  dhcp_packet->hops = 0;
  dhcp_packet->xid = rand();
  dhcp_packet->secs = 0;
  dhcp_packet->flags = __builtin_bswap16(0x8000);
  dhcp_packet->ciaddr = 0;
  dhcp_packet->yiaddr = 0;
  dhcp_packet->siaddr = 0;
  dhcp_packet->giaddr = 0;
  static_cast<DevEthernet *>(dev)->GetEthAddr(dhcp_packet->chaddr);
  dhcp_packet->magic_cookie = __builtin_bswap32(kMagicCookie);

  size_t offset = 0;

  // DHCP Message Type
  dhcp_packet->options[offset++] = 53;
  dhcp_packet->options[offset++] = 1;
  dhcp_packet->options[offset++] = 1;

  // Parameter Request List
  dhcp_packet->options[offset++] = 50;
  dhcp_packet->options[offset++] = 4;
  dhcp_packet->options[offset++] = 1;   // Subnet Mask
  dhcp_packet->options[offset++] = 3;   // Router
  dhcp_packet->options[offset++] = 6;   // Domain Name Server
  dhcp_packet->options[offset++] = 15;  // Domain Name

  // Endmark
  dhcp_packet->options[offset++] = 255;

  for (int i = 0; i < kMaxInfo; i++) {
    if (_info[i].state == DhcpCtrl::Info::State::kNull) {
      _info[i].state = DhcpCtrl::Info::State::kSendDiscover;
      _info[i].dev = dev;
      break;
    }
    if (i == kMaxInfo - 1) {
      kernel_panic("DhcpCtrl", "No software resource");
    }
  }

  UdpCtrl::GetCtrl().SendBroadCast(packet, dev);
}

void DhcpCtrl::Handle(void *) {
  uptr<UdpCtrl::RxPacket> upacket = _rx_queue.Pop();
  if (upacket.IsNull()) {
    return;
  }

  size_t max_len = upacket->data->GetLen();
  if (max_len < __builtin_offsetof(Packet, options)) {
    return;
  }

  Packet *packet = reinterpret_cast<Packet *>(upacket->data->GetRawPtr());

  if (packet->magic_cookie != __builtin_bswap32(kMagicCookie)) {
    return;
  }

  size_t max_options_len = max_len - __builtin_offsetof(Packet, options);

  for (size_t i = 0; i < max_options_len; i++) {
    if (packet->options[i] == 53) {
      if (++i >= max_options_len) {
        return;
      }
      if (packet->options[i] != 1) {
        return;
      }
      if (++i >= max_options_len) {
        return;
      }
      switch (packet->options[i]) {
        case 2:
          // DHCP Offer
          HandleOffer(upacket, packet);
          break;
        case 5:
          // DHCP Ack
          HandleAck(upacket, packet);
          break;
        case 6:
          // DHCP Nak
          // TODO implement
          kernel_panic("DhcpCtrl", "DHCP Request was not acknowledged.");
          break;
        default:
          return;
      }
    } else {
      if (++i >= max_options_len) {
        return;
      }
      i += packet->options[i];
    }
  }
}

void DhcpCtrl::HandleOffer(uptr<UdpCtrl::RxPacket> upacket, Packet *packet) {
  int info_index = -1;
  for (int i = 0; i < kMaxInfo; i++) {
    if (_info[i].state != DhcpCtrl::Info::State::kNull &&
        _info[i].dev == upacket->dev) {
      info_index = i;
      break;
    }
    if (i == kMaxInfo - 1) {
      return;
    }
  }
  if (_info[info_index].state == DhcpCtrl::Info::State::kSendDiscover) {
    size_t max_len = upacket->data->GetLen();
    size_t max_options_len = max_len - __builtin_offsetof(Packet, options);

    // get DHCP server address
    uint32_t siaddr;
    for (size_t i = 0; i < max_options_len; i++) {
      if (packet->options[i] == 54) {
        if (++i >= max_options_len) {
          return;
        }
        if (packet->options[i] != 4) {
          return;
        }
        if (i + 4 >= max_options_len) {
          return;
        }
        i++;
        memcpy(&siaddr, packet->options + i, 4);
        break;
      } else {
        if (++i >= max_options_len) {
          return;
        }
        i += packet->options[i];
      }
      if (i >= max_options_len) {
        return;
      }
    }

    _info[info_index].siaddr = siaddr;
    _info[info_index].state = DhcpCtrl::Info::State::kReceiveOffer;
    SendRequest(_info[info_index].dev, _info[info_index].siaddr,
                packet->yiaddr);
  }
}

void DhcpCtrl::HandleAck(uptr<UdpCtrl::RxPacket> upacket, Packet *packet) {
  int info_index = -1;
  for (int i = 0; i < kMaxInfo; i++) {
    if (_info[i].state != DhcpCtrl::Info::State::kNull &&
        _info[i].dev == upacket->dev) {
      info_index = i;
      break;
    }
    if (i == kMaxInfo - 1) {
      return;
    }
  }
  if (_info[info_index].state == DhcpCtrl::Info::State::kReceiveOffer) {
    _info[info_index].state = DhcpCtrl::Info::State::kReceivePack;
    upacket->dev->AssignIpv4Address(packet->yiaddr);
    uint8_t yiaddr[4];
    memcpy(yiaddr, &packet->yiaddr, 4);
    gtty->Printf("DHCP: assigned ip v4 addr %d.%d.%d.%d to %s\n", yiaddr[0],
                 yiaddr[1], yiaddr[2], yiaddr[3], upacket->dev->GetName());
  }
}

void DhcpCtrl::SendRequest(NetDev *dev, uint32_t siaddr, uint32_t yiaddr) {
  auto packet = make_uptr(new UdpCtrl::Packet);
  memset(packet->dest_ip_addr, 0xFF, 4);
  packet->dest_port = 67;
  packet->source_port = 68;
  packet->data = make_uptr(new Array<uint8_t>(sizeof(Packet)));

  // DHCP Discover
  Packet *dhcp_packet = reinterpret_cast<Packet *>(packet->data->GetRawPtr());

  memset(dhcp_packet, 0, sizeof(Packet));

  dhcp_packet->op = 1;
  dhcp_packet->htype = 1;
  dhcp_packet->hlen = 6;
  dhcp_packet->hops = 0;
  dhcp_packet->xid = rand();
  dhcp_packet->secs = 0;
  dhcp_packet->flags = __builtin_bswap16(0x8000);
  dhcp_packet->ciaddr = 0;
  dhcp_packet->yiaddr = 0;
  dhcp_packet->siaddr = siaddr;
  dhcp_packet->giaddr = 0;
  static_cast<DevEthernet *>(dev)->GetEthAddr(dhcp_packet->chaddr);
  dhcp_packet->magic_cookie = __builtin_bswap32(kMagicCookie);

  size_t offset = 0;

  // DHCP Message Type
  dhcp_packet->options[offset++] = 53;
  dhcp_packet->options[offset++] = 1;
  dhcp_packet->options[offset++] = 3;

  // Requested IP Address
  dhcp_packet->options[offset++] = 50;
  dhcp_packet->options[offset++] = 4;
  memcpy(reinterpret_cast<uint32_t *>(dhcp_packet->options + offset), &yiaddr,
         4);
  offset += 4;

  // DHCP server
  dhcp_packet->options[offset++] = 54;
  dhcp_packet->options[offset++] = 4;
  memcpy(reinterpret_cast<uint32_t *>(dhcp_packet->options + offset), &siaddr,
         4);
  offset += 4;

  // Endmark
  dhcp_packet->options[offset++] = 255;

  UdpCtrl::GetCtrl().SendBroadCast(packet, dev);
}
