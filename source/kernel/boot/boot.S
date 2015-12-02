/*
 *
 * Copyright (c) 2015 OASIS Project
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
#include "multiboot2.h"
#include "../mem/virtmem.h"
#include "../mem/paging.h"

#define MULTIBOOTHEADER_SIZE (multibootheader_end - multibootheader_start)
.global entry
.global multiboot_info
.code32

.section .bootstrap
.balign 8

.extern initial_stack_buffer
.extern initial_PML4T
  
.extern main
  
multibootheader_start:
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long MULTIBOOTHEADER_SIZE
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + MULTIBOOTHEADER_SIZE)

align_tag_start:  
  .short MULTIBOOT_HEADER_TAG_MODULE_ALIGN
	.short 0
	.long align_tag_end - align_tag_start
align_tag_end:  

end_tag_start:  
	.short MULTIBOOT_HEADER_TAG_END
	.short 0
	.long end_tag_end - end_tag_start
end_tag_end:  
  
multibootheader_end:

.balign 4
.text
entry:
  cmp %eax, MULTIBOOT2_BOOTLOADER_MAGIC
  je unknown_state
  mov %ebx, (multiboot_info)
  mov $initial_stack_buffer, %esp
  add 0x1000, %esp

  // >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
  // Originally from http://wiki.osdev.org/Setting_Up_Long_Mode
  //
detect_cpuid:
  pushfl
  pop %eax
  mov %eax, %ecx
  xor $(1 << 21), %eax      // Flip the ID-bit, which is bit 21.
  push %eax
  popfl
  pushfl
  pop %eax
  push %ecx
  popfl
  xor %ecx, %eax
  jz unknown_state

check_longmode_exist: 
  mov $0x80000000, %eax
  cpuid
  cmp $0x80000001, %eax
  jb unknown_state
  
  mov $0x80000001, %eax
  cpuid
  test $(1 << 29), %edx     // Test if the LM-bit is set.
  jz unknown_state

  // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
  
setup_paging:
  // clear page structure tables
  mov $initial_PML4T, %edi
  mov %edi, %cr3
  xor %eax, %eax
  mov $(9*4096/4), %ecx
  rep stosl

  mov $initial_PML4T, %ebx

  // setup pml4 table
  lea (0x0)(%ebx), %edi
  lea 0x1000(%ebx), %eax
  or  $(PML4E_PRESENT_BIT | PML4E_WRITE_BIT), %eax
  mov %eax, (%edi)
  lea (0x0 + 256 * 8)(%ebx), %edi
  lea 0x2000(%ebx), %eax
  or  $(PML4E_PRESENT_BIT | PML4E_WRITE_BIT), %eax
  mov %eax, (%edi)
  lea (0x0 + 511 * 8)(%ebx), %edi
  lea 0x3000(%ebx), %eax
  or  $(PML4E_PRESENT_BIT | PML4E_WRITE_BIT), %eax
  mov %eax, (%edi)

  // setup pdpt1
  lea (0x1000)(%ebx), %edi
  lea 0x4000(%ebx), %eax
  or  $(PDPTE_PRESENT_BIT | PDPTE_WRITE_BIT), %eax
  mov %eax, (%edi)

  // setup pdpt2
  lea (0x2000)(%ebx), %edi
  lea 0x10000(%ebx), %eax
  or  $(PDPTE_PRESENT_BIT | PDPTE_WRITE_BIT), %eax
  mov %eax, (%edi)

  lea (0x10000)(%ebx), %edi
  mov $0, %ecx
  mov $0, %eax
setup_linear_addr_loop:
  or  $(PDE_PRESENT_BIT | PDE_WRITE_BIT | PDE_2MPAGE_BIT | PDE_GLOBAL_BIT), %eax
  mov %eax, (%edi)
  inc %ecx
  cmp $512, %ecx
  je  setup_linear_addr_end
  add $8, %edi
  add $0x200000, %eax
  jmp setup_linear_addr_loop

setup_linear_addr_end: 
  
  // setup pdpt3
  lea (0x3000 + 510 * 8)(%ebx), %edi
  lea 0x5000(%ebx), %eax
  or  $(PDPTE_PRESENT_BIT | PDPTE_WRITE_BIT), %eax
  mov %eax, (%edi)
  lea (0x3000 + 511 * 8)(%ebx), %edi
  lea 0x6000(%ebx), %eax
  or  $(PDPTE_PRESENT_BIT | PDPTE_WRITE_BIT), %eax
  mov %eax, (%edi)

  // setup pd1
  lea 0x4000(%ebx), %edi
  lea 0x7000(%ebx), %eax
  or  $(PDE_PRESENT_BIT | PDE_WRITE_BIT), %eax
  mov %eax, (%edi)

  // setup pd2
  lea 0x5000(%ebx), %edi
  mov $0, %eax
  or  $(PDE_PRESENT_BIT | PDE_WRITE_BIT | PDE_2MPAGE_BIT | PDE_GLOBAL_BIT), %eax
  mov %eax, (%edi)

  // setup pd3
  lea (0x6000 + 511 * 8)(%ebx), %edi
  lea 0x8000(%ebx), %eax
  or  $(PDE_PRESENT_BIT | PDE_WRITE_BIT), %eax
  mov %eax, (%edi)

  // setup pt1
  lea (0x7000 + 256 * 8)(%ebx), %edi
  mov $0x100000, %eax
  or  $(PTE_PRESENT_BIT | PTE_WRITE_BIT | PTE_GLOBAL_BIT), %eax
  mov %eax, (%edi)

  // setup pt3
  lea (0x8000 + 511 * 8)(%ebx), %edi

  mov $initial_stack_buffer, %eax
  or  $(PTE_PRESENT_BIT | PTE_WRITE_BIT | PTE_GLOBAL_BIT), %eax
  mov %eax, (%edi)

  // set PAE&PGE bit
  mov %cr4, %eax
  or $(1 << 5 | 1 << 7), %eax
  mov %eax, %cr4
  
switchto_longmode:
  // set LM bit
  mov $0xC0000080, %ecx
  rdmsr
  or $(1 << 8), %eax
  wrmsr

  // set PG bit 
  mov %cr0, %eax
  or $(1 << 31), %eax
  mov %eax, %cr0
  lgdt gdtr
  ljmp $0x10, $trampoline

unknown_state:
  hlt
  jmp unknown_state

.code64
trampoline:
  // lower address(1MB~)
  // this code call c function which mapped at
  // higher address(last 2GB of virtual memory)
  movl $0x18, %eax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %fs
  mov %ax, %gs
  mov %ax, %ss
  mov $kKernelEndAddr - 3, %rsp
  pushq $0   // return addr
  pushq $0x10
  mov $main, %rax
  pushq %rax
  mov $kLinearAddrOffset, %rax
  add %rax, (gdtr+2)
  // load higher address gdt
  lgdt gdtr
  lretq

.data
multiboot_info:
  .long 0

.balign 8
gdt:
  // null descriptor
  .quad 0
  .quad 0
  .quad 0x00209a0000000000  // code descriptor
  .quad 0x0000920000000000  // data descriptor
  
  .word 0
gdtr:
  .word 8*4 - 1
  .quad gdt

.bss
tmp_stack: 
  .skip 32

