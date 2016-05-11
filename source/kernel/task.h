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

#ifndef __RAPH_KERNEL_TASK_H__
#define __RAPH_KERNEL_TASK_H__

#include <global.h>
#include <spinlock.h>
#include <mem/virtmem.h>
#include <function.h>

class Task;

class TaskCtrl {
 public:
  enum class TaskQueueState {
    kRunning,
    kNotRunning,
  };
  TaskCtrl() {}
  void Setup();
  // RegisterしたTaskはRemoveを呼び出すまで削除されない
  void Register(int cpuid, Task *task);
  void Remove(int cpuid, Task *task);
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
    SpinLock lock;

    TaskQueueState state;
  } *_task_struct = nullptr;
};

class Task {
public:
  Task() {
  }
  virtual ~Task();
  void SetFunc(const GenericFunction &func) {
    _func.Copy(func);
  }
private:
  enum class Status {
    kRunning,
    kWaitingInQueue,
    kOutOfQueue,
    kGuard,
  };
  FunctionBase _func;
  Task *_next;
  Task *_prev;
  Status _status = Status::kOutOfQueue;
  friend TaskCtrl;
};

#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
