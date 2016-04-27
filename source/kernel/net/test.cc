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

bool test_end = false;
Callout tt1, tt2;

void StripNewline(uint8_t *data) {
  data[strcspn(reinterpret_cast<char*>(data), "\n")] = '%';
}

uint32_t ip1;
uint32_t ip2;
bool thread_end = false;

namespace arp {
  ArpSocket socket;
  uint32_t ipaddr;
  uint8_t macaddr[6];
  bool reply_received = false;
  bool request_sent = false;
};

void ARPReplySub(void *p) {
  if(!arp::reply_received) {
    if(arp::socket.ReceivePacket(ArpSocket::kOpARPRequest, &arp::ipaddr, arp::macaddr) >= 0) {
      // need to wait a little
      // because Linux kernel cannot handle packet too quick
      usleep(570);
      arp::reply_received = true;
    }
  } else {
    if(arp::socket.TransmitPacket(ArpSocket::kOpARPReply, arp::ipaddr, arp::macaddr) < 0) {
      fprintf(stderr, "[ARP] failed to send reply packet\n");
    } else {
      fprintf(stderr, "[ARP] request from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
          (arp::ipaddr >> 24), (arp::ipaddr >> 16) & 0xff, (arp::ipaddr >> 8) & 0xff, arp::ipaddr & 0xff,
          arp::macaddr[0], arp::macaddr[1], arp::macaddr[2], arp::macaddr[3], arp::macaddr[4], arp::macaddr[5]);
      fflush(stdout);
      arp::reply_received = false;
      thread_end = true;
    }
  }

  if(!thread_end) {
    tt1.SetHandler(10);
  } else {
    fprintf(stderr, "[ARP] test end\n");
    test_end = true;
  }
}

void ARPReply(uint32_t ip_request, uint32_t ip_reply) {
  if(arp::socket.Open() < 0) {
    fprintf(stderr, "[open] cannot open socket\n");
  }

  // wait for ARP request
  ip1 = ip_reply;
  ip2 = ip_request;
  arp::socket.SetIPAddr(ip_reply);

  new(&tt1) Callout;
  tt1.Init(ARPReplySub, nullptr);
  tt1.SetHandler(10);
}

void ARPRequestSub(void *p) {
  if(!arp::request_sent) {
    if(arp::socket.TransmitPacket(ArpSocket::kOpARPRequest, ip1, nullptr) < 0) {
      fprintf(stderr, "[ARP] failed to send request packet\n");
    } else {
      arp::request_sent = true;
    }
  } else {
    if(arp::socket.ReceivePacket(ArpSocket::kOpARPReply, &arp::ipaddr, arp::macaddr) >= 0) {
      fprintf(stderr, "[ARP] reply from %u.%u.%u.%u (%.2x:%.2x:%.2x:%.2x:%.2x:%.2x)\n",
          (arp::ipaddr >> 24), (arp::ipaddr >> 16) & 0xff, (arp::ipaddr >> 8) & 0xff, arp::ipaddr & 0xff,
          arp::macaddr[0], arp::macaddr[1], arp::macaddr[2], arp::macaddr[3], arp::macaddr[4], arp::macaddr[5]);
      fflush(stdout);
      arp::request_sent = false;
      thread_end = true;
    }
  }

  if(!thread_end) {
    tt2.SetHandler(10);
  } else {
    fprintf(stderr, "[ARP] test end\n");
    test_end = true;
  }
}

void ARPRequest(uint32_t ip_request, uint32_t ip_reply) {
  if(arp::socket.Open() < 0) {
    fprintf(stderr, "[open] cannot open socket\n");
  }

  // send ARP request
  arp::socket.SetIPAddr(ip_request);
  ip1 = ip_reply;
  ip2 = ip_request;

  new(&tt2) Callout;
  tt2.Init(ARPRequestSub, nullptr);
  tt2.SetHandler(10);
}

namespace tcp1 {
  Socket socket;
  const uint32_t size = 0x100;
  uint8_t data[size];
  int32_t rval;
  bool received = false;
  bool preparing = false;
  bool sent = false;
  bool closing = false;
};

void TCPServerSub1(void *p) {
  int32_t listen_rval = tcp1::socket.Listen();
  if(listen_rval >= 0) {
    fprintf(stderr, "[TCP:server] connection established\n");
  } else if(listen_rval == Socket::kResultAlreadyEstablished) {
    if(!tcp1::received) {
      if((tcp1::rval = tcp1::socket.ReceivePacket(tcp1::data, tcp1::size)) >= 0) {
        StripNewline(tcp1::data);
        fprintf(stderr, "[TCP:server] received; %s\n", tcp1::data);
        tcp1::received = true;
      } else if(tcp1::rval == Socket::kResultConnectionClosed) {
        fprintf(stderr, "[TCP:server] closed\n");
        thread_end = true;
      }
    } else {
      tcp1::rval = tcp1::socket.TransmitPacket(tcp1::data, strlen(reinterpret_cast<char*>(tcp1::data)) + 1);
      if(tcp1::rval >= 0) {
        tcp1::received = false;
        fprintf(stderr, "[TCP:server] loopback\n");
      }
    }
  }

  if(!thread_end) {
    tt1.SetHandler(10);
  } else {
    fprintf(stderr, "[TCP:server] test end\n");
    test_end = true;
  }
}

void TCPServer1() {
  if(tcp1::socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  tcp1::socket.SetListenAddr(0x0a000206);
  tcp1::socket.SetListenPort(Socket::kPortTelnet);
  tcp1::socket.SetIPAddr(0x0a000205);
  tcp1::socket.SetPort(Socket::kPortTelnet);

  // TCP server 
  new(&tt1) Callout();
  tt1.Init(TCPServerSub1, nullptr);
  tt1.SetHandler(10);
}

void TCPClientSub1(void *p) {
  int32_t connect_rval = tcp1::socket.Connect();

  if(connect_rval >= 0) {
    fprintf(stderr, "[TCP:client] connection established\n");
    tcp1::preparing = true;
  } else if(connect_rval == Socket::kResultAlreadyEstablished) {
    if(tcp1::closing) {
      if(tcp1::socket.Close() >= 0) {
        fprintf(stderr, "[TCP:client] closed\n");
        thread_end = true;
      }
    } else if(tcp1::preparing) {
      fprintf(stderr, ">> ");
      if(fgets(reinterpret_cast<char*>(tcp1::data), tcp1::size-1, stdin)) {
        tcp1::data[tcp1::size-1] = 0;
      }

      if(!strncmp(reinterpret_cast<char*>(tcp1::data), "q", 1)) {
        tcp1::closing = true;
        tt2.SetHandler(10);
        return;
      }

      tcp1::preparing = false;
    } else if(!tcp1::sent) {
      if(tcp1::socket.TransmitPacket(tcp1::data, strlen(reinterpret_cast<char*>(tcp1::data)) + 1) >= 0){
        StripNewline(tcp1::data);
        fprintf(stderr, "[TCP:client] sent; %s\n", tcp1::data);
        tcp1::sent = true;
      }
    } else {
      if(tcp1::socket.ReceivePacket(tcp1::data, tcp1::size) >= 0) {
        fprintf(stderr, "[TCP:client] received; %s\n", tcp1::data);
        tcp1::sent = false;
        tcp1::preparing = true;
      }
    }
  }

  if(!thread_end) {
    tt2.SetHandler(10);
  } else {
    fprintf(stderr, "[TCP:client] test end\n");
    test_end = true;
  }
}

void TCPClient1() {
  if(tcp1::socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open tcp1::socket\n");
  }
  tcp1::socket.SetListenAddr(0x0a000205);
  tcp1::socket.SetListenPort(Socket::kPortTelnet);
  tcp1::socket.SetIPAddr(0x0a000206);
  tcp1::socket.SetPort(Socket::kPortTelnet);

  // TCP client
  new(&tt2) Callout();
  tt2.Init(TCPClientSub1, nullptr);
  tt2.SetHandler(10);
}

namespace tcp2 {
  Socket socket;
  const uint32_t server_size = Socket::kMSS;
  uint8_t server_data[server_size];
  int32_t rval;

  const uint32_t client_size = 8000;
  uint8_t client_data[client_size];
  uint32_t s = client_size;  // length of packet last time sent
  uint8_t *p = client_data;  // current position in packet
  uint32_t sum = 0;
  bool established = false;
  bool closing = false;
};

void TCPServerSub2(void *p) {
  int32_t listen_rval = tcp2::socket.Listen();

  if(listen_rval >= 0) {
    fprintf(stderr, "[tcp:server] connection established\n");
  } else if(listen_rval == Socket::kResultAlreadyEstablished) {
    tcp2::rval = tcp2::socket.ReceivePacket(tcp2::server_data, tcp2::server_size);
    if(tcp2::rval >= 0) {
      fprintf(stderr, "return value = %d\n", tcp2::rval);
    } else if(tcp2::rval == Socket::kResultConnectionClosed) {
      thread_end = true;
    }
  }

  if(!thread_end) {
    tt1.SetHandler(10);
  } else {
    fprintf(stderr, "[tcp:server] test end\n");
  }
}

void TCPServer2() {
  if(tcp2::socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  tcp2::socket.SetListenAddr(0x0a000206);
  tcp2::socket.SetListenPort(Socket::kPortTelnet);
  tcp2::socket.SetIPAddr(0x0a000205);
  tcp2::socket.SetPort(Socket::kPortTelnet);

  new(&tt1) Callout();
  tt1.Init(TCPServerSub2, nullptr);
  tt1.SetHandler(10);
}

void TCPClientSub2(void *p) {
  int32_t connect_rval = tcp2::socket.Connect();

  if(connect_rval >= 0) {
    fprintf(stderr, "[tcp:client] connection established\n");
    tcp2::established = true;
  } else if(connect_rval == Socket::kResultAlreadyEstablished) {
    if(tcp2::established) {
      if(tcp2::closing) {
        if(tcp2::socket.Close() >= 0) {
          fprintf(stderr, "[TCP:client] closed\n");
          thread_end = true;
        }
      } else if(tcp2::sum < tcp2::client_size) {
        if((tcp2::rval = tcp2::socket.TransmitPacket(tcp2::p, tcp2::s)) >= 0) {
          fprintf(stderr, "return value = %d\n", tcp2::rval);

          if(tcp2::rval >= 0) {
            tcp2::p += tcp2::rval;
            tcp2::s -= tcp2::rval;
            tcp2::sum += tcp2::rval;
          }
        }
      } else {
        tcp2::closing = true;
      }
    }
  }

  if(!thread_end) {
    tt2.SetHandler(10);
  } else {
    fprintf(stderr, "[tcp:client] test end\n");
  }
}

void TCPClient2() {
  if(tcp2::socket.Open() < 0) {
    fprintf(stderr, "[error] cannot open socket\n");
  }
  tcp2::socket.SetListenAddr(0x0a000205);
  tcp2::socket.SetListenPort(Socket::kPortTelnet);
  tcp2::socket.SetIPAddr(0x0a000206);
  tcp2::socket.SetPort(Socket::kPortTelnet);

  memset(tcp2::client_data, 0x41, tcp2::client_size-1);
  tcp2::client_data[tcp2::client_size-1] = 0;

  new(&tt2) Callout();
  tt2.Init(TCPClientSub2, nullptr);
  tt2.SetHandler(10);
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
      fprintf(stderr, "return value = %d\n", rval);
    } else if(rval == Socket::kResultConnectionClosed) {
      break;
    }
  }
}

void TCPClient3() {
  TCPClient2();
}
