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
#include "../global.h"
#include "../naias.h"
#include "virtmem.h"

#ifdef __UNIT_TEST__
PagingCtrl::PagingCtrl() {
  phys_addr addr = ptr2physaddr(malloc(kPageSize * 2 - 1));
  _pml4t = reinterpret_cast<entry_type *>(RoundUpAddrOnPageBoundary(addr));
  _pml4t[_selfref_pml4t_index] = reinterpret_cast<entry_type>(_pml4t);
}
#else
PagingCtrl::PagingCtrl() {
}
#endif // __UNIT_TEST__

bool PagingCtrl::IsVirtAddrMapped(virt_addr vaddr) {
  kassert(false);
  bool rval = false;
  int pml4t_index = GetPML4TIndex(vaddr);
  do {
    if (!IsPageEntryPresent
        (CalcVirtAddrFromStructureTableOffset
         (_selfref_pml4t_index,
          _selfref_pml4t_index,
          _selfref_pml4t_index,
          _selfref_pml4t_index,
          pml4t_index * sizeof(virt_addr)))) {
      break;
    }
    int pdpt_index = GetPDPTIndex(vaddr);
    if (!IsPageEntryPresent
        (CalcVirtAddrFromStructureTableOffset
         (_selfref_pml4t_index,
          _selfref_pml4t_index,
          _selfref_pml4t_index,
          pml4t_index,
          pdpt_index * sizeof(virt_addr)))) {
      break;
    }
    int pd_index = GetPDIndex(vaddr);
    if (!IsPageEntryPresent
        (CalcVirtAddrFromStructureTableOffset
         (_selfref_pml4t_index,
          _selfref_pml4t_index,
          pml4t_index,
          pdpt_index,
          pd_index * sizeof(virt_addr)))) {
      break;
    }
    int pt_index = GetPTIndex(vaddr);
    if (!IsPageEntryPresent
        (CalcVirtAddrFromStructureTableOffset
         (_selfref_pml4t_index,
          pml4t_index,
          pdpt_index,
          pd_index,
          pt_index * sizeof(virt_addr)))) {
      break;
    }
    rval = true;
  } while(false);
  return rval;
}

bool PagingCtrl::MapPhysAddrToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag, phys_addr page_flag) {
  kassert(false);
  int pml4t_index = GetPML4TIndex(vaddr);
  virt_addr pml4t_entry = CalcVirtAddrFromStructureTableOffset
    (_selfref_pml4t_index,
     _selfref_pml4t_index,
     _selfref_pml4t_index,
     _selfref_pml4t_index,
     pml4t_index * sizeof(virt_addr));
  int pdpt_index = GetPDPTIndex(vaddr);
  virt_addr pdpt_entry = CalcVirtAddrFromStructureTableOffset
    (_selfref_pml4t_index,
     _selfref_pml4t_index,
     _selfref_pml4t_index,
     pml4t_index,
     pdpt_index * sizeof(virt_addr));
  int pd_index = GetPDIndex(vaddr);
  virt_addr pd_entry = CalcVirtAddrFromStructureTableOffset
    (_selfref_pml4t_index,
     _selfref_pml4t_index,
     pml4t_index,
     pdpt_index,
     pd_index * sizeof(virt_addr));
  int pt_index = GetPTIndex(vaddr);
  virt_addr pt_entry = CalcVirtAddrFromStructureTableOffset
    (_selfref_pml4t_index,
     pml4t_index,
     pdpt_index,
     pd_index,
     pt_index * sizeof(virt_addr));

  bool need_pst = true;
  PhysAddr pst;
  physmem_ctrl->Alloc(pst, kPageSize);
  virt_addr entry_addr = 0;
  do {
    if (!IsPageEntryPresent(pml4t_entry)) {
      entry_addr = pml4t_entry;
      break;
    } else if (!IsPageEntryPresent(pdpt_entry)) {
      entry_addr = pdpt_entry;
      break;
    } else if (!IsPageEntryPresent(pd_entry)) {
      entry_addr = pd_entry;
      break;
    } else {
      need_pst = false;
      entry_addr = pt_entry;
      if (IsPageEntryPresent(pt_entry)) {
        return false;
      }
    }
    phys_addr page_addr = 0, entry_flag = 0;
    if (need_pst) {
      page_addr = pst.GetAddr();
      entry_flag = pst_flag;
    } else {
      page_addr = paddr.GetAddr();
      entry_addr = page_flag;
    }
    SetPageEntry(entry_addr, page_addr | PTE_PRESENT_BIT | entry_flag);
  } while(false);

  if (!need_pst) {
    physmem_ctrl->Free(pst, kPageSize);
  } else {
    MapPhysAddrToVirtAddr(vaddr, paddr, pst_flag, page_flag);
  }
  return true;
}

extern int kHeapEndAddr;

//TODO deprecated
virt_addr PagingCtrl::SearchUnmappedArea(size_t size) {
  kassert(false);
  kassert(size == align(size, kPageSize));
  virt_addr addr = ptr2virtaddr(&kHeapEndAddr);
  while(true) {
    size_t i = 0;
    for(; i < size; i += kPageSize, addr += kPageSize) {
      if (!IsVirtAddrMapped(addr)) {
        addr += kPageSize;
        break;
      }
    }
    if (i == size) {
      break;
    }
  }
  return addr;
}

#ifdef __UNIT_TEST__

static void test1() {
  
}

void paging_test() {
  test1();
}

#endif // __UNIT_TEST__
