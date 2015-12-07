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

#include <assert.h>
#include "acpi.h"
#include "apic.h"

void ApicCtrl::Setup() {
  if (_madt) {
    int ncpu = 0;
    for(uint32_t offset = 0; offset < _madt->header.Length - sizeof(MADT);) {
      virt_addr ptr = ptr2virtaddr(_madt->table) + offset;
      MADTSt *madtSt = reinterpret_cast<MADTSt *>(ptr);
      switch (madtSt->type) {
      case MADTStType::kLocalAPIC:
	{
	  MADTStLAPIC *madtStLAPIC = reinterpret_cast<MADTStLAPIC *>(ptr);
	  _lapic.SetCtrlAddr(reinterpret_cast<uint32_t *>(p2v(_madt->lapicCtrlAddr)));
	  ncpu++;
	}
	break;
      case MADTStType::kIOAPIC:
	{
	  MADTStIOAPIC *madtStIOAPIC = reinterpret_cast<MADTStIOAPIC *>(ptr);
	  _ioapic.SetReg(reinterpret_cast<uint32_t *>(p2v(madtStIOAPIC->ioapicAddr)));
	}
	break;
      default:
	break;
      }
      offset += madtSt->length;
    }
    _lapic.Setup();
    _ioapic.Setup();
  }
}

void ApicCtrl::Lapic::Setup() {
  if (_ctrlAddr == nullptr) {
    return;
  }
  
  uint32_t msr;
  // check if local apic enabled
  // see intel64 manual vol3 10.4.3 (Enabling or Disabling the Local APIC)
  asm volatile("rdmsr;":"=a"(msr):"c"(kIa32ApicBaseMsr));
  if (!(msr & kApicGlobalEnableFlag)) {
    return;
  }

  _ctrlAddr[kRegSvr] = kRegSvrApicEnableFlag | (32 + 31); // TODO

  // setup timer
  _ctrlAddr[kRegDivConfig] = kDivVal1;
  _ctrlAddr[kRegTimerInitCnt] = 1448895600;

  // disable all local interrupt sources
  _ctrlAddr[kRegLvtTimer] = kRegTimerPeriodic | (32 + 10);
  // TODO : check APIC version before mask tsensor & pcnt
  _ctrlAddr[kRegLvtThermalSensor] = kRegLvtMask;
  _ctrlAddr[kRegLvtPerformanceCnt] = kRegLvtMask;
  _ctrlAddr[kRegLvtLint0] = kRegLvtMask;
  _ctrlAddr[kRegLvtLint1] = kRegLvtMask;
  _ctrlAddr[kRegLvtErr] = 32 + 19; // TODO

  
}

void ApicCtrl::Ioapic::Setup() {
  if (_reg == nullptr) {
    return;
  }

  uint32_t intr = GetMaxIntr();
  // disable all external I/O interrupts
  for (uint32_t i = 0; i < intr; i++) {
    Write(kRegRedTbl + 2 * i, kRegRedTblMask);
    Write(kRegRedTbl + 2 * i + 1, 0);
  }
}
