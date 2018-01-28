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

#include "eth.h"
#include <raph.h>
#include <net/arp.h>
#include <dev/netdev.h>
#include <dev/eth.h>
#include <tty.h>

EthernetCtrl EthernetCtrl::_eth_ctrl;
extern CpuId network_cpu;

void EthernetCtrl::Init() { new (&_eth_ctrl) EthernetCtrl; }

void EthernetCtrl::SetupServer() {
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

                  // uint8_t *ipv4_data = eth_data + ipv4_header_length;
                  if (protocol == 17) {
                    // UDP
                  }

                } while (0);
              }

              eth->ReuseRxBuffer(rpacket);
            },
            dev)));
  }
}
