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

#include "physmem.h"
#include "paging.h"
#include <global.h>
#include <multiboot.h>
#include <tty.h>

extern int *phys_memory_end;

#include <raph.h>
PhysmemCtrl::PhysmemCtrl() {
  _allocated_area = _allocated_area_buffer.Alloc();
  _allocated_area->start_addr = 0;
  kassert(ptr2physaddr(&phys_memory_end) % PagingCtrl::kPageSize == 0);
  // 6MB allocated by boot.S
  kassert(reinterpret_cast<phys_addr>(&phys_memory_end) < 0x600000);
  _allocated_area->end_addr = 0x600000;
  _allocated_area->next = nullptr;
}

void PhysmemCtrl::Init() {
  if (_srat != nullptr) {
    for(uint32_t offset = 0; offset < _srat->header.Length - sizeof(Srat);) {
      virt_addr ptr = ptr2virtaddr(_srat->table) + offset;
      SratStruct *srat_st = reinterpret_cast<SratStruct *>(ptr);
      switch (srat_st->type) {
      case SratStructType::kLocalApicAffinity:
        {
          // SratStructLapic *srat_st_lapic = reinterpret_cast<SratStructLapic *>(ptr);
          // gtty->CprintfRaw("(APIC(%d), domain:%d)", srat_st_lapic->lapic_id, (srat_st_lapic->proximity_domain_high << 24) | (srat_st_lapic->proximity_domain_middle << 16) | srat_st_lapic->proximity_domain_low);
        }
        break;
      case SratStructType::kLocalX2ApicAffinity:
        {
          // SratStructLx2apic *srat_st_lapic = reinterpret_cast<SratStructLx2apic *>(ptr);
          // gtty->CprintfRaw("(APIC(%d), domain:%d)", srat_st_lapic->lapic_id, srat_st_lapic->proximity_domain);
        }
        break;
      case SratStructType::kMemoryAffinity:
        {
          SratStructMemAffinity *srat_st_mem = reinterpret_cast<SratStructMemAffinity *>(ptr);
          if ((srat_st_mem->flags & SratStructMemAffinity::kFlagEnabled) != 0) {
            gtty->CprintfRaw("(Mem domain:%d, base:%llx, length:%llx)\n", srat_st_mem->proximity_domain, srat_st_mem->base_addr, srat_st_mem->length);
          }
        }
        break;
      default:
        break;
      }
      offset += srat_st->length;
    }
  }
  
  _is_initialized = true;
}

void PhysmemCtrl::AllocSub(PhysAddr &paddr, size_t size, PhysmemCtrl::AllocOption &option) {
  kassert(_is_initialized);
  kassert(size > 0);
  kassert(size % PagingCtrl::kPageSize == 0);
  _alloc_lock = false;
  phys_addr allocated_addr = 0;
  AllocatedArea *allocated_area = nullptr;
  AllocatedArea *fraged_area = nullptr;
  {
    Locker lock(_lock);
    allocated_area = _allocated_area;
    while(allocated_area->next) {
      if (fraged_area == nullptr &&
          allocated_area->next->start_addr == allocated_area->end_addr) {
        fraged_area = allocated_area;
      }
      if (allocated_area->next->start_addr - allocated_area->end_addr
          >= static_cast<phys_addr>(size)) {
        allocated_addr = allocated_area->end_addr;
        break;
      }
      allocated_area = allocated_area->next;
    }
    allocated_addr = allocated_area->end_addr;

    allocated_area->end_addr += size;

    if (allocated_area->end_addr >= multiboot_ctrl->GetPhysMemoryEnd()) {
      kernel_panic("PhysmemCtrl", "no physical memory");
    }
    
    // 断片化された領域を結合する
    if (fraged_area != nullptr) {
      AllocatedArea *remove_area = fraged_area->next;
      fraged_area->end_addr = remove_area->end_addr;
      fraged_area->next = remove_area->next;
      fraged_area = remove_area;
    }
  }
  if (fraged_area != nullptr) {
    _allocated_area_buffer.Free(fraged_area);
  }
  paddr.SetAddr(allocated_addr);
  return;
}

void PhysmemCtrl::Free(PhysAddr &paddr, size_t size) {
  kassert(size > 0);
  kassert(size % PagingCtrl::kPageSize == 0);
  AllocatedArea *newarea = _allocated_area_buffer.Alloc();
  bool newarea_notused = false;
  phys_addr addr = paddr.GetAddr();
  paddr.Reset();
  AllocatedArea *allocated_area = nullptr;
  {
    Locker locker(_lock);
    allocated_area = _allocated_area;
    while(allocated_area) {
      if (allocated_area->start_addr <= addr &&
          addr + size <= allocated_area->end_addr) {
        break;
      }
      allocated_area = allocated_area->next;
    }
    if (allocated_area) {
      if (allocated_area->start_addr == addr) {
        newarea_notused = true;
        allocated_area->start_addr += size;
      } else if (allocated_area->end_addr == addr + size) {
        newarea_notused = true;
        allocated_area->end_addr = addr;
      } else {
        newarea_notused = false;
        newarea->start_addr = addr + size;
        newarea->end_addr = allocated_area->end_addr;
        newarea->next = allocated_area->next;
        allocated_area->end_addr = addr;
        // allocated_area->nextの書き込みは最後にする事
        // 他のプロセスのReadで事故る可能性があるので
        allocated_area->next = newarea;
      }
    }
  }
  if (newarea_notused) {
    _allocated_area_buffer.Free(newarea);
  }
}

void PhysmemCtrl::ReserveSub(phys_addr addr, size_t size) {
  kassert(size > 0);
  kassert(size % PagingCtrl::kPageSize == 0);
  kassert(addr % PagingCtrl::kPageSize == 0);
  AllocatedArea *fraged_area = nullptr;
  AllocatedArea *allocated_area = _allocated_area;
  assert(allocated_area != nullptr);
  if (allocated_area->next == nullptr) {
    ReserveSub2(allocated_area, addr, size);
    return;
  }
  while(allocated_area->next) {
    if (fraged_area == nullptr &&
        allocated_area->next->start_addr == allocated_area->end_addr) {
      fraged_area = allocated_area;
    }
    kassert(allocated_area->start_addr <= addr);
    if (allocated_area->end_addr > addr) {
      if (allocated_area->end_addr > addr + size) {
        break;
      }
      if (addr + size <= allocated_area->next->start_addr) {
        allocated_area->end_addr = addr + size;
        break;
      }
      allocated_area->end_addr = allocated_area->next->start_addr;
      ReserveSub(allocated_area->next->start_addr, addr + size - allocated_area->next->start_addr);
      return;
    }
    if (allocated_area->next->start_addr >= addr) {
      if (addr + size <= allocated_area->next->start_addr) {
        AllocatedArea *newarea = _allocated_area_buffer.Alloc();
        newarea->next = allocated_area->next;
        allocated_area->next = newarea;
        newarea->start_addr = addr;
        newarea->end_addr = addr + size;
        break;
      }
      phys_addr tmp = allocated_area->next->start_addr;
      allocated_area->next->start_addr = addr;
      ReserveSub(tmp, addr + size - tmp);
      return;
    }
    allocated_area = allocated_area->next;
    if (allocated_area->next == nullptr) {
      ReserveSub2(allocated_area, addr, size);
      break;
    }
  }
    
  // 断片化された領域を結合する
  if (fraged_area != nullptr) {
    AllocatedArea *remove_area = fraged_area->next;
    fraged_area->end_addr = remove_area->end_addr;
    fraged_area->next = remove_area->next;
    fraged_area = remove_area;
    _allocated_area_buffer.Free(fraged_area);
  }
}

void PhysmemCtrl::ReserveSub2(AllocatedArea *allocated_area, phys_addr addr, size_t size) {
  if (allocated_area->end_addr > addr) {
    if (allocated_area->end_addr < addr + size) {
      allocated_area->end_addr = addr + size;
    }
  } else {
    AllocatedArea *newarea = _allocated_area_buffer.Alloc();
    newarea->next = allocated_area->next;
    allocated_area->next = newarea;
    newarea->start_addr = addr;
    newarea->end_addr = addr + size;
  }
}

void PhysmemCtrl::Show() {
  AllocatedArea *allocated_area = _allocated_area;
  while(allocated_area) {
    gtty->CprintfRaw(">> %llx, %llx\n", allocated_area->start_addr, allocated_area->end_addr);
    allocated_area = allocated_area->next;
  }
}
