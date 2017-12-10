/*
 *
 * Copyright (c) 2017 Raphine Project
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
 * Author: mumumu
 *
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <mem/type.h>
#include <raph.h>
#include <spinlock.h>

// Memory map
// pml4t 0-255    : User Memory Space (this is different from each process.)
// pml4t 256-511  : Kernel Memory Space (this is the same as each process.)

class MemCtrl;

class KernelVirtmemCtrl {
 public:
  void Init();
  virt_addr Alloc(size_t size);
  virt_addr AllocZ(size_t size);
  void Free(virt_addr addr);
  virtual virt_addr Sbrk(int64_t);

  void InitKernelMemorySpace();
  void ReleaseLowMemory();

  static const int kKernelPml4tEntryNum = 256;

 private:
  friend MemCtrl;
  // FIXME: make static
  // Physical address of pml4t's kernel entry
  /*static*/ entry_type pml4t_entry[kKernelPml4tEntryNum];
  // For treating heap memory
  virt_addr _heap_allocated_end;
  virt_addr _brk_end;
  virt_addr _heap_limit;
  // TODO: Check if this lock need on many core machine.
  SpinLock _lock;
};

class UserVirtmemCtrl {
 public:
  static const int kUserPml4tEntryNum = 256;

 private:
  friend MemCtrl;
  // virtual virt_addr Sbrk(int64_t);
  entry_type entry[kUserPml4tEntryNum];
};

static_assert((UserVirtmemCtrl::kUserPml4tEntryNum +
               KernelVirtmemCtrl::kKernelPml4tEntryNum) ==
                  4096 / sizeof(uint64_t),
              "Error: Invalid number of Pml4t entry.");

// Processing Pml4t
class PagingCtrl;
class PhysAddr;
class MemCtrl {
 public:
  MemCtrl() : _pml4t(GetPml4tAddr()) {}
  ~MemCtrl() { MemCtrl::_kvc.Free(_pt_mem); }
  void Init();

  void GetTranslationEntries(virt_addr vaddr, entry_type *pml4e,
                             entry_type *pdpte, entry_type *pde,
                             entry_type *pte);
  bool Map1GPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag,
                           phys_addr page_flag);
  bool MapPhysAddrToVirtAddr(virt_addr vaddr, PhysAddr &paddr, size_t size,
                             phys_addr pst_flag, phys_addr page_flag);
  bool IsVirtAddrMapped(virt_addr vaddr);
  void ConvertVirtMemToPhysMem(virt_addr vaddr, PhysAddr &paddr);

  KernelVirtmemCtrl* GetKernelVirtmemCtrl() {
    return &_kvc;
  }
  void Switch();

 private:
  PageTable *GetPml4tAddr();
  PageTable *const _pml4t;
  // This variable save the memory addr used to get pml4t addr.
  virt_addr _pt_mem;

  // FIXME:Make static
  /*static*/ KernelVirtmemCtrl _kvc;
  UserVirtmemCtrl _uvc;
  PagingCtrl* _paging_ctrl;
};

template <typename ptr>
inline virt_addr ptr2virtaddr(ptr *addr) {
  return reinterpret_cast<virt_addr>(addr);
}

template <typename ptr>
inline ptr *addr2ptr(virt_addr addr) {
  return reinterpret_cast<ptr *>(addr);
}
