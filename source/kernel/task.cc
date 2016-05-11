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
#include <apic.h>

void TaskCtrl::Setup() {
  int cpus = apic_ctrl->GetHowManyCpus();
  _task_struct = reinterpret_cast<TaskStruct *>(virtmem_ctrl->Alloc(sizeof(TaskStruct) * cpus));
  Task *t;
  for (int i = 0; i < cpus; i++) {
    new(&_task_struct[i]) TaskStruct;

    t = virtmem_ctrl->New<Task>();
    t->_status = Task::Status::kGuard;
    t->_next = nullptr;
    t->_prev = nullptr;

    _task_struct[i].top = t;
    _task_struct[i].bottom = t;

    t = virtmem_ctrl->New<Task>();
    t->_status = Task::Status::kGuard;
    t->_next = nullptr;
    t->_prev = nullptr;

    _task_struct[i].top_sub = t;
    _task_struct[i].bottom_sub = t;

    _task_struct[i].state = TaskQueueState::kNotRunning;
  }
}

void TaskCtrl::Remove(int cpuid, Task *task) {
  kassert(task->_status != Task::Status::kGuard);
  Locker locker(_task_struct[cpuid].lock);
  switch(task->_status) {
  case Task::Status::kWaitingInQueue: {
    Task *next = task->_next;
    Task *prev = task->_prev;

    task->_next = nullptr;
    task->_prev = nullptr;

    kassert(prev != nullptr);
    prev->_next = next;

    if (next == nullptr) {
      if (task == _task_struct[cpuid].bottom) {
        _task_struct[cpuid].bottom = prev;
      } else if (task == _task_struct[cpuid].bottom_sub) {
        _task_struct[cpuid].bottom_sub = prev;
      } else {
        kassert(false);
      }
    }

    prev->_next = next;
    next->_prev = prev;
  }
  case Task::Status::kRunning:
  case Task::Status::kOutOfQueue: {
    break;
  }
  default:{
    kassert(false);
  }
  }
  task->_status = Task::Status::kOutOfQueue;  
}

void TaskCtrl::Run() {
  apic_ctrl->SetupTimer();
  while(true) {
    apic_ctrl->StopTimer();
    int cpuid = apic_ctrl->GetCpuId();
    kassert(_task_struct[cpuid].state == TaskQueueState::kNotRunning);
    _task_struct[cpuid].state = TaskQueueState::kRunning;
    while (true){
      Task *t;
      {
        Locker locker(_task_struct[cpuid].lock);
        Task *tt = _task_struct[cpuid].top;
        t = tt->_next;
        if (t == nullptr) {
          kassert(tt == _task_struct[cpuid].bottom);
          break;
        }
        tt->_next = t->_next;
        if (t->_next == nullptr) {
          kassert(_task_struct[cpuid].bottom == t);
          _task_struct[cpuid].bottom = tt;
        } else {
          t->_next->_prev = tt;
        }
        kassert(t->_status == Task::Status::kWaitingInQueue);
        t->_status = Task::Status::kRunning;
        t->_next = nullptr;
        t->_prev = nullptr;
      }
      
      t->_func.Execute();

      if (t->_status == Task::Status::kRunning) {
        Locker locker(_task_struct[cpuid].lock);
        t->_status = Task::Status::kWaitingInQueue;
        _task_struct[cpuid].bottom_sub->_next = t;
        t->_next = nullptr;
        t->_prev = _task_struct[cpuid].bottom_sub;
        _task_struct[cpuid].bottom_sub = t;
      }
    }
    {
      Locker locker(_task_struct[cpuid].lock);
      Task *tmp;
      tmp = _task_struct[cpuid].top;
      _task_struct[cpuid].top = _task_struct[cpuid].top_sub;
      _task_struct[cpuid].top_sub = tmp;

      tmp = _task_struct[cpuid].bottom;
      _task_struct[cpuid].bottom = _task_struct[cpuid].bottom_sub;
      _task_struct[cpuid].bottom_sub = tmp;

      _task_struct[cpuid].state = TaskQueueState::kNotRunning;

      if (_task_struct[cpuid].top->_next != nullptr) {
        continue;
      }
    }
    apic_ctrl->StartTimer();
#ifndef __UNIT_TEST__
    asm volatile("hlt");
#endif // !__UNIT_TEST__
  }
}

void TaskCtrl::Register(int cpuid, Task *task) {
  if (cpuid < 0 || cpuid >= apic_ctrl->GetHowManyCpus()) {
    return;
  }
  kassert(task->_status == Task::Status::kOutOfQueue);
  Locker locker(_task_struct[cpuid].lock);
  task->_next = nullptr;
  task->_status = Task::Status::kWaitingInQueue;
  _task_struct[cpuid].bottom_sub->_next = task;
  task->_prev = _task_struct[cpuid].bottom_sub;
  _task_struct[cpuid].bottom_sub = task;
}

Task::~Task() {
  kassert(_status == Status::kOutOfQueue);
}
