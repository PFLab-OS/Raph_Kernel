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

static inline void lgdt(uint32_t *gdt_desc, int entry_num) {
  volatile uint32_t gdtr[3];
  gdtr[0] = (entry_num * 8 - 1) << 16;
  gdtr[1] = reinterpret_cast<virt_addr>(gdt_desc);
  gdtr[2] = reinterpret_cast<virt_addr>(gdt_desc) >> 32;

  asm volatile("lgdt (%0)"::"r"(reinterpret_cast<virt_addr>(gdtr) + 2));
};

#define get_cpuid(eax, ecx, reg, rval) asm volatile("cpuid;" : "="#reg(rval) : "a"(eax), "c"(ecx))

#endif // __RAPH_KERNEL_X86_H__
