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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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

void Gdt::SetupProc() {
  // setup 64bit TSS
  PhysAddr paddr;
  size_t gdt_size = sizeof(uint64_t) * 6;
  physmem_ctrl->Alloc(paddr, PagingCtrl::RoundUpAddrOnPageBoundary(gdt_size + sizeof(Tss)));
  uint32_t *gdt_desc = reinterpret_cast<uint32_t *>(paddr.GetVirtAddr());
  virt_addr tss_vaddr = paddr.GetVirtAddr() + gdt_size;

  for (int i = 0; i < 12; i++) {
    gdt_desc[i] = 0;
  }

  gdt_desc[4] = 0x00000000;
  gdt_desc[5] = 0x00209a00;
  gdt_desc[6] = 0x00000000;
  gdt_desc[7] = 0x00009200;

  gdt_desc[8] =
    MASK((sizeof(Tss) - 1), 15, 0) |
    (MASK(tss_vaddr, 16, 0) << 16);
  gdt_desc[9] =
    (MASK(tss_vaddr, 23, 16) >> 16) |
    (9 << 8) |
    (3 << 13) |
    (1 << 15) |
    MASK((sizeof(Tss) - 1), 19, 16) |
    MASK(tss_vaddr, 31, 24);
  gdt_desc[10] = tss_vaddr >> 32;

  Tss *tss = reinterpret_cast<Tss *>(tss_vaddr);
  
  PhysAddr rsp0_paddr;
  physmem_ctrl->Alloc(rsp0_paddr, PagingCtrl::kPageSize);
  virt_addr rsp0 = rsp0_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  rsp0 = IntStackInfo::SetupStackInfo(rsp0);
  tss->rsp0l = rsp0;
  tss->rsp0h = rsp0 >> 32;

  PhysAddr rsp1_paddr;
  physmem_ctrl->Alloc(rsp1_paddr, PagingCtrl::kPageSize);
  virt_addr rsp1 = rsp1_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  rsp1 = IntStackInfo::SetupStackInfo(rsp1);
  tss->rsp1l = rsp1;
  tss->rsp1h = rsp1 >> 32;

  PhysAddr rsp2_paddr;
  physmem_ctrl->Alloc(rsp2_paddr, PagingCtrl::kPageSize);
  virt_addr rsp2 = rsp2_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  rsp2 = IntStackInfo::SetupStackInfo(rsp2);
  tss->rsp2l = rsp2;
  tss->rsp2h = rsp2 >> 32;

  PhysAddr dfstack_paddr;
  physmem_ctrl->Alloc(dfstack_paddr, PagingCtrl::kPageSize);
  virt_addr dfstack = dfstack_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  dfstack = IntStackInfo::SetupStackInfo(dfstack);
  tss->ist1l = dfstack;
  tss->ist1h = dfstack >> 32;

  PhysAddr nmistack_paddr;
  physmem_ctrl->Alloc(nmistack_paddr, PagingCtrl::kPageSize);
  virt_addr nmistack = nmistack_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  nmistack = IntStackInfo::SetupStackInfo(nmistack);
  tss->ist2l = nmistack;
  tss->ist2h = nmistack >> 32;

  PhysAddr debugstack_paddr;
  physmem_ctrl->Alloc(debugstack_paddr, PagingCtrl::kPageSize);
  virt_addr debugstack = debugstack_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  debugstack = IntStackInfo::SetupStackInfo(debugstack);
  tss->ist3l = debugstack;
  tss->ist3h = debugstack >> 32;

  PhysAddr mcestack_paddr;
  physmem_ctrl->Alloc(mcestack_paddr, PagingCtrl::kPageSize);
  virt_addr mcestack = mcestack_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  mcestack = IntStackInfo::SetupStackInfo(mcestack);
  tss->ist4l = mcestack;
  tss->ist4h = mcestack >> 32;

  PhysAddr genstack_paddr;
  physmem_ctrl->Alloc(genstack_paddr, PagingCtrl::kPageSize);
  virt_addr genstack = genstack_paddr.GetVirtAddr() + PagingCtrl::kPageSize;
  genstack = IntStackInfo::SetupStackInfo(genstack);
  tss->ist5l = genstack;
  tss->ist5h = genstack >> 32;

  tss->iomap = sizeof(Tss);  // no I/O permission map

  lgdt(gdt_desc, 6);
  asm volatile("ltr %0;"::"r"((uint16_t)TSS));
}
