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

#include <stdint.h>
#include <raph.h>
#include <raph_acpi.h>
#include <cpu.h>

#ifndef __UNIT_TEST__
struct MADT {
  ACPISDTHeader header;
  uint32_t lapicCtrlAddr;
  uint32_t flags;
  uint8_t table[0];
} __attribute__ ((packed));

enum class MADTStType : uint8_t {
  kLocalAPIC = 0,
  kIOAPIC = 1,
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

// see acpi spec
struct MADTStIOAPIC {
  MADTSt st;
  uint8_t ioapicId;
  uint8_t reserved;
  uint32_t ioapicAddr;
  uint32_t glblIntBase;
} __attribute__ ((packed));
#endif // __UNIT_TEST__

class Regs;

#ifndef __UNIT_TEST__
class ApicCtrl {
public:
  class Lapic {
  public:
    // setup local APIC respond to specified index
    void Setup();
    // int GetCpuId() {
    //   if(_ctrlAddr == nullptr) {
    //     return 0;
    //   }
    //   GetCpuIdFromApicId(GetApicId());
    // }   
    int GetCpuIdFromApicId(uint32_t apic_id) {
      for(int n = 0; n < _ncpu; n++) {
        if(apic_id == _apic_info[n].id) {
          return n;
        }
      }
      return 0;
    }
    uint8_t GetApicIdFromCpuId(int cpuid) {
      kassert(cpuid >= 0 && cpuid < _ncpu);
      return _apic_info[cpuid].id;
    }
    // start local APIC respond to specified index with apicId
    void Start(uint8_t apicId, uint64_t entryPoint);
    int _ncpu;
    
    struct ApicInfo {
      uint8_t id;
      volatile uint32_t *ctrl_addr;
    } *_apic_info;

    volatile uint8_t GetApicId() {
      return _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegId] >> 24;
    }
    void SendEoi() {
      _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegEoi] = 0;
    }
    void EnableInt() {
      _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegSvr] |= kRegSvrApicEnableFlag;
    }
    bool DisableInt() {
      //TODO 割り込みが無効化されている事を確認
      if ((_apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegSvr] & kRegSvrApicEnableFlag) == 0) {
        return false;
      } else {
        _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegSvr] &= ~kRegSvrApicEnableFlag;
        return true;
      }
    }
    bool IsIntEnable() {
      return (_apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegSvr] | kRegSvrApicEnableFlag) != 0;
    }
    void SendIpi(uint8_t destid);
    void SetupTimer(int interval);
    void StartTimer() {
      volatile uint32_t tmp = _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegTimerInitCnt];
      _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegTimerInitCnt] = tmp;
      _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegLvtTimer] &= ~kRegLvtMask;
    }
    void StopTimer() {
      _apic_info[cpu_ctrl->GetId()].ctrl_addr[kRegLvtTimer] |= kRegLvtMask;
    }
    static uint64_t GetMsiAddr(uint8_t dest_lapicid) {
      return kMsiAddrRegReserved | (static_cast<uint64_t>(dest_lapicid) << kMsiAddrRegDestOffset);
    }
    static uint16_t GetMsiData(uint8_t vector) {
      return kRegIcrTriggerModeEdge | kDeliverModeLowest | vector;
    }
  private:
    static const int kIa32ApicBaseMsr = 0x1B;
    static const uint32_t kApicGlobalEnableFlag = 1 << 11;
    // see intel64 manual vol3 Table 10-1 (Local APIC Register Address Map)
    static const int kRegId = 0x20 / sizeof(uint32_t);
    static const int kRegEoi = 0xB0 / sizeof(uint32_t);
    static const int kRegSvr = 0xF0 / sizeof(uint32_t);
    static const int kRegIcrLo = 0x300 / sizeof(uint32_t);
    static const int kRegIcrHi = 0x310 / sizeof(uint32_t);
    static const int kRegLvtCmci = 0x2F0 / sizeof(uint32_t);
    static const int kRegLvtTimer = 0x320 / sizeof(uint32_t);
    static const int kRegLvtThermalSensor = 0x330 / sizeof(uint32_t);
    static const int kRegLvtPerformanceCnt = 0x340 / sizeof(uint32_t);
    static const int kRegLvtLint0 = 0x350 / sizeof(uint32_t);
    static const int kRegLvtLint1 = 0x360 / sizeof(uint32_t);
    static const int kRegLvtErr = 0x370 / sizeof(uint32_t);
    static const int kRegTimerInitCnt = 0x380 / sizeof(uint32_t);
    static const int kRegTimerCurCnt = 0x390 / sizeof(uint32_t);
    static const int kRegDivConfig = 0x3E0 / sizeof(uint32_t);

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

    // see intel64 manual vol3 Figure 10-14 (Layout of the MSI Message Address Register)
    static const uint64_t kMsiAddrRegReserved = 0xFEE00000;
    static const int kMsiAddrRegDestOffset = 12;

    // see intel MPspec Appendix B.5
    static const uint32_t kIoRtc = 0x70;

    // write to 64-bit value to local APIC ICR (Interrupt Command Register)
    void WriteIcr(uint32_t hi, uint32_t lo);
    void Outb(int pin, uint8_t data) {
      asm volatile("outb %%al, %%dx"::"d"(pin), "a"(data));
    }
  };
  class Ioapic {
  public:
    void Setup();
    uint32_t Read(uint32_t index) {
      _reg[kIndex] = index;
      return _reg[kData];
    }
    void Write(uint32_t index, uint32_t data) {
      _reg[kIndex] = index;
      _reg[kData] = data;
    }
    void SetReg(uint32_t *reg) {
      kassert(_reg == nullptr);
      _reg = reg;
    }
    bool SetupInt(uint32_t irq, uint8_t lapicid, uint8_t vector) {
      kassert(irq <= this->GetMaxIntr());
      if ((Read(kRegRedTbl + 2 * irq) | kRegRedTblFlagMask) == 0) {
        return false;
      }
      Write(kRegRedTbl + 2 * irq + 1, lapicid << kRegRedTblOffsetDest);
      Write(kRegRedTbl + 2 * irq,
            kRegRedTblFlagValueDeliveryLow |
            kRegRedTblFlagDestModePhys |
            kRegRedTblFlagTriggerModeEdge |
            vector);
      return true;
    }
    static const int kIrqKeyboard = 1;
  private:
    uint32_t GetMaxIntr() {
      // see IOAPIC manual 3.2.2 (IOAPIC Version Register)
      return (Read(kRegVer) >> 16) & 0xff;
    }
    uint32_t *_reg = nullptr;
    
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
  };

  ApicCtrl() {}
  void Setup();

  void SetMADT(MADT *table) {
    _madt = table;
  }

  void StartAPs();

  void BootBSP() {
    _lapic.Setup();
  }

  void BootAP() {
    __atomic_store_n(&_started, true, __ATOMIC_RELEASE);
    _lapic.Setup();
  }
  volatile uint8_t GetApicIdFromCpuId(int cpuid) {
    return _lapic.GetApicIdFromCpuId(cpuid);
  }
  volatile bool IsBootupAll() {
    return _all_bootup;
  }
  // cpu_ctrlを通して呼びだす事
  int GetHowManyCpus() {
    return _lapic._ncpu;
  }
  bool SetupIoInt(uint32_t irq, uint8_t lapicid, uint8_t vector) {
    kassert(vector >= 32);
    return _ioapic.SetupInt(irq, lapicid, vector);
  }
  void EnableInt() {
    _lapic.EnableInt();
  }
  // 割り込みを無効化した上で呼び出す事
  // 戻り値：
  // true 割り込みが有効化されているのを無効化した
  // false 元々無効化されていた
  bool DisableInt() {
    return _lapic.DisableInt();
  }
  void IsIntEnable() {
    _lapic.IsIntEnable();
  }
  void SendEoi() {
    _lapic.SendEoi();
  }

  void SendIpi(uint8_t destid) {
    _lapic.SendIpi(destid);
  }

  void SetupTimer(int interval) {
    _lapic.SetupTimer(interval);
  }

  void StartTimer() {
    _lapic.StartTimer();
  }

  void StopTimer() {
    _lapic.StopTimer();
  }

protected:
  volatile bool _started = false;
  volatile bool _all_bootup = false;

private:
  static void TmrCallback(Regs *rs, void *arg) {
  }
  static void IpiCallback(Regs *rs, void *arg) {
  }

  Lapic _lapic;
  Ioapic _ioapic;
  MADT *_madt = nullptr;

  static const uint32_t kMadtFlagLapicEnable = 1;
};
#else
#include <thread.h>
#endif // !__UNIT_TEST__

#endif /* __RAPH_KERNEL_APIC_H__ */
