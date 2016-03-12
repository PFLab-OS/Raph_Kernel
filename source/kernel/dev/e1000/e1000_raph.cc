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
#include "e1000.h"
#include "../pci.h"

uint16_t pci_get_vendor(device_t dev) {
  return dev->parent->ReadReg<uint16_t>(PCICtrl::kVendorIDReg);
}

uint16_t pci_get_device(device_t dev) {
  return dev->parent->ReadReg<uint16_t>(PCICtrl::kDeviceIDReg);
}

uint16_t pci_get_subvendor(device_t dev) {
  kassert((dev->parent->ReadReg<uint8_t>(PCICtrl::kHeaderTypeReg) & PCICtrl::kHeaderTypeRegMaskDeviceType) == PCICtrl::kHeaderTypeRegValueDeviceTypeNormal);
  return dev->parent->ReadReg<uint16_t>(PCICtrl::kSubVendorIdReg);
}

uint16_t pci_get_subdevice(device_t dev) {
  kassert((dev->parent->ReadReg<uint8_t>(PCICtrl::kHeaderTypeReg) & PCICtrl::kHeaderTypeRegMaskDeviceType) == PCICtrl::kHeaderTypeRegValueDeviceTypeNormal);
  return dev->parent->ReadReg<uint16_t>(PCICtrl::kSubsystemIdReg);
}

void *device_get_softc(device_t dev) {
  void *adapter = reinterpret_cast<void *>(virtmem_ctrl->Alloc(dev->driver->size));
  dev->adapter = reinterpret_cast<struct adapter *>(adapter);
  return adapter;
}


