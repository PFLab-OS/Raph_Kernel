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
#include <dev/eth.h>

class NetSocket {
public:
  /*
   * return code of TransmitPacket / ReceivePacket
   */

  // connection closed by remote host
  static const int32_t kResultConnectionClosed      = - 0x100;
  // connection is already established before Listen / Connect
  static const int32_t kResultAlreadyEstablished    = - 0x101;
  // unknown error
  static const int32_t kErrorUnknown                = - 0x1000;
  // remote host does not reply for retransmission timeout
  static const int32_t kErrorRetransmissionTimeout  = - 0x1001;
  // no new packet reached on wire
  static const int32_t kErrorNoPacketOnWire         = - 0x1002;
  // the packet reached on wire is invalid (destination is wrong, etc.)
  static const int32_t kErrorInvalidPacketOnWire    = - 0x1003;
  // invalid parameter is set in the received packet
  static const int32_t kErrorInvalidPacketParameter = - 0x1004;
  // cannot fetch packet buffer from transmit queue
  static const int32_t kErrorInsufficientBuffer     = - 0x1005;
  // device internal error
  static const int32_t kErrorDeviceInternal         = - 0x1007;

  // open socket, which internally fetch an available network device
  int32_t Open();

  void SetProtocolStackId(uint32_t id) { _ptcl_stack_id = id; }
  uint32_t GetProtocolStackId() { return _ptcl_stack_id; }

protected:
  // reference to network device info
  NetDevCtrl::NetDevInfo *_device_info;

private:
  uint32_t _ptcl_stack_id;
};

// TCP/IP Socket
class Socket : public NetSocket {
public:
  // frequently used port number
  static const uint16_t kPortTelnet = 23;
  static const uint16_t kPortHTTP = 80;

  // TCP flag
  static const uint8_t kFlagFIN = 1 << 0;
  static const uint8_t kFlagSYN = 1 << 1;
  static const uint8_t kFlagRST = 1 << 2;
  static const uint8_t kFlagPSH = 1 << 3;
  static const uint8_t kFlagACK = 1 << 4;
  static const uint8_t kFlagURG = 1 << 5;

  // maximum segment size
  static const uint32_t kMSS = 1460;

  // TCP states (extended)
  // (cf) RFC 793 p.26
  static const int32_t kStateClosed      = 0;
  static const int32_t kStateListen      = 1;
  static const int32_t kStateSynSent     = 2;
  static const int32_t kStateSynReceived = 3;
  static const int32_t kStateEstablished = 4;
  static const int32_t kStateFinWait1    = 5;
  static const int32_t kStateFinWait2    = 6;
  static const int32_t kStateCloseWait   = 7;
  static const int32_t kStateClosing     = 8;
  static const int32_t kStateLastAck     = 9;
  static const int32_t kStateTimeWait    = 10;
  static const int32_t kStateAckWait     = 11;  // extended

  Socket() {}

  void SetIPAddr(uint32_t addr) { _daddr = addr; }
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
  // the process of each layer can be altered by overriding L[2/3/4][T/R]x
  // L[2/3/4]Tx: attach header of each layer while transmission
  // L[2/3/4]Rx: filter packet by referring header of each layer while reception
  virtual uint32_t L2HeaderLength();
  virtual uint32_t L3HeaderLength();
  virtual uint32_t L4HeaderLength();
  virtual uint16_t L4Protocol();
  virtual int32_t L2Tx(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type);
  virtual bool L2Rx(uint8_t *packet, uint8_t *saddr, uint8_t *daddr, uint16_t type);
  virtual int32_t L3Tx(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr);
  virtual bool L3Rx(uint8_t *packet, uint8_t type, uint32_t saddr, uint32_t daddr);
  virtual int32_t L4Tx(uint8_t *header, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport);
  virtual bool L4Rx(uint8_t *packet, uint16_t sport, uint16_t dport);

  // low-level packet receive function
  //   @param buffer          buffer to store received data
  //   @param length          length of buffer
  //   @param is_raw_packet   whether to receive raw packet
  //   @param wait_timeout    whether to wait for timeout
  //   @param rto             timeout count (valid on wait_timeout == true)
  int32_t Receive(uint8_t *buffer, uint32_t length, bool is_raw_packet, bool wait_timeout, uint64_t rto);
  // low-level packet transmit function
  //   @param data            data to be sent
  //   @param length          length of data
  //   @param is_raw_packet   whether data is raw packet
  int32_t Transmit(const uint8_t *data, uint32_t length, bool is_raw_packet);

  // respond to FIN+ACK (4-way handshake)
  int32_t CloseAck();

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
  // TCP state (cf. RFC 793 p.26)
  int32_t _state = kStateClosed;
  // temporary packet length buffer
  int32_t _packet_length = 0;

  // max segment size
  uint16_t _mss = kMSS;
  // window scale
  uint8_t _ws = 1;

  /*
   * TCP Restransmission Timeout Parameters
   *   RTT: Round Trip Time
   *   RTO: Retransmission TimeOut
   */
  // RTO upper bound [usec]
  static const uint64_t kRtoUBound = 5000000;
  // RTO lower bound [usec]
  static const uint64_t kRtoLBound = 1000000;
  // RTT [usec] (default is 3[sec])
  uint64_t _rtt_usec = 3000000;

  int32_t GetEthAddr(uint32_t ipaddr, uint8_t *macaddr);
  void SetSessionType(uint8_t type) { _type = type; }
  void SetSequenceNumber(uint32_t seq) { _seq = seq; }
  void SetAcknowledgeNumber(uint32_t ack) { _ack = ack; }

  uint64_t GetRetransmissionTimeout() {
    // simple algorithm of RTO calculation
    // recommended algorithm is
    //   * RFC793[3.7] Smoothing RTO (https://tools.ietf.org/html/rfc793)
    //   * Van Jacobson, Michael J. Karels, "Congestion Avoidance and Control", Proc. SIGCOMM'88., Appendix A
    return _rtt_usec < kRtoUBound ? (
        kRtoLBound < _rtt_usec ? _rtt_usec : kRtoLBound) :
      kRtoUBound;
  }
};

// UDP Socket
class UdpSocket : public Socket {
protected:
  virtual uint32_t L4HeaderLength() override;
  virtual uint16_t L4Protocol() override;
  virtual int32_t L4Tx(uint8_t *header, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport) override;
  virtual bool L4Rx(uint8_t *packet, uint16_t sport, uint16_t dport) override;
  virtual int32_t TransmitPacket(const uint8_t *data, uint32_t length) override;
  virtual int32_t ReceivePacket(uint8_t *data, uint32_t length) override;

public:
  UdpSocket() {}
};

// ARP Socket
class ArpSocket : public NetSocket {
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

  virtual void SetIPAddr(uint32_t ipaddr) { _ipaddr = ipaddr; }

private:
  static const uint32_t kOperationOffset = 6;

  uint32_t _ipaddr = 0x0a000210;
};

#endif // __RAPH_KERNEL_NET_SOCKET_H__
