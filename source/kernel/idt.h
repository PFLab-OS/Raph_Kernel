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
  void SetupGeneric();
  void SetupProc();
  void SetIntCallback(int n, idt_callback callback);
  // if 0, cpu is not handling interrupt
  volatile int GetHandlingCnt() {
    return _handling_cnt[apic_ctrl->GetApicId()];
  }
  struct ReservedIntVector {
    static const int kIpi      = 32;
    static const int kSpurious = 33;
    static const int kLapicErr = 34;
    static const int kKeyboard = 64;
  };
 private:
  void SetGate(void (*gate)(Regs *rs), int n, uint8_t dpl, bool trap, uint8_t ist);
  static const uint32_t kIdtPresent = 1 << 15;
  volatile uint16_t _idtr[5];
  idt_callback **_callback;
  int *_handling_cnt;
  friend void C::handle_int(Regs *rs);
};

#endif // __RAPH_KERNEL_IDT_H__
