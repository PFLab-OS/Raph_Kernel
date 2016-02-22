/*
 *
 * Copyright (c) 2015 Raphine Project
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
 * User level unit test
 * 
 */

#include <iostream>
#include "spinlock.h"
#include "mem/virtmem.h"
#include "global.h"
#include "dev/raw.h"

#include "net/eth.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/tcp.h"

SpinLockCtrl *spinlock_ctrl;
VirtmemCtrl *virtmem_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;

TCPCtrl *tcp_ctrl;
UDPCtrl *udp_ctrl;
IPCtrl *ip_ctrl;
EthCtrl *eth_ctrl;

void spinlock_test();
void list_test();
void virtmem_test();
void physmem_test();
void paging_test();

int main() {
  //spinlock_test();
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  virtmem_ctrl = nullptr;
  //paging_test();
  //physmem_test();
  //list_test();
  //virtmem_test();
  VirtmemCtrl _virtmem_ctrl;
  virtmem_ctrl = &_virtmem_ctrl;
  //memory_test();

  EthCtrl _eth_ctrl;
  eth_ctrl = &_eth_ctrl;

  IPCtrl _ip_ctrl(eth_ctrl);
  ip_ctrl = &_ip_ctrl;

  UDPCtrl _udp_ctrl(ip_ctrl);
  udp_ctrl = &_udp_ctrl;
 
  TCPCtrl _tcp_ctrl(ip_ctrl);
  tcp_ctrl = &_tcp_ctrl;

  DevRawEthernet eth;

  uint32_t addr = 0x0a00020f;
  uint32_t port = 4000;

  uint8_t buf[] = {
    0x41, 0x42, 0x43, 0x44, 0x00,
  };

  udp_ctrl->Transmit(buf, sizeof(buf), addr, port, port);

//  if(!strncmp(argv[1], "server", 6)) {
//    tcp_ctrl->Listen(port);
//  } else if(!strncmp(argv[1], "client", 6)) {
//    tcp_ctrl->Connect(addr, port, port);
//  } else {
//    std::cout << "specify either `server` or `client`" << std::endl;
//  }

  std::cout << "test passed" << std::endl;
  return 0;
}
