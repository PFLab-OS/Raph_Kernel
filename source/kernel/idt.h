/*
 *
 * Copyright (c) 2015 Raphine Project
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
#include <cpu.h>
#include <spinlock.h>

struct Regs {
  uint64_t rax, rbx, rcx, rdx, rbp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
  uint64_t n, ecode, rip, cs, rflags;
  uint64_t rsp, ss;
}__attribute__((__packed__));

typedef void (*idt_callback)(Regs *rs);
typedef void (*int_callback)(Regs *rs, void *arg);
typedef void (*ioint_callback)(void *arg);

namespace C {
  extern "C" void handle_int(Regs *rs);
}

class Idt {
 public:
  enum class EoiType {
    kNone,
    kLapic,
    kIoapic,
  };
  void SetupGeneric();
  void SetupProc();
  // I/O等用の割り込みハンドラ
  // 確保したvectorが返る(vector >= 64)
  // 確保できなかった場合はReservedIntVector::kErrorが返る
  int SetIntCallback(CpuId cpuid, int_callback callback, void *arg, EoiType eoi);
  // 一括で連続したvectorを確保する
  // rangeは2のn乗である必要がある
  // 戻り値は先頭vectorで、rangeで割り切れる事を保証する
  int SetIntCallback(CpuId cpuid, int_callback *callback, void **arg, int range, EoiType eoi);
  // 例外等用の割り込みハンドラ
  // vector < 64でなければならない
  void SetExceptionCallback(CpuId cpuid, int vector, int_callback callback, void *arg, EoiType eoi);
  // if 0, cpu is not handling interrupt
  volatile int GetHandlingCnt() {
    if (!_is_gen_initialized) {
      return false;
    }
    return _handling_cnt[cpu_ctrl->GetCpuId().GetRawId()];
  }
  struct ReservedIntVector {
    static const int kIpi      = 33;
    static const int k8259Spurious1 = 47;
    static const int k8259Rtc = 48;
    static const int k8259Spurious2 = 55;
    static const int kError    = 63;
  };
 private:
  void SetGate(idt_callback gate, int vector, uint8_t dpl, bool trap, uint8_t ist);
  static void HandlePageFault(Regs *rs, void *arg);
  static void HandleGeneralProtectionFault(Regs *rs, void *arg);
  static const uint32_t kIdtPresent = 1 << 15;
  volatile uint16_t _idtr[5];
  struct IntCallback {
    int_callback callback;
    void *arg;
    EoiType eoi;
  } **_callback;
  int *_handling_cnt;
  friend void C::handle_int(Regs *rs);
  SpinLock _lock;
  bool _is_gen_initialized = false;
};

#endif // __RAPH_KERNEL_IDT_H__
