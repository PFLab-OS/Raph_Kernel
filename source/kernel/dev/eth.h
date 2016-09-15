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
 * Author: Liva
 * 
 */

#ifndef __RAPH_KERNEL_DEV_ETH_H__
#define __RAPH_KERNEL_DEV_ETH_H__

#include <dev/netdev.h>
#include <dev/pci.h>
#include <mem/virtmem.h>

/** broadcast MAC address */
static constexpr uint8_t kBroadcastMacAddress[6] = {0xff};

class DevEthernet : public NetDev {
public:
  /**
   * Ethernet header
   */
  struct EthernetHeader {
    uint8_t  daddr[6];  /** destination MAC address */
    uint8_t  saddr[6];  /** source MAC address */
    uint16_t type;      /** protocol type */
  } __attribute__((packed));

  DevEthernet(uint8_t bus, uint8_t device, bool mf) {
    //TODO is this needed?
    _pci = virtmem_ctrl->New<DevPci>(bus, device, mf);
  } 
  virtual ~DevEthernet() {
    virtmem_ctrl->Delete<DevPci>(_pci);
  }
  DevPci *GetDevPci() {
    return _pci;
  }
  // allocate 6 byte before call
  virtual void GetEthAddr(uint8_t *buffer) = 0;

protected:
  DevEthernet(DevPci *pci) {
    _pci = pci;
  }

  virtual void FilterRxPacket(void *p) override;

  virtual void PrepareTxPacket(NetDev::Packet *packet) override;

  virtual int GetProtocolHeaderLength() override {
    return sizeof(EthernetHeader);
  }

  DevPci *_pci;
};

#endif /* __RAPH_KERNEL_DEV_ETH_H__ */
