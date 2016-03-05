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
#include "polling.h"
#include "mem/physmem.h"
#include "mem/paging.h"
#include "idt.h"
#include "timer.h"

#include "tty.h"
#include "dev/acpipmtmr.h"
#include "dev/hpet.h"

#include "dev/vga.h"
#include "dev/pci.h"

#include "net/netctrl.h"
#include "net/socket.h"

SpinLockCtrl *spinlock_ctrl;
MultibootCtrl *multiboot_ctrl;
AcpiCtrl *acpi_ctrl;
ApicCtrl *apic_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;
VirtmemCtrl *virtmem_ctrl;
PollingCtrl *polling_ctrl;
Idt *idt;
Timer *timer;

PCICtrl *pci_ctrl;

Tty *gtty;

static uint32_t rnd_next = 1;

extern "C" int main() {
  SpinLockCtrl _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  
  MultibootCtrl _multiboot_ctrl;
  multiboot_ctrl = &_multiboot_ctrl;

  AcpiCtrl _acpi_ctrl;
  acpi_ctrl = &_acpi_ctrl;

  ApicCtrl _apic_ctrl;
  apic_ctrl = &_apic_ctrl;
  
  Idt _idt;
  idt = &_idt;

  VirtmemCtrl _virtmem_ctrl;
  virtmem_ctrl = &_virtmem_ctrl;
  
  PhysmemCtrl _physmem_ctrl;
  physmem_ctrl = &_physmem_ctrl;
  
  PagingCtrl _paging_ctrl;
  paging_ctrl = &_paging_ctrl;

  PollingCtrl _polling_ctrl;
  polling_ctrl = &_polling_ctrl;
  
  AcpiPmTimer _atimer;
  Hpet _htimer;
  timer = &_atimer;

  Vga _vga;
  gtty = &_vga;

  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::kPageSize);
  extern int kKernelEndAddr;
  kassert(paging_ctrl->MapPhysAddrToVirtAddr(reinterpret_cast<virt_addr>(&kKernelEndAddr) - PagingCtrl::kPageSize * 3, paddr, PagingCtrl::kPageSize, PDE_WRITE_BIT, PTE_WRITE_BIT || PTE_GLOBAL_BIT));

  multiboot_ctrl->Setup();
  
  // acpi_ctl->Setup() は multiboot_ctrl->Setup()から呼ばれる

  timer->Setup();
  if (_htimer.Setup()) {
    timer = &_htimer;
    gtty->Printf("s","[timer] HPET supported.\n");
  }

  rnd_next = timer->ReadMainCnt();

  // timer->Sertup()より後
  apic_ctrl->Setup();
  
  idt->Setup();

  InitNetCtrl();

  InitDevices<PCICtrl, Device>();

  gtty->Printf("s", "cpu #", "d", apic_ctrl->GetApicId(), "s", " started.\n");
  apic_ctrl->StartAPs();

  gtty->Printf("s", "\n\nkernel initialization completed\n");

  extern int kKernelEndAddr;
  // stackは12K
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr)));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 3) + 1));
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - 4096 * 3));

//  ARPSocket socket;
//  if(socket.Open() < 0) {
//    gtty->Printf("s", "cannot open socket\n");
//  } else {
//    socket.TransmitPacket(ARPSocket::kOpARPRequest, 0x0a000203);
//    socket.ReceivePacket(ARPSocket::kOpARPReply);
//    gtty->Printf("s", "ARP reply received\n");
//  }
  Socket socket;
  if(socket.Open() < 0) {
    gtty->Printf("s", "cannot open socket\n");
  } else {
    uint8_t data[5] = "ABCD";
    socket.SetAddr(0x0a00020f);
    socket.SetPort(4000);
    socket.TransmitPacket(data, 5);
    gtty->Printf("s", "UDP sent\n");
  }

  polling_ctrl->HandleAll();
  while(true) {
    asm volatile("hlt;nop;hlt;");
  }

  DismissNetCtrl();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode
  apic_ctrl->BootAP();

  gtty->Printf("s", "cpu #", "d", apic_ctrl->GetApicId(), "s", " started.\n");
  /*  uint32_t addr = 0x0a00020f;
  uint32_t port = 4000;

  uint8_t buf[] = {
    0x41, 0x42, 0x43, 0x44, 0x00,
    };*/

    //  udp_ctrl->Transmit(buf, sizeof(buf), addr, port, port);
  while(1) {
    asm volatile("hlt;");
  }
  return 0;
}

void kernel_panic(char *class_name, char *err_str) {
  gtty->Printf("s", "Kernel Panic!");
  while(1) {
    asm volatile("hlt;");
  }
}

extern "C" void __cxa_pure_virtual()
{
  kernel_panic("", "");
}

#define RAND_MAX 0x7fff

uint32_t rand() {
    rnd_next = rnd_next * 1103515245 + 12345;
    /* return (unsigned int)(rnd_next / 65536) % 32768;*/
    return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
