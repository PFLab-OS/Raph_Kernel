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


typedef uint32_t pid_t;
#define INVALID_PID 0
#define INIT_PID  1

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
    delete elfobj;
  }

  void Init();

  pid_t GetPid() {
    return pid;
  }

  ProcessStatus GetStatus() {
    return status;
  };

  sptr<TaskWithStack> GetKernelJob() {
    return task;
  } 

  void SetKernelJobFunc(uptr<GenericFunction<>> func) {
    task->SetFunc(func);
  }

  static void ReturnToKernelJob(Process*);
  //static void Exit(Process*);
  uint64_t* saved_rsp = nullptr;
  //TODO:ElfObjectのスーパークラスとしてclass ExecutableObjectをつくりたい(将来サポートする実行形式を増やすため）
  ElfObject* elfobj;
private:
  Process* parent;
  pid_t  pid;
  ProcessStatus status = ProcessStatus::EMBRYO;
  Process* next;
  Process* prev;
  sptr<TaskWithStack> task;
  MemSpace pmem;
  void* chan; //SLEEPING 終了条件
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
      assert(process->status != ProcessStatus::SLEEPING);
      assert(process->status != ProcessStatus::ZOMBIE);
      Locker locker(table_lock);
      process->status = _status;
    }
    ProcessStatus GetStatus(Process* p) {
      return p->status;
    }

    bool MakeProcessSleep(Process* process, void* chan) {
      assert(process->status != ProcessStatus::SLEEPING);
      assert(process->status != ProcessStatus::ZOMBIE);
      Locker locker(table_lock);
      process->chan = chan;
      process->status = ProcessStatus::SLEEPING;
      return true;
    }

    void WakeupProcess(void* chan) {
      Process* cp = current_exec_process;
      Process* p = current_exec_process;
      Locker locker(table_lock);
      do {
        p = p->next;
        if (p->GetStatus() == ProcessStatus::SLEEPING && p->chan == chan) {
          p->status = ProcessStatus::RUNNABLE;
        }
      } while (p != cp);
    }

    //TODO:工事中
    void ExitProcess(Process* process) {
      //Close Files 
      WakeupProcess(process->parent);
      Process* init = FindProcessFromPid(INIT_PID);

      {
        Process* cp = current_exec_process;
        Process* p = current_exec_process;
        Locker locker(table_lock);
        p->status = ProcessStatus::ZOMBIE;
        do {
          p = p->next;
          if (p->parent == process) {
            p->parent = init;
          }
        } while (p != cp);
        WakeupProcess(init);
      }
      delete process->elfobj;
      //TODO:paging memory release 

      process->GetKernelJob()->Wait(0);
    }

    pid_t WaitProcess(Process* process) {
      Process* cp = current_exec_process;
      Process* p = current_exec_process;
      Locker locker(table_lock);
      do {
        p = p->next;
        if (p->parent == process && p->status == ProcessStatus::ZOMBIE) {
          pid_t pid = p->GetPid();
          process_table.FreeProcess(p);
          return pid;
        }
      } while (p != cp);
      return INVALID_PID;
    }  

    Process* FindProcessFromPid(pid_t pid) {
      Process* cp = current_exec_process;
      Process* p = current_exec_process;
      Locker locker(table_lock);
      do {
        p = p->next;
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
