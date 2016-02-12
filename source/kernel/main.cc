/*
 *
 * Copyright (c) 2015 Project Raphine
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

#include "global.h"
#include "spinlock.h"
#include "acpi.h"
#include "apic.h"
#include "multiboot.h"
#include "mem/physmem.h"
#include "mem/paging.h"
#include "idt.h"
#include "tty.h"

#include "dev/vga.h"
#include "dev/pci.h"
#include "dev/e1000.h"

SpinLockCtrl *spinlock_ctrl;
MultibootCtrl *multiboot_ctrl;
AcpiCtrl *acpi_ctrl;
ApicCtrl *apic_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;
VirtmemCtrl *virtmem_ctrl;
Idt *idt;

PCICtrl *pci_ctrl;
E1000 *e1000;
Tty *gtty;

extern "C" int main() {
  SpinLockCtrl _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  
  MultibootCtrl _multiboot_ctrl;
  multiboot_ctrl = &_multiboot_ctrl;

  AcpiCtrl _acpi_ctrl;
  acpi_ctrl = &_acpi_ctrl;
  
  PCICtrl _pci_ctrl;
  pci_ctrl = &_pci_ctrl;

  E1000 _e1000;
  e1000 = &_e1000;

  Idt _idt;
  idt = &_idt;

  VirtmemCtrl _virtmem_ctrl;
  virtmem_ctrl = &_virtmem_ctrl;
  
  PhysmemCtrl _physmem_ctrl;
  physmem_ctrl = &_physmem_ctrl;
  
  PagingCtrl _paging_ctrl;
  paging_ctrl = &_paging_ctrl;
  
  Vga _vga;
  gtty = &_vga;
  
  multiboot_ctrl->Setup();

  apic_ctrl->Setup();

  idt->Setup();

  pci_ctrl->Init();

  e1000->Setup();
  e1000->PrintEthAddr();

  apic_ctrl->StartAPs();
  gtty->Printf("s", "kernel initialization completed");
  while(1) {
    asm volatile("hlt");
  }
}

void kernel_panic(char *class_name, char *err_str) {
  gtty->Printf("s", "Kernel Panic!");
  while(1) {
    asm volatile("hlt");
  }
}

extern "C" void __cxa_pure_virtual()
{
  kernel_panic("", "");
}
