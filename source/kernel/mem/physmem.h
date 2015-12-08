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

#ifndef __RAPH_KERNEL_MEM_PHYSMEM_H__
#define __RAPH_KERNEL_MEM_PHYSMEM_H__

#include <stdint.h>
#include <assert.h>
#include "../spinlock.h"
#include "../list.h"
#include "virtmem.h"

typedef uint64_t phys_addr;
template <typename ptr> static inline phys_addr ptr2physaddr(ptr *addr) {
  return reinterpret_cast<phys_addr>(addr);
}

// リニアマップされた仮想アドレスを物理アドレスから取得する
static inline virt_addr p2v(phys_addr addr) {
  return reinterpret_cast<virt_addr>(0xffff800000000000 + addr);
}

static inline phys_addr v2p(virt_addr addr) {
  assert(addr >= 0xffff800000000000);
  return reinterpret_cast<phys_addr>(addr - 0xffff800000000000);
}

extern char kLinearAddrOffset;
static inline phys_addr k2p(virt_addr addr) {
  virt_addr koffset = ptr2virtaddr(&kLinearAddrOffset);
  assert(addr >= koffset);
  return reinterpret_cast<phys_addr>(addr - koffset);
}

template <typename ptr> static inline ptr *p2v(ptr *addr) {
  return reinterpret_cast<ptr *>(p2v(reinterpret_cast<phys_addr>(addr)));
}

class PhysAddr {
public:
  PhysAddr() {
    Reset();
  }
  PhysAddr(phys_addr addr) {
    Reset();
    SetAddr(addr);
  }
  // 原則的にPhysmemCtrl以外からは呼ばない事
  void SetAddr(phys_addr addr) {
    assert(!_is_initialized);
    _is_initialized = true;
    _addr = addr;
  }
  phys_addr GetAddr() {
    assert(_is_initialized);
    return _addr;
  }
  void Reset() {
    _is_initialized = false;
  }
  virt_addr GetVirtAddr() {
    return p2v(_addr);
  }
private:
  bool _is_initialized;
  phys_addr _addr;
};

class PhysmemCtrl {
 public:
  PhysmemCtrl();
  // Allocate,Freeでは、ページサイズに拡張したsizeを渡す事
  void Alloc(PhysAddr &paddr, size_t size);
  void Free(PhysAddr &paddr, size_t size);
 private:
  struct AllocatedArea {
    phys_addr start_addr;
    phys_addr end_addr;
    AllocatedArea *next;
  } *_allocated_area;
  List<AllocatedArea> _allocated_area_buffer;
  SpinLock _lock;
};

#endif // __RAPH_KERNEL_MEM_PHYSMEM_H__
