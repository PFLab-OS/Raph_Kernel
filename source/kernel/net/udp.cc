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

#include <stdlib.h>
#include "udp.h"
#include <net/arp.h>
#include <dev/netdev.h>
#include <dev/eth.h>
#include <tty.h>
#include <array.h>

UdpCtrl UdpCtrl::_udp_ctrl;
extern CpuId network_cpu;

void UdpCtrl::Init() { new (&_udp_ctrl) UdpCtrl; }

void UdpCtrl::SetupServer() {
  auto devices = netdev_ctrl->GetNamesOfAllDevices();
  for (size_t i = 0; i < devices->GetLen(); i++) {
    auto dev = netdev_ctrl->GetDeviceInfo((*devices)[i])->device;
    dev->SetReceiveCallback(network_cpu,
                            make_uptr(new ClassFunction<UdpCtrl, NetDev *>(
                                this, &UdpCtrl::DummyServer, dev)));
  }
}

void UdpCtrl::Send(uptr<UdpCtrl::Packet> packet) {
  auto full_packet = make_uptr(new UdpCtrl::FullPacket);
  uint32_t dest_addr_int =
      ((packet->dest_ip_addr)[3] << 24) | ((packet->dest_ip_addr)[2] << 16) |
      ((packet->dest_ip_addr)[1] << 8) | (packet->dest_ip_addr)[0];
  full_packet->packet = packet;
  full_packet->dev =
      arp_table->Search(dest_addr_int, full_packet->dest_mac_addr);
  if (full_packet->dev == nullptr) {
    gtty->Printf("cannot solve mac address from ARP Table.\n");
    return;
  }
  uint32_t source_addr_int;
  assert(full_packet->dev->GetIpv4Address(source_addr_int));
  *(reinterpret_cast<uint32_t *>(full_packet->source_ip_addr)) =
      source_addr_int;
  Send(full_packet);
}

void UdpCtrl::SendBroadCast(uptr<UdpCtrl::Packet> packet, NetDev *dev) {
  auto full_packet = make_uptr(new UdpCtrl::FullPacket);
  full_packet->packet = packet;
  full_packet->dev = dev;
  memset(full_packet->dest_mac_addr, 0xFF, 6);
  memset(full_packet->source_ip_addr, 0, 4);
  Send(full_packet);
}

void UdpCtrl::Send(uptr<UdpCtrl::FullPacket> full_packet) {
  uint8_t buf[1518];
  int offset = 0;
  EthernetRawPacket *eth_raw_packet = reinterpret_cast<EthernetRawPacket *>(buf);

  //
  // ethernet
  //

  memcpy(eth_raw_packet->target_addr, full_packet->dest_mac_addr, 6);
  static_cast<DevEthernet *>(full_packet->dev)->GetEthAddr(eth_raw_packet->source_addr);
  
  uint8_t type[2] = {0x08, 0x00}; // IPv4
  memcpy(&eth_raw_packet->type, type, sizeof(eth_raw_packet->type));
  
  offset += sizeof(EthernetRawPacket);

  //
  // IPv4
  //

  int ipv4_header_start = offset;
  IpV4RawPacket *ipv4_raw_packet = reinterpret_cast<IpV4RawPacket *>(&buf[offset]);

  ipv4_raw_packet->ver_and_len = (0x4 << 4) | 0x5;
  ipv4_raw_packet->type = 0;
  int datagram_length_offset = offset + __builtin_offsetof(IpV4RawPacket, blank1);
  ipv4_raw_packet->blank1 = 0;
  ipv4_raw_packet->id = rand() & 0xFFFF;
  ipv4_raw_packet->foffset = __builtin_bswap16(0 | (1 << 14));
  ipv4_raw_packet->ttl = 0xff;
  ipv4_raw_packet->protocol_number = 17;
  int checksum_offset = offset + __builtin_offsetof(IpV4RawPacket, blank2);
  ipv4_raw_packet->blank2 = 0;
  memcpy(ipv4_raw_packet->source_addr, full_packet->source_ip_addr, 4);
  memcpy(ipv4_raw_packet->target_addr, full_packet->packet->dest_ip_addr, 4);

  offset += sizeof(IpV4RawPacket);

  int ipv4_header_end = offset;

  //
  // udp
  //

  int udp_header_start = offset;

  // source port
  uint16_t source_port = __builtin_bswap16(full_packet->packet->source_port);
  memcpy(buf + offset, &source_port, 2);
  offset += 2;

  // dest port
  uint16_t dest_port = __builtin_bswap16(full_packet->packet->dest_port);
  memcpy(buf + offset, &dest_port, 2);
  offset += 2;

  // skip
  int udp_length_offset = offset;
  offset += 2;

  // skip
  int udp_checksum_offset = offset;
  buf[offset] = 0;
  buf[offset + 1] = 0;
  offset += 2;

  // data
  memcpy(buf + offset, full_packet->packet->data->GetRawPtr(),
         full_packet->packet->data->GetLen());
  offset += full_packet->packet->data->GetLen();

  // length
  size_t udp_length = offset - udp_header_start;
  buf[udp_length_offset] = udp_length >> 8;
  buf[udp_length_offset + 1] = udp_length;

  // checksum
  {
    uint32_t checksum = 0;
    uint8_t pseudo_header[] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 17, 0x00, 0x00,
    };
    memcpy(pseudo_header + 0, full_packet->source_ip_addr, 4);
    memcpy(pseudo_header + 4, full_packet->packet->dest_ip_addr, 4);
    pseudo_header[10] = udp_length >> 8;
    pseudo_header[11] = udp_length;
    for (int i = 0; i < 12; i += 2) {
      checksum += (pseudo_header[i] << 8) + pseudo_header[i + 1];
    }

    if ((offset - udp_header_start) % 2 == 1) {
      buf[offset] = 0x0;
    }
    for (int i = udp_header_start; i < offset; i += 2) {
      checksum += (buf[i] << 8) + buf[i + 1];
    }

    while (checksum > 0xffff) {
      checksum = (checksum >> 16) + (checksum & 0xffff);
    }
    checksum = ~checksum;

    buf[udp_checksum_offset] = checksum >> 8;
    buf[udp_checksum_offset + 1] = checksum;
  }

  // datagram length(IPv4)
  size_t ipv4_len = offset - ipv4_header_start;
  buf[datagram_length_offset] = ipv4_len >> 8;
  buf[datagram_length_offset + 1] = ipv4_len;

  // checksum
  {
    uint32_t checksum = 0;
    for (int i = ipv4_header_start; i < ipv4_header_end; i += 2) {
      checksum += (buf[i] << 8) + buf[i + 1];
    }

    while (checksum > 0xffff) {
      checksum = (checksum >> 16) + (checksum & 0xffff);
    }
    checksum = ~checksum;

    buf[checksum_offset] = checksum >> 8;
    buf[checksum_offset + 1] = checksum;
  }

  assert(offset < 1518);

  NetDev::Packet *tpacket;
  kassert(full_packet->dev->GetTxPacket(tpacket));
  memcpy(tpacket->GetBuffer(), buf, offset);
  tpacket->len = offset;

  full_packet->dev->TransmitPacket(tpacket);
}

void UdpCtrl::Send(uint8_t (*target_addr)[4], uint16_t target_port,
                   const uint8_t *data, size_t len) {
  auto packet = make_uptr(new UdpCtrl::Packet);
  memcpy(packet->dest_ip_addr, target_addr, 4);
  packet->dest_port = target_port;
  packet->source_port = 12345;
  packet->data = make_uptr(new Array<uint8_t>(len));
  memcpy(packet->data->GetRawPtr(), data, len);
  Send(packet);
}

void UdpCtrl::DummyServer(NetDev *dev) {
  NetDev::Packet *rpacket;
  if (!dev->ReceivePacket(rpacket)) {
    return;
  }
  IpV4Addr my_addr;
  assert(dev->GetIpv4Address(my_addr.uint32));

  if (rpacket->GetBuffer()[12] == 0x08 && rpacket->GetBuffer()[13] == 0x06) {
    // ARP

    // received packet
    if (rpacket->GetBuffer()[21] == 0x02) {
      // ARP Reply
      IpV4Addr responder_addr;
      memcpy(responder_addr.bytes, &rpacket->GetBuffer()[28], 4);
      gtty->Printf("ARP reply from %d.%d.%d.%d\n", responder_addr.bytes[0],
                   responder_addr.bytes[1], responder_addr.bytes[2],
                   responder_addr.bytes[3]);
      arp_table->Set(responder_addr.uint32, rpacket->GetBuffer() + 22, dev);
    }
    if (rpacket->GetBuffer()[21] == 0x01 &&
        (memcmp(rpacket->GetBuffer() + 38, my_addr.bytes, 4) == 0)) {
      // ARP Request
      uint8_t data[] = {
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Target MAC Address
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source MAC Address
          0x08, 0x06,                          // Type: ARP
          // ARP Packet
          0x00, 0x01,  // HardwareType: Ethernet
          0x08, 0x00,  // ProtocolType: IPv4
          0x06,        // HardwareLength
          0x04,        // ProtocolLength
          0x00, 0x02,  // Operation: ARP Reply
          0x00, 0x00, 0x00, 0x00, 0x00,
          0x00,                    // Source Hardware Address
          0x00, 0x00, 0x00, 0x00,  // Source Protocol Address
          0x00, 0x00, 0x00, 0x00, 0x00,
          0x00,                    // Target Hardware Address
          0x00, 0x00, 0x00, 0x00,  // Target Protocol Address
      };
      memcpy(data, rpacket->GetBuffer() + 6, 6);
      static_cast<DevEthernet *>(dev)->GetEthAddr(data + 6);
      memcpy(data + 22, data + 6, 6);
      memcpy(data + 28, my_addr.bytes, 4);
      memcpy(data + 32, rpacket->GetBuffer() + 22, 6);
      memcpy(data + 38, rpacket->GetBuffer() + 28, 4);

      uint32_t len = sizeof(data) / sizeof(uint8_t);
      NetDev::Packet *tpacket;
      kassert(dev->GetTxPacket(tpacket));
      memcpy(tpacket->GetBuffer(), data, len);
      tpacket->len = len;
      dev->TransmitPacket(tpacket);
    }
  }

  uint8_t *eth_data = rpacket->GetBuffer() + 14;
  if (rpacket->GetBuffer()[12] == 0x08 && rpacket->GetBuffer()[13] == 0x00) {
    // IPv4

    do {
      // version
      if (eth_data[0] >> 4 != 0x4) {
        break;
      }

      // header length
      int ipv4_header_length = (eth_data[0] & 0x0F) * 4;
      if (ipv4_header_length < 20) {
        break;
      }

      // ignore service type

      // ignore id field

      // flag & flagment offset
      uint16_t foffset = (eth_data[6] << 8) | eth_data[7];
      if ((foffset & (1 << 13)) != 0 || (foffset & 0x1FFF) != 0) {
        break;
      }

      // ignore ttl

      // protocol number
      uint8_t protocol = eth_data[9];

      // source address
      uint8_t saddress[4];
      memcpy(saddress, &eth_data[12], 4);

      // dest address
      uint8_t daddress[4];
      memcpy(daddress, &eth_data[16], 4);

      uint8_t broadcast[4] = {0xFF, 0xFF, 0xFF, 0xFF};
      if (!memcmp(daddress, my_addr.bytes, 4) == 0 &&
          !memcmp(daddress, broadcast, 4) == 0) {
        break;
      }

      uint8_t *ipv4_data = eth_data + ipv4_header_length;
      if (protocol == 17) {
        // UDP

        auto packet = make_uptr(new RxPacket);

        memcpy(packet->dest_ip_addr, daddress, 4);
        memcpy(packet->source_ip_addr, saddress, 4);

        packet->source_port = (ipv4_data[0] << 8) | ipv4_data[1];
        packet->dest_port = (ipv4_data[2] << 8) | ipv4_data[3];

        uint16_t udp_length = (ipv4_data[4] << 8) | ipv4_data[5];
        uint16_t udp_data_length = udp_length - 8;

        packet->data = make_uptr(new Array<uint8_t>(udp_data_length));
        memcpy(packet->data->GetRawPtr(), ipv4_data + 8, udp_data_length);

        packet->dev = dev;

        for (int i = 0; i < kSocketNum; i++) {
          if (_socket[i].protocol != nullptr &&
              _socket[i].port == packet->dest_port) {
            _socket[i].protocol->GetRxQueue().Push(packet);
            break;
          }
          if (i == kSocketNum - 1) {
            gtty->Printf("received unknown udp(%d) packet\n",
                         packet->dest_port);
          }
        }
      }

    } while (0);
  }

  dev->ReuseRxBuffer(rpacket);
}
