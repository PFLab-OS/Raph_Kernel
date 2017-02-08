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
 * Author: hikalium
 * 
 */

#include <tty.h>
#include <mem/physmem.h>
#include <string.h>
#include <stdlib.h>
#include <x86.h>
#include <syscall.h>
#include <gdt.h>

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

extern "C" int64_t syscall_handler();

extern "C" int64_t  syscall_handler_sub(int64_t rdi, int64_t rsi, int64_t rdx, int64_t rcx, int64_t r8, int64_t r9)
{
  return SystemCallCtrl::handler(rdi, rsi, rdx, rcx, r8, r9);
}


void SystemCallCtrl::init()
{
  // IA32_EFER.SCE = 1
  const uint64_t bit_efer_SCE = 0x01;
  uint64_t efer = rdmsr(kIA32EFER);
  efer |= bit_efer_SCE;
  wrmsr(kIA32EFER, efer);
  // set vLSTAR
  wrmsr(kIA32STAR, (static_cast<uint64_t>(KERNEL_CS) << 32) | ((static_cast<uint64_t>(USER_DS) - 8) << 48));
  wrmsr(kIA32LSTAR, reinterpret_cast<uint64_t>(syscall_handler));
}

int64_t SystemCallCtrl::handler(int64_t rdi, int64_t rsi, int64_t rdx, int64_t rcx, int64_t r8, int64_t r9)
{
  gtty->CprintfRaw("syscall(%llx) \n", rcx);
  if(rcx == 158){
    gtty->CprintfRaw("sys_arch_prctl code=%llx addr=%llx\n", rdi, rsi);
    switch(rdi){
      case ARCH_SET_GS:
        wrmsr(kIA32GSBase, rsi);
        break;
      case ARCH_SET_FS:
        wrmsr(kIA32FSBase, rsi);
        break;
      case ARCH_GET_FS:
        *(uint64_t *)rsi = rdmsr(kIA32FSBase);
        break;
      case ARCH_GET_GS:
        *(uint64_t *)rsi = rdmsr(kIA32GSBase);
        break;
      default:
        gtty->CprintfRaw("Invalid args\n", rcx);
        kassert(false);
    }
  }
  gtty->CprintfRaw("syscall(%llx) end \n", rcx);
  return 0;
}

