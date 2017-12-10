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
.globl syscall_handler_stack
.globl syscall_handler_caller_stack;
syscall_handler:
  swapgs
  movq %rsp, (syscall_handler_caller_stack)
  movq (syscall_handler_stack), %rsp
  subq $112, %rsp
  movq %rcx, 0(%rsp)  // rip
  movq %r11, 8(%rsp)  // rflags
  movq %rdi, 16(%rsp) // arg1
  movq %rsi, 24(%rsp) // arg2
  movq %rdx, 32(%rsp) // arg3
  movq %r10, 40(%rsp) // arg4
  movq %r8,  48(%rsp) // arg5
  movq %r9,  56(%rsp) // arg6
  movq %rbx, 64(%rsp)
  movq %rbp, 72(%rsp)
  movq %r12, 80(%rsp)
  movq %r13, 88(%rsp)
  movq %r14, 96(%rsp)
  movq %r15, 104(%rsp)
  movq %rsp, %rdi
  movq %rax, %rsi
  call syscall_handler_sub
  movq 0(%rsp), %rcx
  movq 8(%rsp), %r11
  movq 16(%rsp), %rdi
  movq 24(%rsp), %rsi
  movq 32(%rsp), %rdx
  movq 40(%rsp), %r10
  movq 48(%rsp), %r8
  movq 56(%rsp), %r9
  movq 64(%rsp), %rbx
  movq 72(%rsp), %rbp
  movq 80(%rsp), %r12
  movq 88(%rsp), %r13
  movq 96(%rsp), %r14
  movq 104(%rsp), %r15
  addq $112, %rsp
  movq (syscall_handler_caller_stack), %rsp
  swapgs
  sysretq

syscall_handler_stack: 
  .quad 0

syscall_handler_caller_stack: 
  .quad 0
