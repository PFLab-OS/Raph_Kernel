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

#ifndef __RAPH_KERNEL_GDT_H__
#define __RAPH_KERNEL_GDT_H__

#define KERNEL_CS (0x10)
#define KERNEL_DS (0x18)
#define TSS       (0x20)

#ifndef ASM_FILE

#include <stdint.h>
#include <raph.h>
#include <mem/paging.h>
#include <global.h>
#include <cpu.h>
#include <cpuinfo.h>

class Gdt {
 public:
  void SetupProc();
 private:
  struct Tss {
    uint32_t reserved1;
    uint32_t rsp0l;
    uint32_t rsp0h;
    uint32_t rsp1l;
    uint32_t rsp1h;
    uint32_t rsp2l;
    uint32_t rsp2h;
    uint32_t reserved2;
    uint32_t reserved3;
    uint32_t ist1l;
    uint32_t ist1h;
    uint32_t ist2l;
    uint32_t ist2h;
    uint32_t ist3l;
    uint32_t ist3h;
    uint32_t ist4l;
    uint32_t ist4h;
    uint32_t ist5l;
    uint32_t ist5h;
    uint32_t ist6l;
    uint32_t ist6h;
    uint32_t ist7l;
    uint32_t ist7h;
    uint32_t reserved4;
    uint32_t reserved5;
    uint16_t reserved6;
    uint16_t iomap;
  } __attribute__((packed));
};

class IntStackInfo {
public:
  IntStackInfo() : _cpuinfo(cpu_ctrl->GetId()) {
    SetSignature();
  }
  // IMPORTANT: do not call in the outside of stack handler
  static IntStackInfo *GetStackInfo() {
    uint64_t rsp;
    asm volatile("movq %%rsp, %0":"=r"(rsp));
    IntStackInfo *that = reinterpret_cast<IntStackInfo *>(alignUp(rsp, PagingCtrl::kPageSize) - Size());
    that->ValidateSignature();
    return that;
  }
  // addr : end of stack
  // return value : updated end of stack
  static virt_addr SetupStackInfo(virt_addr addr) {
    kassert(align(addr, PagingCtrl::kPageSize) == addr);
    addr -= Size();
    IntStackInfo *that = reinterpret_cast<IntStackInfo *>(addr);
    new(that) IntStackInfo();
    return addr;
  }
  CurrentCpuInfo &GetCpuInfo() {
    return _cpuinfo;
  }
private:
  static int Size() {
    return alignUp(sizeof(IntStackInfo), 8);
  }
  void SetSignature() {
    _signature = reinterpret_cast<uint64_t>(this);
  }
  void ValidateSignature() {
    kassert(_signature == reinterpret_cast<uint64_t>(this));
  }
  uint64_t _signature;
  CurrentCpuInfo _cpuinfo;
};

#endif // ! ASM_FILE
#endif // __RAPH_KERNEL_IDT_H__
