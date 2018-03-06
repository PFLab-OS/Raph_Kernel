/*
 *
 * Copyright (c) 2017 Raphine Project
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
 * Author: mumumu
 *
 */

#include <elf.h>
#include <process.h>
#include <thread.h>
#include <tty.h>
#include <multiboot.h>
#include <syscall.h>
#include <gdt.h>
#include <stdlib.h>
#include <mem/paging.h>
#include <mem/physmem.h>
#include <global.h>

void Process::Init() {
  _thread = ThreadCtrl::GetCtrl(
                cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority))
                .AllocNewThread(Thread::StackState::kIndependent);
  _mem_ctrl = make_sptr(new MemCtrl());
  _mem_ctrl->Init();
}

void ProcessCtrl::Init() {
  {
    Locker locker(_table_lock);
    sptr<Process> p = _process_table.Init();
    CreateFirstProcess(p);
    _current_exec_process = p;
  }

  _scheduler_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                              CpuPurpose::kLowPriority))
                          .AllocNewThread(Thread::StackState::kIndependent);
  auto t_op = _scheduler_thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function0<void>([]() {
    sptr<Process> p = process_ctrl->GetNextExecProcess();
    if (!p.IsNull()) {
      p->_thread->CreateOperator().Schedule();
    }

    ThreadCtrl::GetCurrentThreadOperator().Schedule(90);
  })));

  t_op.Schedule();
}

// TODO: impl better scheduler.
// This function is scheduler.
sptr<Process> ProcessCtrl::GetNextExecProcess() {
  {
    sptr<Process> cp = _current_exec_process;
    sptr<Process> p = _current_exec_process;
    Locker locker(_table_lock);
    do {
      p = p->_next;
      if (p->GetStatus() == ProcessStatus::kRunnable ||
          p->GetStatus() == ProcessStatus::kEmbryo) {
        _current_exec_process = p;
        return _current_exec_process;
      }
    } while (p != cp);
  }
  sptr<Process> res;
  return res;
}

// For init
sptr<Process> ProcessCtrl::CreateFirstProcess(sptr<Process> process) {
  if (process.IsNull()) {
    kernel_panic("ProcessCtrl", "Could not alloc first process space.");
  }
  process->Init();

  auto t_op = process->_thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function1<void, sptr<Process>>(
      [](sptr<Process> p) {
        p->_mem_ctrl->Init();

        p->_mem_ctrl->Switch();

        Loader loader(p->_mem_ctrl);
        // TODO change test.elf to optimal process (such as init).
        auto tmp = multiboot_ctrl->LoadFile("test.elf");
        ElfObject obj(loader, tmp->GetRawPtr());
        if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
          gtty->Printf("error while loading app\n");
          kernel_panic("ProcessCtrl", "Could not load app in ExecProcess()");
        }

        p->_elfobj = &obj;

        while (true) {
          system_memory_space->Switch();
          switch (p->GetStatus()) {
            case ProcessStatus::kEmbryo:
            case ProcessStatus::kRunning:
              process_ctrl->SetStatus(p, ProcessStatus::kRunnable);
              break;
            default:
              break;
          }

          p->_raw_cpuid = CpuId::kCpuIdNotFound;
          ThreadCtrl::WaitCurrentThread();
          p->_raw_cpuid = cpu_ctrl->GetCpuId().GetRawId();

          p->_mem_ctrl->Switch();
          process_ctrl->SetStatus(p, ProcessStatus::kRunning);

          p->_elfobj->Resume();
        }
      },
      process)));

  return process;
}

sptr<Process> ProcessCtrl::ProcessTable::Init() {
  sptr<Process> p = make_sptr(new Process());
  p->_status = ProcessStatus::kEmbryo;
  p->_pid = _next_pid++;
  p->_next = p->_prev = p;

  _current_process = p;

  return p;
}

sptr<Process> ProcessCtrl::ProcessTable::AllocProcess() {
  sptr<Process> p = make_sptr(new Process());
  p->_status = ProcessStatus::kEmbryo;
  p->_pid = _next_pid++;

  sptr<Process> cp = _current_process;

  p->_next = cp->_next;
  p->_prev = cp;
  cp->_next = p;
  cp->_next->_prev = p;

  return p;
}

void ProcessCtrl::ProcessTable::FreeProcess(sptr<Process> p) {
  p->_prev = p->_next;
  p->_next->_prev = p->_prev;
}

// TODO: impl (now, super simple implemetation for debug)
sptr<Process> ProcessCtrl::ProcessTable::GetNextProcess() {
  while (true) {
    sptr<Process> res = _current_process->_next;
    _current_process = res;
    // DBG
    if (res->GetStatus() == ProcessStatus::kZombie) continue;
    return res;
  }
  sptr<Process> n;
  return n;
}
