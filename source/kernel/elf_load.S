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
  #include <gdt.h>

.globl execute_elf_binary  
execute_elf_binary:
  mov $USER_DS, %edx
  mov %dx, %ds
  mov %dx, %es
  mov %dx, %fs
  mov %dx, %gs
  movq $0, %rdx
  pushq $USER_DS
  pushq %rsi
  pushq $USER_CS
  pushq %rdi
  lretq
  addq $64, %rsp
  popq %rbp
  ret

