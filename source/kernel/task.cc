/*
 *
 * Copyright (c) 2016 Project Raphine
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
 * Author: Liva
 * 
 */

#include <task.h>

void TaskCtrl::Setup() {
  int cpus = apic_ctrl->GetHowManyCpus();
  _task_struct = reinterpret_cast<TaskStruct *>(virtmem_ctrl->Alloc(sizeof(TaskStruct) * cpus));
  for (int i = 0; i < cpus; i++) {
    new(&_task_struct[i]) TaskStruct;

    Task *t = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));
    new(&t->func) Function;
    t->next = nullptr;

    _task_struct[i].top = t;
    _task_struct[i].bottom = t;

    t = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));
    new(&t->func) Function;
    t->next = nullptr;

    _task_struct[i].top_sub = t;
    _task_struct[i].bottom_sub = t;

    _task_struct[i].state = TaskCtrlState::kNotRunningTask;
  }
}

void TaskCtrl::Remove(int cpuid, const Function &func) {
  Locker locker(_task_struct[cpuid].lock);
  Task *t = _task_struct[cpuid].top;
  while(t->next != nullptr) {
    if (t->next->func.Equal(func)) {
      Task *tt = t->next;
      t->next = t->next->next;
      if (tt == _task_struct[cpuid].bottom) {
        kassert(tt->next == nullptr);
        _task_struct[cpuid].bottom = t;
      }
      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tt));
      //        _allocator.Free(tt);
    } else {
      t = t->next;
    }
  }
  t = _task_struct[cpuid].top_sub;
  while(t->next != nullptr) {
    if (t->next->func.Equal(func)) {
      Task *tt = t->next;
      t->next = t->next->next;
      if (tt == _task_struct[cpuid].bottom_sub) {
        kassert(tt->next == nullptr);
        _task_struct[cpuid].bottom_sub = t;
      }
      virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tt));
      //        _allocator.Free(tt);
    } else {
      t = t->next;
    }
  }
}

void TaskCtrl::Run() {
  apic_ctrl->SetupTimer();
  while(true) {
    apic_ctrl->StopTimer();
    Function f;
    int cpuid = apic_ctrl->GetCpuId();
    kassert(_task_struct[cpuid].state == TaskCtrlState::kNotRunningTask);
    _task_struct[cpuid].state = TaskCtrlState::kRunningTask;
    while (true){
      Task *t;
      {
        Locker locker(_task_struct[cpuid].lock);
        Task *tt = _task_struct[cpuid].top;
        t = tt->next;
        if (t == nullptr) {
          kassert(tt == _task_struct[cpuid].bottom);
          break;
        }
        tt->next = t->next;
        if (t->next == nullptr) {
          kassert(_task_struct[cpuid].bottom == t);
          _task_struct[cpuid].bottom = tt;
        }
      }
      t->func.Execute();
      if (t->type == TaskType::kPolling) {
        Locker locker(_task_struct[cpuid].lock);
        _task_struct[cpuid].bottom_sub->next = t;
        t->next = nullptr;
        _task_struct[cpuid].bottom_sub = t;
      } else {
        virtmem_ctrl->Free(reinterpret_cast<virt_addr>(t));
        //      _allocator.Free(t);
      }
    }
    Locker locker(_task_struct[cpuid].lock);
    Task *tmp;
    tmp = _task_struct[cpuid].top;
    _task_struct[cpuid].top = _task_struct[cpuid].top_sub;
    _task_struct[cpuid].top_sub = tmp;

    tmp = _task_struct[cpuid].bottom;
    _task_struct[cpuid].bottom = _task_struct[cpuid].bottom_sub;
    _task_struct[cpuid].bottom_sub = tmp;

    _task_struct[cpuid].state = TaskCtrlState::kNotRunningTask;
    apic_ctrl->StartTimer();
    asm volatile("hlt");
  }
}

void TaskCtrl::RegisterSub(int cpuid, const Function &func, TaskType type) {
  if (cpuid < 0 || cpuid >= apic_ctrl->GetHowManyCpus()) {
    return;
  }
  Task *task = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));//_allocator.Alloc();
  task->func = func;
  task->next = nullptr;
  task->type = type;
  Locker locker(_task_struct[cpuid].lock);
  _task_struct[cpuid].bottom->next = task;
  _task_struct[cpuid].bottom = task;
}
