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
  TaskCtrl() {
    _task_queue_top = nullptr;
    _task_queue_bottom = nullptr;
  }
  void Setup() {
    int cpus = apic_ctrl->GetHowManyCpus();
    _task_queue_top = reinterpret_cast<Task **>(virtmem_ctrl->Alloc(sizeof(Task *) * cpus));
    _task_queue_bottom = reinterpret_cast<Task **>(virtmem_ctrl->Alloc(sizeof(Task *) * cpus));
    for (int i = 0; i < cpus; i++) {
      Task *t = reinterpret_cast<Task *>(virtmem_ctrl->Alloc(sizeof(Task)));
      new(&t->func) Function;
      t->next = nullptr;
      _task_queue_top[i] = t;
      _task_queue_bottom[i] = t;
    }
  }
  void Register(const Function &func) {
    RegisterSub(func, TaskType::kNormal);
  }
  void Remove(const Function &func) {
    Locker locker(_lock);
    Task *t = _task_queue_top[apic_ctrl->GetApicId()];
    while(t->next) {
      if (t->next->func.Equal(func)) {
        Task *tt = t->next;
        t->next = t->next->next;
        if (tt == _task_queue_bottom[apic_ctrl->GetApicId()]) {
          kassert(tt->next == nullptr);
          _task_queue_bottom[apic_ctrl->GetApicId()] = t;
        }
        virtmem_ctrl->Free(reinterpret_cast<virt_addr>(tt));
        //        _allocator.Free(tt);
      }
      t = t->next;
      
    }
  }
  void Run() {
    Function f;
    while (true){
      Task *t;
      {
        Locker locker(_lock);
        Task *tt = _task_queue_top[apic_ctrl->GetApicId()];
        t = tt->next;
        if (t == nullptr) {
          return;
        }
        tt->next = t->next;
        if (t->next == nullptr) {
          kassert(_task_queue_bottom[apic_ctrl->GetApicId()] == t);
          _task_queue_bottom[apic_ctrl->GetApicId()] = tt;
        }
      }
      t->func.Execute();
      if (t->type == TaskType::kPolling) {
        Locker locker(_lock);
        _task_queue_bottom[apic_ctrl->GetApicId()]->next = t;
        t->next = nullptr;
        _task_queue_bottom[apic_ctrl->GetApicId()] = t;
      } else {
        virtmem_ctrl->Free(reinterpret_cast<virt_addr>(t));
        //      _allocator.Free(t);
      }
    }
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
    Locker locker(_lock);
    _task_queue_bottom[apic_ctrl->GetApicId()]->next = task;
    _task_queue_bottom[apic_ctrl->GetApicId()] = task;
  }
  Task **_task_queue_top;
  Task **_task_queue_bottom;
  Allocator<Task> _allocator;
  SpinLock _lock;
};


#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
