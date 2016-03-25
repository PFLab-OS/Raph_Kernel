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
#include <thread>

const char DevRawEthernet::kNetworkInterfaceName[] = "br0";

DevRawEthernet::DevRawEthernet() : DevEthernet(0, 0, 0) {
  _pd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (_pd < 0) {
    perror("socket():");
    exit(1);
  }

  struct ifreq ifr;

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, kNetworkInterfaceName, IFNAMSIZ);
  ioctl(_pd, SIOCGIFINDEX, &ifr);
  _ifindex = ifr.ifr_ifindex;

  struct sockaddr_ll sll;

  memset(&sll, 0xff, sizeof(sll));
  sll.sll_family = AF_PACKET;
  sll.sll_protocol = htons(ETH_P_ALL);
  sll.sll_ifindex = _ifindex;
  bind(_pd, (struct sockaddr *)&sll, sizeof sll);

  FetchAddress();
  FlushSocket();

  DevRawEthernet *that = this;
  std::thread t1([&that]{
      while (true) {
        while (!that->_tx_buffered.IsEmpty()) {
          Packet *packet;
          assert(that->_tx_buffered.Pop(packet));
          that->Transmit(packet->buf, packet->len);
          that->ReuseTxBuffer(packet);
        }
      }
    });

  std::thread t2([&that]{
      while (true) {
        Packet *packet;
        if (that->_rx_reserved.Pop(packet)) {
          packet->len = that->Receive(packet->buf, MCLBYTES);
          if (!that->_rx_buffered.Push(packet)) {
            assert(that->_rx_reserved.Push(packet));
          }
        }
      }
    });

  if(!netdev_ctrl->RegisterDevice(this)) {
    // cannot register device
    kassert(false);
  }
}

int32_t DevRawEthernet::Receive(uint8_t *buffer, uint32_t size) {
  return static_cast<int32_t>(recv(_pd, buffer, size, 0));
}

int32_t DevRawEthernet::Transmit(const uint8_t *packet, uint32_t length) {
  struct sockaddr_ll sll;

  memset(&sll, 0, sizeof(sll));
  sll.sll_ifindex = _ifindex;
  int32_t rval = static_cast<int32_t>(sendto(_pd, packet, length, 0, (struct sockaddr *)&sll, sizeof(sll)));

  return rval;
}

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
  strncpy(ifr.ifr_name, kNetworkInterfaceName, IFNAMSIZ-1);
  ioctl(fd, SIOCGIFHWADDR, &ifr);
  memcpy(_ethAddr, ifr.ifr_hwaddr.sa_data, sizeof(uint8_t) * 6);

  // fetch IP address
  ioctl(fd, SIOCGIFADDR, &ifr);
  _ipAddr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;

  close(fd);
}

void DevRawEthernet::PrintAddrInfo() {
  printf(
    "MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\n"
    "IP Address : %d.%d.%d.%d\n",
    _ethAddr[0], _ethAddr[1], _ethAddr[2], _ethAddr[3], _ethAddr[4], _ethAddr[5],
    (_ipAddr) & 0xff, (_ipAddr >> 8) & 0xff, (_ipAddr >> 16) & 0xff, (_ipAddr >> 24) & 0xff);
}

void DevRawEthernet::TestRawARP() {
  FlushSocket();

  uint8_t data[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Target MAC Address
    _ethAddr[0], _ethAddr[1], _ethAddr[2], _ethAddr[3], _ethAddr[4], _ethAddr[5], // Source MAC Address 
    0x08, 0x06, // Type: ARP
    // ARP Packet
    0x00, 0x01, // HardwareType: Ethernet
    0x08, 0x00, // ProtocolType: IPv4
    0x06, // HardwareLength
    0x04, // ProtocolLength
    0x00, 0x01, // Operation: ARP Request
    _ethAddr[0], _ethAddr[1], _ethAddr[2], _ethAddr[3], _ethAddr[4], _ethAddr[5], // Source Hardware Address 
    // Source Protocol Address
    static_cast<uint8_t>((_ipAddr) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 8) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 16) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 24) & 0xff),
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
    0x0a, 0x00, 0x02, 0x03, // Target Protocol Address
  };
  uint32_t len = sizeof(data)/sizeof(uint8_t);
  Packet *tpacket;
  kassert(this->GetTxPacket(tpacket));
  memcpy(tpacket->buf, data, len);
  tpacket->len = len;
  this->TransmitPacket(tpacket);
  printf("ARP Request sent\n");

  const uint32_t kBufsize = 256;
  const int kInitTryCnt = 5;
  uint8_t buf[kBufsize];
  int tryCnt = kInitTryCnt;
  while(tryCnt--) {
    // polling
    Packet *rpacket;
    if(!this->ReceivePacket(rpacket)) {
      continue;
    } 
    printf("#%d receiving ... ", kInitTryCnt - tryCnt);
    // received packet
    if(((rpacket->buf[12] << 8) | rpacket->buf[13]) == 0x0806) {
      // ARP packet
      printf("\n");
      break;
    } else {
      printf("%04x\n", (rpacket->buf[12] << 8) | rpacket->buf[13]);
    }
    this->ReuseRxBuffer(rpacket);
  }

  assert(((buf[12] << 8) | buf[13]) == 0x0806); // Packet should be ARP

  printf("ARP Reply received; %02x:%02x:%02x:%02x:%02x:%02x -> %d.%d.%d.%d\n",
    buf[6], buf[7], buf[8], buf[9], buf[10], buf[11],
    buf[28], buf[29], buf[30], buf[31]);
}

void DevRawEthernet::TestRawUDP() {
  FlushSocket();

  uint8_t data[] = {
    // ===== Ethernet Header =====
//    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, // Target MAC Address
    _ethAddr[0], _ethAddr[1], _ethAddr[2], _ethAddr[3], _ethAddr[4], _ethAddr[5], // Target MAC Address 
    _ethAddr[0], _ethAddr[1], _ethAddr[2], _ethAddr[3], _ethAddr[4], _ethAddr[5], // Source MAC Address 
    0x08, 0x00, // Type: IPv4
    // ===== IPv4 Header =====
    0x45, // IP Version (4bit) | Header Size (4bit)
    0xfc, // TYPE of service
    0x00, 0x20, // total length
    0x00, 0x00, // identification
    0x40, 0x00, // No Fragments
    0x10, // TTL
    0x11, // Protocol: UDP
    0x9e, 0xfe, // Header checksum
    // Source Address
    static_cast<uint8_t>((_ipAddr >> 24) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 16) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 8) & 0xff),
    static_cast<uint8_t>((_ipAddr) & 0xff),
//    0x0f, 0x02, 0x00, 0x0a, // Destination Address
    // Target Address
    static_cast<uint8_t>((_ipAddr >> 24) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 16) & 0xff),
    static_cast<uint8_t>((_ipAddr >> 8) & 0xff),
    static_cast<uint8_t>((_ipAddr) & 0xff),
    // ===== UDP Header =====
    0x00, 0x50, // Source Port
    0x00, 0x50, // Destination Port
    0x00, 0x0c, // length
    0x00, 0x00, // checksum (zero)
    // ===== Datagram Body =====
    0x41, 0x42, 0x43, 0x44,
  };
  uint32_t len = sizeof(data)/sizeof(uint8_t);
  Packet *tpacket;
  kassert(this->GetTxPacket(tpacket));
  memcpy(tpacket->buf, data, len);
  tpacket->len = len;
  this->TransmitPacket(tpacket);
  printf("UDP message sent\n");
}

#endif // __UNIT_TEST__
