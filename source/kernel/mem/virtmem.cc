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

#include <errno.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <global.h>

extern int phys_memory_end;
extern int kLinearAddrOffset;
extern int kHeapEndAddr;

extern "C" {
void *dlmalloc(size_t);
void dlfree(void *);
}

void MemCtrl::Init() {
  //TODO
  //Copy Kernel Memory's pdpt address to new pml4t.
//  for(int i = 1; i <= kKernelPml4tEntryNum; i++) {
//    _pml4t->entry[kUserPml4tEntryNum + i] = kvc.pml4t_entry[i];
//  }
  paging_ctrl = new PagingCtrl(_pml4t);

  //FIXME: make satic
  if(system_memory_space != nullptr) {
    kvc = system_memory_space->kvc;
  }
}

PageTable* MemCtrl::GetPml4tAddr() {
  if(system_memory_space == nullptr){
    extern PageTable initial_PML4T;
    phys_addr pml4t_addr = reinterpret_cast<phys_addr>(&initial_PML4T);
    return reinterpret_cast<PageTable *>(p2v(pml4t_addr));
  }

  _pt_mem = MemCtrl::kvc.Alloc(PagingCtrl::kPageSize*2);
  return reinterpret_cast<PageTable*>((reinterpret_cast<uint64_t>(_pt_mem) + PagingCtrl::kPageSize) & ~(PagingCtrl::kPageSize - 1));
}

void KernelVirtmemCtrl::Init() {
  // カーネル仮想メモリは最大1792MB
  _heap_limit = ptr2virtaddr(&kHeapEndAddr);
  // 6MB allocated by boot.S
  _brk_end = reinterpret_cast<virt_addr>(&kLinearAddrOffset) +
             reinterpret_cast<virt_addr>(&phys_memory_end);
  //TODO: Check Heap Size
  _heap_allocated_end =
      reinterpret_cast<virt_addr>(&kLinearAddrOffset) + 0x600000;
  kassert(_brk_end < _heap_allocated_end);
}

virt_addr KernelVirtmemCtrl::Alloc(size_t size) {
  Locker locker(_lock);
  void *addr = dlmalloc(size);
  if (addr == nullptr) {
    kernel_panic("KernelVirtmemCtrl","failed to allocate kernel heap memory");
  }
  return reinterpret_cast<virt_addr>(addr);
}

virt_addr KernelVirtmemCtrl::AllocZ(size_t size) {
  Locker locker(_lock);
  void *addr = dlmalloc(size);
  if (addr == nullptr) {
    kernel_panic("KernelVirtmemCtrl","failed to allocate kernel heap memory");
  }
  bzero(addr, size);
  return reinterpret_cast<virt_addr>(addr);
}

void KernelVirtmemCtrl::Free(virt_addr addr) {
  Locker locker(_lock);
  dlfree(reinterpret_cast<void *>(addr));
}

virt_addr KernelVirtmemCtrl::Sbrk(int64_t increment) {
  virt_addr _old_brk_end = _brk_end;
  _brk_end += increment;
  if (_brk_end > _heap_allocated_end) {
    virt_addr new_heap_allocated_end = alignUp(_brk_end, PagingCtrl::kPageSize);
    if (new_heap_allocated_end <= _heap_limit) {
      kassert(_heap_allocated_end ==
              align(_heap_allocated_end, PagingCtrl::kPageSize));
      virt_addr psize = new_heap_allocated_end - _heap_allocated_end;

      PhysAddr paddr;
      physmem_ctrl->AllocNonRecursive(paddr, psize);
      kassert(system_memory_space->paging_ctrl->MapPhysAddrToVirtAddr(
          _heap_allocated_end, paddr, psize, PDE_WRITE_BIT | PDE_USER_BIT,
          PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT));
      _heap_allocated_end = new_heap_allocated_end;
    } else {
      kassert(false && "not enough kernel heap memory");
    }
  }
  return _old_brk_end;
}

//TODO:TBI
void InitKernelMemorySpace() {
  extern PageTable initial_PML4T;
  for(int i = 0; i < MemCtrl::kKernelPml4tEntryNum; i++) {
    system_memory_space->kvc.pml4t_entry[i] = initial_PML4T.entry[i];
  }
}

//TODO:TBI
void ReleaseLowMemory() {
  extern PageTable initial_PML4T;
  entry_type entry = initial_PML4T.entry[PagingCtrl::GetPML4TIndex(0)];
  assert((entry & PML4E_PRESENT_BIT) != 0);
  initial_PML4T.entry[PagingCtrl::GetPML4TIndex(0)] = 0;
}

extern "C" void *sbrk(intptr_t increment) {
  virt_addr addr = system_memory_space->kvc.Sbrk(increment);
  if (addr == 0xffffffffffffffff) {
    errno = ENOMEM;
  }
  return reinterpret_cast<void *>(addr);
}

void *operator new(size_t size) {
  return reinterpret_cast<void *>(system_memory_space->kvc.Alloc(size));
}

void *operator new[](size_t size) {
  return reinterpret_cast<void *>(system_memory_space->kvc.Alloc(size));
}

void operator delete(void *p) {
  system_memory_space->kvc.Free(reinterpret_cast<virt_addr>(p));
}

void operator delete(void *p, size_t) {
  system_memory_space->kvc.Free(reinterpret_cast<virt_addr>(p));
}

void operator delete[](void *p) {
  system_memory_space->kvc.Free(reinterpret_cast<virt_addr>(p));
}

void operator delete[](void *p, size_t) {
  system_memory_space->kvc.Free(reinterpret_cast<virt_addr>(p));
}
