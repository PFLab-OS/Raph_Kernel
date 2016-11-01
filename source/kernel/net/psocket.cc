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
 * Author: Levelfour
 * 
 */

#ifndef __KERNEL__

#include <net/psocket.h>
#include <mem/uvirtmem.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int32_t PoolingSocket::Open() {
  if ((_tcp_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("");
    return _tcp_socket;
  } else {
    int yes = 1;
    setsockopt(_tcp_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  }

  if ((_udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("");
    return _udp_socket;
  } else {
    int yes = 1;
    setsockopt(_udp_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  }
  
  // initialize
  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    _tcp_client[i] = -1; // fd not in use
  }
  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    _udp_client[i].enabled = false;
  }

  // turn on non-blocking mode
  int flag = fcntl(_tcp_socket, F_GETFL);
  fcntl(_tcp_socket, F_SETFL, flag | O_NONBLOCK);

  flag = fcntl(_udp_socket, F_GETFL);
  fcntl(_udp_socket, F_SETFL, flag | O_NONBLOCK);

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(_port);
  bind(_tcp_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  bind(_udp_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));

  FD_ZERO(&_fds);

  _timeout.tv_sec = 0;
  _timeout.tv_usec = 0;

  InitPacketBuffer();
  SetupPollingHandler();

  return 0;
}

int32_t PoolingSocket::Close() {
  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    if (_tcp_client[i] != -1) {
      close(_tcp_client[i]);
    }
  }

  close(_tcp_socket); _tcp_socket = -1;
  close(_udp_socket); _udp_socket = -1;

  return 0;
}

int32_t PoolingSocket::RegisterUdpAddress(uint32_t ipaddr, uint16_t port) {
  int32_t index = GetAvailableUdpClientIndex();

  if (index == -1) {
    return -1;
  } else {
    _udp_client[index].addr.sin_family = AF_INET;
    _udp_client[index].addr.sin_port = htons(port);
    _udp_client[index].addr.sin_addr.s_addr = ipaddr;
    _udp_client[index].time_to_live = kDefaultTtlValue;
    _udp_client[index].enabled = true;
    return index + kUdpAddressOffset;
  }
}

void PoolingSocket::InitPacketBuffer() {
  while (!_tx_reserved.IsFull()) {
    Packet *packet = reinterpret_cast<Packet *>(virtmem_ctrl->Alloc(sizeof(Packet)));
    packet->adr = -1; // negative value is invalid address
    packet->len = 0;
    kassert(_tx_reserved.Push(packet));
  }

  while (!_rx_reserved.IsFull()) {
    Packet *packet = reinterpret_cast<Packet *>(virtmem_ctrl->Alloc(sizeof(Packet)));
    packet->adr = -1; // negative value is invalid address
    packet->len = 0;
    kassert(_rx_reserved.Push(packet));
  }
}

void PoolingSocket::Poll(void *arg) {
  // TCP listen
  int32_t tcp_capacity = Capacity();
  if (tcp_capacity > 0) {
    if (listen(_tcp_socket, tcp_capacity) >= 0) {

      int32_t index = GetAvailableTcpClientIndex();

      // at this point, _tcp_client[index] is not used

      if ((_tcp_client[index] = accept(_tcp_socket, nullptr, nullptr)) > 0) {
        FD_SET(_tcp_client[index], &_fds);
      }
    }
  }

  {
    // TCP receive sequence
    Packet *packet;

    fd_set tmp_fds;
    memcpy(&tmp_fds, &_fds, sizeof(_fds));

    // receive packet (non-blocking)
    if (select(GetNfds(), &tmp_fds, 0, 0, &_timeout) >= 0) {
      for (int32_t i = 0; i < kMaxClientNumber; i++) {
        if (_tcp_client[i] == -1) {
          // this socket is not used now
          continue;
        }

        if (FD_ISSET(_tcp_client[i], &tmp_fds)) {
          if (GetRxPacket(packet)) {
            int32_t rval = read(_tcp_client[i], packet->buf, kMaxPacketLength);
            if (rval > 0) {
              packet->adr = i;
              packet->len = rval;
              _rx_buffered.Push(packet);
            } else {
              if (rval == 0) {
                // socket may be closed by foreign host
                close(_tcp_client[i]);
                FD_CLR(_tcp_client[i], &_fds);
                _tcp_client[i] = -1;
              }

              ReuseRxBuffer(packet);
            }
          }
        }
      }
    }
  }

  {
    // UDP receive sequence
    Packet *packet;
    int32_t index = GetAvailableUdpClientIndex();

    if (index != -1 && GetRxPacket(packet)) {
      socklen_t sin_size = sizeof(_udp_client[index].addr);
      int32_t rval = recvfrom(_udp_socket, packet->buf, kMaxPacketLength, 0, reinterpret_cast<struct sockaddr *>(&_udp_client[index].addr), &sin_size);
      if (rval > 0) {
        packet->adr = kUdpAddressOffset + index;
        packet->len = rval;

        // register UDP address info
        _udp_client[index].time_to_live = kDefaultTtlValue;
        _udp_client[index].enabled = true;

        RefreshTtl();

        _rx_buffered.Push(packet);
      } else {
        ReuseRxBuffer(packet);

        if(errno != EAGAIN) {
          perror("udp");
        }
      }
    }
  }

  {
    // transmit sequence
    Packet *packet;

    // transmit packet (if non-sent packet remains in buffer)
    if (_tx_buffered.Pop(packet)) {

      if (IsValidUdpClientIndex(packet->adr)) {
        // the address number is valid UDP address
        int32_t index = packet->adr - kUdpAddressOffset;
        sendto(_udp_socket, packet->buf, packet->len, 0, reinterpret_cast<struct sockaddr *>(&_udp_client[index].addr), sizeof(_udp_client[index].addr));
        ReuseTxBuffer(packet);
      } else if (IsValidTcpClientIndex(packet->adr)) {
        // the address number is valid TCP address
        int32_t rval, residue = packet->len;
        uint8_t *ptr = packet->buf;

        while (residue > 0) {
          // send until EOF (rval == 0)
          rval = write(_tcp_client[packet->adr], ptr, residue);
          residue -= rval;
          ptr += rval;
        }

        ReuseTxBuffer(packet);
      }
    }
  }
}

void PoolingSocket::SetupPollingHandler() {
  ClassFunction<PoolingSocket> polling_func;
  polling_func.Init(this, &PoolingSocket::Poll, nullptr);
  _polling.Init(polling_func);
  _polling.Register(0);
}

int32_t PoolingSocket::Capacity() {
  // count the number of unused fd
  int32_t capacity = 0;

  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    if (_tcp_client[i] == -1) {
      capacity++;
    }
  }

  return capacity;
}

int32_t PoolingSocket::GetAvailableTcpClientIndex() {
  // return index of the first unused TCP client
  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    if (_tcp_client[i] == -1) {
      return i;
    }
  }

  return -1;
}

int32_t PoolingSocket::GetAvailableUdpClientIndex() {
  // return index of the first unused UDP client
  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    if (!_udp_client[i].enabled) {
      return i;
    }
  }

  return -1;
}

int32_t PoolingSocket::GetNfds() {
  // this function is used for 1st-arg of select syscall
  int32_t max_fd = -1;

  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    if (_tcp_client[i] > max_fd) {
      max_fd = _tcp_client[i];
    }
  }

  return max_fd + 1;
}

bool PoolingSocket::IsValidTcpClientIndex(int32_t index) {
  if (0 <= index && index < kMaxClientNumber && _tcp_client[index] != -1) {
    return true;
  } else {
    return false;
  }
}

bool PoolingSocket::IsValidUdpClientIndex(int32_t index) {
  int32_t uadr = index - kUdpAddressOffset;
  if (0 <= uadr && uadr < kMaxClientNumber && _udp_client[uadr].enabled) {
    return true;
  } else {
    return false;
  }
}

void PoolingSocket::RefreshTtl() {
  // decrease ttl value, and if it reaches to 0, clear the address info
  for (int32_t i = 0; i < kMaxClientNumber; i++) {
    if (_udp_client[i].enabled) {
      if (--_udp_client[i].time_to_live <= 0) {
        _udp_client[i].enabled = false;
      }
    }
  }
}

#endif // !__KERNEL__
