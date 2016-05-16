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

#include <spinlock.h>
#include <raph.h>
#include <idt.h>
#include <apic.h>

void SpinLock::Lock() {
#ifndef __UNIT_TEST__
  kassert(idt->GetHandlingCnt() == 0);
  if ((_flag % 2) == 1) {
    kassert(_id != apic_ctrl->GetCpuId());
  }
#endif // __UNIT_TEST__
  volatile unsigned int flag = GetFlag();
  while((flag % 2) == 1 || !SetFlag(flag, flag + 1)) {
    flag = GetFlag();
  }
#ifndef __UNIT_TEST__
  _id = apic_ctrl->GetCpuId();
#endif // __UNIT_TEST__
}

void DebugSpinLock::Lock() {
  kassert(_key == kKey);
#ifndef __UNIT_TEST__
  if ((_flag % 2) == 1) {
    kassert(_id != apic_ctrl->GetCpuId());
  }
#endif // __UNIT_TEST__
  SpinLock::Lock();
}

void SpinLock::Unlock() {
  kassert((_flag % 2) == 1);
  _id = -1;
  _flag++;
}

int SpinLock::Trylock() {
  volatile unsigned int flag = GetFlag();
  if (((flag % 2) == 0) && SetFlag(flag, flag + 1)) {
    return 0;
  } else {
    return -1;
  }
}

void IntSpinLock::Lock() {
  if ((_flag % 2) == 1) {
    kassert(_id != apic_ctrl->GetCpuId());
  }
  volatile unsigned int flag = GetFlag();
  while(true) {
    if ((flag % 2) != 1) {
      uint64_t if_flag;
      asm volatile("pushfq; popq %0; andq $0x200, %0;":"=r"(if_flag));
      if (if_flag != 0) {
        asm volatile("cli;");
      }
      if (SetFlag(flag, flag + 1)) {
        this->DisableInt();
        if (if_flag != 0) {
          asm volatile("sti;");
        }
        break;
      } else {
        if (if_flag != 0) {
          asm volatile("sti;");
        }
      }
    }
    flag = GetFlag();
  }
  _id = apic_ctrl->GetCpuId();
}

void IntSpinLock::Unlock() {
  kassert((_flag % 2) == 1);
  _id = -1;
  _flag++;
  this->EnableInt();
}

int IntSpinLock::Trylock() {
  volatile unsigned int flag = GetFlag();
  if (((flag % 2) == 0) && SetFlag(flag, flag + 1)) {
    return 0;
  } else {
    return -1;
  }
}

void IntSpinLock::DisableInt() {
  _did_stop_interrupt = apic_ctrl->DisableInt();
}

void IntSpinLock::EnableInt() {
  if (_did_stop_interrupt) {
    apic_ctrl->EnableInt();
  }
}
