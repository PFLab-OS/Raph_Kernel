
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


typedef uint32_t pid_t;



enum class ProcessStatus {
  EMBRYO,
  SLEEPING, 
  RUNNABLE, 
  RUNNING, 
  ZOMBIE, 
  UNUSED, //配列管理のため
};

class Process {
public:
  Process() {
  }

  void Init();


  pid_t GetPid() {
    return pid;
  }

  void SetPid(pid_t _pid) {
    pid = _pid;
  }

  //命名　用途などあとで考える
  //TODO：カーネルジョブキューに入っていないかチェックする
  sptr<TaskWithStack> GetKernelJob() {
    return task;
  } 

  void SetKernelJobFunc(uptr<GenericFunction<>> func) {
    task->SetFunc(func);
  }

  ProcessStatus status = ProcessStatus::EMBRYO;
  //TODO:PIDは本当は他人からいじらせたくない
  //今は配列管理だからできないが、リンクリスト形式で一つのオブジェクトが一つのプロセスしか表さないことにして
  //Constメンバにする
  //これはtaskも同様
  pid_t  pid;

  struct Context {
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rflags = 0x200;

    uint64_t gs =USER_DS;
    uint64_t fs = USER_DS;
    uint64_t es = USER_DS;
    const uint64_t ds = USER_DS;
    
    uint64_t rip;
    uint64_t cs = USER_CS;

    uint64_t rsp;
    uint64_t ss = USER_DS;
  } context;

  static void  Resume(Process*);
  static void ReturnToKernelJob(Process*);
  static void Exit(Process*);
  uint64_t* saved_rsp = nullptr;
  //PageTable* pml4t;
  //AA *asa;

private:
  Process* parent;
  sptr<TaskWithStack> task;
};



class ProcessCtrl {
  public:
    void Init();
    Process* CreateProcess();
    Process* CreateFirstProcess();
    Process* GetNextExecProcess();

    Process* GetCurrentProcess();

    void HaltProcess(Process*);

  private:
    Process* current_process = nullptr;
    SpinLock table_lock;

    //リンクリストで実装する
    class ProcessTable { 
    public:
      void Init();
      Process* AllocProcess();

      Process* GetNextProcess();

    private:
      const static int max_process = 0x1000;
      Process processes[max_process];
      pid_t next_pid = 1;
      int current_process = 0;


    } process_table;

};
