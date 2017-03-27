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

.code64
.balign 4
.text

.extern syscall_handler_sub
  
.globl syscall_handler 
syscall_handler:
  push %rcx
  push %r11
  subq $48, %rsp
  movq %rsp, %rbp
  movq %rdi, 0(%rbp)
  movq %rsi, 8(%rbp)
  movq %rdx, 16(%rbp)
  movq %r10, 24(%rbp)
  movq %r8, 32(%rbp) 
  movq %r9, 40(%rbp)
  movq %rbp, %rdi
  movq %rax, %rsi
  movq %rcx, %rdx
  call syscall_handler_sub
  addq $48, %rsp
  pop %r11
  pop %rcx
  sysretq

