/*
 *
 * Copyright (c) 2016 Project Raphine
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

#ifndef __RAPH_KERNEL_NET_SOCKET_H__
#define __RAPH_KERNEL_NET_SOCKET_H__

#include <stdint.h>
#include "../dev/netdev.h"

class NetSocket {
protected:
  NetDev *_dev = nullptr;

public:
  int32_t Open();
};

// TCP/IP Socket
class Socket : public NetSocket {
  uint32_t _daddr = 0x0a00020f;
  uint16_t _dport = 80;

  int32_t GetEthAddr(uint32_t ipaddr, uint8_t *macaddr);

protected:
  // L[2/3/4][T/R]xをオーバーライドすることで特定のレイヤの処理を書き換えられる
  virtual uint32_t L2HeaderLength();
  virtual uint32_t L3HeaderLength();
  virtual uint32_t L4HeaderLength();
  virtual uint16_t L4Protocol();
  virtual int32_t L2Tx(uint8_t *buffer,
                       uint8_t *saddr,
                       uint8_t *daddr,
                       uint16_t type);
  virtual bool L2Rx(uint8_t *packet,
                    uint8_t *saddr,
                    uint8_t *daddr,
                    uint16_t type);
  virtual int32_t L3Tx(uint8_t *buffer,
                       uint32_t length,
                       uint8_t type,
                       uint32_t saddr,
                       uint32_t daddr);
  virtual bool L3Rx(uint8_t *packet,
                    uint8_t type,
                    uint32_t saddr,
                    uint32_t daddr);
  virtual int32_t L4Tx(uint8_t *header,
                       uint32_t length,
                       uint16_t sport,
                       uint16_t dport);
  virtual bool L4Rx(uint8_t *packet,
                    uint16_t sport,
                    uint16_t dport);

public:
  Socket() {}
  void SetAddr(uint32_t addr) { _daddr = addr; }
  void SetPort(uint16_t port) { _dport = port; }
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length);
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length);
};

// UDP Socket
class UDPSocket : public Socket {
protected:
  virtual uint32_t L4HeaderLength() override;
  virtual uint16_t L4Protocol() override;
  virtual int32_t L4Tx(uint8_t *header,
                       uint32_t length,
                       uint16_t sport,
                       uint16_t dport) override;
  virtual bool L4Rx(uint8_t *packet,
                    uint16_t sport,
                    uint16_t dport) override;

public:
  UDPSocket() {}
};

// ARP Socket
class ARPSocket : public NetSocket {
public:
  virtual int32_t TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha = nullptr);
  virtual int32_t ReceivePacket(uint16_t type, uint32_t *spa = nullptr, uint8_t *sha = nullptr);

  static const uint16_t kOpARPRequest = 0x0001;
  static const uint16_t kOpARPReply = 0x0002;
};

#endif // __RAPH_KERNEL_NET_SOCKET_H__
