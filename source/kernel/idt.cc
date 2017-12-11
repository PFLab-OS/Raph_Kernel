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

#include <idt.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <apic.h>
#include <mem/kstack.h>
#include <gdt.h>
#include <global.h>
#include <tty.h>

namespace C {
extern "C" void handle_int(Regs *rs) {
  //TODO 例外処理中のフラグを立て、SpinLock内では弾く
  bool iflag = disable_interrupt();
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  idt->_handling_cnt[cpuid]++;
  if (idt->_callback[cpuid][rs->n].callback == nullptr) {
    if (gtty != nullptr) {
      gtty->DisablePrint();
      gtty->ErrPrintf("[kernel] error: unimplemented interrupt %d at cpuid: %d\nrip: %llx rbp: %llx\n", rs->n, cpuid, rs->rip, rs->rbp);
      show_backtrace(reinterpret_cast<size_t *>(rs->rbp));
    }
    while(true) {
      asm volatile("cli; hlt;");
    }
  } else {
    idt->_callback[cpuid][rs->n].callback(rs, idt->_callback[cpuid][rs->n].arg);
    switch(idt->_callback[cpuid][rs->n].eoi) {
    case Idt::EoiType::kNone:
      break;
    case Idt::EoiType::kLapic:
      apic_ctrl->SendEoi();
      break;
    case Idt::EoiType::kIoapic:
      apic_ctrl->SendEoi(rs->n);
      break;
    default:
      assert(false);
      break;
    }
  }
  idt->_handling_cnt[cpuid]--;
  enable_interrupt(iflag);
}
}

extern idt_callback vectors[256];
extern idt_callback idt_vectors[256];

struct idt_entity {
  uint32_t entry[4];
} __attribute__((aligned(8))) idt_def[256];

void Idt::SetupGeneric() {
  for (int i = 0; i < 256; i++) {
    uint8_t ist;
    switch (i) {
    case 8:
      ist = 1;
      break;
    case 2:
      ist = 2;
      break;
    case 1:
    case 3:
      ist = 3;
      break;
    case 18:
      ist = 4;
      break;
    default:
      ist = 5;
      break;
    };
    SetGate(idt_vectors[i], i, 0, false, ist);
  }
  virt_addr idt_addr = reinterpret_cast<virt_addr>(idt_def);
  _idtr[0] = 8*256-1;
  _idtr[1] = idt_addr & 0xffff;
  _idtr[2] = (idt_addr >> 16) & 0xffff;
  _idtr[3] = (idt_addr >> 32) & 0xffff;
  _idtr[4] = (idt_addr >> 48) & 0xffff;
  kassert(kernel_virtmem_ctrl != nullptr);
  kassert(apic_ctrl != nullptr);
  _callback = reinterpret_cast<IntCallback **>(kernel_virtmem_ctrl->Alloc(sizeof(IntCallback *) * apic_ctrl->GetHowManyCpus()));
  _handling_cnt = reinterpret_cast<int *>(kernel_virtmem_ctrl->Alloc(sizeof(int) * apic_ctrl->GetHowManyCpus()));
  for (int i = 0; i < apic_ctrl->GetHowManyCpus(); i++) {
    _callback[i] = reinterpret_cast<IntCallback *>(kernel_virtmem_ctrl->Alloc(sizeof(IntCallback) * 256));
    _handling_cnt[i] = 0;
    for (int j = 0; j < 256; j++) {
      _callback[i][j].callback = nullptr;
      _callback[i][j].eoi = EoiType::kNone;
    }
  }
  _is_gen_initialized = true;
  for (int i = 0; i < apic_ctrl->GetHowManyCpus(); i++) {
    CpuId cpuid(i);
    SetExceptionCallback(cpuid, 13, HandleGeneralProtectionFault, nullptr, Idt::EoiType::kNone);
    SetExceptionCallback(cpuid, 14, HandlePageFault, nullptr, Idt::EoiType::kNone);
  }
}

void Idt::SetupProc() {
  asm volatile ("lidt (%0)"::"r"(_idtr));
  asm volatile ("sti;");
}

void Idt::SetGate(idt_callback gate, int vector, uint8_t dpl, bool trap, uint8_t ist) {
  virt_addr vaddr = reinterpret_cast<virt_addr>(gate);
  uint32_t type = trap ? 0xF : 0xE;
  idt_def[vector].entry[0] = (vaddr & 0xFFFF) | (KERNEL_CS << 16);
  idt_def[vector].entry[1] = (vaddr & 0xFFFF0000) | (type << 8) | ((dpl & 0x3) << 13) | kIdtPresent | ist;
  idt_def[vector].entry[2] = vaddr >> 32;
  idt_def[vector].entry[3] = 0;
}

int Idt::SetIntCallback(CpuId cpuid, int_callback callback, void *arg, EoiType eoi) {
  kassert(_is_gen_initialized);
  Locker locker(_lock);
  int raw_cpu_id = cpuid.GetRawId();
  for(int vector = 64; vector < 256; vector++) {
    if (_callback[raw_cpu_id][vector].callback == nullptr) {
      _callback[raw_cpu_id][vector].callback = callback;
      _callback[raw_cpu_id][vector].arg = arg;
      _callback[raw_cpu_id][vector].eoi = eoi;
      return vector;
    }
  }
  return ReservedIntVector::kError;
}

int Idt::SetIntCallback(CpuId cpuid, int_callback *callback, void **arg, int range, EoiType eoi) {
  kassert(_is_gen_initialized);
  int _range = 1;
  while(_range < range) {
    _range *= 2;
  }
  if (range != _range) {
    return ReservedIntVector::kError;
  }
  Locker locker(_lock);
  int vector = range > 64 ? range : 64;
  int raw_cpu_id = cpuid.GetRawId();
  for(; vector < 256; vector += range) {
    int i;
    for (i = 0; i < range; i++) {
      if (_callback[raw_cpu_id][vector + i].callback != nullptr) {
        break;
      }
    }
    if (i != range) {
      continue;
    }
    for (i = 0; i < range; i++) {
      _callback[raw_cpu_id][vector + i].callback = callback[i];
      _callback[raw_cpu_id][vector + i].arg = arg[i];
      _callback[raw_cpu_id][vector + i].eoi = eoi;
    }
    return vector;
  }
  return ReservedIntVector::kError;
}

static inline void ShowPagingEntry(const char *str, uint64_t entry) {
  gtty->ErrPrintf("\n%s: 0x%llx P:%d R/W: %d U/S: %d", str, entry, (entry & 1) != 0, (entry & 2) != 0, (entry & 4) != 0);
}

void Idt::SetExceptionCallback(CpuId cpuid, int vector, int_callback callback, void *arg, EoiType eoi) {
  kassert(_is_gen_initialized);
  kassert(vector < 64 && vector >= 1);
  Locker locker(_lock);
  int raw_cpu_id = cpuid.GetRawId();
  _callback[raw_cpu_id][vector].callback = callback;
  _callback[raw_cpu_id][vector].arg = arg;
  _callback[raw_cpu_id][vector].eoi = eoi;
}

void Idt::HandlePageFault(Regs *rs, void *arg) {
  if (gtty != nullptr) {
    uint64_t addr;
    asm volatile("movq %%cr2, %0;":"=r"(addr));
    int cpuid = cpu_ctrl->GetCpuId().GetRawId();
    const uint64_t ECodePBit = 0x01;	// 0: not present, 1: present
    const uint64_t ECodeRWBit = 0x02;	// 0: read, 1: write
    const uint64_t ECodeSUBit = 0x03;	// 0: from privileged mode, 1: from user mode
    gtty->DisablePrint();
    gtty->ErrPrintf("\nUnexpected page fault (INT 0x%x) occured at cpuid %d!", rs->n, cpuid);
    gtty->ErrPrintf("\nwhile trying %s access on addr: 0x%llx (%spresent)",
      rs->ecode & ECodeRWBit ? "write" : "read", addr, rs->ecode & ECodePBit ? "" : "not ");
    gtty->ErrPrintf("\nfrom %s mode", (rs->ecode & ECodeSUBit) ? "user" : "kernel");
    gtty->ErrPrintf("\nrip: %llx rsp: %llx rbp: %llx", rs->rip, rs->rsp, rs->rbp);
    gtty->ErrPrintf("\nrax: %llx rbx:%llx rcx:%llx rdx:%llx", rs->rax, rs->rbx, rs->rcx, rs->rdx);
    gtty->ErrPrintf("\nrsi:%llx r13: %llx cs:%x ss:%x ecode:%llx", rs->rsi, rs->r13, rs->cs, rs->ss, rs->ecode);
    uint64_t pml4e, pdpte, pde, pte;
    kernel_virtmem_ctrl->paging_ctrl->GetTranslationEntries(addr, &pml4e, &pdpte, &pde, &pte);
    ShowPagingEntry("PML4E", pml4e);
    ShowPagingEntry("PDPTE", pdpte);
    ShowPagingEntry("PDE  ", pde);
    ShowPagingEntry("PTE  ", pte);
    gtty->ErrPrintf("\n");
    show_backtrace(reinterpret_cast<size_t *>(rs->rbp));
  }
  while(true){
    asm volatile("cli;hlt");
  }
}

void Idt::HandleGeneralProtectionFault(Regs *rs, void *arg) {
  if (gtty != nullptr) {
    int cpuid = cpu_ctrl->GetCpuId().GetRawId();
    gtty->DisablePrint();
    gtty->ErrPrintf("\nGeneral Protection fault (INT 0x%x) occured at cpuid %d!", rs->n, cpuid);
    gtty->ErrPrintf("\nrip: %llx rsp:%llx rbp: %llx", rs->rip, rs->rsp, rs->rbp);
    gtty->ErrPrintf("\nrax: %llx rbx:%llx rcx:%llx rdx:%llx", rs->rax, rs->rbx, rs->rcx, rs->rdx);
    gtty->ErrPrintf("\nrsi:%llx r13: %llx cs:%x ss:%x ecode:%llx", rs->rsi, rs->r13, rs->cs, rs->ss, rs->ecode);
    gtty->ErrPrintf("\n");
    show_backtrace(reinterpret_cast<size_t *>(rs->rbp));
  }
  while(true){
    asm volatile("cli;hlt");
  }
}
