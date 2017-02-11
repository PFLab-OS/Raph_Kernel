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

#ifndef __RAPH_KERNEL_APIC_H__
#define __RAPH_KERNEL_APIC_H__

#include <x86.h>
#include <stdint.h>
#include <raph.h>
#include <_raph_acpi.h>
#include <_cpu.h>
#include <mem/physmem.h>

struct MADT {
  ACPISDTHeader header;
  uint32_t lapicCtrlAddr;
  uint32_t flags;
  uint8_t table[0];
} __attribute__ ((packed));

enum class MADTStType : uint8_t {
  kLocalAPIC = 0,
  kIOAPIC = 1,
  kLocalX2Apic = 9,
};

struct MADTSt {
 MADTStType type;
  uint8_t length;
} __attribute__ ((packed));

// see acpi spec
struct MADTStLAPIC {
  MADTSt st;
  uint8_t pid;
  uint8_t apicId;
  uint32_t flags;
} __attribute__ ((packed));

struct MadtStructX2Lapic {
  MADTSt st;
  uint16_t reserved;
  uint32_t apic_id;
  uint32_t flags;
  uint32_t pid;
} __attribute__ ((packed));

// see acpi spec
struct MADTStIOAPIC {
  MADTSt st;
  uint8_t ioapicId;
  uint8_t reserved;
  uint32_t ioapicAddr;
  uint32_t glblIntBase;
} __attribute__ ((packed));

class Regs;

class ApicCtrl {
public:
  ApicCtrl() {}
  void Init();
  void Setup();
  bool DidSetup() {
    return _setup;
  }

  void SetMADT(MADT *table) {
    _madt = table;
  }

  void StartAPs();

  void BootBSP() {
    _lapic->Setup();
  }

  void BootAP() {
    __atomic_store_n(&_started, true, __ATOMIC_RELEASE);
    _lapic->SetupAp();
    _lapic->Setup();
  }
  int GetCpuIdFromApicId(uint32_t apic_id) {
    return _lapic->GetCpuIdFromApicId(apic_id);
  }
  volatile uint32_t GetApicIdFromCpuId(CpuId cpuid) {
    return _lapic->GetApicIdFromCpuId(cpuid);
  }
  volatile int GetCpuId() {
    if (_lapic == nullptr) {
      return 0;
    }
    return _lapic->GetCpuId();
  }
  volatile uint32_t GetApicId() {
    return _lapic->GetApicId();
  }
  volatile bool IsBootupAll() {
    return _all_bootup;
  }
  // cpu_ctrlを通して呼びだす事
  int GetHowManyCpus() {
    kassert(_lapic != nullptr);
    return _lapic->_ncpu;
  }
  bool SetupIoInt(uint32_t irq, uint8_t lapicid, uint8_t vector) {
    kassert(vector >= 32);
    return _ioapic.SetupInt(irq, lapicid, vector);
  }
  void SendEoi() {
    _lapic->SendEoi();
  }

  void SendIpi(uint32_t destid) {
    _lapic->SendIpi(destid);
  }

  void SetupTimer(int interval) {
    _lapic->SetupTimer(interval);
  }

  void StartTimer() {
    _lapic->StartTimer();
  }

  void StopTimer() {
    _lapic->StopTimer();
  }

  static uint64_t GetMsiAddr(uint8_t dest_lapicid);
  static uint16_t GetMsiData(uint8_t vector);

  static const int kIrqKeyboard = 1;
protected:
  volatile bool _started = false;
  volatile bool _all_bootup = false;

private:
  class Lapic {
  public:
    // see intel64 manual vol3 Table 10-1 (Local APIC Register Address Map)
    enum class RegisterOffset : int {
      kId = 0x2,
      kEoi = 0xB,
      kSvr = 0xF,
      kLvtCmci = 0x2F,
      kIcrLow = 0x30,
      kIcrHigh = 0x31,
      kLvtTimer = 0x32,
      kLvtThermalSensor = 0x33,
      kLvtPerformanceCnt = 0x34,
      kLvtLint0 = 0x35,
      kLvtLint1 = 0x36,
      kLvtErr = 0x37,
      kTimerInitCnt = 0x38,
      kTimerCurCnt = 0x39,
      kDivConfig = 0x3E,
    };
    virtual ~Lapic() {
    }
    // setup local APIC respond to specified index
    void Setup();
    virtual void SetupAp() = 0;
    int GetCpuId() {
      if (_apic_info == nullptr) {
        return 0;
      }
      return GetCpuIdFromApicId(GetApicId());
    }   
    int GetCpuIdFromApicId(uint32_t apic_id) {
      for(int n = 0; n < _ncpu; n++) {
        if(apic_id == _apic_info[n].id) {
          return n;
        }
      }
      return -1;
    }
    uint32_t GetApicIdFromCpuId(CpuId cpuid) {
      int raw_cpuid = cpuid.GetRawId();
      kassert(raw_cpuid >= 0 && raw_cpuid < _ncpu);
      return _apic_info[raw_cpuid].id;
    }
    // start local APIC respond to specified index with apicId
    void Start(uint32_t apicId, uint64_t entryPoint);
    
    int _ncpu = 0;
    
    struct ApicInfo {
      uint32_t id;
    } *_apic_info = nullptr;

    void SendEoi() {
      WriteReg(RegisterOffset::kEoi, 0);
    }
    void SendIpi(uint32_t destid);
    void SetupTimer(int interval);
    void StartTimer() {
      WriteReg(RegisterOffset::kTimerInitCnt, ReadReg(RegisterOffset::kTimerInitCnt));
      WriteReg(RegisterOffset::kLvtTimer, ReadReg(RegisterOffset::kLvtTimer) & ~kRegLvtMask);
    }
    void StopTimer() {
      WriteReg(RegisterOffset::kLvtTimer, ReadReg(RegisterOffset::kLvtTimer) | kRegLvtMask);
    }
    static uint64_t GetMsiAddr(uint8_t dest_lapicid) {
      return kMsiAddrRegReserved | (static_cast<uint64_t>(dest_lapicid) << kMsiAddrRegDestOffset);
    }
    static uint16_t GetMsiData(uint8_t vector) {
      return kRegIcrTriggerModeEdge | kDeliverModeLowest | vector;
    }
    static const int kIa32ApicBaseMsr = 0x1B;
    static const uint32_t kApicX2ApicEnableFlag = 1 << 10;
    static const uint32_t kApicGlobalEnableFlag = 1 << 11;
    virtual void WriteReg(RegisterOffset offset, uint32_t data) = 0;
    virtual uint32_t ReadReg(RegisterOffset offset) = 0;
    // write to 64-bit value to local APIC ICR (Interrupt Command Register)
    virtual void WriteIcr(uint32_t dest, uint32_t flags) = 0;
    virtual volatile uint32_t GetApicId() = 0;
  protected:

    // see intel64 manual vol3 Figure 10-10 (Divide Configuration Register)
    static const uint32_t kDivVal1 = 0xB;
    static const uint32_t kDivVal16 = 0x3;

    // see intel64 manual vol3 Figure 10-8 (Local Vector Table)
    static const int kRegLvtMask = 1 << 16;
    static const int kRegTimerPeriodic = 1 << 17;

    // see intel64 manual vol3 Figure 10-23 (Spurious-Interrupt Vector Register)
    static const uint32_t kRegSvrApicEnableFlag = 1 << 8;

    // see intel64 manual vol3 10.5.1 (Delivery Mode)
    static const uint32_t kDeliverModeFixed   = 0 << 8;
    static const uint32_t kDeliverModeLowest  = 1 << 8;
    static const uint32_t kDeliverModeSmi     = 2 << 8;
    static const uint32_t kDeliverModeNmi     = 4 << 8;
    static const uint32_t kDeliverModeInit    = 5 << 8;
    static const uint32_t kDeliverModeStartup = 6 << 8;

    // see intel64 manual vol3 Figure 10-12 (Interrupt Command Register)
    static const uint32_t kRegIcrLevelAssert   = 1 << 14;
    static const uint32_t kRegIcrTriggerModeEdge  = 0 << 15;
    static const uint32_t kRegIcrTriggerModeLevel = 1 << 15;
    static const uint32_t kRegIcrDestShorthandNoShortHand    = 0 << 18;
    static const uint32_t kRegIcrDestShorthandSelf           = 1 << 18;
    static const uint32_t kRegIcrDestShorthandAllIncludeSelf = 2 << 18;
    static const uint32_t kRegIcrDestShorthandAllExcludeSelf = 3 << 18;

    // see intel64 manual vol3 Figure 10-24 (Layout of the MSI Message Address Register)
    static const uint64_t kMsiAddrRegReserved = 0xFEE00000;
    static const int kMsiAddrRegDestOffset = 12;

    // see intel MPspec Appendix B.5
    static const uint32_t kIoRtc = 0x70;
    
    void Outb(int pin, uint8_t data) {
      asm volatile("outb %%al, %%dx"::"d"(pin), "a"(data));
    }
  };
  class LapicX : public Lapic {
  public:
    LapicX() {
      _ctrl_addr = reinterpret_cast<uint32_t *>(p2v(kMmioBaseAddr));
    }
    virtual ~LapicX() {
    }
    virtual void SetupAp() override {
    }
    virtual void WriteReg(RegisterOffset offset, uint32_t data) override {
      _ctrl_addr[(static_cast<int>(offset) * 0x10) / sizeof(uint32_t)] = data;
    }
    virtual uint32_t ReadReg(RegisterOffset offset) override {
      return _ctrl_addr[(static_cast<int>(offset) * 0x10) / sizeof(uint32_t)];
    }
    virtual void WriteIcr(uint32_t dest, uint32_t flags) {
      WriteReg(RegisterOffset::kIcrHigh, dest << 24);
      // wait for write to finish, by reading
      GetApicId();
      WriteReg(RegisterOffset::kIcrLow, flags);
      // refer to ia32-sdm vol-3 10-20
      WriteReg(RegisterOffset::kIcrLow, ReadReg(RegisterOffset::kIcrLow) | ((flags & kDeliverModeInit) ? 0 : kRegIcrLevelAssert));
      GetApicId();
    }
    virtual volatile uint32_t GetApicId() override {
      return ReadReg(RegisterOffset::kId) >> 24;
    }
    static const size_t kMmioBaseAddr = 0xFEE00000;
  private:
    uint32_t *_ctrl_addr;
  };
  class LapicX2 : public Lapic {
  public:
    LapicX2() {
    }
    virtual ~LapicX2() {
    }
    virtual void SetupAp() override;
    virtual void WriteReg(RegisterOffset offset, uint32_t data) override {
      x86::wrmsr(kMsrAddr + static_cast<int>(offset), data);
    }
    virtual uint32_t ReadReg(RegisterOffset offset) override {
      return x86::rdmsr(kMsrAddr + static_cast<int>(offset));
    }
    virtual void WriteIcr(uint32_t dest, uint32_t flags) {
      flags |= (flags & kDeliverModeInit) ? 0 : kRegIcrLevelAssert;
      x86::wrmsr(kMsrAddr + static_cast<int>(RegisterOffset::kIcrLow), (static_cast<uint64_t>(dest) << 32) | flags);
    }
    virtual volatile uint32_t GetApicId() override {
      return ReadReg(RegisterOffset::kId);
    }
  private:
    static const uint32_t kMsrAddr = 0x800;
  };
  class Ioapic {
  public:
    void Setup();
    bool SetupInt(uint32_t irq, uint8_t lapicid, uint8_t vector);
    struct Controller {
      // see IOAPIC manual 3.1 (Memory Mapped Registers for Accessing IOAPIC Registers)
      static const int kIndex = 0x0;
      static const int kData = 0x10 / sizeof(uint32_t);

      // see IOAPIC manual 3.2 (IOAPIC Registers)
      static const uint32_t kRegVer = 0x1;
      static const uint32_t kRegRedTbl = 0x10;

      // see IOAPIC manual 3.2.4 (I/O Redirection Table Registers)
      static const uint32_t kRegRedTblFlagValueDeliveryLow = 1 << 8;
      static const uint32_t kRegRedTblFlagDestModePhys = 0 << 11;
      static const uint32_t kRegRedTblFlagDestModeLogical = 1 << 11;
      static const uint32_t kRegRedTblFlagTriggerModeEdge = 0 << 15;
      static const uint32_t kRegRedTblFlagTriggerModeLevel = 1 << 15;
      static const uint32_t kRegRedTblFlagMask = 1 << 16;
      static const int kRegRedTblOffsetDest = 24;
      
      uint32_t *reg;
      uint32_t int_base;
      uint32_t int_max;
      void Setup(MADTStIOAPIC *madt_struct_ioapic);
      uint32_t Read(uint32_t index) {
        reg[kIndex] = index;
        return reg[kData];
      }
      void Write(uint32_t index, uint32_t data) {
        reg[kIndex] = index;
        reg[kData] = data;
      }
      void DisableInt() {
        for (uint32_t i = 0; i <= int_max; i++) {
          Write(kRegRedTbl + 2 * i, kRegRedTblFlagMask);
          Write(kRegRedTbl + 2 * i + 1, 0);
        }
      }
      bool SetupInt(uint32_t irq, uint8_t lapicid, uint8_t vector);
    } *_controller = nullptr;
    int _controller_num = 0;
  private:
  };
  static void TmrCallback(Regs *rs, void *arg) {
  }
  static void IpiCallback(Regs *rs, void *arg) {
  }
  static void PicSpuriousCallback(Regs *rs, void *arg);
  static void PicUnknownCallback(Regs *rs, void *arg);

  Lapic *_lapic = nullptr;
  Ioapic _ioapic;
  MADT *_madt = nullptr;

  static const uint32_t kMadtFlagLapicEnable = 1;
  bool _setup = false;
};

inline uint64_t ApicCtrl::GetMsiAddr(uint8_t dest_lapicid) {
  return Lapic::GetMsiAddr(dest_lapicid);
}

inline uint16_t ApicCtrl::GetMsiData(uint8_t vector) {
  return Lapic::GetMsiData(vector);
}

#endif /* __RAPH_KERNEL_APIC_H__ */
