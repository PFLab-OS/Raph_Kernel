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

#include "paging.h"
#include <assert.h>
#include <global.h>
#include <raph.h>
#include <multiboot.h>
#include "virtmem.h"
#include "physmem.h"

PagingCtrl::PagingCtrl() {
  extern PageTable initial_PML4T;
  phys_addr pml4t_addr = reinterpret_cast<phys_addr>(&initial_PML4T);
  _pml4t = reinterpret_cast<PageTable *>(p2v(pml4t_addr));
}

void PagingCtrl::MapAllPhysMemory() {
  for (virt_addr vaddr = 0; vaddr < multiboot_ctrl->GetPhysMemoryEnd(); vaddr += 0x200000) {
    if (IsVirtAddrMapped(PhysmemCtrl::kLinearMapOffset + vaddr)) continue;
    // まだマップされていない仮想アドレス領域に物理メモリを割り当てる
    PhysAddr paddr;
    paddr.SetAddr(vaddr);
    Map2MPageToVirtAddr(PhysmemCtrl::kLinearMapOffset + vaddr, paddr, PDE_WRITE_BIT | PDE_USER_BIT, PDE_WRITE_BIT | PDE_GLOBAL_BIT | PDE_USER_BIT);
  }
}

void PagingCtrl::ConvertVirtMemToPhysMem(virt_addr vaddr, PhysAddr &paddr) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    return;
  }
  PageTable *pdpt = reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    return;
  }
  if ((entry & PDPTE_1GPAGE_BIT) != 0) {
    paddr.SetAddr(GetPDPTEMaskedAddr(entry) | Get1GPageOffset(vaddr));
    return;
  }
  PageTable * pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
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
  PageTable *pdpt = reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    return false;
  }
  if ((entry & PDPTE_1GPAGE_BIT) != 0) {
    return true;
  }
  PageTable * pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
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

void PagingCtrl::GetTranslationEntries(virt_addr vaddr, entry_type *pml4e, entry_type *pdpte, entry_type *pde, entry_type *pte){
  Locker locker(_lock);
  // PML4
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if(pml4e) *pml4e = entry;
  if(!(entry & PML4E_PRESENT_BIT)){
    // 以降のエントリは存在しない
    if(pdpte) *pdpte = 0;
    if(pde) *pde = 0;
    if(pte) *pte = 0;
    return;
  }
  // PDPT
  PageTable *pdpt = reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if(pdpte) *pdpte = entry;
  if(!(entry & PDPTE_PRESENT_BIT) || (entry & PDPTE_1GPAGE_BIT)){
    // 以降のエントリは存在しない
    if(pde) *pde = 0;
    if(pte) *pte = 0;
    return;
  }
  // PD
  PageTable *pd = reinterpret_cast<PageTable *>(p2v(GetPDPTEMaskedAddr(entry)));
  entry = pd->entry[GetPDIndex(vaddr)];
  if(pde) *pde = entry;
  if(!(entry & PDE_PRESENT_BIT) || (entry & PDE_2MPAGE_BIT)){
    // 以降のエントリは存在しない
    if(pte) *pte = 0;
    return;
  }
  // PT
  PageTable *pt = reinterpret_cast<PageTable *>(p2v(GetPDEMaskedAddr(entry)));
  entry = pt->entry[GetPTIndex(vaddr)];
  if(pte) *pte = entry;
  if (!(entry & PTE_PRESENT_BIT)) {
  }
}

bool PagingCtrl::Map4KPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag, phys_addr page_flag) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = _pml4t->entry[GetPML4TIndex(vaddr)] = tpaddr.GetAddr() | pst_flag | PML4E_PRESENT_BIT;
  }
  PageTable *pdpt = reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = pdpt->entry[GetPDPTIndex(vaddr)] = tpaddr.GetAddr() | pst_flag | PDPTE_PRESENT_BIT;
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
    entry = pd->entry[GetPDIndex(vaddr)] = tpaddr.GetAddr() | pst_flag | PDE_PRESENT_BIT;
  }
  if ((entry & PDE_2MPAGE_BIT) != 0) {
    return false;
  }
  PageTable *pt = reinterpret_cast<PageTable *>(p2v(GetPDEMaskedAddr(entry)));
  entry = pt->entry[GetPTIndex(vaddr)];
  if ((entry & PTE_PRESENT_BIT) == 0) {
    pt->entry[GetPTIndex(vaddr)] = paddr.GetAddr() | page_flag | PTE_PRESENT_BIT | PTE_USER_BIT;
    return true;
  } else {
    return false;
  }
}

bool PagingCtrl::Map2MPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag, phys_addr page_flag) {
  Locker locker(_lock);
  entry_type entry = _pml4t->entry[GetPML4TIndex(vaddr)];
  if ((entry & PML4E_PRESENT_BIT) == 0) {
    // まだメモリ上に存在していないPDPTに登録しようとしたので、PDPTをmallocしてから継続する。
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = _pml4t->entry[GetPML4TIndex(vaddr)] = tpaddr.GetAddr() | pst_flag | PML4E_PRESENT_BIT;
  }
  PageTable *pdpt = reinterpret_cast<PageTable *>(p2v(GetPML4EMaskedAddr(entry)));
  entry = pdpt->entry[GetPDPTIndex(vaddr)];
  if ((entry & PDPTE_PRESENT_BIT) == 0) {
    // まだメモリ上に存在していないPTに登録しようとしたので、PTをmallocしてから継続する。
    PhysAddr tpaddr;
    physmem_ctrl->Alloc(tpaddr, kPageSize);
    bzero(reinterpret_cast<void *>(tpaddr.GetVirtAddr()), kPageSize);
    entry = pdpt->entry[GetPDPTIndex(vaddr)] = tpaddr.GetAddr() | pst_flag | PDPTE_PRESENT_BIT;
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
  pd->entry[GetPDIndex(vaddr)] = paddr.GetAddr() | pst_flag | PDE_PRESENT_BIT | PDE_2MPAGE_BIT;
  return true;
}

