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
#include <functional.h>

class Polling;
// TODO: Functionベースでなく、Taskベースでの登録にすべき
class TaskCtrl {
 public:
  enum class TaskCtrlState {
    kRunningTask,
    kNotRunningTask,
  };
  TaskCtrl() {}
  void Setup();
  void Register(int apicid, const Function &func) {
    RegisterSub(apicid, func, TaskType::kNormal);
  }
  void Remove(int apicid, const Function &func);
  void Run();
  TaskCtrlState GetState(int apicid) {
    return _task_struct[apicid].state;
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
  void RegisterPolling(int apicid, const Function &func) {
    RegisterSub(apicid, func, TaskType::kPolling);
  }
  void RegisterSub(int apicid, const Function &func, TaskType type);
  struct TaskStruct {
    // queue
    Task *top;
    Task *bottom;
    Task *top_sub;
    Task *bottom_sub;
    SpinLock lock;

    TaskCtrlState state;
  } *_task_struct;
  Allocator<Task> _allocator;
};


#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
