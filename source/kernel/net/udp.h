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

#ifndef __RAPH_KERNEL_NET_UDP_H__
#define __RAPH_KERNEL_NET_UDP_H__


#include <net/pstack.h>
#include <net/socket.h>


class UdpSocket : public Socket {
public:

  /** do not specify peer port */
  static const uint16_t kPortAny = 0xffff;

  UdpSocket() {}

  /**
   * Reserve the connection on protocol stack and construct the stack.
   *
   * @return 0 if succeeds.
   */
  virtual int Open() override;

  /**
   * Bind address info to this socket.
   *
   * @param ipv4_addr
   * @param port
   */
  void BindAddress(uint32_t ipv4_addr, uint16_t port) {
    _ipv4_addr = ipv4_addr;
    _port = port;
  }

  /**
   * Bind peer address info to this socket.
   *
   * @param ipv4_addr
   * @param port
   */
  void BindPeer(uint32_t ipv4_addr, uint16_t port) {
    _peer_addr = ipv4_addr;
    _peer_port = port;
  }

  int Read(uint8_t *buf, int len) {
    NetDev::Packet *packet = nullptr;
    if (this->ReceivePacket(packet)) {
      int len0 = packet->len;
      int ret = len0 <= len ? len0 : len;

      memcpy(buf, packet->buf, ret);

      this->ReuseRxBuffer(packet);

      return ret;
    } else {
      return -1;
    }
  }

  int Write(uint8_t *buf, int len) {
    NetDev::Packet *packet = nullptr;
    if (this->FetchTxBuffer(packet) < 0) {
      return -1;
    } else {
      memcpy(packet->buf, buf, len);
      packet->len = len;

      return this->TransmitPacket(packet) ? len : -1;
    }
  }

private:
  /** my IPv4 address */
  uint32_t _ipv4_addr;

  /** my port */
  uint16_t _port;

  /** target IPv4 address */
  uint32_t _peer_addr;

  /** target port */
  uint16_t _peer_port;
};


class UdpLayer : public ProtocolStackLayer {
public:

  UdpLayer() {}

  /**
   * UDP header
   */
  struct Header {
    uint16_t sport;     /** source port */
    uint16_t dport;     /** destination port */
    uint16_t len;       /** length of header + payload */
    uint16_t checksum;  /** checksum */
  } __attribute__((packed));

  /**
   * Set my UDP port.
   *
   * @param port 
   */
  void SetPort(uint16_t port) {
    _port = port;
  }

  /**
   * Set peer UDP port.
   *
   * @param port 
   */
  void SetPeerPort(uint16_t port) {
    _peer_port = port;
  }

protected:
  /**
   * Return UDP header size.
   * 
   * @return protocol header length.
   */
  virtual int GetProtocolHeaderLength() {
    return sizeof(UdpLayer::Header);
  }

  /**
   * Filter the received packet by its header content.
   *
   * @param packet
   * @return if the packet is to be received or not.
   */
  virtual bool FilterPacket(NetDev::Packet *packet);

  /**
   * Make contents of the header before transmitting the packet.
   *
   * @param packet
   * @return if succeeds.
   */
  virtual bool PreparePacket(NetDev::Packet *packet);

private:
  /** port number of this connection */
  uint16_t _port;

  /** port number of the peer */
  uint16_t _peer_port;
};


#endif // __RAPH_KERNEL_NET_UDP_H__
