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

void InitKernelMemorySpace();
void ReleaseLowMemory();

class MemCtrl;

class KernelVirtmemCtrl {
  friend MemCtrl;
  friend void InitKernelMemorySpace();
public:
  void Init();
  virt_addr Alloc(size_t size);
  virt_addr AllocZ(size_t size);
  void Free(virt_addr addr);
  virtual virt_addr Sbrk(int64_t);
protected:
  //FIXME: make static
  //Physical address of pml4t's kernel entry
  /*static*/ entry_type pml4t_entry[256];
  // For treating heap memory
  virt_addr _heap_allocated_end;
  virt_addr _brk_end;
  virt_addr _heap_limit;
  //TODO: Check if this lock need on many core machine.
  SpinLock _lock;
};

class UserVirtmemCtrl {
  friend MemCtrl;
protected:
  //virtual virt_addr Sbrk(int64_t);
  entry_type entry[256];
};

// Processing Pml4t 
class PagingCtrl;
class MemCtrl {
public:
  MemCtrl() : _pml4t(GetPml4tAddr()) {
  }
  ~MemCtrl() {
    MemCtrl::kvc.Free(_pt_mem);
  }
  void Init();

  //FIXME:Make static
  /*static*/ KernelVirtmemCtrl kvc;
  UserVirtmemCtrl uvc;
  static const int kKernelPml4tEntryNum = 256;
  static const int kUserPml4tEntryNum = 256;

  //privateにする
  PagingCtrl* paging_ctrl;
private:
  PageTable* GetPml4tAddr();
  PageTable* const _pml4t;
  //This variable save the memory addr used to get pml4t addr.
  virt_addr _pt_mem;
};

template <typename ptr> inline virt_addr ptr2virtaddr(ptr *addr) {
  return reinterpret_cast<virt_addr>(addr);
}

template <typename ptr> inline ptr *addr2ptr(virt_addr addr) {
  return reinterpret_cast<ptr *>(addr);
}
