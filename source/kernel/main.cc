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
#include <measure.h>
#include <mem/kstack.h>

#include <dev/hpet.h>
#include <dev/keyboard.h>
#include <dev/pci.h>
#include <dev/usb/usb.h>
#include <dev/vga.h>
#include <dev/pciid.h>

#include <dev/netdev.h>
// #include <net/arp.h>
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
// ArpTable *arp_table = nullptr;

MultibootCtrl _multiboot_ctrl;
AcpiCtrl _acpi_ctrl;
ApicCtrl _apic_ctrl;
CpuCtrl _cpu_ctrl;
Gdt _gdt;
Idt _idt;
VirtmemCtrl _virtmem_ctrl;
PhysmemCtrl _physmem_ctrl;
PagingCtrl _paging_ctrl;
TaskCtrl _task_ctrl;
Hpet _htimer;
Vga _vga;
Keyboard _keyboard;
Shell _shell;
AcpicaPciCtrl _acpica_pci_ctrl;
NetDevCtrl _netdev_ctrl;
// ArpTable _arp_table;

CpuId network_cpu;
CpuId pstack_cpu;

static uint32_t rnd_next = 1;

#include <dev/disk/ahci/ahci-raph.h>
AhciChannel *g_channel = nullptr;
#include <dev/fs/fat/fat.h>
FatFs *fatfs;

#ifdef FLAG_MEMBENCH
static const bool do_membench = true;
#else
static const bool do_membench = false;
#endif

void register_membench2_callout();

void halt(int argc, const char* argv[]) {
  acpi_ctrl->Shutdown();
}

void reset(int argc, const char* argv[]) {
  acpi_ctrl->Reset();
}

void cpuinfo(int argc, const char* argv[]) {
  gtty->CprintfRaw("cpu num: %d\n", cpu_ctrl->GetHowManyCpus());
}

void lspci(int argc, const char* argv[]) {
  MCFG *mcfg = acpi_ctrl->GetMCFG();
  if (mcfg == nullptr) {
    gtty->Cprintf("[Pci] error: could not find MCFG table.\n");
    return;
  }

  PciData::Table table;
  table.Init();

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
        table.Search(vid, did, svid, ssid);
      }
    }
  }
}

void ifconfig(int argc, const char* argv[]){
  uptr<Array<const char *>> list = netdev_ctrl->GetNamesOfAllDevices();
  gtty->CprintfRaw("\n");
  for (size_t l = 0; l < list->GetLen(); l++) {
    gtty->CprintfRaw("%s", (*list)[l]);
    NetDev *dev = netdev_ctrl->GetDeviceInfo((*list)[l])->device;
    dev->UpdateLinkStatus();
    gtty->CprintfRaw("  link: %s\n", dev->IsLinkUp() ? "up" : "down");
  }
}

void setup_arp_reply(NetDev *dev);
void send_arp_packet(NetDev *dev, uint8_t *ipaddr);

void bench(int argc, const char* argv[]) {

  if (argc == 1) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "snd") == 0) {
    if (argc == 2) {
      gtty->Cprintf("specify ethernet interface.\n");
      return;
    } else if (argc == 3) {
      gtty->Cprintf("specify ip v4 addr.\n");
      return;
    } else if (argc != 4) {
      gtty->Cprintf("invalid arguments\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Cprintf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    NetDev *dev = info->device;

    uint8_t addr[4] = {0, 0, 0, 0};
    int i = 0;
    const char *c = argv[3];
    while(true) {
      if (i != 3 && *c == '.') {
        i++;
      } else if (i == 3 && *c == '\0') {
        break;
      } else if (*c >= '0' && *c <= '9') {
        addr[i] *= 10;
        addr[i] += *c - '0';
      } else {
        gtty->Cprintf("invalid ip v4 addr.\n");
        return;
      }
      c++;
    }

    send_arp_packet(dev, addr);
  } else if (strcmp(argv[1], "setip") == 0) {
    if (argc == 2) {
      gtty->Cprintf("specify ethernet interface.\n");
      return;
    } else if (argc == 3) {
      gtty->Cprintf("specify ip v4 addr.\n");
      return;
    } else if (argc != 4) {
      gtty->Cprintf("invalid arguments\n");
      return;
    }
    NetDevCtrl::NetDevInfo *info = netdev_ctrl->GetDeviceInfo(argv[2]);
    if (info == nullptr) {
      gtty->Cprintf("no ethernet interface(%s).\n", argv[2]);
      return;
    }
    NetDev *dev = info->device;

    uint8_t addr[4] = {0, 0, 0, 0};
    int i = 0;
    const char *c = argv[3];
    while(true) {
      if (i != 3 && *c == '.') {
        i++;
      } else if (i == 3 && *c == '\0') {
        break;
      } else if (*c >= '0' && *c <= '9') {
        addr[i] *= 10;
        addr[i] += *c - '0';
      } else {
        gtty->Cprintf("invalid ip v4 addr.\n");
        return;
      }
      c++;
    }

    dev->AssignIpv4Address(addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3]);
    
    setup_arp_reply(dev);
  } /* else if (strcmp(argv[1], "qemu") == 0) {
    uint8_t ip1_[] = {10, 0, 2, 9};
    uint8_t ip2_[] = {10, 0, 2, 15};
    memcpy(ip1, ip1_, 4);
    memcpy(ip2, ip2_, 4);
    sdevice = (argc == 2) ? "eth0" : argv[2];
    rdevice = (argc == 2) ? "eth0" : argv[2];
    if (!netdev_ctrl->Exists(rdevice)) {
      gtty->Cprintf("no ethernet interface(%s).\n", rdevice);
      return;
    }
    } */ else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

static void setflag(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "spinlock_timeout") == 0) {
    if (argc == 2) {
      gtty->Cprintf("invalid argument.\n");
      return;
    }
    if (strcmp(argv[2], "true") == 0) {
      SpinLock::_spinlock_timeout = true;
    } else if (strcmp(argv[2], "false") == 0) {
      SpinLock::_spinlock_timeout = false;
    } else {
      gtty->Cprintf("invalid argument.\n");
      return;
    }
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

static void show(int argc, const char *argv[]) {
  if (argc == 1) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "module") == 0) {
    if (argc != 2) {
      gtty->Cprintf("invalid argument.\n");
      return;
    }
    multiboot_ctrl->ShowModuleInfo();
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

static void load(int argc, const char *argv[]) {
  if (argc != 2) {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
  if (strcmp(argv[1], "script.sh") == 0) {
    auto callout_ = make_sptr(new Callout);
    struct Container {
      uptr<Array<uint8_t>> data;
      int i;
    };
    auto container_ = make_sptr(new Container);
    container_->i = 0;
    container_->data = multiboot_ctrl->LoadFile(argv[1]);
    callout_->Init(make_uptr(new Function2<wptr<Callout>, sptr<Container>>([](wptr<Callout> callout, sptr<Container> container){
            size_t i = container->i;
            while(container->i < container->data->GetLen()) {
              if ((*container->data)[container->i] == '\n') {
                (*container->data)[container->i] = '\0';
                auto ec = make_uptr(new Shell::ExecContainer(shell));
                ec = shell->Tokenize(ec, reinterpret_cast<char *>(container->data->GetRawPtr()) + i);
                container->i++;
                if (strcmp(ec->argv[0], "wait") != 0) {
                  shell->Execute(ec);
                } else {
                  if (ec->argc == 2) {
                    int t = 0;
                    for(size_t l = 0; l < strlen(ec->argv[1]); l++) {
                      t = t * 10 + ec->argv[1][l] - '0';
                    }
                    task_ctrl->RegisterCallout(make_sptr(callout), t * 1000 * 1000);
                    return;
                  } else {
                    gtty->Cprintf("invalid argument.\n");
                  }
                }
                task_ctrl->RegisterCallout(make_sptr(callout), 10);
                return;
              }
              container->i++;
            }
          }, make_wptr(callout_), container_)));
    task_ctrl->RegisterCallout(callout_, 10);
  } else {
    gtty->Cprintf("invalid argument.\n");
    return;
  }
}

void freebsd_main();

extern "C" int main() {

  multiboot_ctrl = new (&_multiboot_ctrl) MultibootCtrl;

  acpi_ctrl = new (&_acpi_ctrl) AcpiCtrl;

  apic_ctrl = new (&_apic_ctrl) ApicCtrl;

  cpu_ctrl = new (&_cpu_ctrl) CpuCtrl;

  gdt = new (&_gdt) Gdt;

  idt = new (&_idt) Idt;

  virtmem_ctrl = new (&_virtmem_ctrl) VirtmemCtrl;

  physmem_ctrl = new (&_physmem_ctrl) PhysmemCtrl;

  paging_ctrl = new (&_paging_ctrl) PagingCtrl;

  task_ctrl = new (&_task_ctrl) TaskCtrl;

  timer = new (&_htimer) Hpet;

  gtty = new (&_vga) Vga;

  keyboard = new (&_keyboard) Keyboard;

  shell = new (&_shell) Shell;

  netdev_ctrl = new (&_netdev_ctrl) NetDevCtrl();

  // arp_table = new (&_arp_table) ArpTable();

  physmem_ctrl->Init();

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
  
  network_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  pstack_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  rnd_next = timer->ReadMainCnt();

  task_ctrl->Setup();

  idt->SetupGeneric();

  apic_ctrl->BootBSP();

  gdt->SetupProc();

  idt->SetupProc();
  
  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;

  acpi_ctrl->SetupAcpica();

  UsbCtrl::Init();

  freebsd_main();

  InitDevices<PciCtrl, Device>();

  // 各コアは最低限の初期化ののち、TaskCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、TaskCtrlに登録したジョブとして
  // 実行する事

  apic_ctrl->StartAPs();

  gtty->Init();
  
  // arp_table->Setup();

  keyboard->Setup(make_uptr(new Function<void *>([](void *){
          uint8_t data;
          if(!keyboard->Read(data)){
            return;
          }
          char c = Keyboard::Interpret(data);
          shell->ReadCh(c);
        }, nullptr)));

  if (cpu_ctrl->GetHowManyCpus() <= 16) {
    gtty->Cprintf("[cpu] info: #%d (apic id: %d) started.\n",
                  cpu_ctrl->GetCpuId(),
                  cpu_ctrl->GetCpuId().GetApicId());
  }

  while (!apic_ctrl->IsBootupAll()) {
  }
  gtty->Cprintf("\n\n[kernel] info: initialization completed\n");
  multiboot_ctrl->ShowBuildTimeStamp();

  shell->Setup();
  shell->Register("halt", halt);
  shell->Register("reset", reset);
  shell->Register("bench", bench);
  shell->Register("lspci", lspci);
  shell->Register("cpuinfo", cpuinfo);
  shell->Register("ifconfig", ifconfig);
  shell->Register("show", show);
  shell->Register("load", load);
  shell->Register("setflag", setflag);

  if (!do_membench) {
    CpuId beep_cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
    auto callout_ = make_sptr(new Callout);
    callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout) {
            static int i = 0;
            if(i < 6) {
              uint16_t sound[6] = {905, 761, 452, 570, 508, 380};
              uint8_t a = 0xb6;
              outb(0x43, a);
              uint8_t l = sound[i] & 0x00FF;
              outb(0x42, l);
              uint8_t h = (sound[i] >> 8) & 0x00FF;
              outb(0x42, h);
              uint8_t on = inb(0x61);
              outb(0x61, (on | 0x03) & 0x0f);
              i++;
              task_ctrl->RegisterCallout(make_sptr(callout), 110000);
            } else {
              uint8_t off = inb(0x61);
              outb(0x61, off & 0xd);
            }
          }, make_wptr(callout_))));
    task_ctrl->RegisterCallout(callout_, beep_cpuid, 1);
  } else {
    register_membench2_callout();
  }
  
  task_ctrl->Run();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode
  
  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  if (cpu_ctrl->GetHowManyCpus() <= 16) {
    gtty->Cprintf("[cpu] info: #%d (apic id: %d) started.\n",
                  cpu_ctrl->GetCpuId(),
                  cpu_ctrl->GetCpuId().GetApicId());
  }
 
  // ループ性能測定用
  //#define LOOP_BENCHMARK
#ifdef LOOP_BENCHMARK
#define LOOP_BENCHMARK_CPU  4
  PollingFunc p;
  if (cpu_ctrl->GetCpuId().GetRawId() == LOOP_BENCHMARK_CPU) {
    static int hoge = 0;
    p.Init(make_uptr(new Function<void *>([](void *){
            int hoge2 = timer->GetUsecFromCnt(timer->ReadMainCnt()) - hoge;
            gtty->Cprintf("%d ", hoge2);
            hoge = timer->GetUsecFromCnt(timer->ReadMainCnt());
          }, nullptr)));
    p.Register();
  }
#endif
  
// ワンショット性能測定用
// #define ONE_SHOT_BENCHMARK
#ifdef ONE_SHOT_BENCHMARK
#define ONE_SHOT_BENCHMARK_CPU  5
  if (cpu_ctrl->GetCpuId().GetRawId() == ONE_SHOT_BENCHMARK_CPU) {
    auto callout_ = make_sptr(new Callout);
    callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout){
            if (!apic_ctrl->IsBootupAll()) {
              task_ctrl->RegisterCallout(make_sptr(callout), 1000);
              return;
            }
            // kassert(g_channel != nullptr);
            // FatFs *fatfs = new FatFs();
            // kassert(fatfs->Mount());
            //        g_channel->Read(0, 1);
          }, make_wptr(callout_))));
    task_ctrl->RegisterCallout(callout_, 10);
  }
#endif
  if (do_membench) {
    register_membench2_callout();
  }

  task_ctrl->Run();
  return 0;
}

static int error_output_flag = 0;

void show_backtrace(size_t *rbp) {
    for (int i = 0; i < 3; i++) {
      gtty->CprintfRaw("backtrace(%d): rip:%llx,\n", i, rbp[1]);
      rbp = reinterpret_cast<size_t *>(rbp[0]);
    }
}

extern "C" void _kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    while(!__sync_bool_compare_and_swap(&error_output_flag, 0, 1)) {
    }
    gtty->CprintfRaw("\n!!!! Kernel Panic !!!!\n");
    gtty->CprintfRaw("[%s] error: %s\n",class_name, err_str);
    gtty->CprintfRaw("\n"); 
    gtty->CprintfRaw(">> debugging information >>\n");
    gtty->CprintfRaw("cpuid: %d\n", cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    show_backtrace(rbp);
    __sync_bool_compare_and_swap(&error_output_flag, 1, 0);
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
    while(!__sync_bool_compare_and_swap(&error_output_flag, 0, 1)) {
    }
    gtty->CprintfRaw("assertion failed at %s l.%d (%s) cpuid: %d\n",
                     file, line, func, cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0":"=r"(rbp));
    show_backtrace(rbp);
    __sync_bool_compare_and_swap(&error_output_flag, 1, 0);
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
