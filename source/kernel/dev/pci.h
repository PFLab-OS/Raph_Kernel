/*
 *
 * Copyright (c) 2015 Project Raphine
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

#ifndef __RAPH_KERNEL_DEV_PCI_H__
#define __RAPH_KERNEL_DEV_PCI_H__

#include <stdint.h>
#include "../acpi.h"
#include "../global.h"
#include "device.h"
#include "../mem/virtmem.h"

struct MCFGSt {
  uint8_t reserved1[8];
  uint64_t ecam_base;
  uint16_t pci_seg_gnum;
  uint8_t pci_bus_start;
  uint8_t pci_bus_end;
  uint32_t reserved2;
};

struct MCFG {
  ACPISDTHeader header;
  MCFGSt list[0];
} __attribute__ ((packed));

class DevPCI;

class PCICtrl : public Device {
 public:
  static void Init() {
    PCICtrl *addr = reinterpret_cast<PCICtrl *>(virtmem_ctrl->Alloc(sizeof(PCICtrl)));
    pci_ctrl = new(addr) PCICtrl; 
    pci_ctrl->_Init();
  }
 private:
  void _Init();
  virt_addr GetVaddr(uint8_t bus, uint8_t device, uint8_t func, uint16_t reg) {
    return _base_addr + ((bus & 0xff) << 20) + ((device & 0x1f) << 15) + ((func & 0x7) << 12) + (reg & 0xfff);
  }
  template<class T> static T ReadReg(virt_addr vaddr) {
    return *(reinterpret_cast<T *>(vaddr));
  }
  static const uint16_t kDevIdentifyReg = 0x2;
  static const uint16_t kVendorIDReg = 0x00;
  static const uint16_t kDeviceIDReg = 0x02;
  static const uint16_t kHeaderTypeReg = 0x0E;
  static const uint8_t kHeaderTypeMultiFunction = 0x80;
  MCFG *_mcfg = nullptr;
  virt_addr _base_addr = 0;
};

// !!! important !!!
// 派生クラスはstatic void InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf); を作成する事
class DevPCI : public Device {
 public:
  static void InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {} // dummy
 private:
};

template<class T>
static inline void InitPCIDevices(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  T::InitPCI(vid, did, bus, device, mf);
}

template<class T1, class T2, class... Rest>
static inline void InitPCIDevices(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  T1::InitPCI(vid, did, bus, device, mf);
  InitPCIDevices<T2, Rest...>(vid, did, bus, device, mf);
}


#endif /* __RAPH_KERNEL_DEV_PCI_H__ */
