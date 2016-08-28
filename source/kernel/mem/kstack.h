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

#ifndef __RAPH_KERNEL_MEM_KSTACK_H__
#define __RAPH_KERNEL_MEM_KSTACK_H__

#define INITIAL_STACK_MAGIC 0x1234567890ABCDEF

#ifndef ASM_FILE

#include <raph.h>
#include <spinlock.h>
#include <mem/virtmem.h>
#include <mem/paging.h>

class KernelStackCtrl;
class ThreadId {
private:
  friend KernelStackCtrl;
  uint64_t _tid;
};

class KernelStackCtrl {
public:
  static void Init();
  static bool IsInitialized() {
    return _is_initialized;
  }
  static KernelStackCtrl &GetCtrl() {
    kassert(_is_initialized);
    return _ctrl;
  }
  static void GetCurrentThreadId(ThreadId &tid) {
    StackInfo *sinfo = GetCurrentStackInfoPtr();
    kassert(sinfo->IsValidMagic());
    tid._tid = sinfo->tid;
  }
  static int GetCpuId() {
    StackInfo *sinfo = GetCurrentStackInfoPtr();
    kassert(sinfo->IsValidMagic());
    return sinfo->cpuid;
  }
  virt_addr AllocIntStack(int cpuid);
  virt_addr AllocThreadStack(int cpuid);
  static const int kStackSize = PagingCtrl::kPageSize * 4;
private:
  struct StackInfo {
    uint64_t magic;
    uint64_t flag;
    uint64_t tid;
    uint64_t cpuid;
    bool IsValidMagic() {
      return magic == kMagic;
    }
    static const uint64_t kMagic = 0xABCDEF0123456789;
  };

  KernelStackCtrl() {
  }
  static StackInfo* GetCurrentStackInfoPtr() {
    virt_addr rsp;
    asm volatile("movq %%rsp, %0": "=r"(rsp));
    return GetCurrentStackInfoPtr(rsp);
  }
  static StackInfo* GetCurrentStackInfoPtr(virt_addr rsp) {
    int stack_block_size = kStackSize + PagingCtrl::kPageSize * 2;
    extern int kKernelEndAddr;
    virt_addr stack_top = reinterpret_cast<virt_addr>(&kKernelEndAddr) + 1 - (((reinterpret_cast<virt_addr>(&kKernelEndAddr) + 1 - rsp) / stack_block_size) * stack_block_size + PagingCtrl::kPageSize);
    return reinterpret_cast<StackInfo *>(stack_top) - 1;
  }
  // initialize first kernel stack
  void InitFirstStack();
  
  static bool _is_initialized;
  static KernelStackCtrl _ctrl;
  
  uint64_t _next_tid = 0;
  virt_addr _stack_area_top;

  SpinLock _lock;
};

#endif // ! ASM_FILE

#endif // __RAPH_KERNEL_MEM_KSTACK_H__
