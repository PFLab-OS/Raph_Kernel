/*
 *
 * Copyright (c) 2015 Raphine Project
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

#include "../acpi.h"
#include "../mem/physmem.h"
#include "../global.h"
#include "../tty.h"
#include "pci.h"

#include "e1000/lem.h"

void PCICtrl::_Init() {
  _mcfg = acpi_ctrl->GetMCFG();
  if (_mcfg == nullptr) {
    gtty->Printf("s", "[PCI] error: could not find MCFG table.\n");
    return;
  }
  for (int i = 0; i * sizeof(MCFGSt) < _mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Printf("s", "[PCI] info: multiple MCFG tables.\n");
      break;
    }
    if (_mcfg->list[i].ecam_base >= 0x100000000) {
      gtty->Printf("s", "[PCI] error: ECAM base addr is not exist in low 4GB of memory\n");
      continue;
    }
    _base_addr = p2v(_mcfg->list[i].ecam_base);
    for (int j = _mcfg->list[i].pci_bus_start; j <= _mcfg->list[i].pci_bus_end; j++) {
      for (int k = 0; k < 32; k++) {
        uint16_t vid = ReadReg<uint16_t>(j, k, 0, kVendorIDReg);
        if (vid == 0xffff) {
          continue;
        }
        uint16_t did = ReadReg<uint16_t>(j, k, 0, kDeviceIDReg);
        bool mf = ReadReg<uint8_t>(j, k, 0, kHeaderTypeReg) & kHeaderTypeRegFlagMultiFunction;

        InitPCIDevices<lE1000, DevPCI>(vid, did, j, k, mf);
      }
    }
  }
}

uint16_t PCICtrl::FindCapability(uint8_t bus, uint8_t device, uint8_t func, CapabilityId id) {
  if ((ReadReg<uint16_t>(bus, device, func, kStatusReg) | kStatusRegFlagCapListAvailable) == 0) {
    return 0;
  }
  
  uint8_t ptr = 0;

  switch(ReadReg<uint8_t>(bus, device, func, kHeaderTypeReg)
         & kHeaderTypeRegMaskDeviceType) {
  case kHeaderTypeRegValueDeviceTypeNormal:
  case kHeaderTypeRegValueDeviceTypeBridge:
    ptr = kCapPtrReg;
    break;
  case kHeaderTypeRegValueDeviceTypeCardbus:
  default:
    kassert(false);
  }

  ptr = ReadReg<uint8_t>(bus, device, func, ptr);

  while (true) {
    if (ptr == 0) {
      return 0;
    }
    if (ReadReg<uint8_t>(bus, device, func, ptr + kCapRegId) == static_cast<uint8_t>(id)) {
      return ptr;
    }
    ptr = ReadReg<uint8_t>(bus, device, func, ptr + kCapRegNext);
  }
}
