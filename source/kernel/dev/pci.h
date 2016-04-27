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

#ifndef __RAPH_KERNEL_DEV_Pci_H__
#define __RAPH_KERNEL_DEV_Pci_H__

#include <stdint.h>
#include <raph_acpi.h>
#include <global.h>
#include <spinlock.h>
#include <mem/virtmem.h>
#include <idt.h>
#include <apic.h>
#include <dev/device.h>

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

class DevPci;

class PciCtrl : public Device {
 public:
  enum class CapabilityId : uint8_t {
   kMsi = 0x05,
   kPcie = 0x10,
  };

  static void Init() {
    PciCtrl *addr = reinterpret_cast<PciCtrl *>(virtmem_ctrl->Alloc(sizeof(PciCtrl)));
    pci_ctrl = new(addr) PciCtrl;
    pci_ctrl->_Init();
  }
  virt_addr GetVaddr(uint8_t bus, uint8_t device, uint8_t func, uint16_t reg) {
    return _base_addr + ((bus & 0xff) << 20) + ((device & 0x1f) << 15) + ((func & 0x7) << 12) + (reg & 0xfff);
  }
  template<class T>
    T ReadReg(uint8_t bus, uint8_t device, uint8_t func, uint16_t reg) override {
    return *(reinterpret_cast<T *>(GetVaddr(bus, device, func, reg)));
  }
  template<class T>
    void WriteReg(uint8_t bus, uint8_t device, uint8_t func, uint16_t reg, T value) override {
    *(reinterpret_cast<T *>(GetVaddr(bus, device, func, reg))) = value;
  }
  // Capabilityへのオフセットを返す
  // 見つからなかった時は0
  uint16_t FindCapability(uint8_t bus, uint8_t device, uint8_t func, CapabilityId id);
  // エラーの場合flaseが返る
  bool SetMsi(uint8_t bus, uint8_t device, uint8_t func, uint64_t addr, uint16_t data);
  static const uint16_t kDevIdentifyReg = 0x2;
  static const uint16_t kVendorIDReg = 0x00;
  static const uint16_t kDeviceIDReg = 0x02;
  static const uint16_t kCommandReg = 0x04;
  static const uint16_t kStatusReg = 0x06;
  static const uint16_t kRegRevisionId = 0x08;
  static const uint16_t kHeaderTypeReg = 0x0E;
  static const uint16_t kBaseAddressReg0 = 0x10;
  static const uint16_t kBaseAddressReg1 = 0x14;
  static const uint16_t kSubVendorIdReg = 0x2c;
  static const uint16_t kSubsystemIdReg = 0x2e;
  static const uint16_t kCapPtrReg = 0x34;

  // Capability Registers
  static const uint16_t kCapRegId = 0x0;
  static const uint16_t kCapRegNext = 0x1;

  // MSI Capability Registers
  // see PCI Local Bus Specification Figure 6-9
  static const uint16_t kMsiCapRegControl = 0x2;
  static const uint16_t kMsiCapRegMsgAddr = 0x4;
  // 32bit
  static const uint16_t kMsiCapReg32MsgData = 0x8;
  // 64bit
  static const uint16_t kMsiCapReg64MsgUpperAddr = 0x8;
  static const uint16_t kMsiCapReg64MsgData = 0xC;

  // Message Control for MSI
  // see PCI Local Bus Specification 6.8.1.3
  static const uint16_t kMsiCapRegControlMsiEnableFlag = 1 << 0;
  static const uint16_t kMsiCapRegControlAddr64Flag = 1 << 7;

  static const uint16_t kCommandRegBusMasterEnableFlag = 1 << 2;
  static const uint16_t kCommandRegMemWriteInvalidateFlag = 1 << 4;

  static const uint8_t kHeaderTypeRegFlagMultiFunction = 1 << 7;
  static const uint8_t kHeaderTypeRegMaskDeviceType = (1 << 7) - 1;
  static const uint8_t kHeaderTypeRegValueDeviceTypeNormal = 0x00;
  static const uint8_t kHeaderTypeRegValueDeviceTypeBridge = 0x01;
  static const uint8_t kHeaderTypeRegValueDeviceTypeCardbus = 0x02;

  static const uint16_t kStatusRegFlagCapListAvailable = 1 << 4;

  static const uint32_t kRegBaseAddrFlagIo = 1 << 0;
  static const uint32_t kRegBaseAddrMaskMemType = 3 << 1;
  static const uint32_t kRegBaseAddrValueMemType32 = 0 << 1;
  static const uint32_t kRegBaseAddrValueMemType64 = 2 << 1;
  static const uint32_t kRegBaseAddrIsPrefetchable = 1 << 3;
  static const uint32_t kRegBaseAddrMaskMemAddr = 0xFFFFFFF0;
  static const uint32_t kRegBaseAddrMaskIoAddr = 0xFFFFFFFC;
 private:
  void _Init();
  MCFG *_mcfg = nullptr;
  virt_addr _base_addr = 0;
};

// !!! important !!!
// 派生クラスはstatic void InitPci(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf); を作成する事
class DevPci : public Device {
 public:
 DevPci(uint8_t bus, uint8_t device, bool mf) : _bus(bus), _device(device), _mf(mf) {
  }
  virtual ~DevPci() {
  }
  static bool InitPci(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
    return false;
  } // dummy
  template<class T> T ReadReg(uint16_t reg) {
    kassert(pci_ctrl != nullptr);
    return pci_ctrl->ReadReg<T>(_bus, _device, 0, reg);
  }
  template<class T> void WriteReg(uint16_t reg, T value) {
    kassert(pci_ctrl != nullptr);
    pci_ctrl->WriteReg<T>(_bus, _device, 0, reg, value);
  }
  uint16_t FindCapability(PciCtrl::CapabilityId id) {
    kassert(pci_ctrl != nullptr);
    return pci_ctrl->FindCapability(_bus, _device, 0, id);
  } 
  // 返り値は割り当てられたvector または-1(error)
  int SetMsi(int cpuid, int_callback handler, void *arg) {
    kassert(pci_ctrl != nullptr);
    kassert(idt != nullptr);
    int vector = idt->SetIntCallback(cpuid, handler, arg);
    if(pci_ctrl->SetMsi(_bus, _device, 0, ApicCtrl::Lapic::GetMsiAddr(apic_ctrl->GetApicIdFromCpuId(cpuid)), ApicCtrl::Lapic::GetMsiData(vector))) {
      return vector;
    }
    return -1;
  }
 private:
  DevPci();
  const uint8_t _bus;
  const uint8_t _device;
  const bool _mf;
};

template<class T>
static inline void InitPciDevices(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  T::InitPci(vid, did, bus, device, mf);
}

template<class T1, class T2, class... Rest>
static inline void InitPciDevices(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  if (!T1::InitPci(vid, did, bus, device, mf)) {
    InitPciDevices<T2, Rest...>(vid, did, bus, device, mf);
  }
}


#endif /* __RAPH_KERNEL_DEV_Pci_H__ */
