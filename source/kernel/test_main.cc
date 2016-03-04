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

#include "net/netctrl.h"
#include "net/socket.h"

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

  InitNetCtrl();

  DevRawEthernet eth;

  UDPSocket socket;
  if(socket.Open() < 0) {
	  std::cerr << "cannot open socket" << std::endl;
  } else {
    uint8_t data[5] = "ABCD";
    socket.TransmitPacket(data, 5);
  }

  std::cout << "test passed" << std::endl;

  DismissNetCtrl();

  return 0;
}
