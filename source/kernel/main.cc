#include "global.h"
#include "spinlock.h"
#include "acpi.h"
#include "apic.h"
#include "multiboot.h"
#include "mem/physmem.h"
#include "mem/paging.h"

SpinLockCtrl *spinlock_ctrl;
MultibootCtrl *multiboot_ctrl;
AcpiCtrl *acpi_ctrl;
ApicCtrl *apic_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;
VirtmemCtrl *virtmem_ctrl;

extern "C" int main() {
  SpinLockCtrl _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;

  
  MultibootCtrl _multiboot_ctrl;
  multiboot_ctrl = &_multiboot_ctrl;

  AcpiCtrl _acpi_ctrl;
  acpi_ctrl = &_acpi_ctrl;
  
  ApicCtrl _apic_ctrl;
  apic_ctrl = &_apic_ctrl;

  multiboot_ctrl->Setup();

  VirtmemCtrl _virtmem_ctrl;
  virtmem_ctrl = &_virtmem_ctrl;

  PhysmemCtrl _physmem_ctrl;
  physmem_ctrl = &_physmem_ctrl;

  PagingCtrl _paging_ctrl;
  paging_ctrl = &_paging_ctrl;

  while(1) {
    asm volatile("hlt");
  }
}

void kernel_panic(char *class_name, char *err_str) {
  while(1) {
    asm volatile("hlt");
  }
}
