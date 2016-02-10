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

class PCICtrl {
 public:
  void Init();
  void SetMCFG(MCFG *table) {
    _mcfg = table;
  }
  void RegisterDevice(DevPCI *pci) {
    for (int i = 0; i < _device_register_num; i++) {
      if (_device_map[i] != nullptr) {
	_device_map[i] = pci;
	break;
      }
    }
  }
 private:
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
  static const uint32_t kBassAddress0 = 0x10;
  static const uint32_t kBassAddress1 = 0x14;
  static const uint8_t kHeaderTypeMultiFunction = 0x80;
  MCFG *_mcfg = nullptr;
  virt_addr _base_addr = 0;
  // TODO: 動的にしたい
  static const int _device_register_num = 256;
  DevPCI *_device_map[_device_register_num];
};

class DevPCI : public Device {
 public:
  virtual void Init() override {
    Device::Init();
    pci_ctrl->RegisterDevice(this);
  }
  void Setup(uint16_t vid, uint16_t did) {
    _vid = vid;
    _did = did;
  }
  bool CheckInit(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
    if (_vid == vid && _did == did) {
      InitPCI(bus, device, mf);
      return true;
    }
    return false;
  }
  virtual void InitPCI(uint8_t bus, uint8_t device, bool mf);
 private:
  uint16_t _vid;
  uint16_t _did;
};

#endif /* __RAPH_KERNEL_DEV_PCI_H__ */
