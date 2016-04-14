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

#include <global.h>
#include <spinlock.h>
#include <acpi.h>
#include <acpica.h>
#include <apic.h>
#include <multiboot.h>
#include <task.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <mem/tmpmem.h>
#include <gdt.h>
#include <idt.h>
#include <timer.h>
#include <tty.h>

#include <dev/hpet.h>
#include <dev/vga.h>
#include <dev/pci.h>
#include <dev/keyboard.h>

#include "net/netctrl.h"
#include "net/socket.h"


SpinLockCtrl *spinlock_ctrl;
MultibootCtrl *multiboot_ctrl;
AcpiCtrl *acpi_ctrl;
ApicCtrl *apic_ctrl;
PhysmemCtrl *physmem_ctrl;
PagingCtrl *paging_ctrl;
VirtmemCtrl *virtmem_ctrl;
TmpmemCtrl *tmpmem_ctrl;
TaskCtrl *task_ctrl;
Gdt *gdt;
Idt *idt;
Timer *timer;

Tty *gtty;
Keyboard *keyboard;

PCICtrl *pci_ctrl;
//Acpica *acpica;

static uint32_t rnd_next = 1;

#include <dev/nic/intel/em/bem.h>
bE1000 *eth;
uint64_t cnt;
int time;

#include <callout.h>
Callout tt1;
Callout tt2;


#define FLAG 2
#if FLAG == 3
#define IP1 192, 168, 100, 117
#define IP2 192, 168, 100, 254
#elif FLAG == 2
#define IP1 192, 168, 100, 117
#define IP2 192, 168, 100, 104
#elif FLAG == 1
#define IP1 192, 168, 100, 104
#define IP2 192, 168, 100, 117
#elif FLAG == 0
#define IP1 10, 0, 2, 5
#define IP2 10, 0, 2, 15
#endif

uint8_t ip[] = {
  IP1,
};

extern "C" int main() {
  SpinLockCtrl _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  
  MultibootCtrl _multiboot_ctrl;
  multiboot_ctrl = &_multiboot_ctrl;

  //  Acpica _acpica;
  //acpica = &_acpica;

  AcpiCtrl _acpi_ctrl;
  acpi_ctrl = &_acpi_ctrl;

  ApicCtrl _apic_ctrl;
  apic_ctrl = &_apic_ctrl;

  Gdt _gdt;
  gdt = &_gdt;
  
  Idt _idt;
  idt = &_idt;

  VirtmemCtrl _virtmem_ctrl;
  virtmem_ctrl = &_virtmem_ctrl;

  TmpmemCtrl _tmpmem_ctrl;
  tmpmem_ctrl = &_tmpmem_ctrl;
  
  PhysmemCtrl _physmem_ctrl;
  physmem_ctrl = &_physmem_ctrl;
  
  PagingCtrl _paging_ctrl;
  paging_ctrl = &_paging_ctrl;

  TaskCtrl _task_ctrl;
  task_ctrl = &_task_ctrl;
  
  Hpet _htimer;
  timer = &_htimer;

  Vga _vga;
  gtty = &_vga;

  Keyboard _keyboard;
  keyboard = &_keyboard;
  
  tmpmem_ctrl->Init();

  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::kPageSize * 2);
  extern int kKernelEndAddr;
  kassert(paging_ctrl->MapPhysAddrToVirtAddr(reinterpret_cast<virt_addr>(&kKernelEndAddr) - PagingCtrl::kPageSize * 4, paddr, PagingCtrl::kPageSize * 2, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT));

  multiboot_ctrl->Setup();
  
  // acpi_ctl->Setup() は multiboot_ctrl->Setup()から呼ばれる

  if (timer->Setup()) {
    gtty->Printf("s","[timer] info: HPET supported.\n");
  } else {
    kernel_panic("timer", "HPET not supported.\n");
  }


  // timer->Sertup()より後
    apic_ctrl->Setup();

  rnd_next = timer->ReadMainCnt();

  // apic_ctrl->Setup()より後
  task_ctrl->Setup();

  idt->SetupGeneric();
  
  apic_ctrl->BootBSP();

  gdt->SetupProc();

  idt->SetupProc();

  InitNetCtrl();

  InitDevices<PCICtrl, Device>();

  gtty->Init();

  kassert(eth != nullptr);
  Function func;
  func.Init([](void *){
      bE1000::Packet *rpacket;
      if(!eth->ReceivePacket(rpacket)) {
        return;
      } 
      // received packet
      if(rpacket->buf[12] == 0x08 && rpacket->buf[13] == 0x06 && rpacket->buf[21] == 0x02) {
        uint64_t l = ((uint64_t)(timer->ReadMainCnt() - cnt) * (uint64_t)timer->GetCntClkPeriod()) / 1000;
        cnt = 0;
        // ARP packet
        gtty->Printf(
                     "s", "ARP Reply received; ",
                     "x", rpacket->buf[22], "s", ":",
                     "x", rpacket->buf[23], "s", ":",
                     "x", rpacket->buf[24], "s", ":",
                     "x", rpacket->buf[25], "s", ":",
                     "x", rpacket->buf[26], "s", ":",
                     "x", rpacket->buf[27], "s", " is ",
                     "d", rpacket->buf[28], "s", ".",
                     "d", rpacket->buf[29], "s", ".",
                     "d", rpacket->buf[30], "s", ".",
                     "d", rpacket->buf[31], "s", " ",
                     "s","latency:","d",l,"s","us\n");
      }
      if(rpacket->buf[12] == 0x08 && rpacket->buf[13] == 0x06 && rpacket->buf[21] == 0x01 && (memcmp(rpacket->buf + 38, ip, 4) == 0)) {
        // ARP packet
        uint8_t data[] = {
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target MAC Address
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC Address
          0x08, 0x06, // Type: ARP
          // ARP Packet
          0x00, 0x01, // HardwareType: Ethernet
          0x08, 0x00, // ProtocolType: IPv4
          0x06, // HardwareLength
          0x04, // ProtocolLength
          0x00, 0x02, // Operation: ARP Reply
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Hardware Address
          0x00, 0x00, 0x00, 0x00, // Source Protocol Address
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
          0x00, 0x00, 0x00, 0x00, // Target Protocol Address
        };
        memcpy(data, rpacket->buf + 6, 6);
        eth->GetEthAddr(data + 6);
        memcpy(data + 22, data + 6, 6);
        memcpy(data + 28, ip, 4);
        memcpy(data + 32, rpacket->buf + 22, 6);
        memcpy(data + 38, rpacket->buf + 28, 4);

        uint32_t len = sizeof(data)/sizeof(uint8_t);
        bE1000::Packet *tpacket;
        kassert(eth->GetTxPacket(tpacket));
        memcpy(tpacket->buf, data, len);
        tpacket->len = len;
        eth->TransmitPacket(tpacket);
        gtty->Printf(
                     "s", "ARP Request received; ",
                     "x", rpacket->buf[22], "s", ":",
                     "x", rpacket->buf[23], "s", ":",
                     "x", rpacket->buf[24], "s", ":",
                     "x", rpacket->buf[25], "s", ":",
                     "x", rpacket->buf[26], "s", ":",
                     "x", rpacket->buf[27], "s", ",",
                     "d", rpacket->buf[28], "s", ".",
                     "d", rpacket->buf[29], "s", ".",
                     "d", rpacket->buf[30], "s", ".",
                     "d", rpacket->buf[31], "s", " says who's ",
                     "d", rpacket->buf[38], "s", ".",
                     "d", rpacket->buf[39], "s", ".",
                     "d", rpacket->buf[40], "s", ".",
                     "d", rpacket->buf[41], "s", "\n");
        gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
      }
      eth->ReuseRxBuffer(rpacket);
    }, nullptr);
  eth->SetReceiveCallback(2, func);

  extern int kKernelEndAddr;
  // stackは16K
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr)));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 1) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 2) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 3) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 4) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 5) + 1));
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - 4096 * 6));

  cnt = 0;

  gtty->Printf("s", "[cpu] info: #", "d", apic_ctrl->GetApicId(), "s", " started.\n");

  apic_ctrl->StartAPs();

  gtty->Printf("s", "\n\n[kernel] info: initialization completed\n");

  //acpica
  //  acpica->Setup();
  //  acpica->Shutdown();

  // print keyboard_input
  PollingFunc _keyboard_polling;
  keyboard->Setup(0); //should we define kDefaultLapicid = 0 ?
  _keyboard_polling.Init([](void *) {
    while(keyboard->Count() > 0) {
      char ch[2] = {'\0','\0'};
      ch[0] = keyboard->GetCh();
      gtty->Printf("s", ch);
    }
  }, nullptr);
  _keyboard_polling.Register();

  task_ctrl->Run();

  DismissNetCtrl();

  return 0;
}


extern "C" int main_of_others() {
// according to mp spec B.3, system should switch over to Symmetric I/O mode
  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  gtty->Printf("s", "[cpu] info: #", "d", apic_ctrl->GetApicId(), "s", " started.\n");

  // ループ性能測定用
  // if (apic_ctrl->GetApicId() == 4) {
  //   PollingFunc p;
  //   static int hoge = 0;
  //   p.Init([](void *){
  //       int hoge2 = timer->GetUsecFromCnt(timer->ReadMainCnt()) - hoge;
  //       gtty->Printf("d",hoge2,"s"," ");
  //       hoge = timer->GetUsecFromCnt(timer->ReadMainCnt());
  //     }, nullptr);
  //   p.Register();
  // }

  // ワンショット性能測定用
  if (apic_ctrl->GetApicId() == 5) {
    new(&tt1) Callout;
    tt1.Init([](void *){
        if (!apic_ctrl->IsBootupAll()) {
          tt1.SetHandler(1000);
          return;
        }
      }, nullptr);
    tt1.SetHandler(10);
  }

  if (apic_ctrl->GetApicId() == 3) {
    cnt = 0;
    new(&tt2) Callout;
    time = 10;
    tt2.Init([](void *){
        if (!apic_ctrl->IsBootupAll()) {
          tt2.SetHandler(1000);
          return;
        }
        kassert(eth != nullptr);
        eth->UpdateLinkStatus();
        if (eth->GetStatus() != bE1000::LinkStatus::Up) {
          tt2.SetHandler(1000);
          return;
        }
        if (cnt != 0) {
          tt2.SetHandler(10);
          return;
        }
        uint8_t data[] = {
          0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Target MAC Address
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source MAC Address
          0x08, 0x06, // Type: ARP
          // ARP Packet
          0x00, 0x01, // HardwareType: Ethernet
          0x08, 0x00, // ProtocolType: IPv4
          0x06, // HardwareLength
          0x04, // ProtocolLength
          0x00, 0x01, // Operation: ARP Request
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source Hardware Address
          0x00, 0x00, 0x00, 0x00, // Source Protocol Address
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Target Hardware Address
          // Target Protocol Address
          IP2,
        };
        eth->GetEthAddr(data + 6);
        memcpy(data + 22, data + 6, 6);
        memcpy(data + 28, ip, 4);
        uint32_t len = sizeof(data)/sizeof(uint8_t);
        bE1000::Packet *tpacket;
        kassert(eth->GetTxPacket(tpacket));
        memcpy(tpacket->buf, data, len);
        tpacket->len = len;
        cnt = timer->ReadMainCnt();
        eth->TransmitPacket(tpacket);
        gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
        time--;
        if (time != 0) {
          tt2.SetHandler(3000);
        }
      }, nullptr);
    tt2.SetHandler(10);
  }
  task_ctrl->Run();
  return 0;
}

void kernel_panic(const char *class_name, const char *err_str) {
  gtty->PrintfRaw("s", "\n[","s",class_name,"s","] error: ","s",err_str);
  while(true) {
    asm volatile("hlt;");
  }
}

extern "C" void __cxa_pure_virtual() {
  kernel_panic("", "");
}

extern "C" void __stack_chk_fail() {
  kernel_panic("", "");
}

#define RAND_MAX 0x7fff

uint32_t rand() {
  rnd_next = rnd_next * 1103515245 + 12345;
  /* return (unsigned int)(rnd_next / 65536) % 32768;*/
  return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
