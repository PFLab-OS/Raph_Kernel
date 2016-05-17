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

#include "kvirtmem.h"
#include <assert.h>
#include <raph.h>
#include "physmem.h"
#include "paging.h"

extern int virt_memory_start;
extern int virt_memory_end;
extern int virt_allocatedmemory_end;
extern int phys_memory_end;
extern int kLinearAddrOffset;
extern int kHeapEndAddr;

KVirtmemCtrl::KVirtmemCtrl() {
  virt_addr tmp1 = alignUp(ptr2virtaddr(&virt_memory_end), 8);
  virt_addr tmp2 = align(ptr2virtaddr(&virt_allocatedmemory_end), 8);
  _heap_end = ptr2virtaddr(&kHeapEndAddr);

  _first = reinterpret_cast<AreaManager *>(tmp1);
  _first = new(_first) AreaManager(_first, _first, tmp2 - tmp1);

  // 2MB allocated by boot.S
  virt_addr tmp3 = reinterpret_cast<virt_addr>(&kLinearAddrOffset) + reinterpret_cast<virt_addr>(&phys_memory_end);
  virt_addr tmp4 = reinterpret_cast<virt_addr>(&kLinearAddrOffset) + 0x200000;
  kassert(tmp3 + AreaManager::GetAreaManagerSize() < tmp4);
  AreaManager *second = reinterpret_cast<AreaManager *>(tmp3);
  _first->Append(second, tmp4 - tmp3);
  
  _list = _first;
}

virt_addr KVirtmemCtrl::Alloc(size_t size) {
  Locker locker(_lock);
  size = alignUp(size, 8);
  if (_last_freed != nullptr && _last_freed->GetSize() >= size) {
    AreaManager *best = _last_freed;
    _last_freed = _last_freed->Divide(size);
    best->Allocate();
    return best->GetAreaStartAddr();
  }
  AreaManager *cur = _list;
  AreaManager *min = cur;
  AreaManager *best = nullptr;
  int cnt = 10;
  while(true) {
    if (!cur->IsAllocated()) {
      if (cur->GetSize() >= size) {
        if (best == nullptr) {
          best = cur;
        } else if (best->GetSize() > cur->GetSize()) {
          best = cur;
        }
      }
      if (best != nullptr) {
        cnt--;
        if (cnt == 0) {
          AreaManager *divided = best->Divide(size);
          if (divided != nullptr && min->GetSize() > divided->GetSize()) {
            min = divided;
          }
          if (min == best) {
            min = min->GetNext();
          }
          break;
        }
      }
      if (min->GetSize() > cur->GetSize() && cur != best) {
        min = cur;
      }
    }
    cur = cur->GetNext();
    kassert(cur != nullptr);
    if (cur == _list && best == nullptr) {
      break;
    }
  }
  if (best == nullptr) {
    // 新規にページ割り当て
    AreaManager *end = _first->GetPrev();
    virt_addr allocated_addr_end = end->GetAddr() + end->GetSize() + end->GetAreaManagerSize();
    kassert(allocated_addr_end == align(allocated_addr_end, PagingCtrl::kPageSize));
    size_t psize = alignUp(size + AreaManager::GetAreaManagerSize(), PagingCtrl::kPageSize);
    if (allocated_addr_end + psize < _heap_end) {
      PhysAddr paddr;
      physmem_ctrl->Alloc(paddr, psize);
      kassert(paging_ctrl->MapPhysAddrToVirtAddr(allocated_addr_end, paddr, psize, PDE_WRITE_BIT, PTE_WRITE_BIT || PTE_GLOBAL_BIT));
      AreaManager *next = reinterpret_cast<AreaManager *>(allocated_addr_end);
      end->Append(next, psize);
      best = next;
    } else {
      // マジかよ
      kassert(false && "not enough kernel heap memory");
    }
  }
  kassert(min != nullptr);
  _list = min;
  best->Allocate();
  kassert(best != _last_freed);
  return best->GetAreaStartAddr();
}

void KVirtmemCtrl::Free(virt_addr addr) {
  Locker locker(_lock);
  AreaManager *area = GetAreaManager(addr);
  area->Free();
  AreaManager *tmp = area->GetNext();
  _last_freed = area;
  if (area->UnifyWithNext()) {
    if (_list == tmp) {
      _list = area;
    }
  }
  AreaManager *prev = area->GetPrev();
  if (prev->UnifyWithNext()) {
    _last_freed = prev;
    if (_list == area) {
      _list = prev;
    }
  }
}
