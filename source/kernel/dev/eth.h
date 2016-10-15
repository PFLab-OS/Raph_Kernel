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

/**
 * A class representing Ethernet device, which has MAC address and IP address.
 */
class DevEthernet : public NetDev {
public:
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

  /**
   * Set up the network interface. Usually you must register this interface
   * to network device controller (NetDevCtrl).
   * Other initialization can be done in this method.
   */
  virtual void SetupNetInterface() override {
    kassert(netdev_ctrl->RegisterDevice(this, "en"));
  }

  /**
   * Assign IPv4 address to the device.
   *
   * @param addr IPv4 address.
   * @return if the device supports IPv4 address or not.
   */
  virtual bool AssignIpv4Address(uint32_t addr) {
    _ipv4_addr = addr;
    return true;
  }

  /**
   * Get IPv4 address.
   *
   * @param addr buffer to return.
   * @return if the device supports IPv4 address or not.
   */
  virtual bool GetIpv4Address(uint32_t &addr) {
    addr = _ipv4_addr;
    return true;
  }

protected:
  DevEthernet(DevPci *pci) {
    _pci = pci;
  }

  DevPci *_pci;

private:
  /** IPv4 address */
  uint32_t _ipv4_addr = 0;
};

#endif /* __RAPH_KERNEL_DEV_ETH_H__ */
