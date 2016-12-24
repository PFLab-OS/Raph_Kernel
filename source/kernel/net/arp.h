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

#ifndef __RAPH_KERNEL_NET_ARP_H__
#define __RAPH_KERNEL_NET_ARP_H__

#include <net/socket.h>


class ArpSocket : public Socket {
public:
  ArpSocket() {}

  /**
   * Struct serves as an interface between ArpSocket and ArpLayer.
   * ArpSocket receives / transmits Chunk from / to ArpLayer
   * because ARP packets do not have body part.
   * We shuold use this interface to encode / decode.
   */
  struct Chunk {
    uint16_t operation;
    uint32_t ipv4_addr;
  };

  /** constant value for ARP request operation */
  static const uint16_t kOpRequest = 0x0001;

  /** constant value for ARP reply operation */
  static const uint16_t kOpReply   = 0x0002;

  /**
   * Reserve the connection on protocol stack and construct the stack.
   *
   * @return 0 if succeeeds.
   */
  virtual int Open();

  /**
   * Send an ARP request.
   *
   * @param dest_addr destination IPv4 address.
   * @return 0 if succeeeds.
   */
  int Request(uint32_t dest_addr) {
    NetDev::Packet *packet = nullptr;
    this->FetchTxBuffer(packet);

    ArpSocket::Chunk *chunk = reinterpret_cast<ArpSocket::Chunk *>(packet->buf);
    chunk->operation = kOpRequest;
    chunk->ipv4_addr = dest_addr;

    packet->len = sizeof(ArpSocket::Chunk);

    return this->TransmitPacket(packet) ? 0 : -1;
  }

  /**
   * Send an ARP reply.
   *
   * @param dest_addr destination IPv4 address.
   * @return 0 if succeeeds.
   */
  int Reply(uint32_t dest_addr) {
    NetDev::Packet *packet = nullptr;
    this->FetchTxBuffer(packet);

    ArpSocket::Chunk *chunk = reinterpret_cast<ArpSocket::Chunk *>(packet->buf);
    chunk->operation = kOpReply;
    chunk->ipv4_addr = dest_addr;

    packet->len = 0;

    return this->TransmitPacket(packet) ? 0 : -1;
  }

  /**
   * Try to receive ARP packet.
   *
   * @param op set ARP operation of the received packet.
   * @param addr set source IPv4 address of the received packet.
   * @return 0 if a packet arrived, otherwise -1.
   */
  int Read(uint16_t &op, uint32_t &addr) {
    NetDev::Packet *packet = nullptr;
    if (this->ReceivePacket(packet)) {
      ArpSocket::Chunk *chunk = reinterpret_cast<ArpSocket::Chunk *>(packet->buf);
      op = chunk->operation;
      addr = chunk->ipv4_addr;

      this->ReuseRxBuffer(packet);

      return 0;
    } else {
      return -1;
    }
  }

  /**
   * Update information of the interface, e.g., IP address.
   */
  virtual void Update() {
    kassert(GetIpv4Address(_ipv4_addr));
  }

private:
  /** IPv4 address */
  uint32_t _ipv4_addr = 0;
};


class ArpLayer : public ProtocolStackLayer {
public:
  ArpLayer() {}

  /**
   * ARP header
   */
  struct Header {
    uint16_t  htype;      /** hardware type (fixed to Ethernet) */
    uint16_t  ptcl;       /** protocol type (fixed to IPv4) */
    uint8_t   hlen;       /** hardware address length (fixed to 6) */
    uint8_t   plen;       /** protocol address length (fixed to 4) */
    uint16_t  op;         /** ARP operation */
    uint8_t   hsaddr[6];  /** hardware source address */
    uint32_t  psaddr;     /** protocol source address */
    uint8_t   hdaddr[6];  /** hardware destination address */
    uint32_t  pdaddr;     /** protocol destination address */
  } __attribute__((packed));

  /**
   * Set Ethernet / IPv4 address.
   */
  void SetAddress(uint8_t eth_addr[6], uint32_t ipv4_addr) {
    memcpy(_eth_addr, eth_addr, 6);
    _ipv4_addr = ipv4_addr;
  }

  /**
   * Get hardware destination address from the packet.
   *
   * @param packet
   */
  static void GetHardwareDestinationAddress(NetDev::Packet *packet, uint8_t *addr) {
    ArpLayer::Header *header = reinterpret_cast<ArpLayer::Header *>(packet->buf);
    memcpy(addr, header->hdaddr, 6);
  }

protected:
  /**
   * Return ARP header size.
   * 
   * @return protocol header length.
   */
  virtual int GetProtocolHeaderLength() {
    return sizeof(ArpLayer::Header);
  }

  /**
   * Filter the received packet by its header content.
   * In addition, copy ARP information to body part of the packet.
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
  /** Ethernet address of the network device under this layer */
  uint8_t _eth_addr[6];

  /** IPv4 address assigned to the network device under this layer */
  uint32_t _ipv4_addr;
};


/**
 * ARP table, which holds the pairs of MAC addresses and IPv4 addresses.
 */
class ArpTable {
public:
  ArpTable() {}

  void Setup();

  /**
   * Add the pair of IPv4 address and MAC address.
   *
   * @param ipaddr
   * @param macaddr
   */
  bool Add(uint32_t ipaddr, uint8_t *macaddr);

  /**
   * Find MAC address given an IPv4 address.
   *
   * @param ipaddr
   * @param macaddr
   */
  bool Find(uint32_t ipaddr, uint8_t *macaddr);

  /**
   * Check if the given IPv4 address is registered.
   *
   * @param ipaddr
   * @return if exists.
   */
  bool Exists(uint32_t ipaddr);

private:
  struct ArpTableRecord {
    uint32_t ipaddr;
    uint8_t macaddr[6];
  };

  SpinLock _lock;

  static const int kMaxNumberRecords = 256;

  /** maximum number of probing */
  static const int kMaxProbingNumber = 8;

  /** this table is implemented by open adressing hash table */
  ArpTableRecord _table[kMaxNumberRecords];

  /**
   * Hash function.
   *
   * @param s input key value.
   * @return hash value.
   */
  uint32_t Hash(uint32_t s) { return s & 0xff; }

  /**
   * Probing function.
   * (search for next possible index in the case of conflict)
   *
   * @param s input key value.
   * @return index available.
   */
  uint32_t Probe(uint32_t s) { return (s + 1) & 0xff; }
};


#endif // __RAPH_KERNEL_NET_ARP_H__
