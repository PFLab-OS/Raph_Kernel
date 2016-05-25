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

class DevEthernet : public NetDev {
public:
  DevEthernet(uint8_t bus, uint8_t device, bool mf) {
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
  virtual void SetupNetInterface() = 0;

protected:
  DevEthernet(DevPci *pci) {
    _pci = pci;
  }

  virtual void FilterRxPacket(void *p) override;

  virtual void PrepareTxPacket(NetDev::Packet *packet) override;
  DevPci *_pci;
};

class DevEthernetCtrl : public NetDevCtrl {
  DevEthernet *_devTable[kMaxDevNumber] = {nullptr};
};

#endif /* __RAPH_KERNEL_DEV_ETH_H__ */
