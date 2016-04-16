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
#include <callout.h>
#include <dev/posixtimer.h>
#include <net/test.h>
#include <net/socket.h>

Callout tt1, tt2;

void StripNewline(uint8_t *data) {
  data[strcspn(reinterpret_cast<char*>(data), "\n")] = '%';
}

ArpSocket arpsocket;
uint32_t ipaddr;
uint8_t macaddr[6];
uint32_t ip1;
uint32_t ip2;
bool arp_reply_received = false;
bool thread_end = false;

void ARPReplySub(void *p) {
  if(!arp_reply_received) {
    if(arpsocket.ReceivePacket(ArpSocket::kOpARPRequest, &ipaddr, macaddr) >= 0) {
      // need to wait a little
      // because Linux kernel cannot handle packet too quick
      usleep(570);
      arp_reply_received = true;
    }
  } else {
    if(arpsocket.TransmitPacket(ArpSocket::kOpARPReply, ipaddr, macaddr) < 0) {
      fprintf(stderr, "[ARP] failed to send reply packet\n");
    } else {
      fprintf(stderr, "[ARP] request from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
          (ipaddr >> 24), (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
          macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
      fflush(stdout);
      arp_reply_received = false;
      thread_end = true;
    }
  }

  if(!thread_end) {
    tt1.SetHandler(10);
  } else {
    printf("[ARP] test end\n");
  }
}

void ARPReply(uint32_t ip_request, uint32_t ip_reply) {
  if(arpsocket.Open() < 0) {
    fprintf(stderr, "[open] cannot open socket\n");
  }

  // wait for ARP request
  ip1 = ip_reply;
  ip2 = ip_request;
  arpsocket.SetIPAddr(ip_reply);

  new(&tt1) Callout;
  tt1.Init(ARPReplySub, nullptr);
  tt1.SetHandler(10);
}

bool arp_request_sent = false;

void ARPRequestSub(void *p) {
  if(!arp_request_sent) {
    if(arpsocket.TransmitPacket(ArpSocket::kOpARPRequest, ip1, nullptr) < 0) {
      fprintf(stderr, "[ARP] failed to send request packet\n");
    } else {
      arp_request_sent = true;
    }
  } else {
    if(arpsocket.ReceivePacket(ArpSocket::kOpARPReply, &ipaddr, macaddr) >= 0) {
      fprintf(stderr, "[ARP] reply from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
          (ipaddr >> 24), (ipaddr >> 16) & 0xff, (ipaddr >> 8) & 0xff, ipaddr & 0xff,
          macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
      fflush(stdout);
      arp_request_sent = false;
      thread_end = true;
    }
  }

  if(!thread_end) {
    tt2.SetHandler(10);
  } else {
    printf("[ARP] test end\n");
  }
}

void ARPRequest(uint32_t ip_request, uint32_t ip_reply) {
  if(arpsocket.Open() < 0) {
    fprintf(stderr, "[open] cannot open socket\n");
  }

  // send ARP request
  arpsocket.SetIPAddr(ip_request);
  ip1 = ip_reply;
  ip2 = ip_request;

  new(&tt2) Callout;
  tt2.Init(ARPRequestSub, nullptr);
  tt2.SetHandler(10);
}

void TCPServer1() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenAddr(0x0a000206);
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetIPAddr(0x0a000205);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 0x100;
  uint8_t data[size];
  int32_t rval;
  bool received = false;

  // TCP server 
  while(true) {
    int32_t listen_rval = socket.Listen();
    if(listen_rval >= 0) {
      fprintf(stderr, "[TCP:server] connection established\n");
    } else if(listen_rval == Socket::kResultAlreadyEstablished) {
      if(!received) {
        if((rval = socket.ReceivePacket(data, size)) >= 0) {
          StripNewline(data);
          fprintf(stderr, "[TCP:server] received; %s\n", data);
          received = true;
        } else if(rval == Socket::kResultConnectionClosed) {
          break;
        }
      } else {
        rval = socket.TransmitPacket(data, strlen(reinterpret_cast<char*>(data)) + 1);
        if(rval >= 0) {
          received = false;
          fprintf(stderr, "[TCP:server] loopback\n");
        }
      }
    }
  }
  fprintf(stderr, "[TCP:server] closed\n");
}

void TCPClient1() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenAddr(0x0a000205);
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetIPAddr(0x0a000206);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 0x100;
  uint8_t data[size];
  bool preparing = false;
  bool sent = false;
  bool closing = false;

  // TCP client
  while(true) {
    int32_t connect_rval = socket.Connect();

    if(connect_rval >= 0) {
      fprintf(stderr, "[TCP:client] connection established\n");
      preparing = true;
    } else if(connect_rval == Socket::kResultAlreadyEstablished) {
      if(closing) {
        if(socket.Close() >= 0) {
          fprintf(stderr, "[TCP:client] closed\n");
          break;
        }
      } else if(preparing) {
        printf(">> ");
        if(fgets(reinterpret_cast<char*>(data), size-1, stdin)) {
          data[size-1] = 0;
        }

        if(!strncmp(reinterpret_cast<char*>(data), "q", 1)) {
          closing = true;
          continue;
        }

        preparing = false;
      } else if(!sent) {
        if(socket.TransmitPacket(data, strlen(reinterpret_cast<char*>(data)) + 1) >= 0){
          StripNewline(data);
          fprintf(stderr, "[TCP:client] sent; %s\n", data);
          sent = true;
        }
      } else {
        if(socket.ReceivePacket(data, size) >= 0) {
          fprintf(stderr, "[TCP:client] received; %s\n", data);
          sent = false;
          preparing = true;
        }
      }
    }
  }
}

void TCPServer2() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenAddr(0x0a000206);
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetIPAddr(0x0a000205);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = Socket::kMSS;
  uint8_t data[size];
  int32_t rval;

  while(true) {
    int32_t listen_rval = socket.Listen();

    if(listen_rval >= 0 || listen_rval == Socket::kResultAlreadyEstablished) {
      rval = socket.ReceivePacket(data, size);
      if(rval >= 0) {
        printf("return value = %d\n", rval);
      } else if(rval == Socket::kResultConnectionClosed) {
        break;
      }
    }
  }
}

void TCPClient2() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenAddr(0x0a000205);
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetIPAddr(0x0a000206);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = 8000;
  uint8_t data[size];
  uint32_t s = size;  // length of packet last time sent
  uint8_t *p = data;  // current position in packet
  memset(data, 0x41, size-1);
  data[size-1] = 0;

  while(socket.Connect() < 0);

  int32_t rval;
  uint32_t sum = 0;
  while(sum < size) {
    while((rval = socket.TransmitPacket(p, s)) < 0);
    printf("return value = %d\n", rval);

    if(rval >= 0) {
      p += rval;
      s -= rval;
      sum += rval;
    }
  }

  while(socket.Close() < 0);
}

void TCPServer3() {
  Socket socket;
  if(socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  socket.SetListenAddr(0x0a000206);
  socket.SetListenPort(Socket::kPortTelnet);
  socket.SetIPAddr(0x0a000205);
  socket.SetPort(Socket::kPortTelnet);

  const uint32_t size = Socket::kMSS;
  uint8_t data[size];
  int32_t rval = 0;

  while(socket.Listen() < 0);

  for(uint32_t t = 0; ; t++) {
    // NB: stupid server!!! (test TCP restransmission)
    if(rval >= 0) timer->BusyUwait(5000000);

    rval = socket.ReceivePacket(data, size);
    if(rval >= 0) {
      printf("return value = %d\n", rval);
    } else if(rval == Socket::kResultConnectionClosed) {
      break;
    }
  }
}

void TCPClient3() {
  TCPClient2();
}
