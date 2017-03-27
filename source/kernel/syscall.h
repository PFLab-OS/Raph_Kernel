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

#ifndef __RAPH_LIB_SYSCALL_H__
#define __RAPH_LIB_SYSCALL_H__

#include <x86.h>

class SystemCallCtrl {
public:
  struct Args {
    int64_t arg1;
    int64_t arg2;
    int64_t arg3;
    int64_t arg4;
    int64_t arg5;
    int64_t arg6;
  };
  static void Init();
  static int64_t Handler(Args *args, int index, size_t raddr);
private:
  enum MSRAddr : uint32_t {
    kIA32EFER     = 0xC0000080,
    kIA32STAR     = 0xC0000081,
    kIA32LSTAR    = 0xC0000082,
    kIA32FSBase   = 0xC0000100,
    kIA32GSBase   = 0xC0000101,
  };
  static const int kArchSetGs = 0x1001;
  static const int kArchSetFs = 0x1002;
  static const int kArchGetFs = 0x1003;
  static const int kArchGetGs = 0x1004;
  static uint64_t rdmsr(MSRAddr addr) {
    return x86::rdmsr(addr);
  }
  static void wrmsr(MSRAddr addr, uint64_t data) {
    x86::wrmsr(addr, data);
  }
};

#endif // __RAPH_LIB_SYSCALL_H__
