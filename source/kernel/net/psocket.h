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

#ifndef __RAPH_LIB_NET_PSOCKET_H__
#define __RAPH_LIB_NET_PSOCKET_H__

#ifndef __KERNEL__

#include <stdio.h>
#include <stdint.h>
#include <buf.h>
#include <polling.h>
#include <functional.h>
#include <net/socket_interface.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

class PoolingSocket : public SocketInterface {
public:
  static const uint32_t kMaxPacketLength = 1460;
  struct Packet {
    int32_t adr; // recipient number
    int32_t len; // packet lenght
    uint8_t buf[kMaxPacketLength]; // packet content body
  };

  PoolingSocket(int port) : _port(port) {}
  virtual int32_t Open() override;
  virtual int32_t Close() override;
  virtual void SetReceiveCallback(int cpuid, const Function &func) override {
    _rx_buffered.SetFunction(cpuid, func);
  }

  // pass 32bit-converted IP address to 1st-arg (use inet_addr)
  // return value is client number, but
  // if there is no sufficient capacity, this function returns -1
  int32_t RegisterUdpAddress(uint32_t ipaddr, uint16_t port);

  void ReuseRxBuffer(Packet *packet) {
    kassert(_rx_reserved.Push(packet));
  }
  void ReuseTxBuffer(Packet *packet) {
    kassert(_tx_reserved.Push(packet));
  }
  bool GetTxPacket(Packet *&packet) {
    if (_tx_reserved.Pop(packet)) {
      packet->len = 0;
      return true;
    } else {
      return false;
    }
  }
  bool GetRxPacket(Packet *&packet) {
    return _rx_reserved.Pop(packet);
  }
  bool TransmitPacket(Packet *packet) {
    if(IsValidClientIndex(packet->adr)) {
      return _tx_buffered.Push(packet);
    } else {
      // Invalid client number. Maybe
      //   - your specified client number is non-sense
      //   - UDP address info has been cleared (please re-register)
      return false;
    }
  }
  bool ReceivePacket(Packet *&packet) {
    return _rx_buffered.Pop(packet);
  }

private:
  static const uint32_t kPoolDepth = 300;
  typedef RingBuffer<Packet *, kPoolDepth> PacketPoolRingBuffer;
  typedef FunctionalRingBuffer<Packet *, kPoolDepth> PacketPoolFunctionalRingBuffer;

  void InitPacketBuffer();
  void SetupPollingHandler();
  void Poll(void *arg);

  PacketPoolRingBuffer _tx_buffered;
  PacketPoolRingBuffer _tx_reserved;

  PacketPoolFunctionalRingBuffer _rx_buffered;
  PacketPoolFunctionalRingBuffer _rx_reserved;

  PollingFunc _polling;

  // listening port
  int _port;
  // TCP socket
  int _tcp_socket;
  // UDP socket
  int _udp_socket;
  // TCP socket descriptor of the accepted client
  static const int32_t kMaxClientNumber = 256;
  int _tcp_client[kMaxClientNumber];
  // UDP address information
  static const int32_t kUdpAddressOffset = 0x4000;
  static const int32_t kDefaultTtlValue = kMaxClientNumber;
  struct address_info {
    struct sockaddr_in addr;
    bool enabled;
    int32_t time_to_live;
  } _udp_client[kMaxClientNumber];

  fd_set _fds;
  struct timeval _timeout;

  int32_t Capacity();
  int32_t GetAvailableTcpClientIndex();
  int32_t GetAvailableUdpClientIndex();
  int32_t GetNfds();
  bool IsValidTcpClientIndex(int32_t index);
  bool IsValidUdpClientIndex(int32_t index);
  bool IsValidClientIndex(int32_t index) {
    return IsValidTcpClientIndex(index) || IsValidUdpClientIndex(index);
  }
  void RefreshTtl();
};

#endif // !__KERNEL__

#endif // __RAPH_LIB_NET_PSOCKET_H__
