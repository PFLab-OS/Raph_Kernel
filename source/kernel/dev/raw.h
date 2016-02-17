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
 * Author: Liva
 *
 * User level raw ethernet paceket sender (for debugging on Linux)
 * 
 */

#ifndef __RAPH_KERNEL_DEV_RAW_H__
#define __RAPH_KERNEL_DEV_RAW_H__

#include "eth.h"
#include <stdint.h>

#ifdef __UNIT_TEST__

#include <string.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

class DevRawEthernet : public DevEthernet {
 public:
 DevRawEthernet() : DevEthernet(0, 0, 0) {
    _pd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (_pd < 0) {
      perror("socket():");
      exit(1);
    }

    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, "br0", IFNAMSIZ);
    ioctl(_pd, SIOCGIFINDEX, &ifr);
    _ifindex = ifr.ifr_ifindex;

    struct sockaddr_ll sll;

    memset(&sll, 0xff, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = _ifindex;
    bind(_pd, (struct sockaddr *)&sll, sizeof sll);

    char buf[100];

    // flush buffers
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
  virtual uint32_t ReceivePacket(uint8_t *buffer, uint32_t size) override {
    // TODO ブロックしてしまうため、本来の挙動とは少し異なるのを修正
    return static_cast<uint32_t>(recv(_pd, buffer, size, 0));
  }
  virtual uint32_t TransmitPacket(const uint8_t *packet, uint32_t length) override {
    struct sockaddr_ll sll;

    memset(&sll, 0, sizeof(sll));
    sll.sll_ifindex = _ifindex;
    return static_cast<uint32_t>(sendto(_pd, packet, length, 0, (struct sockaddr *)&sll, sizeof(sll)));
  }
 private:
  int _pd;
  int _ifindex;
};

#endif // __UNIT_TEST__

#endif /* __RAPH_KERNEL_DEV_raw_H__ */
