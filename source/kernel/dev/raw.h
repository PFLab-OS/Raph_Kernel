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

#include "netdev.h"
#include "../net/eth.h"
#include "../global.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <thread>

class DevRawEthernet : public DevEthernet {
public:
  DevRawEthernet();
  virtual ~DevRawEthernet();
  void FetchAddress();
  void FlushSocket();
  void PrintAddrInfo();
  void TestRawARP();
  void TestRawUDP();

  // allocate 6 byte before call
  virtual void GetEthAddr(uint8_t *buffer) override {
    memcpy(buffer, _ethAddr, 6);
  }
  virtual void UpdateLinkStatus() override {}

private:
  int _pd;
  int _ifindex;
  uint32_t _ipAddr;
  uint8_t _ethAddr[6] = {0};

  std::thread *_thTx;
  std::thread *_thRx;

  static const char kNetworkInterfaceName[];

  int32_t Receive(uint8_t *buffer, uint32_t size);
  int32_t Transmit(const uint8_t *packet, uint32_t length);
};

#endif // __UNIT_TEST__

#endif /* __RAPH_KERNEL_DEV_raw_H__ */
