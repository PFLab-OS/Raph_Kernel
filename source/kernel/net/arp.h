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

#ifndef __RAPH_KERNEL_NET_ARP_H__
#define __RAPH_KERNEL_NET_ARP_H__

#include <stdint.h>

struct ArpPacket {
  // hardware type
  uint16_t hwtype;
  // protocol type
  uint16_t protocol;
  // hardware length
  uint8_t hlen;
  // protocol length
  uint8_t plen;
  // operation
  uint16_t op;
  // source hardware address
  uint8_t hw_saddr[6];
  // source protocol address
  uint32_t proto_saddr;
  // destination hardware address
  uint8_t hw_daddr[6];
  // destination protocol address
  uint32_t proto_daddr;
} __attribute__((packed));

class ArpCtrl {
  static const uint8_t kBcastMACAddr[6];

public:
  ArpCtrl() {}
  virtual int32_t GeneratePacket(uint8_t *buffer, uint16_t op, uint8_t *smacaddr, uint32_t sipaddr, uint8_t *dmacaddr, uint32_t dipaddr);
  virtual bool FilterPacket(uint8_t *packet, uint16_t op, uint8_t *smacaddr, uint32_t sipaddr, uint8_t *dmacaddr, uint32_t dipaddr);
  bool RegisterAddress(uint8_t *packet);
  void GetSourceMACAddress(uint8_t *buffer, uint8_t *packet);
  uint32_t GetSourceIPAddress(uint8_t *packet);

  static const uint16_t kHWEthernet = 0x0001;
  static const uint16_t kProtocolIPv4 = 0x0800;
};

class ArpTable {
  struct ArpTableRecord {
    uint32_t ipaddr;
    uint8_t macaddr[6];
  };

  static const uint32_t kMaxNumberRecords = 256;
  ArpTableRecord _table[kMaxNumberRecords];
  uint32_t Hash(uint32_t s);
  uint32_t Probe(uint32_t s);

public:
  ArpTable();
  bool Add(uint32_t ipaddr, uint8_t *macaddr);
  bool Find(uint32_t ipaddr, uint8_t *macaddr);
};

#endif // __RAPH_KERNEL_NET_ARP_H__
