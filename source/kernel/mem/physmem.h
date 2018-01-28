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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#pragma once

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
} __attribute__((packed));

enum class SratStructType : uint8_t {
  kLocalApicAffinity = 0,
  kMemoryAffinity = 1,
  kLocalX2ApicAffinity = 2,
};

struct SratStruct {
  SratStructType type;
  uint8_t length;
} __attribute__((packed));

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
} __attribute__((packed));

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
} __attribute__((packed));

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
} __attribute__((packed));

// physical page allocator
// all arguments (address, size) must be aligned on an page boundary.
class PhysmemCtrl {
 public:
  PhysmemCtrl();
  void Init();
  void Alloc(PhysAddr &paddr, size_t size) {
    kassert(_is_able_to_allocate);
    AllocOption option = {1, false, nullptr};
    AllocSub(paddr, size, option);
  }
  void Alloc(PhysAddr &paddr, size_t size, size_t align) {
    AllocOption option = {align, false, nullptr};
    AllocSub(paddr, size, option);
  }
  // Alloc() allocates virtual memory internally.
  // use this method instead if it is inconvenient.
  void AllocNonRecursive(PhysAddr &paddr, size_t size) {
    AllocOption option = {1, true, nullptr};
    AllocSub(paddr, size, option);
  }
  void Free(PhysAddr &paddr, size_t size);
  void Reserve(phys_addr addr, size_t size) {
    Locker locker(_lock);
    ReserveSub(addr, size);
  }
  void SetSrat(Srat *srat) { _srat = srat; }
  static const virt_addr kLinearMapOffset = 0xffff800000000000;
  void Show();  // for debug
  void EnableToAllocate() {
    kassert(!_is_able_to_allocate);
    _is_able_to_allocate = true;
  }

 private:
  struct AllocOption {
    size_t align;
    bool non_recursive;
    struct Region {
      phys_addr start;
      phys_addr end;
    } * region;
  };
  void AllocSub(PhysAddr &paddr, size_t size, AllocOption &option);
  struct AllocatedArea {
    phys_addr start_addr;
    phys_addr end_addr;
    AllocatedArea *next;
  } * _allocated_area;
  void ReserveSub(phys_addr addr, size_t size);
  void ReserveSub2(AllocatedArea *allocated_area, phys_addr addr, size_t size);
  Allocator<AllocatedArea> _allocated_area_buffer;
  SpinLock _lock;
  bool _alloc_lock = false;
  bool _is_initialized = false;
  bool _is_able_to_allocate = false;
  Srat *_srat = nullptr;
};

template <typename ptr>
static inline phys_addr ptr2physaddr(ptr *addr) {
  return reinterpret_cast<phys_addr>(addr);
}

// convert physical memory to straight mapped virtual memory
static inline virt_addr p2v(phys_addr addr) {
  return reinterpret_cast<virt_addr>(PhysmemCtrl::kLinearMapOffset + addr);
}

// convert straight mapped virtual memory to physical memory
// do not use for kernel virtual memory. use k2p instead.
static inline phys_addr v2p(virt_addr addr) {
  // TODO check upper limit
  kassert(addr >= PhysmemCtrl::kLinearMapOffset);
  return reinterpret_cast<phys_addr>(addr - PhysmemCtrl::kLinearMapOffset);
}

template <typename ptr>
static inline ptr *p2v(ptr *addr) {
  return reinterpret_cast<ptr *>(p2v(reinterpret_cast<phys_addr>(addr)));
}

class PhysAddr {
 public:
  PhysAddr() { Reset(); }
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
  void Reset() { _is_initialized = false; }
  // ストレートマップド仮想メモリを返す
  virt_addr GetVirtAddr() { return p2v(_addr); }

 private:
  bool _is_initialized;
  phys_addr _addr;
};
