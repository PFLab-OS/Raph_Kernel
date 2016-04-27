OUTPUT_FORMAT(elf64-x86-64)

ENTRY (entry)

SECTIONS{
  kLinearAddrOffset = 0xffffffff80000000;

  . = 0x100000;
  virt_memory_start = . + kLinearAddrOffset;
  phys_memory_start = .;
  .boot : {
    *(.bootstrap*) 
    *boot.o (.text*)
    *boot.o (.rodata*)
    *boot.o (.data*)
    *boot.o (.bss*)
  }

  . = 0x8000;
  .boot16 : AT(LOADADDR(.boot) + SIZEOF(.boot)) {
    *bootap.o (.text*)
    *bootap.o (.rodata*)
    *bootap.o (.data*)
    *bootap.o (.bss*)
    . = ALIGN(0x1000);
  }
  boot16_start = LOADADDR(.boot16);
  boot16_end = LOADADDR(.boot16) + SIZEOF(.boot16);

  . = kLinearAddrOffset + LOADADDR(.boot16) + SIZEOF(.boot16);
  .text : AT(LOADADDR(.boot16) + SIZEOF(.boot16)) {
    *(EXCLUDE_FILE (*boot.o *bootap.o) .text*)
    *(.gnu.linkonce.t*)
  }

  .rodata : {
    *(EXCLUDE_FILE (*boot.o *bootap.o) .rodata*)
    *(.gnu.linkonce.r*)
  }

  .data : {
    *(EXCLUDE_FILE (*boot.o *bootap.o) .data*)
    *(.gnu.linkonce.d*)
      }

  data_end_addr = .;

  .bss : {
    *(COMMON*)
    *(.COMMON*)
    *(EXCLUDE_FILE (*boot.o *bootap.o) .bss*)
    lock_data_start = .;
    *(.lock.bss*)
    lock_data_end = .;
    *(.gnu.linkonce.b*)

    virt_memory_end = .;
    . = ALIGN(0x1000);
    virt_allocatedmemory_end = .;

    initial_PML4T = . - kLinearAddrOffset;
    /* kLinearAddrOffset is subtracted because 32bit code refers to initial_PML4T */
    . += 0xC000;

  /*
   * Page Structure Table
   * statically initialized at boot.S
   * 
   *      0 - 0x1000 : PML4T
   * 0x1000 - 0x2000 : PDPT1 (mapped onto PML4T[0] & PML4T[256]) 
   * 0x2000 - 0x3000 : PDPT2 (mapped onto PML4T[511]) 
   * 0x3000 - 0x4000 : PD1 (mapped onto PDPT1[0])                << 1GB linear mapping
   * 0x4000 - 0x5000 : PD2 (mapped onto PDPT1[1])
   * 0x5000 - 0x6000 : PD3 (mapped onto PDPT1[2])
   * 0x6000 - 0x7000 : PD4 (mapped onto PDPT1[3])
   * 0x7000 - 0x8000 : PD5 (mapped onto PDPT2[510]) 
   * 0x8000 - 0x9000 : PD6 (mapped onto PDPT2[511]) 
   * 0x9000 - 0xA000 : PT1 (mapped onto PD6[511]) 
   * 0xA000 - 0xB000 : P1 (mapped onto PT1[509]) 
   * 0xB000 - 0xC000 : P2 (mapped onto PT1[510]) 
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
