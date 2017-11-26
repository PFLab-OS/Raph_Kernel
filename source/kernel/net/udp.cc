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

UdpCtrl *UdpCtrl::_udp_ctrl = nullptr;
extern CpuId network_cpu;

void UdpCtrl::Init() { _udp_ctrl = new UdpCtrl; }

void UdpCtrl::SetupServer() {
  auto devices = netdev_ctrl->GetNamesOfAllDevices();
  for (size_t i = 0; i < devices->GetLen(); i++) {
    auto dev = netdev_ctrl->GetDeviceInfo((*devices)[i])->device;
    if (dev->GetStatus() != NetDev::LinkStatus::kUp) {
      continue;
    }
    uint32_t my_addr_int_;
    if (!dev->GetIpv4Address(my_addr_int_) || my_addr_int_ == 0) {
      continue;
    }
    gtty->Printf("start udp server on %s\n", (*devices)[i]);
    dev->SetReceiveCallback(
        network_cpu,
        make_uptr(new Function<NetDev *>(
            [](NetDev *eth) {
              NetDev::Packet *rpacket;
              if (!eth->ReceivePacket(rpacket)) {
                return;
              }
              uint32_t my_addr_int;
              assert(eth->GetIpv4Address(my_addr_int));
              uint8_t my_addr[4];
              my_addr[0] = (my_addr_int >> 0) & 0xff;
              my_addr[1] = (my_addr_int >> 8) & 0xff;
              my_addr[2] = (my_addr_int >> 16) & 0xff;
              my_addr[3] = (my_addr_int >> 24) & 0xff;

              if (rpacket->GetBuffer()[12] == 0x08 &&
                  rpacket->GetBuffer()[13] == 0x06) {
                // ARP

                // received packet
                if (rpacket->GetBuffer()[21] == 0x02) {
                  // ARP Reply
                  uint32_t target_addr_int = (rpacket->GetBuffer()[31] << 24) |
                                             (rpacket->GetBuffer()[30] << 16) |
                                             (rpacket->GetBuffer()[29] << 8) |
                                             rpacket->GetBuffer()[28];
                  arp_table->Set(target_addr_int, rpacket->GetBuffer() + 22,
                                 eth);
                }
                if (rpacket->GetBuffer()[21] == 0x01 &&
                    (memcmp(rpacket->GetBuffer() + 38, my_addr, 4) == 0)) {
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
                  static_cast<DevEthernet *>(eth)->GetEthAddr(data + 6);
                  memcpy(data + 22, data + 6, 6);
                  memcpy(data + 28, my_addr, 4);
                  memcpy(data + 32, rpacket->GetBuffer() + 22, 6);
                  memcpy(data + 38, rpacket->GetBuffer() + 28, 4);

                  uint32_t len = sizeof(data) / sizeof(uint8_t);
                  NetDev::Packet *tpacket;
                  kassert(eth->GetTxPacket(tpacket));
                  memcpy(tpacket->GetBuffer(), data, len);
                  tpacket->len = len;
                  eth->TransmitPacket(tpacket);
                }
              }

              uint8_t *eth_data = rpacket->GetBuffer() + 14;
              if (rpacket->GetBuffer()[12] == 0x08 &&
                  rpacket->GetBuffer()[13] == 0x00) {
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

                  // host address
                  uptr<uint8_t[4]> haddress;
                  haddress[0] = eth_data[12];
                  haddress[1] = eth_data[13];
                  haddress[2] = eth_data[14];
                  haddress[3] = eth_data[15];

                  // target address
                  uptr<uint8_t[4]> taddress;
                  haddress[0] = eth_data[16];
                  haddress[1] = eth_data[17];
                  haddress[2] = eth_data[18];
                  haddress[3] = eth_data[19];

                  uint8_t *ipv4_data = eth_data + ipv4_header_length;
                  if (protocol == 17) {
                    // UDP

                    // uint16_t hport = (ipv4_data[0] << 8) | ipv4_data[1];
                    // uint16_t tport = (ipv4_data[2] << 8) | ipv4_data[3];

                    uint16_t udp_length = (ipv4_data[4] << 8) | ipv4_data[5];
                    uint16_t udp_data_length = udp_length - 8;

                    uptr<Array<uint8_t>> data(
                        new Array<uint8_t>(udp_data_length));
                    memcpy(data->GetRawPtr(), ipv4_data + 8, udp_data_length);

                    for (uint16_t index = 0; index < udp_data_length; index++) {
                      gtty->Printf("%c", (*data)[index]);
                    }
                    gtty->Printf("\n");
                  }

                } while (0);
              }

              eth->ReuseRxBuffer(rpacket);
            },
            dev)));
  }
}

void UdpCtrl::Send(uint8_t (*target_addr)[4], uint16_t target_port,
                   const uint8_t *data, size_t len) {
  uint32_t target_addr_int = ((*target_addr)[3] << 24) |
                             ((*target_addr)[2] << 16) |
                             ((*target_addr)[1] << 8) | (*target_addr)[0];

  uint8_t target_mac[6];
  NetDev *dev = arp_table->Search(target_addr_int, target_mac);
  if (dev == nullptr) {
    gtty->Printf("cannot solve mac address from ARP Table.\n");
    return;
  }

  uint32_t my_addr_int;
  assert(dev->GetIpv4Address(my_addr_int));
  uint8_t my_addr[4];
  my_addr[0] = (my_addr_int >> 0) & 0xff;
  my_addr[1] = (my_addr_int >> 8) & 0xff;
  my_addr[2] = (my_addr_int >> 16) & 0xff;
  my_addr[3] = (my_addr_int >> 24) & 0xff;

  uint8_t buf[1518];
  int offset = 0;

  //
  // ethernet
  //

  // target MAC address
  memcpy(buf + offset, target_mac, 6);
  offset += 6;

  // source MAC address
  static_cast<DevEthernet *>(dev)->GetEthAddr(buf + offset);
  offset += 6;

  // type: IPv4
  uint8_t type[2] = {0x08, 0x00};
  memcpy(buf + offset, type, 2);
  offset += 2;

  //
  // IPv4
  //

  int ipv4_header_start = offset;

  // version & header length
  buf[offset] = (0x4 << 4) | 0x5;
  offset += 1;

  // service type
  buf[offset] = 0;
  offset += 1;

  // skip
  int datagram_length_offset = offset;
  offset += 2;

  // ID field
  buf[offset] = rand() & 0xff;
  buf[offset + 1] = rand() & 0xff;
  offset += 2;

  // flag & flagment offset
  uint16_t foffset = 0 | (1 << 14);
  buf[offset] = foffset >> 8;
  buf[offset + 1] = foffset;
  offset += 2;

  // ttl
  buf[offset] = 0xff;
  offset += 1;

  // protocol number;
  buf[offset] = 17;
  offset += 1;

  // skip
  int checksum_offset = offset;
  buf[offset] = 0;
  buf[offset + 1] = 0;
  offset += 2;

  // source address
  memcpy(buf + offset, my_addr, 4);
  offset += 4;

  // target address
  memcpy(buf + offset, target_addr, 4);
  offset += 4;

  int ipv4_header_end = offset;

  //
  // udp
  //

  int udp_header_start = offset;

  // source port
  uint8_t source_port[] = {0x30, 0x39};
  memcpy(buf + offset, source_port, 2);
  offset += 2;

  // target port
  uint8_t target_port_[] = {(uint8_t)(target_port >> 8),
                            (uint8_t)(target_port & 0xFF)};
  memcpy(buf + offset, target_port_, 2);
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
  memcpy(buf + offset, data, len);
  offset += len;

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
    memcpy(pseudo_header + 0, my_addr, 4);
    memcpy(pseudo_header + 4, target_addr, 4);
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
  kassert(dev->GetTxPacket(tpacket));
  memcpy(tpacket->GetBuffer(), buf, offset);
  tpacket->len = offset;

  dev->TransmitPacket(tpacket);
}
