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

#include "hpet.h"
#include <idt.h>
#include <global.h>

bool Hpet::SetupSub() {
  _dt = acpi_ctrl->GetHPETDT();
  if (_dt == nullptr) {
    return false;
  }
  phys_addr pbase = _dt->BaseAddr;
  _reg = reinterpret_cast<uint64_t *>(p2v(pbase));

  _cnt_clk_period = ((_reg[kRegGenCap] & kRegGenCapMaskMainCntPeriod) >>
                     kRegGenCapOffsetMainCntPeriod) /
                    (1000 * 1000);

  _table_num = ((_reg[kRegGenCap] & kRegGenCapMaskNumTimer) >>
                kRegGenCapOffsetNumTimer) +
               1;

  if (_table_num == 0) {
    kernel_panic("hpet", "not enough hpet timer");
  }

  // disable all timer
  for (int i = 0; i < _table_num; i++) {
    this->Disable(i);
  }

  // Enable Timer
  _reg[kRegGenConfig] |= kRegGenConfigFlagEnable;
  if ((_reg[kRegGenCap] & kRegGenCapLegacyRoute) != 0) {
    _reg[kRegGenConfig] |= kRegGenConfigFlagLegacy;
  }
  return true;
}

void Hpet::SetPeriodicTimer(CpuId cpuid, uint64_t cnt, int_callback func) {
  uint64_t cnt_val = cnt / _cnt_clk_period;
  int id = 0;
  uint64_t config =
      kRegTmrConfigCapBaseFlagMode32 | kRegTmrConfigCapBaseFlagTypeNonPer;
  config &= ~kRegTmrConfigCapBaseMaskIntRoute;
  bool fsb_delivery = (_reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] &
                       kRegTmrConfigCapBaseFlagFsbIntDel) != 0;
  bool can_periodic = (_reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] &
                       kRegTmrConfigCapBaseFlagPerCap) != 0;

  if (!can_periodic) {
    return;
  }
  config |=
      (kRegTmrConfigCapBaseFlagTypePer | kRegTmrConfigCapBaseFlagValueSet);

  if (fsb_delivery) {
    config |=
        kRegTmrConfigCapBaseFlagFsbEnable | kRegTmrConfigCapBaseFlagIntTypeEdge;
    SetFsb(cpuid, id, func);
  } else {
    int pin = SetLegacyInterrupt(cpuid, id, func);
    config |= (pin << kRegTmrConfigCapBaseOffsetIntRoute) |
              kRegTmrConfigCapBaseFlagIntTypeLevel |
              kRegTmrConfigCapBaseFlagIntEnable;
  }

  _reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] = config;
  _reg[GetRegTmrOfN(id, kBaseRegTmrCmp)] = ReadMainCnt() + cnt_val;
  _reg[GetRegTmrOfN(id, kBaseRegTmrCmp)] = cnt_val;
}

void Hpet::SetOneShotTimer(CpuId cpuid, uint64_t cnt, int_callback func) {
  uint64_t cnt_val = cnt / _cnt_clk_period;
  int id = 0;
  uint64_t config =
      kRegTmrConfigCapBaseFlagMode32 | kRegTmrConfigCapBaseFlagTypeNonPer;
  config &= ~kRegTmrConfigCapBaseMaskIntRoute;
  bool fsb_delivery = (_reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] &
                       kRegTmrConfigCapBaseFlagFsbIntDel) != 0;

  if (fsb_delivery) {
    config |=
        kRegTmrConfigCapBaseFlagFsbEnable | kRegTmrConfigCapBaseFlagIntTypeEdge;
    SetFsb(cpuid, id, func);
  } else {
    int pin = SetLegacyInterrupt(cpuid, id, func);
    config |= (pin << kRegTmrConfigCapBaseOffsetIntRoute) |
              kRegTmrConfigCapBaseFlagIntTypeLevel |
              kRegTmrConfigCapBaseFlagIntEnable;
  }

  _reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] = config;
  _reg[GetRegTmrOfN(id, kBaseRegTmrCmp)] = cnt_val;
}

void Hpet::SetFsb(CpuId cpuid, int id, int_callback func) {
  int vector = idt->SetIntCallback(cpuid, func, reinterpret_cast<void *>(this),
                                   Idt::EoiType::kLapic);
  _reg[kBaseRegFsbIntRoute] =
      (ApicCtrl::GetMsiAddr(apic_ctrl->GetApicIdFromCpuId(cpuid)) << 32) |
      ApicCtrl::GetMsiData(vector);
}

int Hpet::SetLegacyInterrupt(CpuId cpuid, int id, int_callback func) {
  int pin = -1;
  if (_reg[kRegGenConfig] & kRegGenConfigFlagLegacy) {
    if (id == 0) {
      pin = 2;
    } else if (id == 1) {
      pin = 8;
    }
  }
  if (pin == -1) {
    uint32_t routecap = (_reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] &
                         kRegTmrConfigCapBaseMaskIntRouteCap) >>
                        32;
    for (int i = 0; i < 32; i++) {
      if ((routecap & (1 << i)) != 0) {
        pin = i;
        break;
      }
    }
  }
  if (pin == -1) {
    kernel_panic("hpet", "unknown error");
  }
  kassert(apic_ctrl != nullptr);
  int vector = idt->SetIntCallback(cpuid, func, reinterpret_cast<void *>(this),
                                   Idt::EoiType::kIoapic);
  kassert(apic_ctrl->SetupIoInt(pin, apic_ctrl->GetApicIdFromCpuId(cpuid),
                                vector, false, true));
  return pin;
}

void volatile Hpet::ResetMainCnt() {
  _reg[kRegGenConfig] |= ~kRegGenConfigFlagEnable;
  _reg[kRegMainCnt] = 0x0;
  _reg[kRegGenConfig] &= kRegGenConfigFlagEnable;
}

void Hpet::Handle(Regs *rs) {
  int id = 0;
  bool fsb_delivery = (this->_reg[GetRegTmrOfN(id, kBaseRegTmrConfigCap)] &
                       kRegTmrConfigCapBaseFlagFsbIntDel) != 0;
  if (!fsb_delivery) {
    this->_reg[0x20 / sizeof(uint64_t)] |= 1;
  }
}
