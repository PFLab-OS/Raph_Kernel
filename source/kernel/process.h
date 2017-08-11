
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

//TODO
//管理方法をリンクリストにする

#pragma once

#include <ctype.h>
#include <task.h>
#include <gdt.h>
#include <mem/paging.h>
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
  Process() {
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

  //TODO:PIDは本当は他人からいじらせたくない
  //今は配列管理だからできないが、リンクリスト形式で一つのオブジェクトが一つのプロセスしか表さないことにして
  //Constメンバにする
  //これはtaskも同様
  pid_t  pid;

  struct Context {
    uint64_t rdi,rsi,rbp,rbx,rdx,rcx,rax;
    uint64_t r8,r9,r10,r11,r12,r13,r14,r15;
    uint64_t rflags = 0x200;

    uint64_t gs =USER_DS;
    const uint64_t fs = USER_DS;
    const uint64_t es = USER_DS;
    const uint64_t ds = USER_DS;
    
    uint64_t rip;
    const uint64_t cs = USER_CS;

    uint64_t rsp;
    const uint64_t ss = USER_DS;
  } context;


  class ProcmemCtrl {
  private:
    PageTable* GetPml4tAddr() {
      //TODO:メモリリークしてるので修正
      virt_addr t = virtmem_ctrl->Alloc(PagingCtrl::kPageSize*2);
      return reinterpret_cast<PageTable*>((reinterpret_cast<uint64_t>(t) + PagingCtrl::kPageSize) & ~(PagingCtrl::kPageSize - 1));
    }
    bool Map4KPageToVirtAddr(virt_addr vaddr, PhysAddr &paddr, phys_addr pst_flag, phys_addr page_flag);

    SpinLock _lock;

  public:
    ProcmemCtrl() :pml4t(GetPml4tAddr()) {
    }

    void Init();
    bool AllocUserSpace(virt_addr addr,size_t size);


    PageTable* const pml4t;

  } procmem_ctrl;

  static void  Resume(Process*);
  static void ReturnToKernelJob(Process*);
  static void Exit(Process*);
  uint64_t* saved_rsp = nullptr;

private:
  Process* parent;
  sptr<TaskWithStack> task;
  ProcessStatus status = ProcessStatus::EMBRYO;
  Process* next;
  Process* prev;
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
