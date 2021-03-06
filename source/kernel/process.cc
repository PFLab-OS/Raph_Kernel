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

void ProcessCtrl::Init() {
  {
    Locker locker(_table_lock);
    sptr<Process> p = _process_set.Init();
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

  process->_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                             CpuPurpose::kLowPriority))
                         .AllocNewThread(Thread::StackState::kIndependent);
  process->_mem_ctrl = make_sptr(new MemCtrl());
  process->_mem_ctrl->Init();

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

// This fucntion retrun Forked Executable Process
sptr<Process> ProcessCtrl::ForkProcess(sptr<Process> fp) {
  sptr<Process> process = _process_set.AllocProcess();
  if (process.IsNull()) {
    kernel_panic("ProcessCtrl", "Could not alloc first process space.");
  }

  process->_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                             CpuPurpose::kLowPriority))
                         .AllocNewThread(Thread::StackState::kIndependent);

  process->_parent = fp;

  process->_mem_ctrl = make_sptr(new MemCtrl(fp->_mem_ctrl));
  process->_mem_ctrl->Init();

  auto t_op = process->_thread->CreateOperator();
  t_op.SetFunc(make_uptr(new Function2<void, sptr<Process>, sptr<Process>>(
      [](sptr<Process> p, sptr<Process> parentp) {
        p->_mem_ctrl->Switch();

        Loader loader(p->_mem_ctrl);
        ElfObject* obj = new ElfObject(loader, parentp->_elfobj);
        p->_elfobj = obj;

        Loader::CopyContext(p->_elfobj->GetContext(),
                            parentp->_elfobj->GetContext());
        p->_elfobj->GetContext()->rax = 0;  // fork syscall ret val

        process_ctrl->SetStatus(parentp, ProcessStatus::kRunnable);

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
      process, fp)));

  return process;
}

// This fucntion retrun Forked Executable Process
sptr<Process> ProcessCtrl::ExecProcess(sptr<Process> process,
                                       const char* exec_path) {
  if (process.IsNull()) {
    kernel_panic("ProcessCtrl", "invalid process");
  }

  process->_mem_ctrl = make_sptr(new MemCtrl());
  process->_mem_ctrl->Init();

  process_ctrl->SetStatus(process, ProcessStatus::kHalted);

  process->_thread_sub = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                 CpuPurpose::kLowPriority))
                             .AllocNewThread(Thread::StackState::kIndependent);
  auto t_op = process->_thread_sub->CreateOperator();
  t_op.SetFunc(make_uptr(
      new Function2<void, sptr<Process>,
                    uptr<Function2<void, sptr<Process>, const char*>>>(
          [](sptr<Process> pp,
             uptr<Function2<void, sptr<Process>, const char*>> func) {
            auto tmp_thread = pp->_thread;
            tmp_thread->CreateOperator().Stop();

            pp->_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                  CpuPurpose::kLowPriority))
                              .AllocNewThread(Thread::StackState::kIndependent);
            auto pt_op = pp->_thread->CreateOperator();
            pt_op.SetFunc(func);

            process_ctrl->SetStatus(pp, ProcessStatus::kEmbryo);

            ThreadCtrl::WaitCurrentThread();
          },
          process,
          make_uptr(new Function2<void, sptr<Process>, const char*>(
              [](sptr<Process> p, const char* path) {
                gtty->Printf("Foekd Process Execution\n");
                p->_mem_ctrl->Init();

                p->_mem_ctrl->Switch();

                Loader loader(p->_mem_ctrl);
                // TODO change test.elf to optimal process (such as init).
                auto tmp = multiboot_ctrl->LoadFile(path);
                ElfObject obj(loader, tmp->GetRawPtr());
                if (obj.Init() != BinObjectInterface::ErrorState::kOk) {
                  gtty->Printf("error while loading app\n");
                  kernel_panic("ProcessCtrl",
                               "Could not load app in ExecProcess()");
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
              process, exec_path)))));

  t_op.Schedule();

  return process;
}

sptr<Process> ProcessCtrl::ProcessSet::Init() {
  sptr<Process> p = make_sptr(new Process());
  p->_status = ProcessStatus::kEmbryo;
  p->_pid = _next_pid++;
  p->_next = p->_prev = p;

  _current_process = p;

  _set.Push(p);

  return p;
}

sptr<Process> ProcessCtrl::ProcessSet::AllocProcess() {
  sptr<Process> p = make_sptr(new Process());
  p->_status = ProcessStatus::kEmbryo;
  p->_pid = _next_pid++;

  sptr<Process> cp = _current_process;

  p->_next = cp->_next;
  p->_prev = cp;
  cp->_next = p;
  cp->_next->_prev = p;

  _set.Push(p);

  return p;
}

void ProcessCtrl::ProcessSet::FreeProcess(sptr<Process> p) {
  p->_prev = p->_next;
  p->_next->_prev = p->_prev;
}

// TODO: impl (now, super simple implemetation for debug)
sptr<Process> ProcessCtrl::ProcessSet::GetNextProcess() {
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
