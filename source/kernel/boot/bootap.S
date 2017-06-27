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

#define ASM_FILE
#include <gdt.h>
#include <mem/paging.h>

# entry point of APs must be located on the align of 4KB
# see intel MPspec Appendix B.4.2
.text
.code16
.balign 4096

.globl entryothers
.global stack_of_others

entryothers:
  cli

setup_paging_of_others:
  mov $initial_PML4T, %edi
  mov %edi, %cr3

  // set PAE&PGE bit
  mov %cr4, %eax
  or $(1 << 5 | 1 << 7), %eax
  mov %eax, %cr4

switchto_longmode_of_others:
  // set LM bit
  mov $0xC0000080, %ecx
  rdmsr
  or $(1 << 8), %eax
  wrmsr

  // set PG&PE bit
  mov %cr0, %eax
  or $(1 << 31 | 1 << 0), %eax
  andl $(~(1 << 30 | 1 << 29)), %eax
  mov %eax, %cr0
  lgdt gdtr_of_others
  ljmp $KERNEL_CS, $trampoline_of_others


.code64
trampoline_of_others:
  movl $KERNEL_DS, %eax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  mov (stack_of_others), %rsp
  sub $8, %rsp
  pushq $0   // return addr
  pushq $KERNEL_CS
  mov $main_of_others, %rax
  pushq %rax
  // load higher address gdt
  lgdt gdtr
  lretq

.data
  .word 0
gdtr_of_others:
  .word 8*4 - 1
  .quad gdt_desc

stack_of_others:
  .quad 0
