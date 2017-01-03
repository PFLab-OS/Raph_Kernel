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
 * Author: hikalium, Liva
 * 
 */

#include <tty.h>
#include <mem/physmem.h>
#include <string.h>
#include <stdlib.h>
#include <x86.h>

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004


void syscall_handler()
{
  asm volatile("push %rcx; ");
  int64_t id, args[6];
  asm volatile(
    "mov %%rax, %0;"
    "mov %%rdi, %1;"
    :"=r"(id), "=r"(args[0])
  );
  gtty->CprintfRaw("syscall_handler! id=%d \n", id);
  if(id == 158){
    
  }
  asm volatile("pop %rcx; pop %rbp; sysret;");
}

