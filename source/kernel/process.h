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
#include <task.h>
#include <gdt.h>
#include <mem/paging.h>
#include <elf.h>
#include <global.h>


using pid_t = uint32_t;

enum class ProcessStatus {
  EMBRYO,
  SLEEPING, 
  RUNNABLE, 
  RUNNING, 
  ZOMBIE, 
  HALTED, //xv6には無い．Fork等でプロセスの実行を一時的に止めたい気持ちがあるとき
};

class Process {
  friend class ProcessCtrl;
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

  sptr<TaskWithStack> GetKernelJob() {
    return _task;
  } 

  void SetKernelJobFunc(uptr<GenericFunction<>> func) {
    _task->SetFunc(func);
  }

  static void ReturnToKernelJob(Process* p) {
    p->_elfobj->ReturnToKernelJob();
  }

  static void SetContext(Process* p,Context* context) {
    p->_elfobj->SetContext(context);
  }

  static const int kInvalidPid = 0;
  static const int kInitPid = 1;
private:
  //TODO:Implementing class ExecutableObject for super class of ElfObject
  ElfObject* _elfobj;
  Process* _parent;
  pid_t  _pid;
  ProcessStatus _status = ProcessStatus::EMBRYO;
  Process* _next;
  Process* _prev;
  sptr<TaskWithStack> _task;
  MemSpace _pmem;
  void* _chan; //SLEEPING Finish Condition
};

class ProcessCtrl {
  public:
    void Init();
    Process* ForkProcess(Process*);
    Process* ExecProcess(Process*,const char*);
    Process* CreateFirstProcess(Process*);
    Process* GetNextExecProcess();
    Process* GetCurrentExecProcess() {
      return current_exec_process;
    }

    void SetStatus(Process* process,ProcessStatus _status) {
      assert(process->_status != ProcessStatus::SLEEPING);
      assert(process->_status != ProcessStatus::ZOMBIE);
      Locker locker(table_lock);
      process->_status = _status;
    }
    ProcessStatus GetStatus(Process* p) {
      return p->_status;
    }

    bool MakeProcessSleep(Process* process, void* chan) {
      assert(process->_status != ProcessStatus::SLEEPING);
      assert(process->_status != ProcessStatus::ZOMBIE);
      Locker locker(table_lock);
      process->_chan = chan;
      process->_status = ProcessStatus::SLEEPING;
      return true;
    }

    void WakeupProcess(void* chan) {
      Process* cp = current_exec_process;
      Process* p = current_exec_process;
      Locker locker(table_lock);
      do {
        p = p->_next;
        if (p->GetStatus() == ProcessStatus::SLEEPING && p->_chan == chan) {
          p->_status = ProcessStatus::RUNNABLE;
        }
      } while (p != cp);
    }

    //TODO:TBI
    void ExitProcess(Process* process) {
      //Close Files 
      WakeupProcess(process->_parent);
      Process* init = FindProcessFromPid(Process::kInitPid);

      {
        Process* cp = current_exec_process;
        Process* p = current_exec_process;
        Locker locker(table_lock);
        p->_status = ProcessStatus::ZOMBIE;
        do {
          p = p->_next;
          if (p->_parent == process) {
            p->_parent = init;
          }
        } while (p != cp);
        WakeupProcess(init);
      }
      delete process->_elfobj;
      //TODO:paging memory release 

      process->GetKernelJob()->Wait(0);
    }

    pid_t WaitProcess(Process* process) {
      Process* cp = current_exec_process;
      Process* p = current_exec_process;
      Locker locker(table_lock);
      do {
        p = p->_next;
        if (p->_parent == process && p->_status == ProcessStatus::ZOMBIE) {
          pid_t pid = p->GetPid();
          process_table.FreeProcess(p);
          return pid;
        }
      } while (p != cp);
      return Process::kInvalidPid;
    }  

    Process* FindProcessFromPid(pid_t pid) {
      Process* cp = current_exec_process;
      Process* p = current_exec_process;
      Locker locker(table_lock);
      do {
        p = p->_next;
        if (p->GetPid() == pid) {
          return p;
        }
      } while (p != cp);
      return nullptr;
    }

  private:
    Process* current_exec_process = nullptr;
    SpinLock table_lock;

    class ProcessTable { 
    public:
      Process* Init();
      Process* AllocProcess();
      void FreeProcess(Process*);

      Process* GetNextProcess();
      Process* GetNextProcess(Process*);

    private:
      const static int max_process = 0x10;
      pid_t next_pid = 1;
      Process* current_process = nullptr;


    } process_table;

};
