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
#include <raph_acpi.h>
#include <apic.h>
#include <timer.h>
#include <global.h>
#include <idt.h>
#include <cpu.h>
#include <mem/kstack.h>
#include <tty.h>

extern "C" void entryothers();
extern "C" void entry();

void ApicCtrl::Init() {
  // see intel64 manual vol3 10.12.1 (Detecting and Enabling x2APIC Mode)
  uint32_t feature;
  get_cpuid(1, 0, "c", feature);
  if ((feature & (1 << 21)) != 0) {
    // enable x2APIC
    uint64_t msr = x86::rdmsr(Lapic::kIa32ApicBaseMsr);
    if (!(msr & Lapic::kApicGlobalEnableFlag)) {
      kernel_panic("ApicCtrl", "unable to initialize local APIC");
    }
    msr |= Lapic::kApicX2ApicEnableFlag;
    x86::wrmsr(Lapic::kIa32ApicBaseMsr, msr);
    _lapic = new LapicX2;
  } else {
    _lapic = new LapicX;
  }
}

void ApicCtrl::Setup() {
  kassert(timer->DidSetup());
  kassert(_madt != nullptr);
  int ncpu = 0;
  int ioapic_num = 0;
  for(uint32_t offset = 0; offset < _madt->header.Length - sizeof(MADT);) {
    virt_addr ptr = ptr2virtaddr(_madt->table) + offset;
    MADTSt *madtSt = reinterpret_cast<MADTSt *>(ptr);
    switch (madtSt->type) {
    case MADTStType::kLocalAPIC:
      {
        MADTStLAPIC *madtStLAPIC = reinterpret_cast<MADTStLAPIC *>(ptr);
        if ((madtStLAPIC->flags & kMadtLapicFlagLapicEnable) == 0) {
          break;
        }
        ncpu++;
      }
      break;
    case MADTStType::kLocalX2Apic:
      {
        MadtStructX2Lapic *madt = reinterpret_cast<MadtStructX2Lapic *>(ptr);
        if ((madt->flags & kMadtLapicFlagLapicEnable) == 0) {
          break;
        }
        ncpu++;
      }
      break;
    case MADTStType::kIOAPIC:
      {
        ioapic_num++;
      }
    default:
      break;
    }
    offset += madtSt->length;
  }

  _lapic->_ncpu = ncpu;
  _lapic->_apic_info = new Lapic::ApicInfo[ncpu];

  if (ioapic_num == 0) {
    kernel_panic("ApicCtrl", "No I/O APIC controller");
  }
  _ioapic._controller_num = ioapic_num;
  _ioapic._controller = new Ioapic::Controller[ioapic_num];

  ncpu = 0;
  ioapic_num = 0;
  for(uint32_t offset = 0; offset < _madt->header.Length - sizeof(MADT);) {
    virt_addr ptr = ptr2virtaddr(_madt->table) + offset;
    MADTSt *madtSt = reinterpret_cast<MADTSt *>(ptr);
    switch (madtSt->type) {
    case MADTStType::kLocalAPIC:
      {
        MADTStLAPIC *madtStLAPIC = reinterpret_cast<MADTStLAPIC *>(ptr);
        if ((madtStLAPIC->flags & kMadtLapicFlagLapicEnable) == 0) {
          break;
        }
        if (_madt->lapicCtrlAddr != LapicX::kMmioBaseAddr) {
          kernel_panic("lapic", "unexpected local APIC control address");
        }
        _lapic->_apic_info[ncpu].id = madtStLAPIC->apicId;
        ncpu++;
      }
      break;
    case MADTStType::kLocalX2Apic:
      {
        MadtStructX2Lapic *madt = reinterpret_cast<MadtStructX2Lapic *>(ptr);
        if ((madt->flags & kMadtLapicFlagLapicEnable) == 0) {
          break;
        }
        _lapic->_apic_info[ncpu].id = madt->apic_id;
        ncpu++;
      }
      break;
    case MADTStType::kIOAPIC:
      {
        _ioapic._controller[ioapic_num].Setup(reinterpret_cast<MADTStIOAPIC *>(ptr));
        ioapic_num++;
      }
      break;
    default:
      break;
    }
    offset += madtSt->length;
  }

  _setup = true;
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
  for(int i = 0; i < _lapic->_ncpu; i++) {
    // skip BSP
    if(_lapic->_apic_info[i].id == _lapic->GetApicId()) {
      continue;
    }
    _started = false;
    CpuId cpuid(i);
    *stack_of_others = KernelStackCtrl::GetCtrl().AllocThreadStack(cpuid);
    _lapic->Start(_lapic->_apic_info[i].id, reinterpret_cast<uint64_t>(entryothers));
    while(!__atomic_load_n(&_started, __ATOMIC_ACQUIRE)) {}
  }
  _all_bootup = true;
  _ioapic.Setup();
}

void ApicCtrl::PicSpuriousCallback(Regs *rs, void *arg) {
  gtty->Printf("[APIC] info: spurious 8259A interrupt\n");
}

void ApicCtrl::PicUnknownCallback(Regs *rs, void *arg) {
  gtty->Printf("[APIC] info: unknown 8259A interrupt\n");
}

void ApicCtrl::Lapic::Setup() {
  kassert(cpu_ctrl->GetCpuId().GetRawId() == GetCpuId());
  kassert(cpu_ctrl->GetCpuId().GetApicId() == GetApicId());
  
  // check if local apic enabled
  // see intel64 manual vol3 10.4.3 (Enabling or Disabling the Local APIC)
  uint64_t msr = x86::rdmsr(Lapic::kIa32ApicBaseMsr);
  if (!(msr & kApicGlobalEnableFlag)) {
    kernel_panic("ApicCtrl", "unable to initialize local APIC");
  }

  // disable all local interrupt sources
  WriteReg(RegisterOffset::kLvtTimer, kRegLvtMask | kRegTimerPeriodic);
  // TODO : check APIC version before mask tsensor & pcnt
  WriteReg(RegisterOffset::kLvtCmci, kRegLvtMask);
  WriteReg(RegisterOffset::kLvtThermalSensor, kRegLvtMask);
  WriteReg(RegisterOffset::kLvtPerformanceCnt, kRegLvtMask);
  WriteReg(RegisterOffset::kLvtLint0, kRegLvtMask);
  WriteReg(RegisterOffset::kLvtLint1, kRegLvtMask);
  // WriteReg(RegisterOffset::kLvtLint0, kDeliverModeFixed | kDeliverModeExtint);
  // WriteReg(RegisterOffset::kLvtLint1, kDeliverModeFixed | kDeliverModeNmi);
  WriteReg(RegisterOffset::kLvtErr, kDeliverModeFixed | Idt::ReservedIntVector::kError);

  kassert(idt != nullptr);
  idt->SetExceptionCallback(cpu_ctrl->GetCpuId(), Idt::ReservedIntVector::kIpi, IpiCallback, nullptr, Idt::EoiType::kLapic);
  idt->SetExceptionCallback(cpu_ctrl->GetCpuId(), Idt::ReservedIntVector::k8259Spurious1, PicSpuriousCallback, nullptr, Idt::EoiType::kNone);
  idt->SetExceptionCallback(cpu_ctrl->GetCpuId(), Idt::ReservedIntVector::k8259Spurious2, PicSpuriousCallback, nullptr, Idt::EoiType::kNone);
  idt->SetExceptionCallback(cpu_ctrl->GetCpuId(), Idt::ReservedIntVector::k8259Rtc, PicUnknownCallback, nullptr, Idt::EoiType::kNone);

  WriteReg(RegisterOffset::kSvr, kRegSvrApicEnableFlag | Idt::ReservedIntVector::kError);
}

void ApicCtrl::Lapic::Start(uint32_t apicId, uint64_t entryPoint) {
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
  WriteIcr(apicId, kDeliverModeInit | kRegIcrTriggerModeLevel | kRegIcrLevelAssert);
  timer->BusyUwait(200);
  WriteIcr(apicId, kDeliverModeInit | kRegIcrTriggerModeLevel);
  timer->BusyUwait(100);

  // Application Processor Setup (defined in mp spec Appendix B.4)
  // see mp spec Appendix B.4.2
  for(int i = 0; i < 2; i++) {
    WriteIcr(apicId, kDeliverModeStartup | ((entryPoint >> 12) & 0xff));
    timer->BusyUwait(200);
  }
}

void ApicCtrl::Lapic::SetupTimer(int interval) {
  WriteReg(RegisterOffset::kDivConfig, kDivVal16);
  WriteReg(RegisterOffset::kTimerInitCnt, 0xFFFFFFFF);
  uint64_t timer1 = timer->ReadMainCnt();
  while(true) {
    volatile uint32_t cur = ReadReg(RegisterOffset::kTimerCurCnt);
    if (cur < 0xFFF00000) {
      break;
    }
  }
  uint64_t timer2 = timer->ReadMainCnt();
  kassert((timer2 - timer1) > 0);
  uint32_t base_cnt = ((int64_t)interval * 1000 * 0xFFFFF) / ((timer2 - timer1) * timer->GetCntClkPeriod());
  kassert(base_cnt > 0);

  kassert(idt != nullptr);
  int vector = idt->SetIntCallback(cpu_ctrl->GetCpuId(), TmrCallback, nullptr, Idt::EoiType::kLapic);
  WriteReg(RegisterOffset::kTimerInitCnt, base_cnt);

  WriteReg(RegisterOffset::kLvtTimer, kRegLvtMask | kRegTimerPeriodic | vector);
}

void ApicCtrl::Lapic::SendIpi(uint32_t destid) {
  WriteIcr(destid, kDeliverModeFixed | kRegIcrTriggerModeLevel | kRegIcrDestShorthandNoShortHand | Idt::ReservedIntVector::kIpi);
}

void ApicCtrl::LapicX2::SetupAp() {
  // see intel64 manual vol3 10.12.1 (Detecting and Enabling x2APIC Mode)
  uint32_t feature;
  get_cpuid(1, 0, "c", feature);
  if ((feature & (1 << 21)) != 0) {
    // enable x2APIC
    uint64_t msr = x86::rdmsr(Lapic::kIa32ApicBaseMsr);
    if (!(msr & Lapic::kApicGlobalEnableFlag)) {
      kernel_panic("ApicCtrl", "unable to initialize local APIC");
    }
    msr |= Lapic::kApicX2ApicEnableFlag;
    x86::wrmsr(Lapic::kIa32ApicBaseMsr, msr);
  } else {
    kernel_panic("ApicCtrl", "unable to initialize local APIC");
  }
}

void ApicCtrl::Ioapic::Setup() {
  // setup 8259 PIC
  asm volatile("mov $0xff, %al; out %al, $0xa1; out %al, $0x21;");
  asm volatile("mov $0x11, %al; out %al, $0xa0;");
  asm volatile("out %%al, $0xa1;"::"a"(Idt::ReservedIntVector::k8259Spurious1 - 7));
  asm volatile("mov $0x4, %al; out %al, $0xa1;");
  asm volatile("mov $0x3, %al; out %al, $0xa1;");
  asm volatile("mov $0x11, %al; out %al, $0x20;");
  asm volatile("out %%al, $0x21;"::"a"(Idt::ReservedIntVector::k8259Spurious2 - 7));
  asm volatile("mov $0x2, %al; out %al, $0x21;");
  asm volatile("mov $0x3, %al; out %al, $0x21;");
  asm volatile("mov $0x68, %al; out %al, $0xa0");
  asm volatile("mov $0x0a, %al; out %al, $0xa0");
  asm volatile("mov $0x68, %al; out %al, $0x20");
  asm volatile("mov $0x0a, %al; out %al, $0x20");

  // disable all external I/O interrupts
  for (int i = 0; i < _controller_num; i++) {
    _controller[i].DisableInt();
  }
}

int ApicCtrl::Ioapic::GetControllerIndexFromIrq(uint32_t irq) {
  int controller_index = -1;
  for (int j = 0; j < _controller_num; j++) {
    if (_controller[j].int_base <= irq && irq <= _controller[j].int_base + _controller[j].int_max) {
      controller_index = j;
      break;
    }
  }
  if (controller_index == -1) {
    kernel_panic("APIC", "IRQ out of range");
  }
  return controller_index;
}

void ApicCtrl::Ioapic::Controller::Setup(MADTStIOAPIC *madt_struct_ioapic) {
  reg = reinterpret_cast<uint32_t *>(p2v(madt_struct_ioapic->ioapicAddr));
  int_base = madt_struct_ioapic->glblIntBase;
  // get maximum redirection entry
  // see IOAPIC manual 3.2.2 (IOAPIC Version Register)
  int_max = (Read(kRegVer) >> 16) & 0xff;
  _has_eoi = (Read(kRegVer) & 0xff) >= 0x20;
}

bool ApicCtrl::Ioapic::Controller::SetupInt(uint32_t irq, uint8_t lapicid, uint8_t vector, bool trigger_mode, bool polarity) {
  if ((Read(kRegRedTbl + 2 * irq) | kRegRedTblFlagMask) == 0) {
    return false;
  }
  if (IsIrqPending(irq)) {
    kernel_panic("APIC", "try to assign to a pending irq");
  }
  Write(kRegRedTbl + 2 * irq + 1, lapicid << kRegRedTblOffsetDest);
  Write(kRegRedTbl + 2 * irq,
        kRegRedTblFlagValueDeliveryFixed |
        kRegRedTblFlagDestModePhys |
        (trigger_mode ? kRegRedTblFlagTriggerModeEdge : kRegRedTblFlagTriggerModeLevel) |
        (polarity ? kRegRedTblFlagPolarityHighActive : kRegRedTblFlagPolarityLowActive) |
        vector);
  return true;
}
