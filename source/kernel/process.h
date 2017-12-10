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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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


using pid_t = uint32_t;

enum class ProcessStatus {
  EMBRYO,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE,
};

class Process {
public:
  Process() {
  }

  ~Process() {
    delete _elfobj;
  }

  void Init();

  pid_t GetPid() {
    return _pid;
  }

  ProcessStatus GetStatus() {
    return _status;
  };

  static void ReturnToKernelJob(Process* p) {
    p->_elfobj->ReturnToKernelJob();
  }

  static void SetContext(Process* p,Context* context) {
    p->_elfobj->SetContext(context);
  }

  static const int kInvalidPid = 0;
  static const int kInitPid = 1;
private:
  friend class ProcessCtrl;
  //TODO:Implementing class ExecutableObject for super class of ElfObject
  ElfObject* _elfobj;
  Process* _parent;
  pid_t  _pid;
  ProcessStatus _status = ProcessStatus::EMBRYO;
  Process* _next;
  Process* _prev;
  uptr<Thread> _thread;
  void* _chan; //SLEEPING Finish Condition
  sptr<MemCtrl> _mem_ctrl;
  int _raw_cpuid = CpuId::kCpuIdNotFound;
};

class ProcessCtrl {
  public:
    void Init();
    Process* ForkProcess(Process*);
    Process* ExecProcess(Process*,const char*);
    Process* CreateFirstProcess(Process*);
    Process* GetNextExecProcess();
    Process* GetCurrentExecProcess(CpuId cpuid) {
      Process* cp = _current_exec_process;
      Process* p = _current_exec_process;
      do {
        p = p->_next;
        if (p->_raw_cpuid == cpuid.GetRawId() && p->GetStatus() == ProcessStatus::RUNNING) {
          return p;
        }
      } while (p != cp);
      return nullptr;
    }

    void SetStatus(Process* process,ProcessStatus _status) {
      assert(process->_status != ProcessStatus::SLEEPING);
      assert(process->_status != ProcessStatus::ZOMBIE);
      Locker locker(_table_lock);
      process->_status = _status;
    }
    ProcessStatus GetStatus(Process* p) {
      return p->_status;
    }

    bool MakeProcessSleep(Process* process, void* chan) {
      assert(process->_status != ProcessStatus::SLEEPING);
      assert(process->_status != ProcessStatus::ZOMBIE);
      Locker locker(_table_lock);
      process->_chan = chan;
      process->_status = ProcessStatus::SLEEPING;
      return true;
    }

    void WakeupProcessSub(void* chan) {
      Process* cp = _current_exec_process;
      Process* p = _current_exec_process;
      do {
        p = p->_next;
        if (p->GetStatus() == ProcessStatus::SLEEPING && p->_chan == chan) {
          p->_status = ProcessStatus::RUNNABLE;
        }
      } while (p != cp);
    }

    void WakeupProcess(void *chan) {
      Locker locker(_table_lock);
      WakeupProcessSub(chan);
    }

    //TODO:impl
    void ExitProcess(Process* process) {
      //Close Files
      WakeupProcess(process->_parent);
      Process* init = FindProcessFromPid(Process::kInitPid);

      {
        Process* cp = _current_exec_process;
        Process* p = _current_exec_process;
        Locker locker(_table_lock);
        p->_status = ProcessStatus::ZOMBIE;
        do {
          p = p->_next;
          if (p->_parent == process) {
            p->_parent = init;
          }
        } while (p != cp);
        WakeupProcessSub(init);
      }
      delete process->_elfobj;
      //TODO:paging memory release

      //Now, we are in the exit process's thread.
      ThreadCtrl::WaitCurrentThread();
    }

    pid_t WaitProcess(Process* process) {
      Process* cp = _current_exec_process;
      Process* p = _current_exec_process;
      Locker locker(_table_lock);
      do {
        p = p->_next;
        if (p->_parent == process && p->_status == ProcessStatus::ZOMBIE) {
          pid_t pid = p->GetPid();
          _process_table.FreeProcess(p);
          return pid;
        }
      } while (p != cp);
      return Process::kInvalidPid;
    }  

    Process* FindProcessFromPid(pid_t pid) {
      Process* cp = _current_exec_process;
      Process* p = _current_exec_process;
      Locker locker(_table_lock);
      do {
        p = p->_next;
        if (p->GetPid() == pid) {
          return p;
        }
      } while (p != cp);
      return nullptr;
    }

  private:
    Process* _current_exec_process = nullptr;
    SpinLock _table_lock;
    uptr<Thread> _scheduler_thread;

    class ProcessTable {
    public:
      Process* Init();
      Process* AllocProcess();
      void FreeProcess(Process*);

      Process* GetNextProcess();
      Process* GetNextProcess(Process*);

    private:
      const static int _max_process = 0x10;
      pid_t _next_pid = 1;
      Process* _current_process = nullptr;
    } _process_table;

};
