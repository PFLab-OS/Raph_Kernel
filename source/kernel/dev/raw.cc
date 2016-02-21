/*
 *
 * Copyright (c) 2016 Raphine Project
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
 * User level raw ethernet paceket sender (for debugging on Linux)
 * 
 */

#ifdef __UNIT_TEST__

#include "../global.h"
#include "raw.h"

void DevRawEthernet::FlushSocket() {
  char buf[100];
  int i;
  do {
    fd_set fds;
    struct timeval t;
    FD_ZERO(&fds);
    FD_SET(_pd, &fds);
    memset(&t, 0, sizeof(t));
    i = select(FD_SETSIZE, &fds, NULL, NULL, &t);
    if (i > 0)
      recv(_pd, buf, sizeof(buf), 0);
  } while (i);
}

void DevRawEthernet::FetchAddress() {
  int fd;
  struct ifreq ifr;

  fd = socket(AF_INET, SOCK_DGRAM, 0);

  // fetch MAC address
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, NETWORK_INTERFACE, IFNAMSIZ-1);
  ioctl(fd, SIOCGIFHWADDR, &ifr);
  memcpy(_macAddr, ifr.ifr_hwaddr.sa_data, sizeof(uint8_t) * 6);

  // fetch IP address
  ioctl(fd, SIOCGIFADDR, &ifr);
  _ipAddr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

  close(fd);
}

void DevRawEthernet::PrintAddrInfo() {
  printf(
    "MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n"
    "IP Address : %d.%d.%d.%d\n",
    _macAddr[0], _macAddr[1], _macAddr[2], _macAddr[3], _macAddr[4], _macAddr[5],
    (_ipAddr) & 0xff, (_ipAddr >> 8) & 0xff, (_ipAddr >> 16) & 0xff, (_ipAddr >> 24) & 0xff);
}

void DevRawEthernet::TestRawARP() {
  FlushSocket();

  uint8_t data[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Target MAC Address
    _macAddr[0], _macAddr[1], _macAddr[2], _macAddr[3], _macAddr[4], _macAddr[5], // Source MAC Address 
    0x08, 0x06, // Type: ARP
    // ARP Packet
    0x00, 0x01, // HardwareType: Ethernet
    0x08, 0x00, // ProtocolType: IPv4
    0x06, // HardwareLength
    0x04, // ProtocolLength
    0x00, 0x01, // Operation: ARP Request
    _macAddr[0], _macAddr[1], _macAddr[2], _macAddr[3], _macAddr[4], _macAddr[5], // Source Hardware Address 
    // Source Protocol Address
    static_cast<uint8_t>((_ipAddr) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 8) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 16) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 24) & 0xff),
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
    0x0a, 0x00, 0x02, 0x03, // Target Protocol Address
  };
  uint32_t len = sizeof(data)/sizeof(uint8_t);
  this->TransmitPacket(data, len);
  printf("ARP Request sent\n");

  const uint32_t kBufsize = 256;
  const int kInitTryCnt = 5;
  uint8_t buf[kBufsize];
  int tryCnt = kInitTryCnt;
  while(tryCnt--) {
    // polling
    printf("#%d receiving ... ", kInitTryCnt - tryCnt);
    if(this->ReceivePacket(buf, kBufsize) != -1) {
      // received packet
      if(((buf[12] << 8) | buf[13]) == 0x0806) {
        // ARP packet
        printf("\n");
        break;
      } else {
        printf("%04x\n", (buf[12] << 8) | buf[13]);
      }
    }
  }

  assert(((buf[12] << 8) | buf[13]) == 0x0806); // Packet should be ARP

  printf("ARP Reply received; %02x:%02x:%02x:%02x:%02x:%02x -> %d.%d.%d.%d\n",
    buf[6], buf[7], buf[8], buf[9], buf[10], buf[11],
    buf[28], buf[29], buf[30], buf[31]);
}

#endif // __UNIT_TEST__
