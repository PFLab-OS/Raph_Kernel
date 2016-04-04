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

#include <stdint.h>
#include <apic.h>

struct Regs {
  uint64_t rax, rbx, rcx, rdx, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t n, ecode, rip, cs, rflags;
  uint64_t rsp, ss;
}__attribute__((__packed__));

typedef void (*idt_callback)(Regs *rs);

namespace C {
  extern "C" void handle_int(Regs *rs);
}

class Idt {
 public:
  enum class HandlingStatus {
    kHandling,
    kNotHandling
  };
  void SetupGeneric();
  void SetupProc();
  void SetIntCallback(int n, idt_callback callback);
  // is cpu handling interrupt?
  volatile HandlingStatus IsHandling() {
    return _is_handling[apic_ctrl->GetApicId()];
  }
 private:
  void SetGate(void (*gate)(Regs *rs), int n, uint8_t dpl, bool trap, uint8_t ist);
  static const uint32_t kIdtPresent = 1 << 15;
  volatile uint16_t _idtr[5];
  idt_callback **_callback;
  HandlingStatus *_is_handling;
  friend void C::handle_int(Regs *rs);
};

#endif // __RAPH_KERNEL_IDT_H__
