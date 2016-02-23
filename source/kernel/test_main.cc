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

SpinLockCtrl *spinlock_ctrl;
VirtmemCtrl *virtmem_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;

void spinlock_test();
void list_test();
void virtmem_test();
void physmem_test();
void paging_test();

int main(int argc, char **argv) {
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

  DevRawEthernet eth;

  uint32_t addr = 0x0a00020f;
  uint32_t port = 23;

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
