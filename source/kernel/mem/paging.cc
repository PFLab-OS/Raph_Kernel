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
  return false;
}

bool PagingCtrl::MapPhysAddrToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag, phys_addr page_flag) {
  kassert(false);
  return false;
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
