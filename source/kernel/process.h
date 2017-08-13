
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
class ProcmemCtrl;


enum class ProcessStatus {
  EMBRYO,
  SLEEPING, 
  RUNNABLE, 
  RUNNING, 
  ZOMBIE, 
  UNUSED, //配列管理のため
};

class Process {
  friend class ProcessCtrl;
public:
  Process() : task(make_sptr(new TaskWithStack(cpu_ctrl->GetCpuId()))){
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


  class ProcmemCtrl {
  private:
    virt_addr pt_mem;
    PageTable* GetPml4tAddr() {
      pt_mem = virtmem_ctrl->Alloc(PagingCtrl::kPageSize*2);
      return reinterpret_cast<PageTable*>((reinterpret_cast<uint64_t>(pt_mem) + PagingCtrl::kPageSize) & ~(PagingCtrl::kPageSize - 1));
    }

    SpinLock _lock;

  public:
    ProcmemCtrl() :pml4t(GetPml4tAddr()) {
    }

    ~ProcmemCtrl() {
      virtmem_ctrl->Free(pt_mem);
    }

    void Init();

    PageTable* const pml4t;

  } procmem_ctrl;

  static void ReturnToKernelJob(Process*);
  static void Exit(Process*);
  uint64_t* saved_rsp = nullptr;
  //TODO:ElfObjectのスーパークラスとしてclass ExecutableObjectをつくりたい(将来サポートする実行形式を増やすため）
  ElfObject* elfobj;

private:
  pid_t  pid;
  Process* parent;
  ProcessStatus status = ProcessStatus::EMBRYO;
  Process* next;
  Process* prev;
  sptr<TaskWithStack> task;
};



class ProcessCtrl {
  public:
    void Init();
    Process* CreateProcess();
    Process* CreateFirstProcess(Process*);
    Process* GetNextExecProcess();
    Process* GetCurrentExecProcess() {
      return current_exec_process;
    }


    void SetStatus(Process* p,ProcessStatus _status) {
      Locker locker(table_lock);
      p->status = _status;
    }

    ProcessStatus GetStatus(Process* p) {
      return p->status;
    }

    void HaltProcess(Process*);

  private:
    Process* current_exec_process = nullptr;
    SpinLock table_lock;

    //リンクリストで実装する
    class ProcessTable { 
    public:
      Process* Init();
      Process* AllocProcess();
      void FreeProcess(Process*);

      Process* GetNextProcess();

    private:
      const static int max_process = 0x10;
      //Process* list_head = nullptr;
      //Process processes[max_process];
      pid_t next_pid = 1;
      Process* current_process = nullptr;


    } process_table;

};
