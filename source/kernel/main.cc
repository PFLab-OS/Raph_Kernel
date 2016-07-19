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

#include <apic.h>
#include <cpu.h>
#include <gdt.h>
#include <global.h>
#include <idt.h>
#include <multiboot.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <mem/virtmem.h>
#include <raph_acpi.h>
#include <task.h>
#include <timer.h>
#include <tty.h>
#include <shell.h>

#include <dev/hpet.h>
#include <dev/keyboard.h>
#include <dev/pci.h>
#include <dev/vga.h>

#include <net/netctrl.h>
#include <net/socket.h>

AcpiCtrl *acpi_ctrl = nullptr;
ApicCtrl *apic_ctrl = nullptr;
MultibootCtrl *multiboot_ctrl = nullptr;
PagingCtrl *paging_ctrl = nullptr;
PhysmemCtrl *physmem_ctrl = nullptr;
VirtmemCtrl *virtmem_ctrl = nullptr;
Gdt *gdt = nullptr;
Idt *idt = nullptr;
Keyboard *keyboard = nullptr;
Shell *shell = nullptr;
PciCtrl *pci_ctrl = nullptr;

MultibootCtrl _multiboot_ctrl;
AcpiCtrl _acpi_ctrl;
ApicCtrl _apic_ctrl;
CpuCtrl _cpu_ctrl;
Gdt _gdt;
Idt _idt;
KVirtmemCtrl _virtmem_ctrl;
PhysmemCtrl _physmem_ctrl;
PagingCtrl _paging_ctrl;
TaskCtrl _task_ctrl;
Hpet _htimer;
Vga _vga;
Keyboard _keyboard;
Shell _shell;
AcpicaPciCtrl _acpica_pci_ctrl;

static uint32_t rnd_next = 1;

#include <freebsd/net/if_var.h>
BsdDevEthernet *eth;
uint64_t cnt;
int64_t sum;
static const int stime = 3000;
int time, rtime;

Callout tt1;
Callout tt2;
Callout tt3;

#define QEMU 0
#define SND  1
#define RCV  2
#define TEST 3

#define FLAG QEMU 
#if FLAG == TEST
#define IP1 192, 168, 100, 117
#define IP2 192, 168, 100, 254
#elif FLAG == SND
#define IP1 192, 168, 100, 117
#define IP2 192, 168, 100, 104
#elif FLAG == RCV
#define IP1 192, 168, 100, 104
#define IP2 192, 168, 100, 117
#elif FLAG == QEMU
#define IP1 10, 0, 2, 9
#define IP2 10, 0, 2, 15
#endif


uint8_t ip[] = {
  IP1,
};

void shell_test(int argc, const char* argv[]) {  //this function is for testing
  gtty->Printf("s", "shell-test function is called\n");
  gtty->Printf("d", argc, "s", " arguments.\n");
  for (int i =0; i < argc; i++) gtty->Printf("s", argv[i], "s", "\n");
  if (argv[argc] == nullptr) gtty->Printf("s", "the last member is nullptr.\n");
}

extern "C" int main() {
  
  multiboot_ctrl = new (&_multiboot_ctrl) MultibootCtrl;

  acpi_ctrl = new (&_acpi_ctrl) AcpiCtrl;

  apic_ctrl = new (&_apic_ctrl) ApicCtrl;

  cpu_ctrl = new (&_cpu_ctrl) CpuCtrl;

  gdt = new (&_gdt) Gdt;
  
  idt = new (&_idt) Idt;

  virtmem_ctrl = new (&_virtmem_ctrl) KVirtmemCtrl;

  physmem_ctrl = new (&_physmem_ctrl) PhysmemCtrl;
  
  paging_ctrl = new (&_paging_ctrl) PagingCtrl;

  task_ctrl = new (&_task_ctrl) TaskCtrl;
  
  timer = new (&_htimer) Hpet;

  gtty = new (&_vga) Vga;

  keyboard = new (&_keyboard) Keyboard;

  shell = new (&_shell) Shell;

  multiboot_ctrl->Setup();

  paging_ctrl->MapAllPhysMemory();

  PhysAddr paddr;
  physmem_ctrl->Alloc(paddr, PagingCtrl::kPageSize * 2);
  extern int kKernelEndAddr;
  kassert(paging_ctrl->MapPhysAddrToVirtAddr(reinterpret_cast<virt_addr>(&kKernelEndAddr) - PagingCtrl::kPageSize * 4, paddr, PagingCtrl::kPageSize * 2, PDE_WRITE_BIT, PTE_WRITE_BIT | PTE_GLOBAL_BIT));

  
  acpi_ctrl->Setup();

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

  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;
  
  acpi_ctrl->SetupAcpica();
  // acpi_ctrl->Shutdown();

  InitDevices<PciCtrl, Device>();

  gtty->Init();

  keyboard->Setup(1);

  cnt = 0;
  sum = 0;
  time = stime;
  rtime = 0;

  extern int kKernelEndAddr;
  // stackは16K
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr)));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 1) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 2) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 3) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 4) + 1));
  kassert(paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - (4096 * 5) + 1));
  kassert(!paging_ctrl->IsVirtAddrMapped(reinterpret_cast<virt_addr>(&kKernelEndAddr) - 4096 * 6));

  gtty->Printf("s", "[cpu] info: #", "d", cpu_ctrl->GetId(), "s", "(apic id:", "d", apic_ctrl->GetApicIdFromCpuId(cpu_ctrl->GetId()), "s", ") started.\n");
  if (eth != nullptr) {
    Function func;
    func.Init([](void *){
        BsdDevEthernet::Packet *rpacket;
        if(!eth->ReceivePacket(rpacket)) {
          return;
        }
        // received packet
        if(rpacket->buf[12] == 0x08 && rpacket->buf[13] == 0x06 && rpacket->buf[21] == 0x02) {
          uint64_t l = ((uint64_t)(timer->ReadMainCnt() - cnt) * (uint64_t)timer->GetCntClkPeriod()) / 1000;
          cnt = 0;
          sum += l;
          rtime++;
          // ARP packet
          char buf[40];
          memcpy(buf, rpacket->buf,40);
          // gtty->Printf(
          //              "s", "ARP Reply received; ",
          //              "x", rpacket->buf[22], "s", ":",
          //              "x", rpacket->buf[23], "s", ":",
          //              "x", rpacket->buf[24], "s", ":",
          //              "x", rpacket->buf[25], "s", ":",
          //              "x", rpacket->buf[26], "s", ":",
          //              "x", rpacket->buf[27], "s", " is ",
          //              "d", rpacket->buf[28], "s", ".",
          //              "d", rpacket->buf[29], "s", ".",
          //              "d", rpacket->buf[30], "s", ".",
          //              "d", rpacket->buf[31], "s", " ",
          //              "s","latency:","d",l,"s","us\n");
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
          BsdDevEthernet::Packet *tpacket;
          kassert(eth->GetTxPacket(tpacket));
          memcpy(tpacket->buf, data, len);
          tpacket->len = len;
          eth->TransmitPacket(tpacket);
          // gtty->Printf(
          //              "s", "ARP Request received; ",
          //              "x", rpacket->buf[22], "s", ":",
          //              "x", rpacket->buf[23], "s", ":",
          //              "x", rpacket->buf[24], "s", ":",
          //              "x", rpacket->buf[25], "s", ":",
          //              "x", rpacket->buf[26], "s", ":",
          //              "x", rpacket->buf[27], "s", ",",
          //              "d", rpacket->buf[28], "s", ".",
          //              "d", rpacket->buf[29], "s", ".",
          //              "d", rpacket->buf[30], "s", ".",
          //              "d", rpacket->buf[31], "s", " says who's ",
          //              "d", rpacket->buf[38], "s", ".",
          //              "d", rpacket->buf[39], "s", ".",
          //              "d", rpacket->buf[40], "s", ".",
          //              "d", rpacket->buf[41], "s", "\n");
          // gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
        }
        eth->ReuseRxBuffer(rpacket);
      }, nullptr);
    eth->SetReceiveCallback(2, func);
  }

  // 各コアは最低限の初期化ののち、TaskCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、TaskCtrlに登録したジョブとして
  // 実行する事

  apic_ctrl->StartAPs();

  gtty->Printf("s", "\n\n[kernel] info: initialization completed\n");

  // shell->Setup();
  // shell->Register("test", shell_test);
  
  do {
    // print keyboard_input
    // TODO: Functional FIFOにすべき
    PollingFunc _keyboard_polling;
    Function func;
    func.Init([](void *) {
        // print keyboard_input
        while(keyboard->Count() > 0) {
          char ch[2] = {'\0','\0'};
          ch[0] = keyboard->GetCh();
          gtty->Printf("s", ch);
          shell->ReadCh(ch[0]);
        }
      }, nullptr);
    _keyboard_polling.Init(func);
    // _keyboard_polling.Register(1);
  } while(0);
  
  task_ctrl->Run();

  DismissNetCtrl();

  return 0;
}

extern "C" int main_of_others() {
// according to mp spec B.3, system should switch over to Symmetric I/O mode
  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  gtty->Printf("s", "[cpu] info: #", "d", cpu_ctrl->GetId(), "s", "(apic id:", "d", apic_ctrl->GetApicIdFromCpuId(cpu_ctrl->GetId()), "s", ") started.\n");
 
  // ループ性能測定用 
  // PollingFunc p;
  // if (cpu_ctrl->GetId() == 4) {
  //   static int hoge = 0;
  //   p.Init([](void *){
  //       int hoge2 = timer->GetUsecFromCnt(timer->ReadMainCnt()) - hoge;
  //       gtty->Printf("d",hoge2,"s"," ");
  //       hoge = timer->GetUsecFromCnt(timer->ReadMainCnt());
  //     }, nullptr);
  //   p.Register();
  // }
  
  // ワンショット性能測定用
  if (cpu_ctrl->GetId() == 5) {
    new(&tt1) Callout;
    Function func;
    func.Init([](void *){
        if (!apic_ctrl->IsBootupAll()) {
          tt1.SetHandler(1000);
          return;
        }
      }, nullptr);
    tt1.Init(func);
    tt1.SetHandler(10);
  }

  if (cpu_ctrl->GetId() == 6 && eth != nullptr) {
#if FLAG != RCV
    new(&tt3) Callout;
    Function func;
    func.Init([](void *){
        if (rtime > 0) {
          gtty->Printf("s","ARP Reply average latency:","d",sum / rtime,"s","us [","d",rtime,"s","/","d",stime,"s","]\n");
        } else {
          if (eth->GetStatus() == BsdDevEthernet::LinkStatus::kUp) {
            gtty->Printf("s","Link is Up, but no ARP Reply\n");
          } else {
            gtty->Printf("s","Link is Down, please wait...\n");
          }
        }
        if (rtime != stime) {
          tt3.SetHandler(1000*1000*3);
        }
      }, nullptr);
    tt3.Init(func);
    tt3.SetHandler(1000*1000*3);
#endif
  }

  if (cpu_ctrl->GetId() == 3 && eth != nullptr) {
    static int state = 0;
    cnt = 0;
    new(&tt2) Callout;
    Function func;
    func.Init([](void *){
        if (state == 0) {
          if (!apic_ctrl->IsBootupAll()) {
            tt2.SetHandler(1000);
            return;
          } else {
            state = 1;
          }
        }
        if (state == 1) {
          eth->UpdateLinkStatus();
          if (eth->GetStatus() != BsdDevEthernet::LinkStatus::kUp) {
            tt2.SetHandler(1000);
            return;
          }
          state = 2;
        }
        if (state == 2) {
          state = 3;
          tt2.SetHandler(0);
          return ;
        }
        if (state == 3) {
#if FLAG != RCV
          if (cnt != 0) {
            tt2.SetHandler(1000);
            return;
          }
          for(int k = 0; k < 1; k++) {
            if (time == 0) {
              break;
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
            BsdDevEthernet::Packet *tpacket;
            kassert(eth->GetTxPacket(tpacket));
            memcpy(tpacket->buf, data, len);
            tpacket->len = len;
            cnt = timer->ReadMainCnt();
            eth->TransmitPacket(tpacket);
            // gtty->Printf("s", "[debug] info: Packet sent (length = ", "d", len, "s", ")\n");
            time--;
          }
          if (time != 0) {
            tt2.SetHandler(1000);
          }
#else
          gtty->Printf("s", "[debug] info: Link is Up\n");
#endif
        }
      }, nullptr);
    tt2.Init(func);
    tt2.SetHandler(10);
  }
  task_ctrl->Run();
  return 0;
}

extern "C" void kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    gtty->PrintfRaw("s", "\n[","s",class_name,"s","] error: ","s",err_str);
  }
  while(true) {
    asm volatile("cli;hlt;");
  }
}

extern "C" void checkpoint(int id, const char *str) {
  if (id < 0 || cpu_ctrl->GetId() == id) {
    gtty->PrintfRaw("s",str);
  }
}

extern "C" void _checkpoint(const char *func, const int line) {
  gtty->CprintfRaw("[%s:%d]", func, line);
}

extern "C" void abort() {
  if (gtty != nullptr) {
    gtty->Cprintf("system stopped by unexpected error.\n");
  }
  while(true){
    asm volatile("cli;hlt");
  }
}

extern "C" void _kassert(const char *file, int line, const char *func) {
  if (gtty != nullptr) {
    gtty->Cprintf("assertion failed at %s l.%d (%s) cpuid: %d\n", file, line, func, cpu_ctrl->GetId());
  }
  while(true){
    asm volatile("cli;hlt");
  }
}

extern "C" void __cxa_pure_virtual() {
  kernel_panic("", "");
}

extern "C" void __stack_chk_fail() {
  kernel_panic("", "");
}

#define RAND_MAX 0x7fff

extern "C" uint32_t rand() {
  rnd_next = rnd_next * 1103515245 + 12345;
  /* return (unsigned int)(rnd_next / 65536) % 32768;*/
  return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
