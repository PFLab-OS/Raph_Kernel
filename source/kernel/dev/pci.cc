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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#include <raph_acpi.h>
#include <mem/physmem.h>
#include <global.h>
#include <tty.h>
#include <cpu.h>
#include "pci.h"

#include <dev/usb/uhci.h>
#include <dev/usb/ehci.h>
#include <dev/nic/intel/ix/ix.h>
#include <dev/nic/intel/em/em.h>
#include <dev/nic/intel/em/lem.h>
#include <dev/storage/ahci/ahci-raph.h>
#include <dev/nic/realtek/rtl8139/rl.h>

void PciCtrl::Probe() {
  _cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);

  _mcfg = acpi_ctrl->GetMCFG();
  if (_mcfg == nullptr) {
    gtty->Printf("[Pci] error: could not find MCFG table.\n");
    return;
  }

  for (int i = 0;
       i * sizeof(MCFGSt) < _mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Printf("[Pci] info: multiple MCFG tables.\n");
      break;
    }
    if (_mcfg->list[i].ecam_base >= 0x100000000) {
      gtty->Printf(
          "[Pci] error: ECAM base addr does not exist in low 4GB of memory\n");
      continue;
    }
    _base_addr = p2v(_mcfg->list[i].ecam_base);
  }
}

void PciCtrl::_Attach() {
  for (int i = 0;
       i * sizeof(MCFGSt) < _mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    for (int j = _mcfg->list[i].pci_bus_start; j <= _mcfg->list[i].pci_bus_end;
         j++) {
      for (int k = 0; k < 32; k++) {
        int maxf = ((ReadPciReg<uint16_t>(j, k, 0, kHeaderTypeReg) &
                     kHeaderTypeRegFlagMultiFunction) != 0)
                       ? 7
                       : 0;
        for (int l = 0; l <= maxf; l++) {
          uint16_t vid = ReadPciReg<uint16_t>(j, k, l, kVendorIDReg);
          if (vid == 0xffff) {
            continue;
          }
          WritePciReg<uint16_t>(
              j, k, l, PciCtrl::kCommandReg,
              ReadPciReg<uint16_t>(j, k, l, PciCtrl::kCommandReg) |
                  PciCtrl::kCommandRegInterruptDisableFlag);
          DevPci *dev = InitPciDevices(j, k, l);
          if (dev != nullptr) {
            _devices.PushBack(dev);
          }
        }
      }
    }
  }
  auto iter = _devices.GetBegin();
  while (!iter.IsNull()) {
    (*(*iter))->Attach();
    iter = iter->GetNext();
  }
}

DevPci *PciCtrl::InitPciDevices(uint8_t bus, uint8_t device, uint8_t func) {
  return _InitPciDevices<IxGbe, E1000, lE1000, Rtl8139, DevEhci, DevUhci,
                         /*AhciCtrl,*/ DevPci>(bus, device, func);
}

uint16_t PciCtrl::FindCapability(uint8_t bus, uint8_t device, uint8_t func,
                                 CapabilityId id) {
  if ((ReadPciReg<uint16_t>(bus, device, func, kStatusReg) |
       kStatusRegFlagCapListAvailable) == 0) {
    return 0;
  }

  uint8_t ptr = 0;

  switch (ReadPciReg<uint8_t>(bus, device, func, kHeaderTypeReg) &
          kHeaderTypeRegMaskDeviceType) {
    case kHeaderTypeRegValueDeviceTypeNormal:
    case kHeaderTypeRegValueDeviceTypeBridge:
      ptr = kCapPtrReg;
      break;
    case kHeaderTypeRegValueDeviceTypeCardbus:
    default:
      kassert(false);
  }

  ptr = ReadPciReg<uint8_t>(bus, device, func, ptr);

  while (true) {
    if (ptr == 0) {
      return 0;
    }
    if (ReadPciReg<uint8_t>(bus, device, func, ptr + kCapRegId) ==
        static_cast<uint8_t>(id)) {
      return ptr;
    }
    ptr = ReadPciReg<uint8_t>(bus, device, func, ptr + kCapRegNext);
  }
}

void PciCtrl::SetMsi(uint8_t bus, uint8_t device, uint8_t func, uint64_t addr,
                     uint16_t data) {
  uint16_t offset = FindCapability(bus, device, func, CapabilityId::kMsi);
  if (offset == 0) {
    return;
  }

  WritePciReg<uint16_t>(
      bus, device, func, PciCtrl::kCommandReg,
      ReadPciReg<uint16_t>(bus, device, func, PciCtrl::kCommandReg) |
          PciCtrl::kCommandRegInterruptDisableFlag);

  uint16_t control =
      ReadPciReg<uint16_t>(bus, device, func, offset + kMsiCapRegControl);

  if (control & kMsiCapRegControlAddr64Flag) {
    // addr 64bit
    WritePciReg<uint32_t>(bus, device, func, offset + kMsiCapRegMsgAddr,
                          static_cast<uint32_t>(addr));
    WritePciReg<uint32_t>(bus, device, func, offset + kMsiCapReg64MsgUpperAddr,
                          static_cast<uint32_t>(addr >> 32));
    WritePciReg<uint16_t>(bus, device, func, offset + kMsiCapReg64MsgData,
                          static_cast<uint16_t>(data));
  } else {
    kassert(addr < 0x100000000);
    WritePciReg<uint32_t>(bus, device, func, offset + kMsiCapRegMsgAddr,
                          static_cast<uint32_t>(addr));
    WritePciReg<uint16_t>(bus, device, func, offset + kMsiCapReg32MsgData,
                          static_cast<uint16_t>(data));
  }
  WritePciReg<uint16_t>(bus, device, func, offset + kMsiCapRegControl,
                        control | kMsiCapRegControlMsiEnableFlag);
}

int PciCtrl::RegisterLegacyIntHandler(int irq, DevPci *device,
                                      Idt::EoiType type) {
  Locker lock(_irq_container_lock);
  IrqContainer *ic = _irq_container;
  while (ic->next != nullptr) {
    IrqContainer *nic = ic->next;
    if (nic->irq == irq) {
      IntHandler *ih = nic->inthandler;
      while (ih->next != nullptr) {
        ih = ih->next;
      }
      ih->Add(device);
      return nic->vector;
    }
    ic = nic;
  }
  int vector = idt->SetIntCallback(_cpuid, LegacyIntHandler,
                                   reinterpret_cast<void *>(this), type);
  apic_ctrl->SetupIoInt(irq, _cpuid.GetApicId(), vector, false, false);
  ic->Add(irq, vector);
  ic->next->inthandler->Add(device);
  device->WritePciReg<uint16_t>(
      PciCtrl::kCommandReg, device->ReadPciReg<uint16_t>(PciCtrl::kCommandReg) &
                                ~PciCtrl::kCommandRegInterruptDisableFlag);
  return vector;
}

void PciCtrl::IrqContainer::Handle() {
  IntHandler *ih = inthandler;
  while (ih->next != nullptr) {
    IntHandler *nih = ih->next;
    nih->device->LegacyIntHandler();
    ih = nih;
  }
}
