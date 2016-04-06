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
#include <time.h>
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
void virtmem_test();
void physmem_test();
void paging_test();

void ARPReply(uint32_t ipRequest, uint32_t ipReply);
void ARPRequest(uint32_t ipRequest, uint32_t ipReply);

void TCPServer();
void TCPClient();

int main(int argc, char **argv) {

  srand((unsigned) time(NULL));

  //spinlock_test();
  //SpinLockCtrlTest _spinlock_ctrl;
  //spinlock_ctrl = &_spinlock_ctrl;
  //virtmem_ctrl = nullptr;
  //paging_test();
  //physmem_test();
  //virtmem_test();
  //VirtmemCtrl _virtmem_ctrl;
  //virtmem_ctrl = &_virtmem_ctrl;
  //memory_test();

  InitNetCtrl();

  DevRawEthernet eth;

  if(!strncmp(argv[1], "arp", 3)) {
    uint32_t ipRequest = 0x0a000210;
    uint32_t ipReply = 0x0a000211;
    if(!strncmp(argv[2], "reply", 5)) {
      ARPReply(ipRequest, ipReply);
	} else if(!strncmp(argv[2], "request", 7)) {
	  ARPRequest(ipRequest, ipReply);
	} else {
      std::cerr << "[error] specify ARP command" << std::endl;
	}
  } else if(!strncmp(argv[1], "tcp", 3)) {
    if(!strncmp(argv[2], "server", 6)) {
      TCPServer();
	} else if(!strncmp(argv[2], "client", 6)) {
	  TCPClient();
	} else {
      std::cerr << "[error] specify TCP command" << std::endl;
	}
  } else {
    std::cerr << "[error] specify protocol" << std::endl;
  }

  DismissNetCtrl();

  return 0;
}

void ARPReply(uint32_t ipRequest, uint32_t ipReply) {
  ARPSocket socket;
  if(socket.Open() < 0) {
      std::cerr << "[open] cannot open socket" << std::endl;
  }

  uint32_t ipaddr;
  uint8_t macaddr[6];

  // wait for ARP request
  socket.SetIPAddr(ipReply);
  socket.ReceivePacket(ARPSocket::kOpARPRequest, &ipaddr, macaddr);

  // need to wait a little
  // because Linux kernel cannot handle packet too quick
  usleep(570);

  // ARP reply
  if(socket.TransmitPacket(ARPSocket::kOpARPReply, ipaddr, macaddr) < 0) {
    std::cerr << "[ARP] failed to send reply packet" << std::endl;
  } else {
    std::printf("[ARP] request from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
      (ipaddr >> 24), (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
      macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    std::fflush(stdout);
  }
}

void ARPRequest(uint32_t ipRequest, uint32_t ipReply) {
  ARPSocket socket;
  if(socket.Open() < 0) {
      std::cerr << "[open] cannot open socket" << std::endl;
  }

  uint32_t ipaddr;
  uint8_t macaddr[6];

  // send ARP request
  socket.SetIPAddr(ipRequest);
  if(socket.TransmitPacket(ARPSocket::kOpARPRequest, ipReply, nullptr) < 0) {
    std::cerr << "[ARP] failed to send request packet" << std::endl;
  } else {
    socket.ReceivePacket(ARPSocket::kOpARPReply, &ipaddr, macaddr);
  
    std::printf("[ARP] reply from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
      (ipaddr >> 24), (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
      macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    std::fflush(stdout);
  }
}

void TCPServer() {
  Socket socket;
  if(socket.Open() < 0) {
      std::cerr << "[error] cannot open socket" << std::endl;
  }
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 0x100;
  uint8_t data[size];
  int32_t rval;

  // TCP server 
  socket.Listen();
  std::cerr << "[TCP:server] connection established" << std::endl;

  // loopback
  while(1) {
    if((rval = socket.ReceivePacket(data, size)) >= 0) {
      std::cerr << "[TCP:server] received; " << data << std::endl;
      socket.TransmitPacket(data, strlen(reinterpret_cast<char*>(data)) + 1);
      std::cerr << "[TCP:server] loopback" << std::endl;
    } else if(rval == Socket::kConnectionClosed) {
      break;
    }
  }
  std::cerr << "[TCP:server] closed" << std::endl;
}

void TCPClient() {
  Socket socket;
  if(socket.Open() < 0) {
      std::cerr << "[error] cannot open socket" << std::endl;
  }
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 0x100;
  uint8_t data[size];

  // TCP client
  socket.Connect();
  std::cerr << "[TCP:client] connection established" << std::endl;

  while(1) {
    std::cin >> data;
    if(!strncmp(reinterpret_cast<char*>(data), "q", 1)) break;

    socket.TransmitPacket(data, strlen(reinterpret_cast<char*>(data)) + 1);
    std::cerr << "[TCP:client] sent; " << data << std::endl;

    while(1) {
      if(socket.ReceivePacket(data, size) >= 0) break;
    }
    std::cerr << "[TCP:client] received; " << data << std::endl;
  }

  socket.Close();
  std::cerr << "[TCP:client] closed" << std::endl;
}
