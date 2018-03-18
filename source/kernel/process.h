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

#pragma once

#include <ctype.h>
#include <thread.h>
#include <gdt.h>
#include <mem/virtmem.h>
#include <elf.h>
#include <global.h>
#include <set.h>
#include <function.h>

using pid_t = uint32_t;

enum class ProcessStatus {
  kEmbryo,
  kSleeping,
  kRunning,
  kRunnable,
  kZombie,
};

class Process {
 public:
  Process() {}

  ~Process() { delete _elfobj; }

  void Init();

  pid_t GetPid() { return _pid; }

  ProcessStatus GetStatus() { return _status; };

  static void ReturnToKernelJob(sptr<Process> p) {
    p->_elfobj->ReturnToKernelJob();
  }

  static void SetContext(sptr<Process> p, ContextWrapper& context) {
    p->_elfobj->SetContext(context.GetContext());
  }

  static const int kInvalidPid = 0;
  static const int kInitPid = 1;

 private:
  friend class ProcessCtrl;
  // TODO:Implementing class ExecutableObject for super class of ElfObject
  ElfObject* _elfobj;
  sptr<Process> _parent;
  pid_t _pid;
  ProcessStatus _status = ProcessStatus::kEmbryo;
  sptr<Process> _next, _prev;
  uptr<Thread> _thread;
  sptr<Process> _chan;  // kSleeping Finish Condition
  sptr<MemCtrl> _mem_ctrl;
  int _raw_cpuid = CpuId::kCpuIdNotFound;
};

class ProcessCtrl {
 public:
  void Init();
  sptr<Process> ForkProcess(sptr<Process>);
  sptr<Process> ExecProcess(sptr<Process>, const char*);
  sptr<Process> CreateFirstProcess(sptr<Process>);
  sptr<Process> GetNextExecProcess();
  sptr<Process> GetCurrentExecProcess(CpuId cpuid) {
    sptr<Process> cp = _current_exec_process;
    sptr<Process> p = _current_exec_process;
    do {
      p = p->_next;
      if (p->_raw_cpuid == cpuid.GetRawId() &&
          p->GetStatus() == ProcessStatus::kRunning) {
        return p;
      }
    } while (p != cp);
    sptr<Process> res;
    return res;
  }

  void SetStatus(sptr<Process> process, ProcessStatus _status) {
    assert(process->_status != ProcessStatus::kSleeping);
    assert(process->_status != ProcessStatus::kZombie);
    Locker locker(_table_lock);
    process->_status = _status;
  }
  ProcessStatus GetStatus(sptr<Process> p) { return p->_status; }

  bool MakeProcessSleep(sptr<Process> process, sptr<Process> chan) {
    assert(process->_status != ProcessStatus::kSleeping);
    assert(process->_status != ProcessStatus::kZombie);
    Locker locker(_table_lock);
    process->_chan = chan;
    process->_status = ProcessStatus::kSleeping;
    return true;
  }

  void WakeupProcessSub(sptr<Process> chan) {
    auto f = make_uptr(new Function1<bool, sptr<Process>, sptr<Process>>(
        [](sptr<Process> cchan, sptr<Process> p) {
          return (p->GetStatus() == ProcessStatus::kSleeping &&
                  p->_chan == cchan)
                     ? true
                     : false;
        },
        chan));
    while (true) {
      sptr<Process> pp = _process_set.Pop(f);
      if (pp.IsNull()) break;
      SetStatus(pp, ProcessStatus::kRunnable);
      _process_set.Push(pp);
    }
  }

  void WakeupProcess(sptr<Process> chan) {
    Locker locker(_table_lock);
    WakeupProcessSub(chan);
  }

  // TODO:impl
  void ExitProcess(sptr<Process> process) {
    // Close Files
    WakeupProcess(process->_parent);
    sptr<Process> init = FindProcessFromPid(Process::kInitPid);

    auto f = make_uptr(new Function1<bool, sptr<Process>, sptr<Process>>(
        [](sptr<Process> parent, sptr<Process> p) {
          return (p->_parent == parent) ? true : false;
        },
        process));
    while (true) {
      sptr<Process> pp = _process_set.Pop(f);
      if (pp.IsNull()) break;
      pp->_parent = init;
      _process_set.Push(pp);
    }
    WakeupProcessSub(init);

    delete process->_elfobj;
    // TODO:paging memory release

    // Now, we are in the exit process's thread.
    ThreadCtrl::WaitCurrentThread();
  }

  pid_t WaitProcess(sptr<Process> process) {
    auto f = make_uptr(new Function1<bool, sptr<Process>, sptr<Process>>(
        [](sptr<Process> parent, sptr<Process> p) {
          return (p->_parent == parent && p->_status == ProcessStatus::kZombie)
                     ? true
                     : false;
        },
        process));
    sptr<Process> pp = _process_set.Pop(f);

    if (!pp.IsNull()) {
      pid_t pid = pp->GetPid();
      _process_set.FreeProcess(pp);
      return pid;
    }
    return Process::kInvalidPid;
  }

  sptr<Process> FindProcessFromPid(pid_t pid) {
    auto f = make_uptr(new Function1<bool, pid_t, sptr<Process>>(
        [](pid_t id, sptr<Process> p) {
          return (p->GetPid() == id) ? true : false;
        },
        pid));
    return _process_set.Pop(f);
  }

 private:
  sptr<Process> _current_exec_process;
  SpinLock _table_lock;
  uptr<Thread> _scheduler_thread;

  class ProcessSet {
   public:
    sptr<Process> Init();
    sptr<Process> AllocProcess();
    void FreeProcess(sptr<Process>);

    sptr<Process> Pop() { return _set.Pop(); }
    sptr<Process> Pop(uptr<Function0<bool, sptr<Process>>> func) {
      return _set.Pop(func);
    }
    bool Push(sptr<Process> p) { return _set.Push(p); }

    sptr<Process> GetNextProcess();
    sptr<Process> GetNextProcess(sptr<Process>);

   private:
    pid_t _next_pid = 1;
    sptr<Process> _current_process;
    Set<Process> _set;
  } _process_set;
};
