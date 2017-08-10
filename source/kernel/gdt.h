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

#define KERNEL_CS  (0x10)
#define KERNEL_DS  (0x18)
#define USER_DS    (0x23)
#define USER_CS    (0x2B)
#define KERNEL_CPU (0x30)
#define TSS        (0x40)

#define MSR_IA32_FS_BASE (0xC0000100)
#define MSR_IA32_GS_BASE (0xC0000101)

#ifndef ASM_FILE

#include <stdint.h>
#include <raph.h>
#include <mem/paging.h>
#include <global.h>
#include <_cpu.h>

class Gdt {
 public:
  void SetupProc();
  static bool IsInitializedPerCpuStruct() {
    return x86::rdmsr(MSR_IA32_FS_BASE) != 0;
  }
  static CpuId GetCpuId() {
    return GetCurrentContextInfo()->cpuid;
  }
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
  static const int kGdtEntryNum = 10;
  struct ContextInfo {
    ContextInfo *info; // self_reference
    CpuId cpuid;
  } __attribute__ ((aligned (8)));
  static ContextInfo *GetCurrentContextInfo() {
    ContextInfo *ptr;
    asm volatile("movq %%fs:%1, %0":"=r"(ptr):"m"(*(ContextInfo *)__builtin_offsetof(ContextInfo, info)));
    return ptr;
  }
};

#endif // ! ASM_FILE
#endif // __RAPH_KERNEL_IDT_H__
