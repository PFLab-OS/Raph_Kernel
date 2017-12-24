/*
 *
 * Copyright (c) 2017 Raphine Project
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

#pragma once

#ifndef ASM_FILE

#include <raph.h>
#include <spinlock.h>
#include <mem/virtmem.h>
#include <mem/paging.h>
#include <_queue.h>
#include <_cpu.h>

class KernelStackCtrl;

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
  virt_addr AllocIntStack(CpuId cpuid);
  virt_addr AllocThreadStack(CpuId cpuid);
  void FreeThreadStack(virt_addr addr);
  static const int kStackSize = PagingCtrl::kPageSize * 4;
private:
  class FreedAddr : public QueueContainer<FreedAddr> {
  public:
    FreedAddr() : QueueContainer<FreedAddr>(this) {
    }
    virt_addr addr;
  };
  
  KernelStackCtrl() {
  }
  // initialize first kernel stack
  void InitFirstStack();
  
  static bool _is_initialized;
  static KernelStackCtrl _ctrl;
  
  virt_addr _stack_area_top;

  SpinLock _lock;

  Queue<FreedAddr> _freed;
};

#endif // ! ASM_FILE

