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

#include <spinlock.h>
#include <raph.h>
#include <cpu.h>
#include <libglobal.h>
#include <idt.h>
#include <apic.h>
#include <timer.h>
#include <tty.h>
#include <global.h>

bool SpinLock::_spinlock_timeout = true;

void SpinLock::Lock() {
  if ((_flag % 2) == 1 && _cpuid == cpu_ctrl->GetCpuId()) {
    assert(_cpuid.IsValid());
    gtty->DisablePrint();
    gtty->ErrPrintf("SpinLock is holded by cpuid %d.\n(current interrupt handling cnt: %d)\n", _cpuid.GetRawId(), idt->GetHandlingCnt());
    for (size_t i = 0; i < sizeof(_rip) / sizeof(_rip[0]); i++) {
      gtty->ErrPrintf("backtrace(%d): rip:%llx,\n", i, _rip[i]);
    }
    kernel_panic("SpinLock", "self lock! need to solve deadlock.");
  }
  bool showed_timeout_warning = false;
  uint64_t t1;
  if (_spinlock_timeout && timer != nullptr && timer->DidSetup()) {
    t1 = timer->GetCntAfterPeriod(timer->ReadMainCnt(), 10 * 1000000); // 10s
  }
  volatile unsigned int flag = GetFlag();
  while(true) {
    if ((flag % 2) != 1) {
      bool iflag = disable_interrupt();
      if (SetFlag(flag, flag + 1)) {
        _did_stop_interrupt = iflag;
        break;
      }
      enable_interrupt(iflag);
    }
    if (_spinlock_timeout && !showed_timeout_warning && timer != nullptr && timer->DidSetup() && timer->IsTimePassed(t1)) {
      gtty->DisablePrint();
      gtty->ErrPrintf("[error]: unable to take SpinLock for a long time on cpuid %d.\nA deadlock may occur.\n", cpu_ctrl->GetCpuId().GetRawId());
      size_t *rbp;
      asm volatile("movq %%rbp, %0":"=r"(rbp));
      show_backtrace(rbp);
      assert(_cpuid.IsValid());
      gtty->ErrPrintf("SpinLock is holded by cpuid %d.\n", _cpuid.GetRawId());
      for (size_t i = 0; i < sizeof(_rip) / sizeof(_rip[0]); i++) {
        gtty->ErrPrintf("backtrace(%d): rip:%llx,\n", i, _rip[i]);
      }
      showed_timeout_warning = true;
    }
    flag = GetFlag();
  }
  _cpuid = cpu_ctrl->GetCpuId();
  size_t *rbp;
  asm volatile("movq %%rbp, %0":"=r"(rbp));
  size_t top_rbp = reinterpret_cast<size_t>(rbp);
  for (size_t i = 0; i < sizeof(_rip) / sizeof(_rip[0]); i++) {
    if (top_rbp <= rbp[1] || top_rbp - 4096 > rbp[1]) {
      break;
    }
    _rip[i] = rbp[1];
    rbp = reinterpret_cast<size_t *>(rbp[0]);
  }
}

void SpinLock::Unlock() {
  kassert((_flag % 2) == 1);
  _cpuid = CpuId();
  _flag++;
  enable_interrupt(_did_stop_interrupt);
}

ReturnState SpinLock::Trylock() {
  volatile unsigned int flag = GetFlag();
  bool iflag = disable_interrupt();
  if (((flag % 2) == 0) && SetFlag(flag, flag + 1)) {
    _did_stop_interrupt = iflag;
    _cpuid = cpu_ctrl->GetCpuId();
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    for (size_t i = 0; i < sizeof(_rip) / sizeof(_rip[0]); i++) {
      _rip[i] = rbp[1];
      rbp = reinterpret_cast<size_t *>(rbp[0]);
    }
    return ReturnState::kOk;
  } else {
    enable_interrupt(iflag);
    return ReturnState::kError;
  }
}

