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

/* ported from newlib */
/*
 * ====================================================
 * Copyright (C) 2007 by Ellips BV. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */
  
  .global setjmp
  .global longjmp
  
setjmp: 
  movq    %rbx,  0 (%rdi)
  movq    %rbp,  8 (%rdi)
  movq    %r12, 16 (%rdi)
  movq    %r13, 24 (%rdi)
  movq    %r14, 32 (%rdi)
  movq    %r15, 40 (%rdi)
  leaq    8 (%rsp), %rax
  movq    %rax, 48 (%rdi)
  movq    (%rsp), %rax
  movq    %rax, 56 (%rdi)
  movq    $0, %rax
  ret

longjmp:  
  movq    %rsi, %rax        /* Return value */

  movq     8 (%rdi), %rbp

  #ifdef __KERNEL__
  // clear interrupt
  pushfq
  popq    %rcx
  cli

  #endif /* __KERNEL__ */
  
  movq    48 (%rdi), %rsp
  pushq   56 (%rdi)
  movq     0 (%rdi), %rbx
  movq    16 (%rdi), %r12
  movq    24 (%rdi), %r13
  movq    32 (%rdi), %r14
  movq    40 (%rdi), %r15

  #ifdef __KERNEL__
  // enable interrupt
  andq    $0x200, %rcx
  cmpq    $0, %rcx
  jz      1f
  sti
1:
  #endif /* __KERNEL__ */

  ret

 /* end of ported code */
