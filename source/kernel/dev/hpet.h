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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 * see IA-PC HPET Specification
 *
 */

#pragma once

#include <stdint.h>
#include <raph.h>
#include <mem/physmem.h>
#include <raph_acpi.h>
#include <timer.h>
#include <apic.h>
#include <ptr.h>
#include <function.h>
#include <idt.h>

struct HPETDT {
  ACPISDTHeader header;
  uint32_t EventTimerBlockID;
  uint8_t BaseAddrSpaceID;
  uint8_t BaseAddrRegisterBitWidth;
  uint8_t BaseAddrRegisterBitOffset;
  uint8_t BaseAddrReserved;
  uint64_t BaseAddr;
  uint8_t SeqNum;
  uint16_t MinumClockTick;
  uint8_t PageProtectionAndOEMAttribute;
} __attribute__((packed));

class Hpet : public Timer {
 public:
  virtual bool SetupSub() override;
  virtual volatile uint64_t ReadMainCnt() override {
    if (!DidSetup()) {
      return 0;
    }
    return _reg[kRegMainCnt];
  }
  void volatile ResetMainCnt();

  // cnt - nano seconds
  virtual void SetPeriodicTimer(CpuId cpuid, uint64_t cnt,
                                int_callback func) override;
  virtual void SetOneShotTimer(CpuId cpuid, uint64_t cnt,
                               int_callback func) override;

  virtual void Start10msecPeriodicTimer() override {
    SetPeriodicTimer(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority),
                     1000 * 1000 * 10, Handle);
  }

 private:
  void Disable(int table) {
    _reg[GetRegTmrOfN(table, kBaseRegTmrConfigCap)] &=
        ~kRegTmrConfigCapBaseFlagIntEnable & ~kRegTmrConfigCapBaseFlagFsbEnable;
    _reg[kBaseRegFsbIntRoute] = 0;
  }
  static void Handle(Regs *rs, void *arg);

  void SetFsb(CpuId cpuid, int id, int_callback func);
  int SetLegacyInterrupt(CpuId cpuid, int id, int_callback func);

  HPETDT *_dt;
  uint64_t *_reg;

  int _table_num = 0;

  // see manual 2.3
  static const int kRegGenCap = 0x0 / sizeof(uint64_t);
  static const int kRegGenConfig = 0x10 / sizeof(uint64_t);
  static const int kRegMainCnt = 0xF0 / sizeof(uint64_t);
  static const int kBaseRegTmrConfigCap = 0x100 / sizeof(uint64_t);
  static const int kBaseRegTmrCmp = 0x108 / sizeof(uint64_t);
  static const int kBaseRegFsbIntRoute = 0x110 / sizeof(uint64_t);
  static const int GetRegTmrOfN(int n, int base) {
    return base + (n * 0x20) / sizeof(uint64_t);
  }

  // General Configuration Register Bit Definitions
  static const uint64_t kRegGenConfigFlagEnable = 1 << 0;
  static const uint64_t kRegGenConfigFlagLegacy = 1 << 1;

  static const int kRegGenCapOffsetMainCntPeriod = 32;
  static const uint64_t kRegGenCapMaskMainCntPeriod = 0xffffffff00000000;
  static const int kRegGenCapLegacyRoute = 1 << 15;
  static const int kRegGenCapOffsetNumTimer = 8;
  static const uint64_t kRegGenCapMaskNumTimer = 0x1F00;

  // Timer N Configuration and Capability Register Field Definitions
  static const uint64_t kRegTmrConfigCapBaseFlagIntTypeEdge = 0 << 1;
  static const uint64_t kRegTmrConfigCapBaseFlagIntTypeLevel = 1 << 1;
  static const uint64_t kRegTmrConfigCapBaseFlagIntEnable = 1 << 2;
  static const uint64_t kRegTmrConfigCapBaseFlagTypeNonPer = 0 << 3;
  static const uint64_t kRegTmrConfigCapBaseFlagTypePer = 1 << 3;
  static const uint64_t kRegTmrConfigCapBaseFlagPerCap = 1 << 4;
  static const uint64_t kRegTmrConfigCapBaseFlagSize32 = 0 << 5;
  static const uint64_t kRegTmrConfigCapBaseFlagSize64 = 1 << 5;
  static const uint64_t kRegTmrConfigCapBaseFlagValueSet = 1 << 6;
  static const uint64_t kRegTmrConfigCapBaseFlagMode32 = 1 << 8;
  static const int kRegTmrConfigCapBaseOffsetIntRoute = 9;
  static const uint64_t kRegTmrConfigCapBaseMaskIntRoute = 0x3E00;
  static const uint64_t kRegTmrConfigCapBaseFlagFsbEnable = 1 << 14;
  static const uint64_t kRegTmrConfigCapBaseFlagFsbIntDel = 1 << 15;
  static const uint64_t kRegTmrConfigCapBaseMaskIntRouteCap =
      0xffffffff00000000;

  static const bool kTimerOneShot = true;
  static const bool kTimerPeriodic = false;
};
