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
#include <dev/pci.h>
#include <mem/virtmem.h>

uint16_t pci_get_vendor(device_t dev) {
  return dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kVendorIDReg);
}

uint16_t pci_get_device(device_t dev) {
  return dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kDeviceIDReg);
}


uint16_t pci_get_subvendor(device_t dev) {
  switch(dev->GetPciClass()->ReadReg<uint8_t>(PciCtrl::kHeaderTypeReg) & PciCtrl::kHeaderTypeRegMaskDeviceType) {
  case PciCtrl::kHeaderTypeRegValueDeviceTypeNormal:
    return dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kSubVendorIdReg);
  case PciCtrl::kHeaderTypeRegValueDeviceTypeBridge:
    return 0xffff;
  case PciCtrl::kHeaderTypeRegValueDeviceTypeCardbus:
  default:
    kassert(false);
  }
}

uint16_t pci_get_subdevice(device_t dev) {
  switch(dev->GetPciClass()->ReadReg<uint8_t>(PciCtrl::kHeaderTypeReg) & PciCtrl::kHeaderTypeRegMaskDeviceType) {
  case PciCtrl::kHeaderTypeRegValueDeviceTypeNormal:
    return dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kSubsystemIdReg);
  case PciCtrl::kHeaderTypeRegValueDeviceTypeBridge:
    return 0xffff;
  case PciCtrl::kHeaderTypeRegValueDeviceTypeCardbus:
  default:
    kassert(false);
  }
}

struct adapter *device_get_softc(device_t dev) {
  return dev->adapter;
}
