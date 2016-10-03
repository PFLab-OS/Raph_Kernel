/*
 *
 * Copyright (c) 2015 Raphine Project
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
#include <mem/kstack.h>

#include <dev/hpet.h>
#include <dev/keyboard.h>
#include <dev/pci.h>
#include <dev/vga.h>

#include <net/netctrl.h>
#include <net/socket.h>
#include <arpa/inet.h>

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

#include <freebsd/net/if_var-raph.h>
BsdEthernet *eth;
uint64_t cnt;
int64_t sum;
static const int stime = 3000;
int time, rtime;

#include <dev/disk/ahci/ahci-raph.h>
AhciChannel *g_channel = nullptr;

Callout tt1;
Callout tt2;
Callout tt3;
Callout tt4;

uint8_t ip1[] = {0, 0, 0, 0};
uint8_t ip2[] = {0, 0, 0, 0};
enum class BenchState {
  kIdle,
  kSnd,
  kRcv,
  kQemu,
};
BenchState bstate = BenchState::kIdle;

void halt(int argc, const char* argv[]) {
  acpi_ctrl->Shutdown();
}

void reset(int argc, const char* argv[]) {
  acpi_ctrl->Reset();
}

void bench(int argc, const char* argv[]) {
  if (argc != 2) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (eth == nullptr) {
    gtty->Cprintf("no ethernet interface.\n");
    return;
  }
  if (strcmp(argv[1], "snd") == 0) {
    uint8_t ip1_[] = {192, 168, 100, 100};
    uint8_t ip2_[] = {192, 168, 100, 104};
    memcpy(ip1, ip1_, 4);
    memcpy(ip2, ip2_, 4);
    bstate = BenchState::kSnd;
  } else if (strcmp(argv[1], "rcv") == 0) {
    uint8_t ip1_[] = {192, 168, 100, 104};
    uint8_t ip2_[] = {192, 168, 100, 100};
    memcpy(ip1, ip1_, 4);
    memcpy(ip2, ip2_, 4);
    bstate = BenchState::kRcv;
  } else if (strcmp(argv[1], "qemu") == 0) {
    uint8_t ip1_[] = {10, 0, 2, 9};
    uint8_t ip2_[] = {10, 0, 2, 15};
    memcpy(ip1, ip1_, 4);
    memcpy(ip2, ip2_, 4);
    bstate = BenchState::kQemu;
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }

  {
    static ArpSocket socket;
    if(socket.Open() < 0) {
      gtty->Cprintf("[error] failed to open socket\n");
    } else {
      socket.SetIpAddr(inet_atoi(ip1));
    }
    cnt = 0;
    new(&tt2) Callout;
    Function func;
    func.Init([](void *){
        if (!apic_ctrl->IsBootupAll()) {
          tt2.SetHandler(1000);
          return;
        }
        eth->UpdateLinkStatus();
        if (eth->GetStatus() != BsdEthernet::LinkStatus::kUp) {
          tt2.SetHandler(1000);
          return;
        }
        if (bstate != BenchState::kRcv) {
          if (cnt != 0) {
            tt2.SetHandler(1000);
            return;
          }
          for(int k = 0; k < 1; k++) {
            if (time == 0) {
              break;
            }
            cnt = timer->ReadMainCnt();
            if(socket.TransmitPacket(ArpSocket::kOpArpRequest, inet_atoi(ip2), nullptr) < 0) {
              gtty->Cprintf("[arp] failed to transmit request\n");
            }
            time--;
          }
          if (time != 0) {
            tt2.SetHandler(1000);
          }
        } else {
          gtty->Cprintf("[debug] info: Link is Up\n");
        }
      }, nullptr);
    tt2.Init(func);
    tt2.SetHandler(3, 10);
  }

  if (bstate != BenchState::kRcv) {
    new(&tt3) Callout;
    Function func;
    func.Init([](void *){
        if (rtime > 0) {
          gtty->Cprintf("ARP Reply average latency: %d us [%d / %d]\n", sum / rtime, rtime, stime);
        } else {
          if (eth->GetStatus() == BsdEthernet::LinkStatus::kUp) {
            gtty->Cprintf("Link is Up, but no ARP Reply\n");
          } else {
            gtty->Cprintf("Link is Down, please wait...\n");
          }
        }
        if (rtime != stime) {
          tt3.SetHandler(1000*1000*3);
        }
      }, nullptr);
    tt3.Init(func);
    tt3.SetHandler(6, 1000*1000*3);
  }
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

  KernelStackCtrl::Init();

  apic_ctrl->Init();

  acpi_ctrl->Setup();

  if (timer->Setup()) {
    gtty->Cprintf("[timer] info: HPET supported.\n");
  } else {
    kernel_panic("timer", "HPET not supported.\n");
  }

  apic_ctrl->Setup();

  rnd_next = timer->ReadMainCnt();

  task_ctrl->Setup();

  idt->SetupGeneric();
  
  apic_ctrl->BootBSP();

  gdt->SetupProc();

  idt->SetupProc();

  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;
  
  acpi_ctrl->SetupAcpica();

  InitNetCtrl();

  InitDevices<PciCtrl, Device>();

  // 各コアは最低限の初期化ののち、TaskCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、TaskCtrlに登録したジョブとして
  // 実行する事

  apic_ctrl->StartAPs();

  gtty->Init();

  Function2<Task> func;
  func.Init([](Task *, void *){
      uint8_t data;
      if(!keyboard->Read(data)){
        return;
      }
      char c = Keyboard::Interpret(data);
      gtty->Cprintf("%c", c);
      shell->ReadCh(c);
    }, nullptr);
  keyboard->Setup(1, func);

  cnt = 0;
  sum = 0;
  time = stime;
  rtime = 0;

  gtty->Cprintf("[cpu] info: #%d (apic id: %d) started.\n", cpu_ctrl->GetId(), apic_ctrl->GetApicIdFromCpuId(cpu_ctrl->GetId()));
  
  if (eth != nullptr) {
    static ArpSocket socket;
    if(socket.Open() < 0) {
      gtty->Cprintf("[error] failed to open socket\n");
    }
    socket.SetIpAddr(inet_atoi(ip1));
    Function2<Task> func;
    func.Init([](Task *, void *){
        uint32_t ipaddr;
        uint8_t macaddr[6];
	
        int32_t rval = socket.ReceivePacket(0, &ipaddr, macaddr);
        if(rval == ArpSocket::kOpArpReply) {
          uint64_t l = ((uint64_t)(timer->ReadMainCnt() - cnt) * (uint64_t)timer->GetCntClkPeriod()) / 1000;
          cnt = 0;
          sum += l;
          rtime++;
        } else if(rval == ArpSocket::kOpArpRequest) {
          socket.TransmitPacket(ArpSocket::kOpArpReply, ipaddr, macaddr);
        }
      }, nullptr);
    socket.SetReceiveCallback(2, func);
  }

  gtty->Cprintf("\n\n[kernel] info: initialization completed\n");

  shell->Setup();
  shell->Register("halt", halt);
  shell->Register("reset", reset);
  shell->Register("bench", bench);

  task_ctrl->Run();

  DismissNetCtrl();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode
  kassert(apic_ctrl->GetCpuId() == cpu_ctrl->GetId());
  
  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  gtty->Cprintf("[cpu] info: #%d (apic id: %d) started.\n", cpu_ctrl->GetId(), apic_ctrl->GetApicIdFromCpuId(cpu_ctrl->GetId()));
 
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

  if (cpu_ctrl->GetId() == 5) {
    new(&tt4) Callout;
    Function func;
    func.Init([](void *){
        kassert(g_channel != nullptr);
        //        g_channel->Read(0, 1);
      }, nullptr);
    tt4.Init(func);
    tt4.SetHandler(10);
  }

  task_ctrl->Run();
  return 0;
}

extern "C" void kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    gtty->CprintfRaw("\n[%s] error: %s",class_name, err_str);
  }
  while(true) {
    asm volatile("cli;hlt;");
  }
}

extern "C" void checkpoint(int id, const char *str) {
  if (id < 0 || cpu_ctrl->GetId() == id) {
    gtty->CprintfRaw(str);
  }
}

extern "C" void _checkpoint(const char *func, const int line) {
  gtty->CprintfRaw("[%s:%d]", func, line);
}

extern "C" void abort() {
  if (gtty != nullptr) {
    gtty->CprintfRaw("system stopped by unexpected error.\n");
  }
  while(true){
    asm volatile("cli;hlt");
  }
}

extern "C" void _kassert(const char *file, int line, const char *func) {
  if (gtty != nullptr) {
    gtty->CprintfRaw("assertion failed at %s l.%d (%s) cpuid: %d\n", file, line, func, cpu_ctrl->GetId());
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
