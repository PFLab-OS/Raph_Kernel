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
#include <spinlock.h>
#include <allocator.h>
#include <mem/virtmem.h>

typedef uint64_t phys_addr;

class PhysAddr;

class PhysmemCtrl {
 public:
  PhysmemCtrl();
  // Allocate,Free,Reserveでは、ページサイズに拡張したsizeを渡す事
  void Alloc(PhysAddr &paddr, size_t size);
  void Free(PhysAddr &paddr, size_t size);
  // addrはページサイズにアラインされている事
  void Reserve(phys_addr addr, size_t size) {
    Locker locker(_lock);
    ReserveSub(addr, size);
  }
  // Alloc内部で再度Allocが呼ばれるような場合を回避するための処理
  // page structure table用なので、4Kメモリしか割り当てられない
  void AllocFromBuffer(PhysAddr &paddr);
  static const virt_addr kLinearMapOffset = 0xffff800000000000;
 private:
  void ReserveSub(phys_addr addr, size_t size);
  struct AllocatedArea {
    phys_addr start_addr;
    phys_addr end_addr;
    AllocatedArea *next;
  } *_allocated_area;
  Allocator<AllocatedArea> _allocated_area_buffer;
  SpinLock _lock;
  bool _alloc_lock = false;
};

template <typename ptr> static inline phys_addr ptr2physaddr(ptr *addr) {
  return reinterpret_cast<phys_addr>(addr);
}

// リニアマップされた仮想アドレスを物理アドレスから取得する
static inline virt_addr p2v(phys_addr addr) {
  return reinterpret_cast<virt_addr>(PhysmemCtrl::kLinearMapOffset + addr);
}

// ストレートマップド仮想メモリを物理メモリに変換する
// カーネル仮想メモリ等は変換できないのでk2pを使う
static inline phys_addr v2p(virt_addr addr) {
  //TODO 上限
  kassert(addr >= PhysmemCtrl::kLinearMapOffset);
  return reinterpret_cast<phys_addr>(addr - PhysmemCtrl::kLinearMapOffset);
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
  // 原則的にPhysmemCtrl、PagingCtrl以外からは呼ばない事
  void SetAddr(phys_addr addr) {
    kassert(!_is_initialized);
    _is_initialized = true;
    _addr = addr;
  }
  phys_addr GetAddr() {
    kassert(_is_initialized);
    return _addr;
  }
  void Reset() {
    _is_initialized = false;
  }
  // ストレートマップド仮想メモリを返す
  virt_addr GetVirtAddr() {
    return p2v(_addr);
  }
private:
  bool _is_initialized;
  phys_addr _addr;
};

#endif // __RAPH_KERNEL_MEM_PHYSMEM_H__
