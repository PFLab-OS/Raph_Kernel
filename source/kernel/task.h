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

#ifndef __RAPH_KERNEL_DEV_TASK_H__
#define __RAPH_KERNEL_DEV_TASK_H__

#include <allocator.h>
#include <apic.h>
#include <mem/virtmem.h>
#include <global.h>
#include <spinlock.h>

#include <timer.h>

class Function {
 public:
  Function() {
  }
  Function(const Function &func) {
    _func = func._func;
    _arg = func._arg;
  }
  void Init(void (*func)(void *), void *arg) {
    _func = func;
    _arg = arg;
  }
  void Execute() {
    if (_func != nullptr) {
      _func(_arg);
    }
  }
  volatile bool CanExecute() {
    return (_func != nullptr);
  }
  void Clear() {
    _func = nullptr;
  }
  bool Equal(const Function &func) {
    return (_func == func._func) && (_arg == func._arg);
  }
 private:
  void (*_func)(void *) = nullptr;
  void *_arg;
};

class Polling;
// TODO: Functionベースでなく、Taskベースでの登録にすべき
class TaskCtrl {
 public:
  TaskCtrl() {}
  void Setup() {
    int cpus = apic_ctrl->GetHowManyCpus();
    _task_queue = reinterpret_cast<TaskQueue *>(virtmem_ctrl->Alloc(sizeof(TaskQueue) * cpus));
    for (int i = 0; i < cpus; i++) {
      new(&_task_queue[i].lock) SpinLock;

      Task *t = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));
      new(&t->func) Function;
      t->next = nullptr;

      _task_queue[i].top = t;
      _task_queue[i].bottom = t;

      t = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));
      new(&t->func) Function;
      t->next = nullptr;

      _task_queue[i].top_sub = t;
      _task_queue[i].bottom_sub = t;
    }
  }
  void Register(const Function &func) {
    RegisterSub(func, TaskType::kNormal);
  }
  void Remove(const Function &func) {
    int apicid = apic_ctrl->GetApicId();
    Locker locker(_task_queue[apicid].lock);
    Task *t = _task_queue[apicid].top;
    while(t->next != nullptr) {
      if (t->next->func.Equal(func)) {
        Task *tt = t->next;
        t->next = t->next->next;
        if (tt == _task_queue[apicid].bottom) {
          kassert(tt->next == nullptr);
          _task_queue[apicid].bottom = t;
        }
        virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tt));
        //        _allocator.Free(tt);
      } else {
        t = t->next;
      }
    }
    t = _task_queue[apicid].top_sub;
    while(t->next != nullptr) {
      if (t->next->func.Equal(func)) {
        Task *tt = t->next;
        t->next = t->next->next;
        if (tt == _task_queue[apicid].bottom_sub) {
          kassert(tt->next == nullptr);
          _task_queue[apicid].bottom_sub = t;
        }
        virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tt));
        //        _allocator.Free(tt);
      } else {
        t = t->next;
      }
    }
  }
  void Run() {
    Function f;
    int apicid = apic_ctrl->GetApicId();
    while (true){
      Task *t;
      {
        Locker locker(_task_queue[apicid].lock);
        Task *tt = _task_queue[apicid].top;
        t = tt->next;
        if (t == nullptr) {
          kassert(tt == _task_queue[apicid].bottom);
          break;
        }
        tt->next = t->next;
        if (t->next == nullptr) {
          kassert(_task_queue[apicid].bottom == t);
          _task_queue[apicid].bottom = tt;
        }
      }
      t->func.Execute();
      if (t->type == TaskType::kPolling) {
        Locker locker(_task_queue[apicid].lock);
        _task_queue[apicid].bottom_sub->next = t;
        t->next = nullptr;
        _task_queue[apicid].bottom_sub = t;
      } else {
        virtmem_ctrl->Free(reinterpret_cast<virt_addr>(t));
        //      _allocator.Free(t);
      }
    }
    Locker locker(_task_queue[apicid].lock);
    Task *tmp;
    tmp = _task_queue[apicid].top;
    _task_queue[apicid].top = _task_queue[apicid].top_sub;
    _task_queue[apicid].top_sub = tmp;

    tmp = _task_queue[apicid].bottom;
    _task_queue[apicid].bottom = _task_queue[apicid].bottom_sub;
    _task_queue[apicid].bottom_sub = tmp;
  }
 private:
  friend Polling;
  enum class TaskType {
    kNormal,
    kPolling,
  };
  struct Task {
    Function func;
    Task *next;
    TaskType type;
  };
  void RegisterPolling(const Function &func) {
    RegisterSub(func, TaskType::kPolling);
  }
  void RegisterSub(const Function &func, TaskType type) {
    Task *task = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));//_allocator.Alloc();
    task->func = func;
    task->next = nullptr;
    task->type = type;
    int apicid = apic_ctrl->GetApicId();
    Locker locker(_task_queue[apicid].lock);
    _task_queue[apicid].bottom->next = task;
    _task_queue[apicid].bottom = task;
  }
  struct TaskQueue {
    Task *top;
    Task *bottom;
    Task *top_sub;
    Task *bottom_sub;
    SpinLock lock;
  } *_task_queue;
  Allocator<Task> _allocator;
};


#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
