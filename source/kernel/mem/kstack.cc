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
#include <tty.h>
#include <apic.h>

bool KernelStackCtrl::_is_initialized = false;
KernelStackCtrl KernelStackCtrl::_ctrl;

extern int kKernelEndAddr;

void KernelStackCtrl::Init() {
  // initial stack is 1 page (allocated at boot/boot.S)
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (PagingCtrl::kPageSize * 0)));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (PagingCtrl::kPageSize * 1)));
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (PagingCtrl::kPageSize * 2)));

  kassert(reinterpret_cast<StackInfo *>(reinterpret_cast<virt_addr>(&kKernelEndAddr) - PagingCtrl::kPageSize + 1) - 1 == GetCurrentStackInfoPtr());
  kassert(GetCurrentStackInfoPtr()->magic == INITIAL_STACK_MAGIC);

  if (PagingCtrl::kPageSize < kStackSize) {
    // expand initial stack(1 page) to default stack size(4 page)
    int size = kStackSize - PagingCtrl::kPageSize;
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, size);
    kassert(paging_ctrl->MapPhysAddrToVirtAddr(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (kStackSize + PagingCtrl::kPageSize) + 1, paddr, size, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT));
  }

  new (&_ctrl) KernelStackCtrl();
  _ctrl.InitFirstStack();
  _is_initialized = true;
}

void KernelStackCtrl::InitFirstStack() {
  Locker locker(_lock);
  StackInfo *sinfo = GetCurrentStackInfoPtr();
  sinfo->magic = StackInfo::kMagic;
  sinfo->tid = _next_tid;
  sinfo->cpuid = apic_ctrl->GetCpuId();
  _next_tid++;

  _stack_area_top = reinterpret_cast<virt_addr>(&kKernelEndAddr) + 1 - (kStackSize + PagingCtrl::kPageSize * 2);
}

virt_addr KernelStackCtrl::AllocThreadStack(int cpuid) {
  virt_addr addr;
  if (!_freed.Pop(addr)) {
    Locker locker(_lock);

    _stack_area_top -= kStackSize + PagingCtrl::kPageSize * 2;
    addr = _stack_area_top + kStackSize + PagingCtrl::kPageSize - sizeof(StackInfo);
  
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, kStackSize);
    kassert(paging_ctrl->MapPhysAddrToVirtAddr(_stack_area_top + PagingCtrl::kPageSize, paddr, kStackSize, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT));
    bzero(reinterpret_cast<void *>(_stack_area_top + PagingCtrl::kPageSize), PagingCtrl::kPageSize);
  }

  StackInfo *sinfo = GetStackInfoPtr(addr);
  sinfo->magic = StackInfo::kMagic;
  sinfo->tid = _next_tid;
  sinfo->cpuid = cpuid;
  _next_tid++;

  return addr;
}

void KernelStackCtrl::CopyThreadStackFromCurrent(virt_addr addr) {
  StackInfo *info = GetStackInfoPtr(addr);
  kassert(info->IsValidMagic());
  StackInfo *current_info = GetCurrentStackInfoPtr();
  kassert(current_info->IsValidMagic());
  info->tid = current_info->tid;
  info->flag = current_info->flag;
  info->cpuid = current_info->cpuid;
}

void KernelStackCtrl::FreeThreadStack(virt_addr addr) {
  _freed.Push(addr);
}

virt_addr KernelStackCtrl::AllocIntStack(int cpuid) {
  Locker locker(_lock);

  _stack_area_top -= kStackSize + PagingCtrl::kPageSize * 2;
  
  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, kStackSize);
  kassert(paging_ctrl->MapPhysAddrToVirtAddr(_stack_area_top + PagingCtrl::kPageSize, paddr, kStackSize, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT));


  StackInfo *sinfo = GetStackInfoPtr(_stack_area_top + PagingCtrl::kPageSize);
  sinfo->magic = StackInfo::kMagic;
  sinfo->tid = _next_tid;
  sinfo->cpuid = cpuid;
  _next_tid++;

  return _stack_area_top + kStackSize + PagingCtrl::kPageSize - sizeof(StackInfo);
}
