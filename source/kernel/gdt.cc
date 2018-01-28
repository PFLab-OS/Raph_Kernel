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
 */

#include <x86.h>
#include <gdt.h>
#include <raph.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <mem/paging.h>
#include <mem/kstack.h>
#include <cpu.h>

void Gdt::SetupProc() {
  // re-setup GDT & setup 64bit TSS
  PhysAddr paddr;
  size_t gdt_size = sizeof(uint64_t) * kGdtEntryNum;
  physmem_ctrl->Alloc(paddr, PagingCtrl::RoundUpAddrOnPageBoundary(
                                 gdt_size + sizeof(ContextInfo) + sizeof(Tss)));
  uint32_t *gdt_desc = reinterpret_cast<uint32_t *>(paddr.GetVirtAddr());
  virt_addr context_info_vaddr = paddr.GetVirtAddr() + gdt_size;
  virt_addr tss_vaddr = paddr.GetVirtAddr() + gdt_size + sizeof(ContextInfo);

  assert(KERNEL_CS / 8 + 1 <= Gdt::kGdtEntryNum);
  assert(KERNEL_DS / 8 + 1 <= Gdt::kGdtEntryNum);
  assert(USER_CS / 8 + 1 <= Gdt::kGdtEntryNum);
  assert(USER_DS / 8 + 1 <= Gdt::kGdtEntryNum);
  assert(KERNEL_CPU / 8 + 1 <= Gdt::kGdtEntryNum);
  assert(TSS / 8 + 2 <= Gdt::kGdtEntryNum);

  for (int i = 0; i < kGdtEntryNum * 2; i++) {
    gdt_desc[i] = 0;
  }

  gdt_desc[(KERNEL_CS / sizeof(uint32_t))] = 0x00000000;
  gdt_desc[(KERNEL_CS / sizeof(uint32_t)) + 1] = 0x00209a00;
  gdt_desc[(KERNEL_DS / sizeof(uint32_t))] = 0x00000000;
  gdt_desc[(KERNEL_DS / sizeof(uint32_t)) + 1] = 0x00009200;
  gdt_desc[(USER_DS / sizeof(uint32_t))] = 0x00000000;
  gdt_desc[(USER_DS / sizeof(uint32_t)) + 1] = 0x0000F200;
  gdt_desc[(USER_CS / sizeof(uint32_t))] = 0x00000000;
  gdt_desc[(USER_CS / sizeof(uint32_t)) + 1] = 0x0020FA00;
  gdt_desc[(KERNEL_CPU / sizeof(uint32_t))] = 0;
  gdt_desc[(KERNEL_CPU / sizeof(uint32_t)) + 1] = 0;

  gdt_desc[(TSS / sizeof(uint32_t))] =
      MASK((sizeof(Tss) - 1), 15, 0) | (MASK(tss_vaddr, 16, 0) << 16);
  gdt_desc[(TSS / sizeof(uint32_t)) + 1] =
      (MASK(tss_vaddr, 23, 16) >> 16) | (9 << 8) | (3 << 13) | (1 << 15) |
      MASK((sizeof(Tss) - 1), 19, 16) | MASK(tss_vaddr, 31, 24);
  gdt_desc[(TSS / sizeof(uint32_t)) + 2] = tss_vaddr >> 32;
  gdt_desc[(TSS / sizeof(uint32_t)) + 3] = 0;

  ContextInfo *context_info =
      reinterpret_cast<ContextInfo *>(context_info_vaddr);
  context_info->info = context_info;
  context_info->cpuid = cpu_ctrl->GetCpuId();

  Tss *tss = reinterpret_cast<Tss *>(tss_vaddr);
  CpuId cpuid = cpu_ctrl->GetCpuId();

  virt_addr rsp0 = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->rsp0l = rsp0;
  tss->rsp0h = rsp0 >> 32;

  virt_addr rsp1 = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->rsp1l = rsp1;
  tss->rsp1h = rsp1 >> 32;

  virt_addr rsp2 = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->rsp2l = rsp2;
  tss->rsp2h = rsp2 >> 32;

  virt_addr dfstack = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->ist1l = dfstack;
  tss->ist1h = dfstack >> 32;

  virt_addr nmistack = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->ist2l = nmistack;
  tss->ist2h = nmistack >> 32;

  virt_addr debugstack = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->ist3l = debugstack;
  tss->ist3h = debugstack >> 32;

  virt_addr mcestack = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->ist4l = mcestack;
  tss->ist4h = mcestack >> 32;

  virt_addr genstack = KernelStackCtrl::GetCtrl().AllocIntStack(cpuid);
  tss->ist5l = genstack;
  tss->ist5h = genstack >> 32;

  tss->iomap = sizeof(Tss);  // no I/O permission map

  x86::lgdt(gdt_desc, kGdtEntryNum);
  asm volatile("ltr %0;" ::"r"((uint16_t)TSS));
  x86::wrmsr(MSR_IA32_FS_BASE, context_info_vaddr);
  GetCurrentContextInfo();
}
