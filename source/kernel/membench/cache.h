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

#include <cpu.h>
#include <tty.h>
#include <global.h>

class CacheCtrl {
public:
  void Init() {
    int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  
    if (cpuid == 0) {
      // CLFLUSHOPT exists?
      uint32_t ebx;
      asm volatile("cpuid;":"=b"(ebx):"a"(7),"c"(0));
      if ((ebx & (1 << 23)) != 0) {
        gtty->CprintfRaw("info: CLFLUSHOPT exists!\n");
        _use_clflushopt = true;
      }

      // CLFLUSH exists?
      uint32_t edx;
      asm volatile("cpuid;":"=d"(edx):"a"(1));
      if ((edx & (1 << 19)) == 0) {
        gtty->CprintfRaw("error: CLFLUSH not exists!\n");
        kassert(false);
      }

      // cache line size
      asm volatile("cpuid;":"=b"(ebx):"a"(1));
      _cacheline_size = ((ebx >> 8) & 0xFF) * 8;
      gtty->CprintfRaw("info: cacheline size is %d.\n", _cacheline_size);
    }
  }
  template<class T>
  void Clear(T *ptr, size_t size) {
    // use clflushopt if it exists
    for(size_t i = 0; i < size; i += _cacheline_size) {
      __builtin_ia32_clflush(reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(ptr) + i));
    }
  }
private:
  bool _use_clflushopt = false;
  size_t _cacheline_size = 0;
};

extern CacheCtrl *cache_ctrl;
