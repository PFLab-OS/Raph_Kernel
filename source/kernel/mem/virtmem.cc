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

#include "virtmem.h"
#include <assert.h>
#include <errno.h>
#include <raph.h>
#include "physmem.h"
#include "paging.h"

extern int phys_memory_end;
extern int kLinearAddrOffset;
extern int kHeapEndAddr;

extern "C" {
  void* dlmalloc(size_t);
  void  dlfree(void*);
}

VirtmemCtrl::VirtmemCtrl() {
  // カーネル仮想メモリは最大1792MB
  _heap_limit = ptr2virtaddr(&kHeapEndAddr);
  
  // 6MB allocated by boot.S
  _brk_end = reinterpret_cast<virt_addr>(&kLinearAddrOffset) + reinterpret_cast<virt_addr>(&phys_memory_end);
  _heap_allocated_end = reinterpret_cast<virt_addr>(&kLinearAddrOffset) + 0x600000;
  kassert(_brk_end < _heap_allocated_end);
}

virt_addr VirtmemCtrl::Alloc(size_t size) {
  Locker locker(_lock);
  return reinterpret_cast<virt_addr>(dlmalloc(size));
}

void VirtmemCtrl::Free(virt_addr addr) {
  Locker locker(_lock);
  dlfree(reinterpret_cast<void *>(addr));
}

virt_addr VirtmemCtrl::Sbrk(int64_t increment) {
  virt_addr _old_brk_end = _brk_end;
  _brk_end += increment;
  if (_brk_end > _heap_allocated_end) {
    virt_addr new_heap_allocated_end = alignUp(_brk_end, PagingCtrl::kPageSize);
    if (new_heap_allocated_end <= _heap_limit) {
      kassert(_heap_allocated_end == align(_heap_allocated_end, PagingCtrl::kPageSize));
      virt_addr psize = new_heap_allocated_end - _heap_allocated_end;
    
      PhysAddr paddr;
      physmem_ctrl->Alloc(paddr, psize);
      kassert(paging_ctrl->MapPhysAddrToVirtAddr(_heap_allocated_end, paddr, psize, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT));
      _heap_allocated_end = new_heap_allocated_end;
    } else {
      kassert(false && "not enough kernel heap memory");
    }
  }
  return _old_brk_end;
}

extern "C" void *sbrk(intptr_t increment) {
  virt_addr addr = virtmem_ctrl->Sbrk(increment);
  if (addr == 0xffffffffffffffff) {
    errno = ENOMEM;
  }
  return reinterpret_cast<void *>(addr);
}

void *operator new(size_t size) {
  return reinterpret_cast<void *>(virtmem_ctrl->Alloc(size));
}
 
void *operator new[](size_t size) {
  return reinterpret_cast<void *>(virtmem_ctrl->Alloc(size));
}
 
void operator delete(void *p) {
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(p));
}

void operator delete(void *p, size_t) {
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(p));
}

void operator delete[](void *p) {
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(p));
}
 
void operator delete[](void *p, size_t) {
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(p));
}
