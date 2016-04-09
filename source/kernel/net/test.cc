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
 * Author: levelfour
 * 
 */

#include <unistd.h>
#include <cstdio>
#include <net/test.h>
#include <net/socket.h>

void ARPReply(uint32_t ipRequest, uint32_t ipReply) {
  ARPSocket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[open] cannot open socket\n");
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
    fprintf(stderr, "[ARP] failed to send reply packet\n");
  } else {
    fprintf(stderr, "[ARP] request from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
      (ipaddr >> 24), (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
      macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    fflush(stdout);
  }
}

void ARPRequest(uint32_t ipRequest, uint32_t ipReply) {
  ARPSocket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[open] cannot open socket\n");
  }

  uint32_t ipaddr;
  uint8_t macaddr[6];

  // send ARP request
  socket.SetIPAddr(ipRequest);
  if(socket.TransmitPacket(ARPSocket::kOpARPRequest, ipReply, nullptr) < 0) {
    fprintf(stderr, "[ARP] failed to send request packet\n");
  } else {
    socket.ReceivePacket(ARPSocket::kOpARPReply, &ipaddr, macaddr);
  
    fprintf(stderr, "[ARP] reply from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
      (ipaddr >> 24), (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
      macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
    fflush(stdout);
  }
}

void TCPServer() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 0x100;
  uint8_t data[size];
  int32_t rval;

  // TCP server 
  socket.Listen();
  fprintf(stderr, "[TCP:server] connection established\n");

  // loopback
  while(1) {
    if((rval = socket.ReceivePacket(data, size)) >= 0) {
      fprintf(stderr, "[TCP:server] received; %s\n", data);
      socket.TransmitPacket(data, strlen(reinterpret_cast<char*>(data)) + 1);
      fprintf(stderr, "[TCP:server] loopback\n");
    } else if(rval == Socket::kConnectionClosed) {
      break;
    }
  }
  fprintf(stderr, "[TCP:server] closed\n");
}

void TCPClient() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 0x100;
  uint8_t data[size];

  // TCP client
  socket.Connect();
  fprintf(stderr, "[TCP:client] connection established\n");

  while(1) {
    printf(">> ");
    if(scanf("%s", data) < 1) {
      // do nothing (just avoid warning)
    }
    if(!strncmp(reinterpret_cast<char*>(data), "q", 1)) break;

    socket.TransmitPacket(data, strlen(reinterpret_cast<char*>(data)) + 1);
    fprintf(stderr, "[TCP:client] sent; %s\n", data);

    while(1) {
      if(socket.ReceivePacket(data, size) >= 0) break;
    }
    fprintf(stderr, "[TCP:client] received; %s\n", data);
  }

  socket.Close();
  fprintf(stderr, "[TCP:client] closed\n");
}

void TCPServer2() {
}

void TCPClient2() {
}
