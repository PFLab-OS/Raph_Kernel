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
#include "../raph.h"
#include "physmem.h"
#include "paging.h"

extern int virt_memory_start;
extern int virt_memory_end;
extern int virt_allocatedmemory_end;
extern int phys_memory_end;
extern int kLinearAddrOffset;
extern int kHeapEndAddr;

#ifdef __UNIT_TEST__

#include <cassert>
#include <iostream>
#include <random>
#include <map>

static void check() {
  VirtmemCtrl::AreaManager *area = virtmem_ctrl->_list;
  bool flag = false;
  while(true) {
    assert(area->_size == align(area->_size, 8));
    assert(area->GetAddr() + area->_size == area->_next->GetAddr() || area->_next <= area);
    assert(area->_next > area || (!flag && area->_next <= area));
    if (area->_next <= area) {
      flag = true;
    }
    area = area->_next;
    if (area == virtmem_ctrl->_list) {
      break;
    }
  }
  assert(flag);
  flag = false;
  while(true) {
    assert(area->_prev->GetAddr() + area->_prev->_size == area->GetAddr() || area <= area->_prev);
    assert(area > area->_prev || (!flag && area <= area->_prev));
    if (area <= area->_prev) {
      flag = true;
    }
    area = area->_prev;
    if (area == virtmem_ctrl->_list) {
      break;
    }
  }
}

static void test1() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    assert(virtmem_ctrl->_list != nullptr);
    assert(virtmem_ctrl->_list->_next == virtmem_ctrl->_list);
    assert(virtmem_ctrl->_list->_prev == virtmem_ctrl->_list);
    assert(!virtmem_ctrl->_list->_is_allocated);
    check();
  } catch(const char*) {
    assert(false);
  }
}

static void test2() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    VirtmemCtrl::AreaManager *am1 = virtmem_ctrl->_list;
    VirtmemCtrl::AreaManager *am2 = am1->Divide(64);
    assert(am2 != nullptr);
    assert(am1->_next == am2);
    assert(am1->_prev == am2);
    assert(am2->_next == am1);
    assert(am2->_prev == am1);
    check();

    VirtmemCtrl::AreaManager *am3 = am2->Divide(64);
    assert(am3 != nullptr);
    assert(am1->_next == am2);
    assert(am1->_prev == am3);
    assert(am2->_next == am3);
    assert(am2->_prev == am1);
    assert(am3->_next == am1);
    assert(am3->_prev == am2);
    check();

    VirtmemCtrl::AreaManager *am4 = am1->Divide(8);
    assert(am4 != nullptr);
    assert(am1->_next == am4);
    assert(am1->_prev == am3);
    assert(am4->_next == am2);
    assert(am4->_prev == am1);
    assert(am2->_next == am3);
    assert(am2->_prev == am4);
    assert(am3->_next == am1);
    assert(am3->_prev == am2);
    check();

    assert(am1->UnifyWithNext());
    assert(am1->UnifyWithNext());
    assert(am1->UnifyWithNext());
    assert(!am1->UnifyWithNext());
    assert(am1 == virtmem_ctrl->_list);
    assert(am1->_next = am1);
    assert(am1->_prev = am1);
    check();
  } catch(const char*) {
    assert(false);
  }
}

static void test3() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    VirtmemCtrl::AreaManager *am1 = virtmem_ctrl->_list;
    VirtmemCtrl::AreaManager *am2 = am1->Divide(64);
    VirtmemCtrl::AreaManager *am3 = am2->Divide(64);
    VirtmemCtrl::AreaManager *am4 = am1->Divide(8);
    check();

    am2->Allocate();

    assert(!am4->UnifyWithNext());
    assert(am1->UnifyWithNext());
    assert(!am1->UnifyWithNext());
    assert(am1->_prev == am3);
    assert(am1->_next == am2);
    assert(am3->_next == am1);
    assert(am2->_prev == am1);
    check();

    assert(!am3->UnifyWithNext());
    assert(!am2->UnifyWithNext());
    assert(am3->_prev == am2);
    assert(am3->_next == am1);
    assert(am2->_next == am3);
    assert(am1->_prev == am3);
    check();
  } catch(const char*) {
    assert(false);
  }
}

static void test4() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    virt_addr addr = virtmem_ctrl->Alloc(16);
    VirtmemCtrl::AreaManager *am = VirtmemCtrl::GetAreaManager(addr);
    assert(am->_next == virtmem_ctrl->_list);
    assert(am->_prev == virtmem_ctrl->_list);
    assert(am->_is_allocated);
    assert(!virtmem_ctrl->_list->_is_allocated);
    check();
  } catch(const char*) {
    assert(false);
  }
}

static void test5() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    virt_addr addr1 = virtmem_ctrl->Alloc(128);
    check();
    virtmem_ctrl->Free(addr1);
    check();
    virt_addr addr2 = virtmem_ctrl->Alloc(128);
    assert(addr1 == addr2);
    check();

    virt_addr addr3 = virtmem_ctrl->Alloc(128);
    assert(addr2 != addr3);
    check();
  } catch(const char*){
    assert(false);
  }
}

static void test6() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    virtmem_ctrl->Alloc(256);
    check();
    virt_addr addr2 = virtmem_ctrl->Alloc(16);
    check();
    virt_addr addr3 = virtmem_ctrl->Alloc(256);
    check();
    virtmem_ctrl->Free(addr2);
    virt_addr addr4 = virtmem_ctrl->Alloc(16);
    check();
    assert(VirtmemCtrl::GetAreaManager(addr4) != VirtmemCtrl::GetAreaManager(addr3)->GetNext());
    assert(addr2 == addr4);
  } catch(const char*) {
    assert(false);
  }
}

static void test7() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    std::map<virt_addr, size_t> map;
    std::random_device rnd;
    std::mt19937 mt(rnd());
    std::uniform_int_distribution<> rndsize(1, 20); 
    for(int i = 0; i < 1000000; i++){
      assert(virtmem_ctrl->_list != nullptr);
      std::uniform_int_distribution<> rndop(0, (map.size() * 3) / 2); 
      int op = rndop(mt);
      if (map.size() <= static_cast<size_t>(op)) {
        size_t size = rndsize(mt);
        virt_addr addr;
        map.insert(std::pair<virt_addr, size_t>(addr=virtmem_ctrl->Alloc(size), size));
        for (size_t j = 0; j < size; j++) {
          reinterpret_cast<char *>(addr)[j] = 0;
        }
        check();
        fflush(stdout);
      } else {
        int cnt = 0;
        auto it = map.begin();
        while(it != map.end()) {
          if (cnt == op) {
            break;
          }
          cnt++;
          ++it;
        }
        fflush(stdout);
        virtmem_ctrl->Free((*it).first);
        check();
        map.erase(it);
      }
    }
  } catch(const char*) {
    assert(false);
  }
}

static void test8() {
  try {
    VirtmemCtrl virtmem_ctrl_;
    virtmem_ctrl = &virtmem_ctrl_;
    std::map<virt_addr, size_t> map;
    std::random_device rnd;
    std::mt19937 mt(rnd());
    std::uniform_int_distribution<> rndsize(100, 2000); 
    for(int i = 0; i < 1000000; i++){
      assert(virtmem_ctrl->_list != nullptr);
      std::uniform_int_distribution<> rndop(0, map.size() * 3); 
      int op = rndop(mt);
      if (map.size() <= static_cast<size_t>(op)) {
        size_t size = rndsize(mt);
        virt_addr addr;
        map.insert(std::pair<virt_addr, size_t>(addr=virtmem_ctrl->Alloc(size), size));
        for (size_t j = 0; j < size; j++) {
          reinterpret_cast<char *>(addr)[j] = 0;
        }
        check();
        fflush(stdout);
      } else {
        int cnt = 0;
        auto it = map.begin();
        while(it != map.end()) {
          if (cnt == op) {
            break;
          }
          cnt++;
          ++it;
        }
        fflush(stdout);
        virtmem_ctrl->Free((*it).first);
        check();
        map.erase(it);
      }
    }
  } catch(const char*) {
    assert(false);
  }
}

void virtmem_test() {
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
}

static void *virtmem_test_mem;

VirtmemCtrl::VirtmemCtrl() {
  size_t init_size = PagingCtrl::kPageSize;
  size_t allocate_size = 0x1000000;
  virtmem_test_mem = malloc(allocate_size);
  virt_addr tmp1 = reinterpret_cast<virt_addr>(virtmem_test_mem);
  tmp1 = alignUp(tmp1, 8);
  virt_addr tmp2 = align(tmp1 + init_size, 8);
  virt_addr tmp3 = align(tmp1 + allocate_size, 8);
  _heap_end = tmp3;

  _first = reinterpret_cast<VirtmemCtrl::AreaManager *>(tmp1);
  _first = new(_first) VirtmemCtrl::AreaManager(_first, _first, tmp2 - tmp1);
  _list = _first;
}

VirtmemCtrl::~VirtmemCtrl() {
  free(virtmem_test_mem);
}

#else

VirtmemCtrl::VirtmemCtrl() {
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

VirtmemCtrl::~VirtmemCtrl() {}

#endif // __UNIT_TEST__

#ifndef __UNIT_TEST__

virt_addr VirtmemCtrl::Alloc(size_t size) {
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
#else
virt_addr VirtmemCtrl::Alloc(size_t size) {
  return reinterpret_cast<virt_addr>(new uint8_t[size]);
}
#endif

#ifndef __UNIT_TEST__
void VirtmemCtrl::Free(virt_addr addr) {
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
#else
void VirtmemCtrl::Free(virt_addr addr) {
  delete [] reinterpret_cast<uint64_t*>(addr);
}
#endif
