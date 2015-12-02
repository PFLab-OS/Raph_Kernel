OUTPUT_FORMAT(elf64-x86-64)

ENTRY (entry)

SECTIONS{
  kLinearAddrOffset = 0xffffffff80000000;
  . = 0x100000;
  virt_memory_start = . + kLinearAddrOffset;
  phys_memory_start = .;
  .bootstrap : {
    *(.bootstrap*) 
    *boot.o (.text*)
    *boot.o (.rodata*)
    *boot.o (.data*)
    *boot.o (.bss*)
  }

  . += kLinearAddrOffset;
  .text : AT(LOADADDR(.bootstrap) + SIZEOF(.bootstrap)) {
    *(EXCLUDE_FILE (*boot.o) .text*)
    *(.gnu.linkonce.t*)
  }

  .rodata : {
    *(EXCLUDE_FILE (*boot.o) .rodata*)
    *(.gnu.linkonce.r*)
  }

  .data : {
    *(EXCLUDE_FILE (*boot.o) .data*)
    *(.gnu.linkonce.d*)
  }

  data_end_addr = .;

  .bss : {
    *(.COMMON*)
    *(EXCLUDE_FILE (*boot.o) .bss*)
    lock_data_start = .;
    *(.lock.bss*)
    lock_data_end = .;
    *(.gnu.linkonce.b*)

    virt_memory_end = .;
    . = ALIGN(0x1000);
    virt_allocatedmemory_end = .;

    initial_PML4T = . - kLinearAddrOffset;
    /* kLinearAddrOffset is subtracted because 32bit code refers to initial_PML4T */
    . += 0x10000;

  /*
   * Page Structure Table
   * statically initialized at boot.S
   * 
   *      0 - 0x1000 : PML4T
   * 0x1000 - 0x2000 : PDPT1 (mapped onto PML4T[0]) 
   * 0x2000 - 0x3000 : PDPT2 (mapped onto PML4T[256]) 
   * 0x3000 - 0x4000 : PDPT3 (mapped onto PML4T[511]) 
   * 0x4000 - 0x5000 : PD1 (mapped onto PDPT1[0]) 
   * 0x5000 - 0x6000 : PD2 (mapped onto PDPT3[510]) 
   * 0x6000 - 0x7000 : PD3 (mapped onto PDPT3[511]) 
   * 0x7000 - 0x8000 : PT1 (mapped onto PD1[0]) 
   * 0x8000 - 0x9000 : PT3 (mapped onto PD3[511]) 
   * 
   */

   initial_stack_buffer = . - kLinearAddrOffset;
   . += 0x1000;
   phys_memory_end = . - kLinearAddrOffset;
  }
  bss_end_addr = .;

  kHeapEndAddr   = 0xffffffff90000000;
  kKernelEndAddr = 0xffffffffffffffff;
}
