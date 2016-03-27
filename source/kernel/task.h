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
      void (*func)(void *) = _func;
      _func = nullptr;
      func(_arg);
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

class TaskCtrl {
 public:
  TaskCtrl() {
    _task_list = nullptr;
  }
  void Setup() {
    int cpus = apic_ctrl->GetHowManyCpus();
    _task_list = reinterpret_cast<Task **>(virtmem_ctrl->Alloc(sizeof(Task *) * cpus));
    for (int i = 0; i < cpus; i++) {
      _task_list[i] = nullptr;
    }
  }
  void Register(const Function &func) {
    Task *task = _allocator.Alloc();
    task->func = func;
    task->next = nullptr;
    Locker locker(_lock);
    Task *t = _task_list[apic_ctrl->GetApicId()];
    if (t == nullptr) {
      _task_list[apic_ctrl->GetApicId()] = task;
      return;
    }
    while(t->next) {
      t = t->next;
    }
    t->next = task;
  }
  void Remove(const Function &func) {
    Locker locker(_lock);
    Task *t = _task_list[apic_ctrl->GetApicId()];
    if (t == nullptr) {
      return;
    }
    if (t->func.Equal(func)) {
      _task_list[apic_ctrl->GetApicId()] = t->next;
      t = t->next;
    }

    while(t->next) {
      if (t->next->func.Equal(func)) {
        t->next = t->next->next;
      }

      t = t->next;
    }
  }
  void Run() {
    Function f;
    while (true){
      {
        Locker locker(_lock);
        Task *t = _task_list[apic_ctrl->GetApicId()];
        if (t == nullptr) {
          return;
        }
        _task_list[apic_ctrl->GetApicId()] = t->next;
        _allocator.Free(t);
        f = t->func;
      }
      f.Execute();
    }
  }
 private:
  struct Task {
    Function func;
    Task *next;
  };
  Task **_task_list;
  Allocator<Task> _allocator;
  SpinLock _lock;
};


#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
