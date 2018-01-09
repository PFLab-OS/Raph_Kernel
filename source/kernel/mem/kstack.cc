/*
 *
 * Copyright (c) 2016 Raphine Project
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

#include "kstack.h"
#include <string.h>
#include <mem/paging.h>
#include <mem/physmem.h>
#include <global.h>
#include <cpu.h>
#include <tty.h>
#include <cpu.h>

bool KernelStackCtrl::_is_initialized = false;
KernelStackCtrl KernelStackCtrl::_ctrl;

extern int kKernelEndAddr;

void KernelStackCtrl::Init() {
  // initial stack is 1 page (allocated at boot/boot.S)
  kassert(!system_memory_space->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (PagingCtrl::kPageSize * 0)));
  kassert(system_memory_space->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (PagingCtrl::kPageSize * 1)));
  kassert(!system_memory_space->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (PagingCtrl::kPageSize * 2)));

  if (PagingCtrl::kPageSize < kStackSize) {
    // expand initial stack(1 page) to default stack size(4 page)
    int size = kStackSize - PagingCtrl::kPageSize;
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, size);
    kassert(system_memory_space->MapPhysAddrToVirtAddr(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (kStackSize + PagingCtrl::kPageSize) + 1, paddr, size, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT));
  }

  new (&_ctrl) KernelStackCtrl();
  _ctrl.InitFirstStack();
  _is_initialized = true;
}

void KernelStackCtrl::InitFirstStack() {
  Locker locker(_lock);
  _stack_area_top = reinterpret_cast<virt_addr>(&kKernelEndAddr) + 1 - (kStackSize + PagingCtrl::kPageSize * 2);
}

virt_addr KernelStackCtrl::AllocThreadStack(CpuId cpuid) {
  FreedAddr *fac;
  virt_addr addr;
  if (!_freed.Pop(fac)) {
    Locker locker(_lock);

    _stack_area_top -= kStackSize + PagingCtrl::kPageSize * 2;
    addr = _stack_area_top + kStackSize + PagingCtrl::kPageSize;
  
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, kStackSize);
    kassert(system_memory_space->MapPhysAddrToVirtAddr(_stack_area_top + PagingCtrl::kPageSize, paddr, kStackSize, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT));
    bzero(reinterpret_cast<void *>(_stack_area_top + PagingCtrl::kPageSize), PagingCtrl::kPageSize);
  } else {
    addr = fac->addr;
    delete fac;
  }

  return addr;
}

void KernelStackCtrl::FreeThreadStack(virt_addr addr) {
  FreedAddr *fac = new FreedAddr;
  fac->addr = addr;
  _freed.Push(fac);
}

virt_addr KernelStackCtrl::AllocIntStack(CpuId cpuid) {
  Locker locker(_lock);

  _stack_area_top -= kStackSize + PagingCtrl::kPageSize * 2;
  
  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, kStackSize);
  kassert(system_memory_space->MapPhysAddrToVirtAddr(_stack_area_top + PagingCtrl::kPageSize, paddr, kStackSize, PDE_WRITE_BIT | PDE_USER_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT | PTE_USER_BIT));

  return _stack_area_top + kStackSize + PagingCtrl::kPageSize;
}
