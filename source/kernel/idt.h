/*
 *
 * Copyright (c) 2015 Project Raphine
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

#ifndef __RAPH_KERNEL_IDT_H__
#define __RAPH_KERNEL_IDT_H__

#define KERNEL_CS (0x10)
#define KERNEL_DS (0x18)

#ifndef ASM_FILE
#include <stdint.h>

struct Regs {
  uint64_t rax, rbx, rcx, rdx, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t n, ecode, rip, cs, rflags;
  uint64_t rsp, ss;
}__attribute__((__packed__));

class Idt {
 public:
  void Setup();
 private:
  void SetGate(void (*gate)(Regs *rs), int n, uint8_t dpl, bool trap);
  static const uint32_t kIdtPresent = 1 << 15;
};

#endif // ! ASM_FILE
#endif // __RAPH_KERNEL_IDT_H__
