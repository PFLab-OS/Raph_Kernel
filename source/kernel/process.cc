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
  _task = make_sptr(new TaskWithStack(cpu_ctrl->GetCpuId()));
  _task->Init();
  _pmem.Init();
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

    Process* p = process_ctrl->GetNextExecProcess();
    if (p != nullptr) {
      task_ctrl->Register(cpu_ctrl->GetCpuId(),p->GetKernelJob());
    }

    task_ctrl->RegisterCallout(lcallout,cpu_ctrl->GetCpuId(),90);

  },lcallout_)));

  task_ctrl->RegisterCallout(lcallout_,cpu_ctrl->GetCpuId(),90);

}

//This function is scheduler.
Process* ProcessCtrl::GetNextExecProcess() {
  {
    Process* cp = current_exec_process;
    Process* p = current_exec_process;
    Locker locker(table_lock);
    do {
      p = p->_next;
      if (p->GetStatus() == ProcessStatus::RUNNABLE || p->GetStatus() == ProcessStatus::EMBRYO) {
        current_exec_process = p;
        return current_exec_process;
      }
    } while (p != cp);
  }
  return nullptr;
}

//This fucntion retrun Forked Executable Process 
Process* ProcessCtrl::ForkProcess(Process* fp) { 
  Process* process = process_table.AllocProcess();
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc process space.");
  }
  process->Init();
  process->_parent = fp;

  process->SetKernelJobFunc(make_uptr(new Function3<sptr<TaskWithStack>,Process*,Process*>
          ([](sptr<TaskWithStack> task,Process* p,Process* _parent) {
    paging_ctrl->SetMemSpace(&(_parent->_pmem));

    Loader loader;
    ElfObject* obj = new ElfObject(loader, _parent->_elfobj);
    p->_elfobj = obj;

    MemSpace::CopyMemSapce(&(p->_pmem),&(_parent->_pmem));
    Loader::CopyContext(p->_elfobj->GetContext(), _parent->_elfobj->GetContext());
    p->_elfobj->GetContext()->rax = 0; //fork syscall ret addr 

    process_ctrl->SetStatus(_parent,ProcessStatus::RUNNABLE);

    paging_ctrl->SetMemSpace(&(p->_pmem));

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

      paging_ctrl->SetMemSpace(&(p->_pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      p->_elfobj->Resume();

    }
  },process->GetKernelJob(), process, fp)));

  return process;
}

Process* ProcessCtrl::ExecProcess(Process* process,const char* _path) { 
  kassert(process);
  
  process->Init();
  process_ctrl->SetStatus(process,ProcessStatus::EMBRYO);

  process->SetKernelJobFunc(make_uptr(new Function3<sptr<TaskWithStack>,Process*,const char*>
          ([](sptr<TaskWithStack> task,Process* p,const char* path) {
    gtty->Printf("Foekd Process Execution\n");
    paging_ctrl->SetMemSpace(&(p->_pmem));


    Loader loader;
    char *pptr = reinterpret_cast<char*>(multiboot_ctrl->LoadFile(path)->GetRawPtr());
    process_ctrl->SetStatus(p,ProcessStatus::EMBRYO);
    ElfObject* obj = new ElfObject(loader, pptr);
    if (obj->Init() != BinObjectInterface::ErrorState::kOk) {
      gtty->Printf("error while loading app\n");
      kernel_panic("ProcessCtrl","Could not load app in ExecProcess()");
    }

    p->_elfobj = obj;

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

      paging_ctrl->SetMemSpace(&(p->_pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      p->_elfobj->Resume();
    }

  },process->GetKernelJob(), process, _path)));

  return process;
}

//For init
Process* ProcessCtrl::CreateFirstProcess(Process* process) {
  if(process == nullptr) {
    kernel_panic("ProcessCtrl","Could not alloc first process space.");
  }
  process->Init();

  process->SetKernelJobFunc(make_uptr(new Function2<sptr<TaskWithStack>,Process*>([](sptr<TaskWithStack> task,Process* p) {

    paging_ctrl->SetMemSpace(&(p->_pmem));


    Loader loader;
    ElfObject* obj = new ElfObject(loader, multiboot_ctrl->LoadFile("init.elf")->GetRawPtr());
    if (obj->Init() != BinObjectInterface::ErrorState::kOk) {
      gtty->Printf("error while loading app\n");
      kernel_panic("ProcessCtrl","Could not load app in ExecProcess()");
    }    

    p->_elfobj = obj;

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

      paging_ctrl->SetMemSpace(&(p->_pmem));
      process_ctrl->SetStatus(p,ProcessStatus::RUNNING);

      p->_elfobj->Resume();

    }

  },process->GetKernelJob(),process)));

  return process;
}

Process* ProcessCtrl::ProcessTable::Init() {
  Process* p = new Process();
  p->_status = ProcessStatus::EMBRYO;
  p->_pid = next_pid++;
  p->_next = p->_prev = p;

  current_process = p;

  return p;

}

Process* ProcessCtrl::ProcessTable::AllocProcess() { 
  Process* p = new Process();
  p->_status = ProcessStatus::EMBRYO;
  p->_pid = next_pid++;

  Process* cp = current_process;

  p->_next = cp->_next;
  p->_prev = cp;
  cp->_next = p;
  cp->_next->_prev = p;

  return p;
}

void ProcessCtrl::ProcessTable::FreeProcess(Process* p) {
  p->_prev = p->_next;
  p->_next->_prev = p->_prev;
  delete p;
}

//TODO: TBI (now, super simple implemetation for debug) 
Process* ProcessCtrl::ProcessTable::GetNextProcess() {
  while(true) { 
    Process* res = current_process->_next;
    current_process = res;
    //DBG
    if (res->GetStatus() == ProcessStatus::ZOMBIE) continue;
    return res;
  }
  return nullptr;
}



