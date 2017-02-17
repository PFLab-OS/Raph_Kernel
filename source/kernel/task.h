/*
 *
 * Copyright (c) 2016 Raphine Project
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

#ifndef __RAPH_LIB_TASK_H__
#define __RAPH_LIB_TASK_H__

#include <setjmp.h>
#include <_task.h>
#include <_queue.h>
#include <cpu.h>
#include <global.h>

// enable to blocking operation
class TaskWithStack : public Task {
public:
  TaskWithStack() = delete;
  TaskWithStack(CpuId cpuid) : _tthread(cpuid) {
  }
  virtual ~TaskWithStack() {
  }
  void Init() {
    _tthread.Init();
  }
  virtual void SetFunc(uptr<GenericFunction<>> func) override {
    _tthread.SetFunc(func);
  }
  virtual void Execute() override {
    _tthread.SwitchTo();
  }
  virtual void Wait() override {
    _tthread.Wait();
  }
  virtual void Kill() override {
    kernel_panic("TaskWithStack", "not implemented");
  }
  class TaskThread {
  public:
    TaskThread() = delete;
    TaskThread(CpuId cpuid) : _cpuid(cpuid) {
      assert(cpuid.IsValid());
    }
    void Init();
    void SetFunc(uptr<GenericFunction<>> func) {
      _func = func;
    }
    ~TaskThread() {
    }
    void InitBuffer();
    void Wait();
    void SwitchTo();
  private:  
    virt_addr _stack;
    jmp_buf _buf;
    jmp_buf _return_buf;
    uptr<GenericFunction<>> _func;
    CpuId _cpuid;
    enum State {
      kRunning,
      kWaiting,
    } _state = State::kWaiting;
  };
private:
  TaskThread _tthread;
};

class TaskCtrl {
public:
  enum class TaskQueueState {
    kNotStarted,
    kNotRunning,
    kRunning,
    kSlept,
  };
  TaskCtrl() {}
  void Setup();
  void Register(CpuId cpuid, sptr<Task> task);
  void Remove(sptr<Task> task);
  void Run();
  TaskQueueState GetState(CpuId cpuid) {
    if (_task_struct == nullptr) {
      return TaskQueueState::kNotStarted;
    }
    return _task_struct[cpuid.GetRawId()].state;
  }
  void RegisterCallout2(sptr<Callout> task, int us);
  void RegisterCallout(sptr<Callout> task, int us) {
    RegisterCallout(task, cpu_ctrl->GetCpuId(), us);
  }
  void RegisterCallout(sptr<Callout> task, CpuId cpuid, int us);
  bool CancelCallout(sptr<Callout> task);
private:
  
  void ForceWakeup(CpuId cpuid); 
  class TaskStruct {
  public:
    TaskStruct();
    // queue
    sptr<Task> top;
    sptr<Task> bottom;
    sptr<Task> top_sub;
    sptr<Task> bottom_sub;
    SpinLock lock;

    TaskQueueState state;

    // for Callout
    SpinLock dlock;
    sptr<Callout> dtop;

  } *_task_struct = nullptr;
  // this const value defines interval of wakeup task controller when all task slept
  // (task controller doesn't sleep if there is any registered tasks)
  static const int kTaskExecutionInterval = 1000; // us
};

#endif /* __RAPH_LIB_TASK_H__ */
