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

.code64
.balign 4
.text

  #define ASM_FILE

.globl execute_elf_binary  
.globl resume_elf_binary  
.globl execute_kernel_elf_binary  
#TODO:Context構造体を受け取るようにする
execute_elf_binary:
  mov %cx, %ds
  mov %cx, %es
  pushq %rcx
  pushq %rsi
  pushq %rdx
  pushq %rdi
  movq $0, %rdx
  lretq
  addq $64, %rsp
  popq %rbp
  ret

execute_kernel_elf_binary:
  mov %rsi, %rsp
  jmp *%rdi
  ret

resume_elf_binary:
  mov %ds, %ax
  pushq %rax
  mov %es, %ax
  pushq %rax
  pushfq
  pushq %rbx
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
  pushq %rbp
  jmp _returner
_saver:
  movq %rsp, (%rsi)
  
#コンテキスト復旧
  subq $0xc0, %rsp
  movq %rdi, %rsi
  movq %rsp, %rdi
  movq $0xc0, %rcx
  rep movsb

  popq %rax
  #mov %ax,%fs
  popq %rax
  #mov %ax,%gs
  popq %rax
  mov %ax,%es
  popq %rax
  mov %ax,%ds

  popq %rdi
  popq %rsi
  popq %rbp
  popq %rbx
  popq %rdx
  popq %rcx
  popq %rax
  popq %r8
  popq %r9
  popq %r10
  popq %r11
  popq %r12
  popq %r13
  popq %r14
  popq %r15
  popfq
  lretq

_returner:
  callq _saver
  popq %rbp
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %rbx
  popfq
  popq %rax
  mov %ax,%es
  popq %rax
  mov %ax,%ds
  ret



