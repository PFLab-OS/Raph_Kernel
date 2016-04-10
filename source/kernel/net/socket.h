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
#include "../dev/eth.h"

class NetSocket {
protected:
  DevEthernet *_dev = nullptr;

public:
  int32_t Open();
};

// TCP/IP Socket
class Socket : public NetSocket {
public:
  // frequently used port number
  static const uint16_t kPortTelnet = 23;
  static const uint16_t kPortHTTP = 80;

  // TCP flag
  static const uint8_t kFlagFIN           = 1 << 0;
  static const uint8_t kFlagSYN           = 1 << 1;
  static const uint8_t kFlagRST           = 1 << 2;
  static const uint8_t kFlagPSH           = 1 << 3;
  static const uint8_t kFlagACK           = 1 << 4;
  static const uint8_t kFlagURG           = 1 << 5;

  // maximum segment size
  static const uint32_t kMSS = 1460;

  // return code of ReceivePacket
  static const int32_t kConnectionClosed  = - 0x100;

  Socket() {}
  void SetAddr(uint32_t addr) { _daddr = addr; }
  void SetPort(uint16_t port) { _dport = port; }
  void SetListenAddr(uint32_t addr) { _ipaddr = addr; }
  void SetListenPort(uint16_t port) { _sport = port; }

  // transmit TCP data (header will be attached)
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length);

  // transmit raw packet (Ethernet/IPv4/TCP header will not be attached)
  virtual int32_t TransmitRawPacket(const uint8_t *data, uint32_t length);

  // receive TCP data (header will be detached)
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length);

  // receive raw packet (Ethernet/IPv4/TCP header will not be detached)
  virtual int32_t ReceiveRawPacket(uint8_t *data, uint32_t length);

  // server: wait for client connection (3-way handshake)
  int32_t Listen();

  // client: connect to server (3-way handshake)
  int32_t Connect();

  // close TCP connection (4-way handshake)
  int32_t Close();

protected:
  // L[2/3/4][T/R]xをオーバーライドすることで特定のレイヤの処理を書き換えられる
  // Tx ... 送信時に該当レイヤのヘッダを付与する
  // Rx ... 受信時に該当レイヤのヘッダを参照してフィルタリングを行う
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
  int32_t Receive(uint8_t *data, uint32_t length, bool isRawPacket);
  int32_t Transmit(const uint8_t *data, uint32_t length, bool isRawPacket);

  // respond to FIN+ACK (4-way handshake)
  int32_t CloseAck(uint8_t flag);

private:
  // my IP address
  uint32_t _ipaddr = 0x0a000210;
  // destination IP address
  uint32_t _daddr = 0x0a000210;
  // destination port
  uint16_t _dport = kPortHTTP;
  // source port
  uint16_t _sport = kPortHTTP;
  // TCP session type
  // set before both tx/rx by Socket::SetSessionType()
  uint8_t _type   = kFlagRST;
  // TCP sequence number
  uint32_t _seq   = 0;
  // TCP acknowledge number
  uint32_t _ack   = 0;
  // flag for whether connection is established
  bool _established = false;

  int32_t GetEthAddr(uint32_t ipaddr, uint8_t *macaddr);
  void SetSessionType(uint8_t type) { _type = type; }
  void SetSequenceNumber(uint32_t seq) { _seq = seq; }
  void SetAcknowledgeNumber(uint32_t ack) { _ack = ack; }
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
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length) override;
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length) override;

public:
  UDPSocket() {}
};

// ARP Socket
class ARPSocket : public NetSocket {
public:
  static const int16_t kOpARPRequest = 0x0001;
  static const int16_t kOpARPReply = 0x0002;

  // transmit ARP packet
  //   @type: request / reply
  //   @tpa: target IP address
  //   @tha: target MAC address
  //    (if tha = nullptr, does not care @tha (usually used in request packet))
  // return value is
  //   * ARP operation (if succeed)
  //   * -1            (otherwise)
  virtual int32_t TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha);

  // receive ARP packet
  //   @type: request / reply
  //   @spa: source IP address
  //   @sha: source MAC address
  //
  // type:0 spa:nullptr sha:nullptr => does not filter by these parameters
  //
  // return value is
  //   * ARP operation (if succeed)
  //   * -1            (otherwise)
  virtual int32_t ReceivePacket(uint16_t type, uint32_t *spa, uint8_t *sha);

  virtual void SetIPAddr(uint32_t ipaddr);

private:
  static const uint32_t kOperationOffset = 6;

  uint32_t _ipaddr = 0x0a000210;
};

#endif // __RAPH_KERNEL_NET_SOCKET_H__
