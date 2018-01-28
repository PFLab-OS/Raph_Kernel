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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
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
#include <mem/virtmem.h>
#include <raph_acpi.h>
#include <thread.h>
#include <timer.h>
#include <tty.h>
#include <shell.h>
#include <measure.h>
#include <mem/kstack.h>
#include <elf.h>
#include <syscall.h>

#include <dev/hpet.h>
#include <dev/pci.h>
#include <dev/usb/usb.h>
#include <dev/framebuffer.h>
#include <dev/pciid.h>
#include <dev/8042.h>
#include <dev/storage/storage.h>
#include <dev/storage/ramdisk.h>

#include <fs.h>
#include <v6fs.h>

#include <dev/netdev.h>
#include <dev/eth.h>
#include <net/arp.h>
#include <net/udp.h>
#include <net/dhcp.h>

AcpiCtrl *acpi_ctrl = nullptr;
ApicCtrl *apic_ctrl = nullptr;
MultibootCtrl *multiboot_ctrl = nullptr;
PhysmemCtrl *physmem_ctrl = nullptr;
MemCtrl *system_memory_space = nullptr;
Gdt *gdt = nullptr;
Idt *idt = nullptr;
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
PhysmemCtrl _physmem_ctrl;
Hpet _htimer;
FrameBuffer _framebuffer;
Shell _shell;
AcpicaPciCtrl _acpica_pci_ctrl;
NetDevCtrl _netdev_ctrl;
MemCtrl _system_memory_space;
// ArpTable _arp_table;

CpuId network_cpu;
CpuId pstack_cpu;

static uint32_t rnd_next = 1;

#include <dev/storage/ahci/ahci-raph.h>
AhciChannel *g_channel = nullptr;
ArpTable *arp_table = nullptr;

void RegisterDefaultShellCommands();
void load_script(uptr<Array<uint8_t>> data);

void freebsd_main();
extern "C" int main() {
  _system_memory_space.GetKernelVirtmemCtrl()->Init();

  multiboot_ctrl = new (&_multiboot_ctrl) MultibootCtrl;

  acpi_ctrl = new (&_acpi_ctrl) AcpiCtrl;

  apic_ctrl = new (&_apic_ctrl) ApicCtrl;

  cpu_ctrl = new (&_cpu_ctrl) CpuCtrl;

  gdt = new (&_gdt) Gdt;

  idt = new (&_idt) Idt;

  system_memory_space = new (&_system_memory_space) MemCtrl;

  physmem_ctrl = new (&_physmem_ctrl) PhysmemCtrl;

  timer = new (&_htimer) Hpet;

  gtty = new (&_framebuffer) FrameBuffer;

  shell = new (&_shell) Shell;

  netdev_ctrl = new (&_netdev_ctrl) NetDevCtrl();

  // arp_table = new (&_arp_table) ArpTable();

  physmem_ctrl->Init();

  multiboot_ctrl->Setup();

  system_memory_space->GetKernelVirtmemCtrl()->InitKernelMemorySpace();

  system_memory_space->Init();

  gtty->Init();

  multiboot_ctrl->ShowMemoryInfo();

  KernelStackCtrl::Init();

  apic_ctrl->Init();

  acpi_ctrl->Setup();

  if (timer->Setup()) {
    gtty->Printf("[timer] info: HPET supported.\n");
  } else {
    kernel_panic("timer", "HPET not supported.\n");
  }

  apic_ctrl->Setup();

  cpu_ctrl->Init();

  network_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  pstack_cpu = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance);

  rnd_next = timer->ReadTime().GetRaw();

  ThreadCtrl::Init();

  idt->SetupGeneric();

  apic_ctrl->BootBSP();

  gdt->SetupProc();

  // 各コアは最低限の初期化ののち、ThreadCtrlに制御が移さなければならない
  // 特定のコアで専用の処理をさせたい場合は、ThreadCtrlに登録したジョブとして
  // 実行する事
  apic_ctrl->StartAPs();

  system_memory_space->GetKernelVirtmemCtrl()->ReleaseLowMemory();

  gtty->Setup();

  idt->SetupProc();

  pci_ctrl = new (&_acpica_pci_ctrl) AcpicaPciCtrl;

  pci_ctrl->Probe();

  acpi_ctrl->SetupAcpica();

  UsbCtrl::Init();

  freebsd_main();

  AttachDevices<PciCtrl, LegacyKeyboard, Ramdisk, Device>();

  SystemCallCtrl::Init();

  gtty->Printf("\n\n[kernel] info: initialization completed\n");

  arp_table = new ArpTable;
  UdpCtrl::Init();
  DhcpCtrl::Init();

  shell->Setup();
  RegisterDefaultShellCommands();

  auto thread = ThreadCtrl::GetCtrl(
                    cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority))
                    .AllocNewThread(Thread::StackState::kIndependent);
  auto t_op = thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function<void *>(
      [](void *) { load_script(multiboot_ctrl->LoadFile("init.sh")); },
      nullptr)));
  t_op.Schedule();

  ThreadCtrl::GetCurrentCtrl().Run();

  return 0;
}

extern "C" int main_of_others() {
  // according to mp spec B.3, system should switch over to Symmetric I/O mode

  apic_ctrl->BootAP();

  gdt->SetupProc();
  idt->SetupProc();

  ThreadCtrl::GetCurrentCtrl().Run();
  return 0;
}

void show_backtrace(size_t *rbp) {
  size_t top_rbp = reinterpret_cast<size_t>(rbp);
  for (int i = 0; i < 10; i++) {
    if (top_rbp >= rbp[0] || top_rbp <= rbp[0] - 4096) {
      break;
    }
    gtty->ErrPrintf("backtrace(%d): rip:%llx,\n", i, rbp[1]);
    rbp = reinterpret_cast<size_t *>(rbp[0]);
  }
}

extern "C" void _kernel_panic(const char *class_name, const char *err_str) {
  if (gtty != nullptr) {
    gtty->DisablePrint();
    gtty->ErrPrintf("\n!!!! Kernel Panic !!!!\n");
    gtty->ErrPrintf("[%s] error: %s\n", class_name, err_str);
    gtty->ErrPrintf("\n");
    gtty->ErrPrintf(">> debugging information >>\n");
    gtty->ErrPrintf("cpuid: %d\n", cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0" : "=r"(rbp));
    show_backtrace(rbp);
  }
  while (true) {
    asm volatile("cli;hlt;");
  }
}

extern "C" void checkpoint(int id, const char *str) {
  if (id < 0 || cpu_ctrl->GetCpuId().GetRawId() == id) {
    gtty->Printf(str);
    gtty->Flush();
  }
}

extern "C" void _checkpoint(const char *func, const int line) {
  gtty->Printf("[%s:%d]", func, line);
  gtty->Flush();
}

extern "C" void abort() {
  if (gtty != nullptr) {
    gtty->DisablePrint();
    gtty->ErrPrintf("system stopped by unexpected error.\n");
    size_t *rbp;
    asm volatile("movq %%rbp, %0" : "=r"(rbp));
    show_backtrace(rbp);
  }
  while (true) {
    asm volatile("cli;hlt");
  }
}

extern "C" void _kassert(const char *file, int line, const char *func) {
  if (gtty != nullptr) {
    gtty->DisablePrint();
    gtty->ErrPrintf("assertion failed at %s l.%d (%s) cpuid: %d\n", file, line,
                    func, cpu_ctrl->GetCpuId().GetRawId());
    size_t *rbp;
    asm volatile("movq %%rbp, %0" : "=r"(rbp));
    show_backtrace(rbp);
  }
  while (true) {
    asm volatile("cli;hlt");
  }
}

extern "C" void __cxa_pure_virtual() { kernel_panic("", ""); }

extern "C" void __stack_chk_fail() { kernel_panic("", ""); }

#define RAND_MAX 0x7fff

extern "C" uint32_t rand() {
  rnd_next = rnd_next * 1103515245 + 12345;
  /* return (unsigned int)(rnd_next / 65536) % 32768;*/
  return (uint32_t)(rnd_next >> 16) & RAND_MAX;
}
