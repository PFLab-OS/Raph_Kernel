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

#ifdef __KERNEL__
#include <idt.h>
#include <apic.h>
#endif // __KERNEL__

void IntSpinLock::Lock() {
  if ((_flag % 2) == 1) {
    kassert(_cpuid.GetRawId() != cpu_ctrl->GetCpuId().GetRawId());
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
    flag = GetFlag();
  }
  _cpuid = cpu_ctrl->GetCpuId();
}

void IntSpinLock::Unlock() {
  kassert((_flag % 2) == 1);
  _cpuid = CpuId();
  _flag++;
  enable_interrupt(_did_stop_interrupt);
}

int IntSpinLock::Trylock() {
  volatile unsigned int flag = GetFlag();
  bool iflag = disable_interrupt();
  if (((flag % 2) == 0) && SetFlag(flag, flag + 1)) {
    _did_stop_interrupt = iflag;
    return 0;
  } else {
    enable_interrupt(iflag);
    return -1;
  }
}

