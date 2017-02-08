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

#ifndef __RAPH_KERNEL_X86_H__
#define __RAPH_KERNEL_X86_H__

#include <stdint.h>
#include <mem/virtmem.h>

namespace x86 {
  void lgdt(uint32_t *gdt_desc, int entry_num);
  
#define get_cpuid(eax, ecx, reg, rval) asm volatile("cpuid;" : "="#reg(rval) : "a"(eax), "c"(ecx))
  
  static inline uint64_t rdmsr(uint32_t addr) {
    uint32_t msr_high, msr_low;
    asm volatile("rdmsr": "=d"(msr_high), "=a"(msr_low) : "c"(addr));
    return (static_cast<uint64_t>(msr_high) << 32) | msr_low; 
  }
  
  static inline void wrmsr(uint32_t addr, uint64_t data) {
    uint32_t msr_high = (data >> 32) & 0xffffffff;
    uint32_t msr_low = data & 0xffffffff;
    asm volatile("wrmsr":: "d"(msr_high), "a"(msr_low), "c"(addr));
  }
  
  static inline uint16_t get_display_family_model() {
    uint32_t eax;
    asm volatile("cpuid;":"=a"(eax):"a"(1));
    uint8_t family_id = (eax >> 8) & 0b1111;
    uint8_t ex_family_id = (eax >> 20) & 0b11111111;
    uint8_t model_id = (eax >> 4) & 0b1111;
    uint8_t ex_model_id = (eax >> 16) & 0b1111;
    uint8_t display_family = (family_id != 0x0f) ? family_id : (ex_family_id + family_id);
    uint8_t display_model = (family_id == 0x06 || family_id == 0x0f) ? ((ex_model_id << 4) + model_id) : model_id;
    return (static_cast<uint16_t>(display_family << 8)) + display_model;
  }

#define READ_MEM_VOLATILE(ptr) (__sync_fetch_and_or(ptr, 0))
}

#endif // __RAPH_KERNEL_X86_H__
