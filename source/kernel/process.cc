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

//TODO:プロセスが死んだ後処理
//TODO:pml4tを変更処理
//以前でprocmem_ctrlすでに一部作っていたので，残しています
//問題があるようでしたら消して必要なコードをProcessクラス内に書きます


#include <elfhead.h>
#include <process.h>
#include <task.h>
#include <tty.h>
#include <multiboot.h>
#include <syscall.h>
#include <gdt.h>
#include <stdlib.h>
#include <mem/paging.h>
#include <mem/physmem.h>
#include <global.h>

void Process::Init() {
  task->Init();
  paging_ctrl->InitProcmem(&pmem);
}


void Process::ReturnToKernelJob(Process* p) {
  gtty->Printf("called ReturnToKernelJob()\n");
  gtty->Flush();

  p->elfobj->ReturnToKernelJob();
}

void Process::Exit(Process* p) {
  process_ctrl->HaltProcess(p);
}

void ProcessCtrl::Init() {
  {
    Locker locker(table_lock);
    Process* p = process_table.Init();
    CreateFirstProcess(p);
  }

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function<sptr<Task>>([](sptr<Task> ltask) {

    //TODO:デバッグ用簡易実装
    Process* p = process_ctrl->GetNextExecProcess();
    if(p != nullptr && p->GetStatus() == ProcessStatus::RUNNABLE) {
      //処理したいプロセスが存在すれば、それを追加
      task_ctrl->Register(cpu_ctrl->GetCpuId(),p->GetKernelJob());
      task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
    } 

    //誰がディスパッチ用のタスク（これ）をカーネルジョブに追加する？
    //プリエンプティブ・ノンプリエンプティブで異なるため要検討
    if(p != nullptr && p->GetStatus() ==ProcessStatus::EMBRYO){
      task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
    }

  },ltask_)));

  task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask_);

}

//TODO:ここでスケジューリングをする
Process* ProcessCtrl::GetNextExecProcess() {

  {
    Locker locker(table_lock);
    current_exec_process = process_table.GetNextProcess();
  }
  return current_exec_process;
}

//TODO:forkして実行可能になったプロセスを返す
Process* ProcessCtrl::CreateProcess() { 
  return nullptr;
  Process* process = process_table.AllocProcess();
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc process space.");
  }
  process->Init();

  process->SetKernelJobFunc(make_uptr(new Function2<sptr<TaskWithStack>,Process*>([](sptr<TaskWithStack> task,Process* p) {
    
    while(true) {
      //task->Wait(0);

      gtty->Printf("Context Switch\n");
      gtty->Flush();

    }
  },process->GetKernelJob(),process)));

  return process;
}

//プロセスはすべてfork/execで生成することを前提に設計していたが，
//もし，それ以外の方法で生成するつもりならばこの関数の名前を適切なものに変えます．
//すべての親となるプロセスを生成する
Process* ProcessCtrl::CreateFirstProcess(Process* process) {
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc first process space.");
  }
  process->Init();

  process->SetKernelJobFunc(make_uptr(new Function2<sptr<TaskWithStack>,Process*>([](sptr<TaskWithStack> task,Process* p) {

    paging_ctrl->SetProcmem(&(p->pmem));


    //TODO:すべての親になるプロセスとして test.elf は不適切
    Loader loader;
    ElfObject* obj = new ElfObject(loader, multiboot_ctrl->LoadFile("test.elf")->GetRawPtr());
    if (obj->Init() != BinObjectInterface::ErrorState::kOk) {
      gtty->Printf("error while loading app\n");
    }    

    p->elfobj = obj;

    while(true) {
      paging_ctrl->ReleaseProcmem();
      process_ctrl->SetStatus(p,ProcessStatus::RUNNABLE);

      gtty->Printf("Wait\n");
      gtty->Flush();

      task->Wait(0);

      paging_ctrl->SetProcmem(&(p->pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      obj->Resume();

    }

  },process->GetKernelJob(),process)));

  task_ctrl->Register(cpu_ctrl->GetCpuId(),process->GetKernelJob());

  return process;
}

Process* ProcessCtrl::ProcessTable::Init() {
  Process* p = new Process();
  p->status = ProcessStatus::EMBRYO;
  p->pid = next_pid++;
  p->next = p->prev = p;

  current_process = p;

  return p;

}

Process* ProcessCtrl::ProcessTable::AllocProcess() { 
  Process* p = new Process();
  p->status = ProcessStatus::EMBRYO;
  p->pid = next_pid++;

  Process* cp = current_process;

  p->next = cp->next;
  p->prev = current_process;
  cp->next = p;
  p->prev = p;

  return p;
}

void ProcessCtrl::ProcessTable::FreeProcess(Process* p) {
  p->prev = p->next;
  p->next->prev = p->prev;
  delete p;
}

//TODO:デバッグ用簡易実装
//どんな実装にするのか考える
Process* ProcessCtrl::ProcessTable::GetNextProcess() {
  while(true) { 
    Process* res = current_process->next;
    current_process = res;
    return res;
  }
  return nullptr;
}


//TODO:デバッグ用簡易実装
//deleteとかなんとか
void ProcessCtrl::HaltProcess(Process* p) {
  p->GetKernelJob()->Wait(0);
}

