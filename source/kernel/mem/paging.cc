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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#include "paging.h"
#include <assert.h>
#include <global.h>
#include <raph.h>
#include <multiboot.h>
#include "virtmem.h"
#include "physmem.h"

void PagingCtrl::MapAllPhysMemory() {
  for (virt_addr vaddr = 0; vaddr < multiboot_ctrl->GetPhysMemoryEnd();
       vaddr += 0x200000) {
    if (IsVirtAddrMapped(PhysmemCtrl::kLinearMapOffset + vaddr)) continue;
    // まだマップされていない仮想アドレス領域に物理メモリを割り当てる
    PhysAddr paddr;
    paddr.SetAddr(vaddr);
    Map2MPageToVirtAddr(PhysmemCtrl::kLinearMapOffset + vaddr, paddr,
                        PDE_WRITE_BIT | PDE_USER_BIT,
                        PDE_WRITE_BIT | PDE_GLOBAL_BIT | PDE_USER_BIT);
  }
}

void PagingCtrl::ReleaseLowMemory() {
  // release low address(1MB~) which is set in boot/boot.S
  entry_type entry = _pml4t->entry[GetPML4TIndex(0)];
  assert((entry & PML4E_PRESENT_BIT) != 0);
  _pml4t->entry[GetPML4TIndex(0)] = 0;
}

void PagingCtrl::CopyMemorySpaceSub(entry_type *dst, const entry_type *src,
                                    int depth) {
  for (int i = 0; i < 512; i++) {
    switch (depth) {
      case 0:  // PDPTE
        if (!(src[i] & PDPTE_PRESENT_BIT)) {
          dst[i] = 0;
          break;
        }
        if (src[i] & PDPTE_1GPAGE_BIT) {
          PhysAddr tpaddr;
          physmem_ctrl->Alloc(tpaddr, PagingCtrl::kPageSize * 512 * 512);
          dst[i] = tpaddr.GetAddr() |
                   (src[i] ^ PagingCtrl::GetPDPTEMaskedAddr(src[i]));
          memcpy(reinterpret_cast<void *>(tpaddr.GetVirtAddr()),
                 reinterpret_cast<entry_type *>(
                     p2v(PagingCtrl::GetPDPTEMaskedAddr(src[i]))),
                 PagingCtrl::kPageSize * 512 * 512);
        } else {
          PhysAddr tpaddr;
          physmem_ctrl->Alloc(tpaddr, PagingCtrl::kPageSize);
          bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()),
                PagingCtrl::kPageSize);
          dst[i] = tpaddr.GetAddr() |
                   (src[i] ^ PagingCtrl::GetPDPTEMaskedAddr(src[i]));
          CopyMemorySpaceSub(
              reinterpret_cast<entry_type *>(tpaddr.GetVirtAddr()),
              reinterpret_cast<entry_type *>(
                  p2v(PagingCtrl::GetPDPTEMaskedAddr(src[i]))),
              depth + 1);
        }
        break;
      case 1:  // PDE
        if (!(src[i] & PDE_PRESENT_BIT)) {
          dst[i] = 0;
          break;
        }
        if (src[i] & PDE_2MPAGE_BIT) {
          PhysAddr tpaddr;
          physmem_ctrl->Alloc(tpaddr, PagingCtrl::kPageSize * 512);
          memcpy(reinterpret_cast<entry_type *>(tpaddr.GetVirtAddr()),
                 reinterpret_cast<entry_type *>(
                     p2v(PagingCtrl::GetPDEMaskedAddr(src[i]))),
                 PagingCtrl::kPageSize * 512 * 512);
        } else {
          PhysAddr tpaddr;
          physmem_ctrl->Alloc(tpaddr, PagingCtrl::kPageSize);
          bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()),
                PagingCtrl::kPageSize);
          dst[i] = tpaddr.GetAddr() |
                   (src[i] ^ PagingCtrl::GetPDEMaskedAddr(src[i]));
          CopyMemorySpaceSub(
              reinterpret_cast<entry_type *>(tpaddr.GetVirtAddr()),
              reinterpret_cast<entry_type *>(
                  p2v(PagingCtrl::GetPDEMaskedAddr(src[i]))),
              depth + 1);
        }
        break;
      case 2:  // PTE
        if (src[i] & PTE_PRESENT_BIT) {
          PhysAddr tpaddr;
          physmem_ctrl->Alloc(tpaddr, PagingCtrl::kPageSize);
          dst[i] = tpaddr.GetAddr() |
                   (src[i] ^ PagingCtrl::GetPTEMaskedAddr(src[i]));
          memcpy(reinterpret_cast<void *>(tpaddr.GetVirtAddr()),
                 reinterpret_cast<entry_type *>(
                     p2v(PagingCtrl::GetPDEMaskedAddr(src[i]))),
                 PagingCtrl::kPageSize);
        }
        break;
    }
  }
}

void PagingCtrl::CopyMemorySpace(PageTable *mdst, const PageTable *msrc) {
  // Kernel Memory
  for (int i = 256; i < 512; i++) {
    mdst->entry[i] = msrc->entry[i];
  }
  // Process Memory
  PhysAddr tpaddr;
  for (int i = 0; i < 256; i++) {
    if (msrc->entry[i] & PML4E_PRESENT_BIT) {
      physmem_ctrl->Alloc(tpaddr, PagingCtrl::kPageSize);
      bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()),
            PagingCtrl::kPageSize);
      mdst->entry[i] =
          tpaddr.GetAddr() |
          (msrc->entry[i] ^ PagingCtrl::GetPML4EMaskedAddr(msrc->entry[i]));
      CopyMemorySpaceSub(reinterpret_cast<entry_type *>(tpaddr.GetVirtAddr()),
                         reinterpret_cast<entry_type *>(p2v(
                             PagingCtrl::GetPML4EMaskedAddr(msrc->entry[i]))),
                         0);
    }
  }
}

void PagingCtrl::Switch() {
  PageTable *pt;
  phys_addr pa;
  asm volatile("movq %%cr3,%%rax;movq %%rax,%0" : "=m"(pa) : : "%rax");
  pt = reinterpret_cast<PageTable *>(p2v(pa));

  for (int i = 0; i < KernelVirtmemCtrl::kKernelPml4tEntryNum; i++) {
    _pml4t->entry[UserVirtmemCtrl::kUserPml4tEntryNum + i] =
        pt->entry[UserVirtmemCtrl::kUserPml4tEntryNum + i];
  }

  asm volatile("movq %0,%%cr3" : : "r"(k2p(ptr2virtaddr(_pml4t))) :);
}

void PagingCtrl::ConvertVirtMemToPhysMem(virt_addr vaddr, PhysAddr &paddr) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    return;
  }
  PageTable *pdpt =
      reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    return;
  }
  if ((entry & PDPTE_1GPAGE_BIT) != 0) {
    paddr.SetAddr(GetPDPTEMaskedAddr(entry) | Get1GPageOffset(vaddr));
    return;
  }
  PageTable *pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
  entry = pd->entry[GetPDIndex(vaddr)];
  if ((entry & PDE_PRESENT_BIT) == 0) {
    return;
  }
  if ((entry & PDE_2MPAGE_BIT) != 0) {
    paddr.SetAddr(GetPDEMaskedAddr(entry) | Get2MPageOffset(vaddr));
    return;
  }
  PageTable *pt = reinterpret_cast<PageTable *>(p2v(GetPDEMaskedAddr(entry)));
  entry = pt->entry[GetPTIndex(vaddr)];
  if ((entry & PTE_PRESENT_BIT) == 0) {
    return;
  }
  paddr.SetAddr(GetPTEMaskedAddr(entry) | Get4KPageOffset(vaddr));
  return;
}

bool PagingCtrl::IsVirtAddrMapped(virt_addr vaddr) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    return false;
  }
  PageTable *pdpt =
      reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    return false;
  }
  if ((entry & PDPTE_1GPAGE_BIT) != 0) {
    return true;
  }
  PageTable *pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
  entry = pd->entry[GetPDIndex(vaddr)];
  if ((entry & PDE_PRESENT_BIT) == 0) {
    return false;
  }
  if ((entry & PDE_2MPAGE_BIT) != 0) {
    return true;
  }
  PageTable *pt = reinterpret_cast<PageTable *>(p2v(GetPDEMaskedAddr(entry)));
  entry = pt->entry[GetPTIndex(vaddr)];
  if ((entry & PTE_PRESENT_BIT) == 0) {
    return false;
  }
  return true;
}

void PagingCtrl::GetTranslationEntries(virt_addr vaddr, entry_type *pml4e,
                                       entry_type *pdpte, entry_type *pde,
                                       entry_type *pte) {
  Locker locker(_lock);
  // PML4
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if (pml4e) *pml4e = entry;
  if (!(entry & PML4E_PRESENT_BIT)) {
    // 以降のエントリは存在しない
    if (pdpte) *pdpte = 0;
    if (pde) *pde = 0;
    if (pte) *pte = 0;
    return;
  }
  // PDPT
  PageTable *pdpt =
      reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if (pdpte) *pdpte = entry;
  if (!(entry & PDPTE_PRESENT_BIT) || (entry & PDPTE_1GPAGE_BIT)) {
    // 以降のエントリは存在しない
    if (pde) *pde = 0;
    if (pte) *pte = 0;
    return;
  }
  // PD
  PageTable *pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
  entry = pd->entry[GetPDIndex(vaddr)];
  if (pde) *pde = entry;
  if (!(entry & PDE_PRESENT_BIT) || (entry & PDE_2MPAGE_BIT)) {
    // 以降のエントリは存在しない
    if (pte) *pte = 0;
    return;
  }
  // PT
  PageTable *pt = reinterpret_cast<PageTable *>(p2v(GetPDEMaskedAddr(entry)));
  entry = pt->entry[GetPTIndex(vaddr)];
  if (pte) *pte = entry;
  if (!(entry & PTE_PRESENT_BIT)) {
  }
}

bool PagingCtrl::Map4KPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr,
                                     phys_addr pst_flag, phys_addr page_flag) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = _pml4t->entry[GetPML4TIndex(vaddr)] =
        tpaddr.GetAddr() | pst_flag | PML4E_PRESENT_BIT;
  }
  PageTable *pdpt =
      reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = pdpt->entry[GetPDPTIndex(vaddr)] =
        tpaddr.GetAddr() | pst_flag | PDPTE_PRESENT_BIT;
  }
  if ((entry & PDPTE_1GPAGE_BIT) != 0) {
    return false;
  }
  PageTable *pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
  entry = pd->entry[GetPDIndex(vaddr)];
  if ((entry & PDE_PRESENT_BIT) == 0) {
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = pd->entry[GetPDIndex(vaddr)] =
        tpaddr.GetAddr() | pst_flag | PDE_PRESENT_BIT;
  }
  if ((entry & PDE_2MPAGE_BIT) != 0) {
    return false;
  }
  PageTable *pt = reinterpret_cast<PageTable *>(p2v(GetPDEMaskedAddr(entry)));
  entry = pt->entry[GetPTIndex(vaddr)];
  if ((entry & PTE_PRESENT_BIT) == 0) {
    pt->entry[GetPTIndex(vaddr)] =
        paddr.GetAddr() | page_flag | PTE_PRESENT_BIT;
    return true;
  } else {
    return false;
  }
}

bool PagingCtrl::Map2MPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr,
                                     phys_addr pst_flag, phys_addr page_flag) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    // まだメモリ上に存在していないPDPTに登録しようとしたので、PDPTをmallocしてから継続する。
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = _pml4t->entry[GetPML4TIndex(vaddr)] =
        tpaddr.GetAddr() | pst_flag | PML4E_PRESENT_BIT;
  }
  PageTable *pdpt =
      reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    // まだメモリ上に存在していないPTに登録しようとしたので、PTをmallocしてから継続する。
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = pdpt->entry[GetPDPTIndex(vaddr)] =
        tpaddr.GetAddr() | pst_flag | PDPTE_PRESENT_BIT;
  }
  if ((entry & PDPTE_1GPAGE_BIT) != 0) {
    // すでに1GPageとしてマップされていたので中止
    return false;
  }
  PageTable *pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
  entry = pd->entry[GetPDIndex(vaddr)];
  if (entry & PDE_PRESENT_BIT) {
    // すでにマップされていたので中止
    return false;
  }
  // マップする
  pd->entry[GetPDIndex(vaddr)] =
      paddr.GetAddr() | pst_flag | PDE_PRESENT_BIT | PDE_2MPAGE_BIT;
  return true;
}

bool PagingCtrl::Map1GPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr,
                                     phys_addr pst_flag, phys_addr page_flag) {
  Locker locker(_lock);
  assert((paddr.GetAddr() % 0x40000000) == 0);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    // まだメモリ上に存在していないPDPTに登録しようとしたので、PDPTをmallocしてから継続する。
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = _pml4t->entry[GetPML4TIndex(vaddr)] =
        tpaddr.GetAddr() | pst_flag | PML4E_PRESENT_BIT;
  }
  PageTable *pdpt =
      reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if (entry & PDPTE_PRESENT_BIT) {
    // すでにマップされていたので中止
    return false;
  }
  pdpt->entry[GetPDPTIndex(vaddr)] =
      paddr.GetAddr() | pst_flag | PDPTE_PRESENT_BIT | PDPTE_1GPAGE_BIT;
  return true;
}
