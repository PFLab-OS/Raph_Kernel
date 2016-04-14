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
#include <mem/virtmem.h>
#include <acpi.h>
#include <apic.h>
#include <timer.h>
#include <global.h>
#include <idt.h>

extern "C" void entryothers();
extern "C" void entry();

void ApicCtrl::Setup() {
  kassert(_madt != nullptr);
  int ncpu = 0;
  for(uint32_t offset = 0; offset < _madt->header.Length - sizeof(MADT);) {
    virt_addr ptr = ptr2virtaddr(_madt->table) + offset;
    MADTSt *madtSt = reinterpret_cast<MADTSt *>(ptr);
    switch (madtSt->type) {
    case MADTStType::kLocalAPIC:
      {
        MADTStLAPIC *madtStLAPIC = reinterpret_cast<MADTStLAPIC *>(ptr);
        _lapic.SetCtrlAddr(reinterpret_cast<uint32_t *>(p2v(_madt->lapicCtrlAddr)));
        if ((madtStLAPIC->flags & kMadtFlagLapicEnable) == 0) {
          break;
        }
        _lapic._apicIds[ncpu] = madtStLAPIC->apicId;
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
  _ioapic.Setup();
  _lapic._ncpu = ncpu;
}

extern uint64_t boot16_start;
extern uint64_t boot16_end;
extern virt_addr stack_of_others[1];

void ApicCtrl::StartAPs() {
  uint64_t *src = &boot16_start;
  uint64_t *dest = reinterpret_cast<uint64_t *>(0x8000);
  // TODO : replace this code with memcpy
  for (; src < &boot16_end; src++, dest++) {
    *dest = *src;
  }
  for(int i = 0; i < _lapic._ncpu; i++) {
    // skip BSP
    if(_lapic._apicIds[i] == _lapic.GetApicId()) {
      continue;
    }
    _started = false;
    static const int stack_size = 4096*5;
    *stack_of_others = ((virtmem_ctrl->Alloc(stack_size + 8) + stack_size + 7) / 8) * 8 - 8;
    _lapic.Start(_lapic._apicIds[i], reinterpret_cast<uint64_t>(entryothers));
    while(!_started) {}
  }
  _all_bootup = true;
}

void ApicCtrl::Lapic::Setup() {
  if (_ctrlAddr == nullptr) {
    return;
  }
  
  uint32_t msr;
  // check if local apic enabled
  // see intel64 manual vol3 10.4.3 (Enabling or Disabling the Local APIC)
  asm volatile("rdmsr":"=a"(msr):"c"(kIa32ApicBaseMsr));
  if (!(msr & kApicGlobalEnableFlag)) {
    return;
  }

  _ctrlAddr[kRegSvr] = kRegSvrApicEnableFlag | Idt::ReservedIntVector::kSpurious;

  // disable all local interrupt sources
  _ctrlAddr[kRegLvtTimer] = kRegLvtMask | kRegTimerPeriodic;
  // TODO : check APIC version before mask tsensor & pcnt
  _ctrlAddr[kRegLvtCmci] = kRegLvtMask;
  _ctrlAddr[kRegLvtThermalSensor] = kRegLvtMask;
  _ctrlAddr[kRegLvtPerformanceCnt] = kRegLvtMask;
  _ctrlAddr[kRegLvtLint0] = kRegLvtMask;
  _ctrlAddr[kRegLvtLint1] = kRegLvtMask;
  _ctrlAddr[kRegLvtErr] = kRegLvtMask | Idt::ReservedIntVector::kLapicErr;

  kassert(idt != nullptr);
  idt->SetIntCallback(Idt::ReservedIntVector::kIpi, IpiCallback);
}

void ApicCtrl::Lapic::Start(uint8_t apicId, uint64_t entryPoint) {
  // set AP shutdown handling
  // see mp spec Appendix B.5
  uint16_t *warmResetVector;
  Outb(kIoRtc, 0xf); // 0xf = shutdown code
  Outb(kIoRtc + 1, 0x0a);
  warmResetVector = reinterpret_cast<uint16_t *>(p2v((0x40 << 4 | 0x67)));
  warmResetVector[0] = 0;
  warmResetVector[1] = (entryPoint >> 4) & 0xffff;

  // Universal startup algorithm
  // see mp spec Appendix B.4.1
  WriteIcr(apicId << 24, kDeliverModeInit | kRegIcrTriggerModeLevel | kRegIcrLevelAssert);
  timer->BusyUwait(200);
  WriteIcr(apicId << 24, kDeliverModeInit | kRegIcrTriggerModeLevel);
  timer->BusyUwait(100);

  // Application Processor Setup (defined in mp spec Appendix B.4)
  // see mp spec Appendix B.4.2
  for(int i = 0; i < 2; i++) {
    WriteIcr(apicId << 24, kDeliverModeStartup | ((entryPoint >> 12) & 0xff));
    timer->BusyUwait(200);
  }
}

void ApicCtrl::Lapic::WriteIcr(uint32_t hi, uint32_t lo) {
  _ctrlAddr[kRegIcrHi] = hi;
  // wait for write to finish, by reading
  GetApicId();
  _ctrlAddr[kRegIcrLo] = lo;
  GetApicId();
}

void ApicCtrl::Lapic::SetupTimer(uint32_t irq) {
  _ctrlAddr[kRegDivConfig] = kDivVal16;
  _ctrlAddr[kRegTimerInitCnt] = 0xFFFFFFFF;
  uint64_t timer1 = timer->ReadMainCnt();
  while(true) {
    volatile uint32_t cur = _ctrlAddr[kRegTimerCurCnt];
    if (cur < 0xFFF00000) {
      break;
    }
  }
  uint64_t timer2 = timer->ReadMainCnt();
  kassert((timer2 - timer1) > 0);
  uint32_t base_cnt = 0xFFFFF / ((timer2 - timer1) * timer->GetCntClkPeriod() / (10 * 1000));
  kassert(base_cnt > 0);

  kassert(idt != nullptr);
  idt->SetIntCallback(irq, TmrCallback);
  _ctrlAddr[kRegTimerInitCnt] = base_cnt;
      
  _ctrlAddr[kRegLvtTimer] = kRegLvtMask | kRegTimerPeriodic | irq;
}

void ApicCtrl::Lapic::SendIpi(uint8_t destid) {
  WriteIcr(destid << 24, kDeliverModeFixed | kRegIcrTriggerModeLevel | kRegIcrDestShorthandNoShortHand | Idt::ReservedIntVector::kIpi);
}

void ApicCtrl::Ioapic::Setup() {
  if (_reg == nullptr) {
    return;
  }

  // disable 8259 PIC
  asm volatile("mov $0xff, %al; out %al, $0xa1; out %al, $0x21;");

  uint32_t intr = this->GetMaxIntr();
  // disable all external I/O interrupts
  for (uint32_t i = 0; i <= intr; i++) {
    this->Write(kRegRedTbl + 2 * i, kRegRedTblFlagMask);
    this->Write(kRegRedTbl + 2 * i + 1, 0);
  }
}
