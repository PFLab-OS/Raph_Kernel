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

#include "e1000_raph.h"
#include "e1000_api.h"
#include "e1000_osdep.h"
#include "../pci.h"
#include "../../mem/virtmem.h"

uint16_t pci_get_vendor(device_t dev) {
  return dev->parent->ReadReg<uint16_t>(PCICtrl::kVendorIDReg);
}

uint16_t pci_get_device(device_t dev) {
  return dev->parent->ReadReg<uint16_t>(PCICtrl::kDeviceIDReg);
}


uint16_t pci_get_subvendor(device_t dev) {
  switch(dev->parent->ReadReg<uint8_t>(PCICtrl::kHeaderTypeReg) & PCICtrl::kHeaderTypeRegMaskDeviceType) {
  case PCICtrl::kHeaderTypeRegValueDeviceTypeNormal:
    return dev->parent->ReadReg<uint16_t>(PCICtrl::kSubVendorIdReg);
  case PCICtrl::kHeaderTypeRegValueDeviceTypeBridge:
    return 0xffff;
  case PCICtrl::kHeaderTypeRegValueDeviceTypeCardbus:
  default:
    kassert(false);
  }
}

uint16_t pci_get_subdevice(device_t dev) {
  switch(dev->parent->ReadReg<uint8_t>(PCICtrl::kHeaderTypeReg) & PCICtrl::kHeaderTypeRegMaskDeviceType) {
  case PCICtrl::kHeaderTypeRegValueDeviceTypeNormal:
    return dev->parent->ReadReg<uint16_t>(PCICtrl::kSubsystemIdReg);
  case PCICtrl::kHeaderTypeRegValueDeviceTypeBridge:
    return 0xffff;
  case PCICtrl::kHeaderTypeRegValueDeviceTypeCardbus:
  default:
    kassert(false);
  }
}

struct resource *bus_alloc_resource_from_bar(device_t dev, int bar) {
  struct resource *r = reinterpret_cast<struct resource *>(virtmem_ctrl->Alloc(sizeof(struct resource)));
  uint32_t addr = dev->parent->ReadReg<uint32_t>(static_cast<uint32_t>(bar));
  if ((addr & PCICtrl::kRegBaseAddrFlagIo) != 0) {
    r->type = BUS_SPACE_PIO;
    r->addr = addr & PCICtrl::kRegBaseAddrMaskIoAddr;
  } else {
    r->type = BUS_SPACE_MEMIO;
    r->data.mem.is_prefetchable = ((addr & PCICtrl::kRegBaseAddrIsPrefetchable) != 0);
    r->addr = addr & PCICtrl::kRegBaseAddrMaskMemAddr;

    if ((addr & PCICtrl::kRegBaseAddrMaskMemType) == PCICtrl::kRegBaseAddrValueMemType64) {
      r->addr |= static_cast<uint64_t>(dev->parent->ReadReg<uint32_t>(static_cast<uint32_t>(bar + 4))) << 32;
    }
    r->addr = p2v(r->addr);
  }
  return r;
}

struct adapter *device_get_softc(device_t dev) {
  return dev->adapter;
}
