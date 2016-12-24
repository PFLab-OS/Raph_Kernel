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

#ifndef __RAPH_KERNEL_NET_ETH_H__
#define __RAPH_KERNEL_NET_ETH_H__

#include <net/pstack.h>


/**
 * Physical layer in protocol stacks, which respond to Ethernet,
 * Infiniband and so on.
 * This class is interface class, so you must override virtual pure functions.
 */
class EthernetLayer final : public ProtocolStackPhysicalLayer {
public:

  /**
   * Ethernet header
   */
  struct Header {
    uint8_t  daddr[6];  /** destination MAC address */
    uint8_t  saddr[6];  /** source MAC address */
    uint16_t type;      /** protocol type */
  } __attribute__((packed));

  static const uint16_t kProtocolIpv4 = 0x0800;
  static const uint16_t kProtocolArp  = 0x0806;


  EthernetLayer() {}

  /**
   * Set MAC address.
   *
   * @param addr given MAC address.
   */
  virtual void SetAddress(uint8_t *addr) override {
    memcpy(_mac_address, addr, 6);
  }

  /**
   * Set protocol type of the upper layer. L3 protocol is expected.
   *
   * @param protocol
   */
  void SetUpperProtocolType(uint16_t protocol) {
    _protocol = protocol;
  }

  /**
   * Compare Ethernet address.
   *
   * @param addr0 target address.
   * @param addr1 expected address.
   * @return true if the given addresses are the same, otherwise false.
   */
  static bool CompareAddress(uint8_t *addr0, uint8_t *addr1) {
    if (memcmp(addr0, addr1, 6) != 0) {
      if (memcmp(addr0, EthernetLayer::kBroadcastMacAddress, 6) != 0 &&
          memcmp(addr0, EthernetLayer::kArpRequestMacAddress, 6) != 0) {
        return false;
      }
    }

    return true;
  }

  /**
   * Set broadcast MAC address.
   *
   * @param addr target address buffer.
   */
  static void SetBroadcastAddress(uint8_t *addr) {
    memcpy(addr, EthernetLayer::kBroadcastMacAddress, 6);
  }

protected:
  virtual int GetProtocolHeaderLength() override {
    return sizeof(Header);
  }

  virtual bool FilterPacket(NetDev::Packet *packet) override;

  virtual bool PreparePacket(NetDev::Packet *packet) override;

private:
  /** broadcast MAC address */
  static const uint8_t kBroadcastMacAddress[6];

  static const uint8_t kArpRequestMacAddress[6];

  /** physical address */
  uint8_t _mac_address[6];

  /** upper layer protocol (L3) */
  uint16_t _protocol;
};


#endif // __RAPH_KERNEL_NET_ETH_H__
