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

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <sys/types-raph.h>
#include <sys/bus-raph.h>

extern "C" {

  int pci_msi_count(device_t dev) {
    return dev->GetPciClass()->GetMsiCount();
  }

  int pci_msix_count(device_t dev) {
    return 0;
  }

  int pci_alloc_msi(device_t dev, int *count) {
    int dcount = dev->GetPciClass()->GetMsiCount();
    if (dcount == 0) {
      return -1;
    }
    if (dcount < *count) {
      *count = dcount;
    }
    dev->GetPciClass()->SetupMsi();
    return 0;
  }

  int pci_release_msi(device_t dev) {
    dev->GetPciClass()->ReleaseMsi();
    return 0;
  }

  int pci_alloc_msix(device_t dev, int *count) {
    return -1;
  }

  uint16_t pci_get_vendor(device_t dev) {
    return dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kVendorIDReg);
  }

  uint16_t pci_get_device(device_t dev) {
    return dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kDeviceIDReg);
  }

  uint32_t pci_get_devid(device_t dev) {
    return (static_cast<uint32_t>(dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kDeviceIDReg)) << 16) | dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kVendorIDReg);
  }

  uint8_t pci_get_class(device_t dev) {
    return dev->GetPciClass()->ReadReg<uint8_t>(PciCtrl::kRegBaseClassCode);
  }

  uint8_t pci_get_subclass(device_t dev) {
    return dev->GetPciClass()->ReadReg<uint8_t>(PciCtrl::kRegSubClassCode);
  }
  
  uint8_t pci_get_progif(device_t dev) {
    return dev->GetPciClass()->ReadReg<uint8_t>(PciCtrl::kRegInterfaceClassCode);
  }
  
  uint8_t pci_get_revid(device_t dev) {
    return dev->GetPciClass()->ReadReg<uint8_t>(PciCtrl::kRegRevisionId);
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

  void pci_write_config(device_t dev, int reg, uint32_t val, int width) {
    switch (width) {
    case 1:
      dev->GetPciClass()->WriteReg<uint8_t>(static_cast<uint16_t>(reg), static_cast<uint8_t>(val));
      return;
    case 2:
      dev->GetPciClass()->WriteReg<uint16_t>(static_cast<uint16_t>(reg), static_cast<uint16_t>(val));
      return;
    case 4:
      dev->GetPciClass()->WriteReg<uint32_t>(static_cast<uint16_t>(reg), static_cast<uint32_t>(val));
      return;
    default:
      kassert(false);
      return;
    }
  }

  uint32_t pci_read_config(device_t dev, int reg, int width) {
    switch (width) {
    case 1:
      return dev->GetPciClass()->ReadReg<uint8_t>(static_cast<uint16_t>(reg));
    case 2:
      return dev->GetPciClass()->ReadReg<uint16_t>(static_cast<uint16_t>(reg));
    case 4:
      return dev->GetPciClass()->ReadReg<uint32_t>(static_cast<uint16_t>(reg));
    default:
      kassert(false);
      return 0;
    };
  }

  int pci_enable_busmaster(device_t dev) {
    dev->GetPciClass()->WriteReg<uint16_t>(PciCtrl::kCommandReg, dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);
    return 0;
  }

  int pci_find_cap(device_t dev, int capability, int *capreg) {
    PciCtrl::CapabilityId id;
    switch(capability) {
    case PCIY_EXPRESS:
      id = PciCtrl::CapabilityId::kPcie;
      break;
    default:
      kassert(false);
    }
    uint16_t cap;
    if ((cap = dev->GetPciClass()->FindCapability(id)) != 0) {
      *capreg = cap;
      return 0;
    } else {
      return -1;
    }
  }

}
