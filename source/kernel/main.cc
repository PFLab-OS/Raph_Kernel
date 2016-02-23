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

#include "net/eth.h"
#include "net/ip.h"
#include "net/udp.h"

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

Tty *gtty;

EthCtrl *eth_ctrl;
IPCtrl *ip_ctrl;
UDPCtrl *udp_ctrl;
PCICtrl *pci_ctrl;

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

  EthCtrl _eth_ctrl;
  eth_ctrl = &_eth_ctrl;

  IPCtrl _ip_ctrl(eth_ctrl);
  ip_ctrl = &_ip_ctrl;

  UDPCtrl _udp_ctrl(ip_ctrl);
  udp_ctrl = &_udp_ctrl;
  
  InitDevices<PCICtrl, Device>();

  gtty->Printf("s", "cpu #", "d", apic_ctrl->GetApicId(), "s", " started.\n");
  apic_ctrl->StartAPs();

  gtty->Printf("s", "\n\nkernel initialization completed\n");

  extern int kKernelEndAddr;
  // stackは12K
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr)));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 3) + 1));
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - 4096 * 3));

  polling_ctrl->HandleAll();
  while(true) {
    asm volatile("hlt;nop;hlt;");
  }
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

  uint8_t ipv4addr[] = {
    0x0a, 0x00, 0x02, 0x0f
  };

  // ARP Reply
  if (eth_ctrl->OpenSocket()) {
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, PagingCtrl::kPageSize);
    PhysAddr paddr2;
    physmem_ctrl->Alloc(paddr2, PagingCtrl::kPageSize);
    uint8_t *buf = reinterpret_cast<uint8_t *>(paddr.GetVirtAddr());
    uint8_t *buf2 = reinterpret_cast<uint8_t *>(paddr2.GetVirtAddr());
    while(true) {
      if (eth_ctrl->ReceiveData(buf, PagingCtrl::kPageSize) > 0) {
        bool target = true;
        for (int i = 0; i < 6; i++) {
          target &= (buf[i] == 0xff);
        }
        uint8_t *src_eth_addr = buf + 6;
        bool is_arp = (buf[12] == 0x08) && (buf[13] == 0x06);
        bool is_eth = (buf[14] == 0x0) && (buf[15] == 0x01);
        bool is_ipv4 = (buf[16] == 0x80) && (buf[17] == 0x00);
        bool is_valid = (buf[18] == 0x06) && (buf[19] == 0x04);
        bool is_req = (buf[20] == 0x00) && (buf[21] == 0x01);
        uint8_t *source_eth_addr = buf + 22;
        uint8_t *source_proto_addr = buf + 28;
        uint8_t *target_proto_addr = buf + 38;
        // ARP req
        gtty->Printf("s","x");
        if (is_arp && is_eth && is_ipv4 && is_valid && is_req && (*(reinterpret_cast<uint32_t *>(target_proto_addr)) == *(reinterpret_cast<uint32_t *>(ipv4addr)))) {
          // ARP reply
          memcpy(buf2, src_eth_addr, 6);
          eth_ctrl->GetEthAddr(buf2 + 6);
          buf2[12] = 0x08;
          buf2[13] = 0x06;
          buf2[14] = 0x00;
          buf2[15] = 0x01;
          buf2[16] = 0x08;
          buf2[17] = 0x00;
          buf2[18] = 0x06;
          buf2[19] = 0x04;
          buf2[20] = 0x00;
          buf2[21] = 0x02;
          eth_ctrl->GetEthAddr(buf2 + 22);
          memcpy(buf2 + 28, ipv4addr, 4);
          memcpy(buf2 + 32, source_eth_addr, 6);
          memcpy(buf2 + 38, source_proto_addr, 4);
          gtty->Printf("s","r");
        }
      }
    }
  }
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
