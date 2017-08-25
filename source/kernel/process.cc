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


#include <elf.h>
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
  task = make_sptr(new TaskWithStack(cpu_ctrl->GetCpuId()));
  task->Init();
  paging_ctrl->InitMemSpace(&pmem);
}


void Process::ReturnToKernelJob(Process* p) {
  p->elfobj->ReturnToKernelJob();
}

void ProcessCtrl::Init() {
  {
    Locker locker(table_lock);
    Process* p = process_table.Init();
    CreateFirstProcess(p);
    current_exec_process = p;
  }

  auto lcallout_ = make_sptr(new Callout);
  lcallout_->Init(make_uptr(new Function<sptr<Callout>>([](sptr<Callout> lcallout) {

    //TODO:デバッグ用簡易実装
    Process* p = process_ctrl->GetNextExecProcess();
    if(p != nullptr && p->GetStatus() == ProcessStatus::RUNNABLE) {
      //処理したいプロセスが存在すれば、それを追加
      task_ctrl->Register(cpu_ctrl->GetCpuId(),p->GetKernelJob());
      //task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
    } 

    //誰がディスパッチ用のタスク（これ）をカーネルジョブに追加する？
    //プリエンプティブ・ノンプリエンプティブで異なるため要検討
    if(p != nullptr && p->GetStatus() == ProcessStatus::EMBRYO){
      task_ctrl->Register(cpu_ctrl->GetCpuId(),p->GetKernelJob());
    }
//    if(p != nullptr && p->GetStatus() == ProcessStatus::HALTED){
//      task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
//    }
//    if(p != nullptr && p->GetStatus() == ProcessStatus::SLEEPING){
//      task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask);
//    }
    task_ctrl->RegisterCallout(lcallout,cpu_ctrl->GetCpuId(),30);

  },lcallout_)));

  task_ctrl->RegisterCallout(lcallout_,cpu_ctrl->GetCpuId(),30);
  //task_ctrl->Register(cpu_ctrl->GetCpuId(),ltask_);

}

//This function is scheduler.
Process* ProcessCtrl::GetNextExecProcess() {
  {
    Process* cp = current_exec_process;
    Process* p = current_exec_process;
    Locker locker(table_lock);
    do {
      p = p->next;
      if (p->GetStatus() == ProcessStatus::RUNNABLE || p->GetStatus() == ProcessStatus::EMBRYO) {
        current_exec_process = p;
        return current_exec_process;
      }
    } while (p != cp);
  }
  return nullptr;
}

//forkして実行可能になったプロセスを返す
Process* ProcessCtrl::ForkProcess(Process* _parent) { 
  Process* process = process_table.AllocProcess();
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc process space.");
  }
  process->Init();
  process->parent = _parent;

  //TODO:設計考える
  process->SetKernelJobFunc(make_uptr(new Function3<sptr<TaskWithStack>,Process*,Process*>
          ([](sptr<TaskWithStack> task,Process* p,Process* parent) {
    //この方式だと，以下の処理が走るタイミングが制御できない
    //するとFork元のプロセスのメモリ空間やレジスタが変わってしまうため
    paging_ctrl->SetMemSpace(&(parent->pmem));

    //TODO:メモリ開放
    Loader loader;
    ElfObject* obj = new ElfObject(loader, parent->elfobj);

    p->elfobj = obj;

    MemSpace::CopyMemSapce(&(p->pmem),&(parent->pmem)); //TODO:時間かかる
    //これはコンストラクタでできるのでは？
    Loader::CopyContext(p->elfobj->GetContext(), parent->elfobj->GetContext());
    p->elfobj->GetContext()->rax = 0; //fork syscall ret addr 

    process_ctrl->SetStatus(parent,ProcessStatus::RUNNABLE);

    paging_ctrl->SetMemSpace(&(p->pmem));

    while(true) {
      paging_ctrl->ReleaseMemSpace();
      switch (p->GetStatus()) {
        case ProcessStatus::EMBRYO:
        case ProcessStatus::RUNNING:
          process_ctrl->SetStatus(p,ProcessStatus::RUNNABLE);
          break;
        default:
          break;
      }

      task->Wait(0);

      paging_ctrl->SetMemSpace(&(p->pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      p->elfobj->Resume();

    }
  },process->GetKernelJob(), process, _parent)));

  //task_ctrl->Register(cpu_ctrl->GetCpuId(),process->GetKernelJob());

  return process;
}

Process* ProcessCtrl::ExecProcess(Process* process,void* _ptr) { 
  kassert(process);
  //TODO:ホントにinitしていいの？
  process->Init();
  process_ctrl->SetStatus(process,ProcessStatus::EMBRYO);

  //paging_ctrl->InitMemSpace(&(process->pmem));

  gtty->Printf("%p\n",_ptr);
  gtty->Flush();

  process->SetKernelJobFunc(make_uptr(new Function3<sptr<TaskWithStack>,Process*,void*>
          ([](sptr<TaskWithStack> task,Process* p,void* ptr) {
    gtty->Printf("Foekd Process Execution\n");
    //paging_ctrl->SetMemSpace(&(p->pmem));
    paging_ctrl->ReleaseMemSpace();


    //TODO:前のLoaderってメモリリークしてない?
    //TODO:ptrが指しているアドレスちがう？
    Loader loader;
    char *pptr = reinterpret_cast<char*>(ptr);
    gtty->Printf("%x %x %x\n", pptr[0],pptr[1],pptr[2]);
    gtty->Printf("%p\n",ptr);
    gtty->Flush();
    while(1) {asm("hlt");}
    process_ctrl->SetStatus(p,ProcessStatus::EMBRYO);
    ElfObject* obj = new ElfObject(loader, ptr);
    if (obj->Init() != BinObjectInterface::ErrorState::kOk) {
      gtty->Printf("error while loading app\n");
    }

    p->elfobj = obj;

    while(true) {
      paging_ctrl->ReleaseMemSpace();
      switch (p->GetStatus()) {
        case ProcessStatus::EMBRYO:
        case ProcessStatus::RUNNING:
          process_ctrl->SetStatus(p,ProcessStatus::RUNNABLE);
          break;
        default:
          break;
      }

      task->Wait(0);

      paging_ctrl->SetMemSpace(&(p->pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      //gtty->Printf("execed process run \n");
      //gtty->Flush();
      p->elfobj->Resume();

    }

  },process->GetKernelJob(), process, _ptr)));

  //task_ctrl->Register(cpu_ctrl->GetCpuId(),process->GetKernelJob());

  return process;
}

//For init
Process* ProcessCtrl::CreateFirstProcess(Process* process) {
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc first process space.");
  }
  process->Init();

  process->SetKernelJobFunc(make_uptr(new Function2<sptr<TaskWithStack>,Process*>([](sptr<TaskWithStack> task,Process* p) {

    paging_ctrl->SetMemSpace(&(p->pmem));


    //TODO:すべての親になるプロセスとして test.elf は不適切
    Loader loader;
    ElfObject* obj = new ElfObject(loader, multiboot_ctrl->LoadFile("test.elf")->GetRawPtr());
    if (obj->Init() != BinObjectInterface::ErrorState::kOk) {
      gtty->Printf("error while loading app\n");
    }    

    p->elfobj = obj;

    while(true) {
      paging_ctrl->ReleaseMemSpace();
      //TODO:ここが正しいか？　正しいなら他の箇所にも移す
      switch (p->GetStatus()) {
        case ProcessStatus::EMBRYO:
        case ProcessStatus::RUNNING:
          process_ctrl->SetStatus(p,ProcessStatus::RUNNABLE);
          break;
        default:
          break;
      }

      task->Wait(0);

      paging_ctrl->SetMemSpace(&(p->pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      p->elfobj->Resume();

    }

  },process->GetKernelJob(),process)));

  //task_ctrl->Register(cpu_ctrl->GetCpuId(),process->GetKernelJob());

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
  p->prev = cp;
  cp->next = p;
  cp->next->prev = p;

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
    //DBG
    if (res->GetStatus() == ProcessStatus::ZOMBIE) continue;
    return res;
  }
  return nullptr;
}



