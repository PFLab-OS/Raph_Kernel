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

#include <dev/eth.h>
#include <net/arp.h>
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
NetDevCtrl *netdev_ctrl = nullptr;
ArpTable *arp_table = nullptr;

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
NetDevCtrl _netdev_ctrl;
ArpTable _arp_table;

CpuId network_cpu;
CpuId pstack_cpu;

static uint32_t rnd_next = 1;

uint64_t cnt;
int64_t sum;
static const int stime = 3000;
int time, rtime;

#include <dev/disk/ahci/ahci-raph.h>
AhciChannel *g_channel = nullptr;
#include <dev/fs/fat/fat.h>
FatFs *fatfs;

Callout tt1;
Callout tt2;
Callout tt3;

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

void lspci(int argc, const char* argv[]){
  MCFG *mcfg = acpi_ctrl->GetMCFG();
  if (mcfg == nullptr) {
    gtty->Cprintf("[Pci] error: could not find MCFG table.\n");
    return;
  }

  for (int i = 0; i * sizeof(MCFGSt) < mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Cprintf("[Pci] info: multiple MCFG tables.\n");
      break;
    }
    for (int j = mcfg->list[i].pci_bus_start; j <= mcfg->list[i].pci_bus_end; j++) {
      for (int k = 0; k < 32; k++) {
        uint16_t vid = pci_ctrl->ReadReg<uint16_t>(j, k, 0, PciCtrl::kVendorIDReg);
        if (vid == 0xffff) {
          continue;
        }
	uint16_t did = pci_ctrl->ReadReg<uint16_t>(j, k, 0, PciCtrl::kDeviceIDReg);
	uint16_t svid = pci_ctrl->ReadReg<uint16_t>(j, k, 0, PciCtrl::kSubsystemVendorIdReg);
	uint16_t ssid = pci_ctrl->ReadReg<uint16_t>(j, k, 0, PciCtrl::kSubsystemIdReg);
	gtty->Cprintf("VendorID:%u, DeviceID:%u, SubVendorID:%u, SubSystemID:%u\n", vid, did, svid, ssid);
      }
    }
  }
}

void bench(int argc, const char* argv[]) {
  if (argc != 2) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (!netdev_ctrl->Exists("en0")) {
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

  netdev_ctrl->AssignIpv4Address("en0", inet_atoi(ip1));

  {
    static ArpSocket socket;
    socket.AssignNetworkDevice("en0");

    if(socket.Open() < 0) {
      gtty->Cprintf("[error] failed to open socket\n");
    }
    cnt = 0;
    new(&tt2) Callout;
    Function func;
    func.Init([](void *){
        if (!apic_ctrl->IsBootupAll()) {
          tt2.SetHandler(1000);
          return;
        }
        if (!netdev_ctrl->IsLinkUp("en0")) {
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
            if(socket.Request(inet_atoi(ip2)) < 0) {
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
    CpuId cpuid(3);
    tt2.SetHandler(cpuid, 10);
  }

  if (bstate != BenchState::kRcv) {
    new(&tt3) Callout;
    Function func;
    func.Init([](void *){
        if (rtime > 0) {
          gtty->Cprintf("ARP Reply average latency: %d us [%d / %d]\n", sum / rtime, rtime, stime);
        } else {
          if (netdev_ctrl->IsLinkUp("en0")) {
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
    CpuId cpuid(6);
    tt3.SetHandler(cpuid, 1000*1000*3);
  }

  static ArpSocket socket;
  socket.AssignNetworkDevice("en0");

  if(socket.Open() < 0) {
    gtty->Cprintf("[error] failed to open socket\n");
  } else {
    Function func;
    func.Init([](void *){
        uint32_t ipaddr;
        uint16_t op;
        
        if (socket.Read(op, ipaddr) >= 0) {
          if(op == ArpSocket::kOpReply) {
            uint64_t l = ((uint64_t)(timer->ReadMainCnt() - cnt) * (uint64_t)timer->GetCntClkPeriod()) / 1000;
            cnt = 0;
            sum += l;
            rtime++;
          } else if(op == ArpSocket::kOpRequest) {
            socket.Reply(ipaddr);
          }
        }
      }, nullptr);
    CpuId cpuid(2);
    socket.SetReceiveCallback(cpuid, func);
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

  netdev_ctrl = new (&_netdev_ctrl) NetDevCtrl();

  arp_table = new (&_arp_table) ArpTable();

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

  cpu_ctrl->Init();

  CpuId network_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);
;
  CpuId pstack_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);
;

  rnd_next = timer->ReadMainCnt();

  task_ctrl->Setup();

  idt->SetupGeneric();
  
  apic_ctrl->BootBSP();

  gdt->SetupProc();

  idt->SetupProc();

  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;
  
  acpi_ctrl->SetupAcpica();

  InitDevices<PciCtrl, Device>();

  // 各コアは最低限の初期化ののち、TaskCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、TaskCtrlに登録したジョブとして
  // 実行する事

  apic_ctrl->StartAPs();

  gtty->Init();
  
  arp_table->Setup();

  Function kbd_func;
  kbd_func.Init([](void *){
      uint8_t data;
      if(!keyboard->Read(data)){
        return;
      }
      char c = Keyboard::Interpret(data);
      gtty->Cprintf("%c", c);
      shell->ReadCh(c);
    }, nullptr);
  keyboard->Setup(kbd_func);

  cnt = 0;
  sum = 0;
  time = stime;
  rtime = 0;

  gtty->Cprintf("[cpu] info: #%d (apic id: %d) started.\n",
                cpu_ctrl->GetCpuId(),
                cpu_ctrl->GetCpuId().GetApicId());

  gtty->Cprintf("\n\n[kernel] info: initialization completed\n");

  shell->Setup();
  shell->Register("halt", halt);
  shell->Register("reset", reset);
  shell->Register("bench", bench);
  shell->Register("lspci", lspci);

  task_ctrl->Run();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode
  
  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  gtty->Cprintf("[cpu] info: #%d (apic id: %d) started.\n",
  cpu_ctrl->GetCpuId(),
  cpu_ctrl->GetCpuId().GetApicId());
 
// ループ性能測定用
//#define LOOP_BENCHMARK
#ifdef LOOP_BENCHMARK
  #define LOOP_BENCHMARK_CPU  4
  PollingFunc p;
  if (cpu_ctrl->GetCpuId().GetRawId() == LOOP_BENCHMARK_CPU) {
    Function f;
    static int hoge = 0;
    f.Init([](void *){
      int hoge2 = timer->GetUsecFromCnt(timer->ReadMainCnt()) - hoge;
      gtty->Cprintf("%d ", hoge2);
      hoge = timer->GetUsecFromCnt(timer->ReadMainCnt());
    }, nullptr);
    p.Init(f);
    p.Register();
  }
#endif
  
// ワンショット性能測定用
#define ONE_SHOT_BENCHMARK
#ifdef ONE_SHOT_BENCHMARK
  #define ONE_SHOT_BENCHMARK_CPU  5
  if (cpu_ctrl->GetCpuId().GetRawId() == ONE_SHOT_BENCHMARK_CPU) {
    new(&tt1) Callout;
    Function oneshot_bench_func;
    oneshot_bench_func.Init([](void *){
        if (!apic_ctrl->IsBootupAll()) {
          tt1.SetHandler(1000);
          return;
        }
        // kassert(g_channel != nullptr);
        // FatFs *fatfs = new FatFs();
        // kassert(fatfs->Mount());
        //        g_channel->Read(0, 1);
      }, nullptr);
    tt1.Init(oneshot_bench_func);
    tt1.SetHandler(10);
  }
#endif

  task_ctrl->Run();
  return 0;
}

extern "C" void _kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    gtty->CprintfRaw("\n!!!! Kernel Panic !!!!\n");
    gtty->CprintfRaw("[%s] error: %s\n",class_name, err_str);
    gtty->CprintfRaw("\n"); 
    gtty->CprintfRaw(">> debugging information >>\n");
    gtty->CprintfRaw("cpuid: %d\n", cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    for (int i = 0; i < 3; i++) {
      gtty->CprintfRaw("backtrace(%d): rip:%llx,\n", i, rbp[1]);
      rbp = reinterpret_cast<size_t *>(rbp[0]);
    }
  }
  while(true) {
    asm volatile("cli;hlt;");
  }
}

extern "C" void checkpoint(int id, const char *str) {
  if (id < 0 || cpu_ctrl->GetCpuId().GetRawId() == id) {
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
    gtty->CprintfRaw("assertion failed at %s l.%d (%s) cpuid: %d\n",
      file, line, func, cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    for (int i = 0; i < 3; i++) {
      gtty->CprintfRaw("backtrace(%d): rip:%llx,\n", i, rbp[1]);
      rbp = reinterpret_cast<size_t *>(rbp[0]);
    }
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
