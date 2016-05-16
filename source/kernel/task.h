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

#include <global.h>
#include <spinlock.h>

class Task;

class TaskCtrl {
public:
  enum class TaskQueueState {
    kNotRunning,
    kRunning,
    kSleeped,
  };
  TaskCtrl() {}
  void Setup();
  void Register(int cpuid, Task *task);
  // void Remove(int cpuid, Task *task);
  void Run();
  TaskQueueState GetState(int cpuid) {
    if (_task_struct == nullptr) {
      return TaskQueueState::kNotRunning;
    }
    return _task_struct[cpuid].state;
  }
 private:
  struct TaskStruct {
    // queue
    Task *top;
    Task *bottom;
    Task *top_sub;
    Task *bottom_sub;
    IntSpinLock lock;

    TaskQueueState state;
  } *_task_struct = nullptr;
};

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

class Task {
public:
  Task() {
    _cnt = 0;
  }
  virtual ~Task();
  void SetFunc(const Function &func) {
    _func = func;
  }
  enum class Status {
    kRunning,
    kWaitingInQueue,
    kOutOfQueue,
    kGuard,
  };
  Status GetStatus() {
    return _status;
  }
  void Execute() {
    _func.Execute();
  }
private:
  Function _func;
  Task *_next;
  Task *_prev;
  Status _status = Status::kOutOfQueue;
  int _cnt;
  friend TaskCtrl;
};

#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
