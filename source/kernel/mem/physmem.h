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

#include <_raph_acpi.h>
#include <stdint.h>
#include <assert.h>
#include <spinlock.h>
#include <allocator.h>
#include <mem/virtmem.h>

typedef uint64_t phys_addr;

class PhysAddr;

// see ACPI specification 5.2.16 System Resouce Affinity Table(SRAT)
struct Srat {
  ACPISDTHeader header;
  uint32_t reserved1;
  uint64_t reserved2;
  uint8_t table[0];
} __attribute__ ((packed));

enum class SratStructType : uint8_t {
  kLocalApicAffinity = 0,
  kMemoryAffinity = 1,
  kLocalX2ApicAffinity = 2,
};

struct SratStruct {
  SratStructType type;
  uint8_t length;
} __attribute__ ((packed));

struct SratStructLapic {
  SratStruct st;
  uint8_t proximity_domain_low;
  uint8_t lapic_id;
  uint32_t flags;
  uint8_t lsapic_eid;
  uint16_t proximity_domain_middle;
  uint8_t proximity_domain_high;
  uint32_t clock_domain;

  // const values
  static const uint32_t kFlagEnabled = 1;
} __attribute__ ((packed));

struct SratStructMemAffinity {
  SratStruct st;
  uint32_t proximity_domain;
  uint16_t reserved1;
  uint64_t base_addr;
  uint64_t length;
  uint32_t reserved2;
  uint32_t flags;
  uint64_t reserved3;

  // const values
  static const uint32_t kFlagEnabled = 1;
} __attribute__ ((packed));

struct SratStructLx2apic {
  SratStruct st;
  uint16_t reserved1;
  uint32_t proximity_domain;
  uint32_t lapic_id;
  uint32_t flags;
  uint32_t clock_domain;
  uint32_t reserved2;
  
  // const values
  static const uint32_t kFlagEnabled = 1;
} __attribute__ ((packed));

class PhysmemCtrl {
 public:
  PhysmemCtrl();
  void Init();
  // Allocate,Free,Reserveでは、ページサイズに拡張したsizeを渡す事
  void Alloc(PhysAddr &paddr, size_t size);
  // TODO support these functions
  // void Alloc(PhysAddr &paddr, size_t size, size_t align) {
  //   AllocOption option = {{align}, nullptr};
  //   AllocSub(paddr, size, option);
  // }
  // void Alloc(PhysAddr &paddr, size_t size, phys_addr start, phys_addr end) {
  //   AllocOption option = {nullptr, {start, end}};
  //   AllocSub(paddr, size, option);
  // }
  void Free(PhysAddr &paddr, size_t size);
  // addrはページサイズにアラインされている事
  void Reserve(phys_addr addr, size_t size) {
    Locker locker(_lock);
    ReserveSub(addr, size);
  }
  // Alloc内部で再度Allocが呼ばれるような場合を回避するための処理
  // page structure table用なので、4Kメモリしか割り当てられない
  void AllocFromBuffer(PhysAddr &paddr);
  void SetSrat(Srat *srat) {
    _srat = srat;
  }
  static const virt_addr kLinearMapOffset = 0xffff800000000000;
  void Show(); // for debug
 private:
  struct AllocOption {
    struct Align {
      size_t val;
    } *align;
    struct Region {
      phys_addr start;
      phys_addr end;
    } *region;
  };
  void AllocSub(PhysAddr &paddr, size_t size, AllocOption &option);
  struct AllocatedArea {
    phys_addr start_addr;
    phys_addr end_addr;
    AllocatedArea *next;
  } *_allocated_area;
  void ReserveSub(phys_addr addr, size_t size);
  void ReserveSub2(AllocatedArea *allocated_area, phys_addr addr, size_t size);
  Allocator<AllocatedArea> _allocated_area_buffer;
  SpinLock _lock;
  bool _alloc_lock = false;
  bool _is_initialized = false;
  Srat *_srat = nullptr;
};

inline void PhysmemCtrl::Alloc(PhysAddr &paddr, size_t size) {
  AllocOption option = {nullptr, nullptr};
  AllocSub(paddr, size, option);
}

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
