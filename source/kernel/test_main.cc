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

#include <spinlock.h>
#include <mem/virtmem.h>
#include <global.h>
#include <thread.h>
#include <task.h>
#include <dev/posixtimer.h>
#include <dev/raw.h>
#include <net/test.h>
#include <net/netctrl.h>
#include <net/socket.h>

SpinLockCtrl *spinlock_ctrl;
VirtmemCtrl *virtmem_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;

PthreadCtrl *apic_ctrl;
TaskCtrl *task_ctrl;

Timer *timer;

int main(int argc, char **argv) {
  srand((unsigned) time(NULL));

  PthreadCtrl _thread_ctrl;
  apic_ctrl = &_thread_ctrl;
  apic_ctrl->Setup();

  TaskCtrl _task_ctrl;
  task_ctrl = &_task_ctrl;
  task_ctrl->Setup();

  PosixTimer _timer;
  timer = &_timer;
  timer->Setup();

  InitNetCtrl();

  DevRawEthernet eth;

  if(!strncmp(argv[1], "arp", 3)) {
    uint32_t ipRequest = 0x0a000210;
    uint32_t ipReply = 0x0a000211;
    if(!strncmp(argv[2], "reply", 5)) {
      ArpReply(ipRequest, ipReply);
    } else if(!strncmp(argv[2], "request", 7)) {
      ArpRequest(ipRequest, ipReply);
    } else {
      fprintf(stderr, "[error] specify ARP command\n");
    }
  } else if(!strncmp(argv[1], "tcp", 3)) {
    if(!strncmp(argv[2], "server", 6)) {
      TcpServer1();
    } else if(!strncmp(argv[2], "client", 6)) {
      TcpClient1();
    } else {
      fprintf(stderr, "[error] specify TCP command\n");
    }
  } else if(!strncmp(argv[1], "segment", 7)) {
    if(!strncmp(argv[2], "server", 6)) {
      TcpServer2();
    } else if(!strncmp(argv[2], "client", 6)) {
      TcpClient2();
    } else {
      fprintf(stderr, "[error] command\n");
    }
  } else if(!strncmp(argv[1], "retry", 5)) {
    if(!strncmp(argv[2], "server", 6)) {
      TcpServer3();
    } else if(!strncmp(argv[2], "client", 6)) {
      TcpClient3();
    } else {
      fprintf(stderr, "[error] command\n");
    }
  } else {
    fprintf(stderr, "[error] specify protocol\n");
  }

  task_ctrl->Run();

  DismissNetCtrl();

  return 0;
}

