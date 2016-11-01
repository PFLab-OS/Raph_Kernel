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

#include <task.h>
#include <cpu.h>
#include <setjmp.h>

#ifdef __KERNEL__
#include <apic.h>
#include <mem/kstack.h>
#else
#include <raph.h>
#include <unistd.h>
#endif // __KERNEL__

void TaskCtrl::Setup() {
#ifdef __KERNEL__
  kassert(apic_ctrl->DidSetup());
#endif // __KERNEL__
  int cpus = cpu_ctrl->GetHowManyCpus();
  _task_struct = new TaskStruct[cpus];
  for (int i = 0; i < cpus; i++) {
    new(&_task_struct[i]) TaskStruct;
  }
}

void TaskCtrl::Run() {
  CpuId cpuid = cpu_ctrl->GetCpuId();
  int raw_cpu_id = cpuid.GetRawId();
  TaskStruct *ts = &_task_struct[raw_cpu_id];

  ts->state = TaskQueueState::kNotRunning;
#ifdef __KERNEL__
  apic_ctrl->SetupTimer(kTaskExecutionInterval);
#endif // __KERNEL__
  while(true) {
    TaskQueueState oldstate;
    {
      Locker locker(ts->lock);
      oldstate = ts->state;
#ifdef __KERNEL__
      if (oldstate == TaskQueueState::kNotRunning) {
        apic_ctrl->StopTimer();
      }
#endif // __KERNEL__
      kassert(oldstate == TaskQueueState::kNotRunning
              || oldstate == TaskQueueState::kSlept);
      ts->state = TaskQueueState::kRunning;
    }
    if (oldstate == TaskQueueState::kNotRunning) {
      uint64_t time = timer->GetCntAfterPeriod(timer->ReadMainCnt(), kTaskExecutionInterval);
      
      Callout *dt = ts->dtop;
      while(true) {
        Callout *dtt;
        {
          Locker locker(ts->dlock);
          dtt = dt->_next;
          if (dtt == nullptr) {
            break;
          }
          if (timer->IsGreater(dtt->_time, time)) {
            break;
          }
          if (dtt->_lock.Trylock() < 0) {
            // retry
            continue;
          }
          dt->_next = dtt->_next;
        }
        dtt->_next = nullptr;
        dtt->_state = Callout::CalloutState::kTaskQueue;
        Register(cpuid, &dtt->_task);
        dtt->_lock.Unlock();
        break;
      }
    }
    while(true) {
      while(true) {
        Task *t;
        {
          Locker locker(ts->lock);
          Task *tt = ts->top;
          t = tt->_next;
          if (t == nullptr) {
            kassert(tt == ts->bottom);
            break;
          }
          tt->_next = t->_next;
          if (t->_next == nullptr) {
            kassert(ts->bottom == t);
            ts->bottom = tt;
          } else {
            t->_next->_prev = tt;
          }
          kassert(t->_status == Task::Status::kWaitingInQueue);
          t->_status = Task::Status::kRunning;
          t->_next = nullptr;
          t->_prev = nullptr;
        }

        t->Execute();

        {
          Locker locker(ts->lock);
          if (t->_status == Task::Status::kRunning) {
            t->_status = Task::Status::kOutOfQueue;
          }
        }
      }
      Locker locker(ts->lock);

      if (ts->top->_next == nullptr &&
          ts->top_sub->_next == nullptr) {
        ts->state = TaskQueueState::kSlept;
        break;
      }
      Task *tmp;
      tmp = ts->top;
      ts->top = ts->top_sub;
      ts->top_sub = tmp;

      tmp = ts->bottom;
      ts->bottom = ts->bottom_sub;
      ts->bottom_sub = tmp;

      //TODO : FIX THIS : callout isn't executed while this loop is running.
    }
    
    kassert(ts->state == TaskQueueState::kSlept);

    {
      Locker locker(ts->dlock);
      if (ts->dtop->_next != nullptr) {
        ts->state = TaskQueueState::kNotRunning;
      }
    }
#ifdef __KERNEL__
    apic_ctrl->StartTimer();
    asm volatile("hlt");
#else
    usleep(10);
#endif // __KERNEL__
  }
}

void TaskCtrl::Register(CpuId cpuid, Task *task) {
  if (!(task->_status == Task::Status::kOutOfQueue ||
        (task->_status == Task::Status::kRunning && task->_cpuid.GetRawId() == cpuid.GetRawId()))) {
    kernel_panic("TaskCtrl", "unable to register queued Task.");
  }
  int raw_cpuid = cpuid.GetRawId();
  // TODO should'nt we add SpinLock to Task?
  Locker locker(_task_struct[raw_cpuid].lock);
  if (task->_status == Task::Status::kWaitingInQueue) {
    return;
  }
  task->_next = nullptr;
  task->_status = Task::Status::kWaitingInQueue;
  task->_cpuid = cpuid;
  _task_struct[raw_cpuid].bottom_sub->_next = task;
  task->_prev = _task_struct[raw_cpuid].bottom_sub;
  _task_struct[raw_cpuid].bottom_sub = task;
  
  ForceWakeup(cpuid);
}

void TaskCtrl::Remove(Task *task) {
  kassert(task->_status != Task::Status::kGuard);
  if (task->_status != Task::Status::kOutOfQueue) {
    int cpuid = cpu_ctrl->GetCpuId().GetRawId();
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
      } else {
        next->_prev = prev;
      }

      prev->_next = next;
      break;
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
}

void TaskCtrl::Wait() {
  // TODO
  // detect if lock acquired in current task
  // it will cause deadlock
}

extern "C" {
  static void initialize_TaskThread(TaskWithStack::TaskThread *t) {
    t->InitBuffer();
  }
}

void TaskWithStack::TaskThread::Init() {
  _stack = KernelStackCtrl::GetCtrl().AllocThreadStack(cpu_ctrl->GetCpuId());
  if (setjmp(_return_buf) == 0) {
    asm volatile("movq %0, %%rsp;"
                 "call initialize_TaskThread;"
                 :: "r"(_stack), "D"(this));
  }
  // return from TaskThread::InitBuffer()
}

void TaskWithStack::TaskThread::InitBuffer() {
  if (setjmp(_buf) == 0) {
    longjmp(_return_buf, 1);
    // return back to TaskThread::Init()
  } else {
    // call from TaskThread::SwitchTo()
    while(true) {
      _func.Execute();
      Wait();
    }
  }
  kernel_panic("TaskThread", "unexpectedly ended.");
}

void TaskWithStack::TaskThread::Wait() {
  if (setjmp(_buf) == 0) {
    longjmp(_return_buf, 1);
  }
}

void TaskWithStack::TaskThread::SwitchTo() {
  // TODO have to set current cpuid
  kassert(false);
  if (setjmp(_return_buf) == 0) {
    longjmp(_buf, 1);
  }
}

void TaskCtrl::RegisterCallout(Callout *task) {
  TaskStruct *ts = &_task_struct[task->_cpuid.GetRawId()];
  {
    Locker locker(ts->dlock);
    Callout *dt = ts->dtop;
    while(true) {
      Callout *dtt = dt->_next;
      if (dt->_next != nullptr) {
        task->_state = Callout::CalloutState::kCalloutQueue;
      	task->_next = dtt;
      	dt->_next = task;
      	break;
      }
      if (timer->IsGreater(dtt->_time, task->_time)) {
        task->_state = Callout::CalloutState::kCalloutQueue;
        task->_next = dtt;
        dt->_next = task;
        break;
      }
      dt = dtt;
    }
  }

  ForceWakeup(task->_cpuid);
}

void TaskCtrl::CancelCallout(Callout *task) {
  switch(task->_state) {
  case Callout::CalloutState::kCalloutQueue: {
    int cpuid = task->_cpuid.GetRawId();
    Locker locker(_task_struct[cpuid].dlock);
    Callout *dt = _task_struct[cpuid].dtop;
    while(dt->_next != nullptr) {
      Callout *dtt = dt->_next;
      if (dtt == task) {
        dt->_next = dtt->_next;
        break;
      }
      dt = dtt;
    }
    task->_next = nullptr;
    break;
  }
  case Callout::CalloutState::kTaskQueue: {
    Remove(&task->_task);
    break;
  }
  case Callout::CalloutState::kHandling:
  case Callout::CalloutState::kStopped: {
    break;
  }
  default:
    kassert(false);
  }
  task->_state = Callout::CalloutState::kStopped;
}

void TaskCtrl::ForceWakeup(CpuId cpuid) {
#ifdef __KERNEL__
  int raw_cpuid = cpuid.GetRawId();
  if (_task_struct[raw_cpuid].state == TaskQueueState::kSlept) {
    if (cpu_ctrl->GetCpuId().GetRawId() != raw_cpuid) {
      apic_ctrl->SendIpi(raw_cpuid);
    }
  }
#endif // __KERNEL__
}

void CountableTask::Inc() {
  //TODO CASを使って高速化
  Locker locker(_lock);
  _cnt++;
  if (_cnt == 1) {
    task_ctrl->Register(_cpuid, &_task);
  }
}

void CountableTask::HandleSub(void *) {
  _func.Execute();
  {
    Locker locker(_lock);
    _cnt--;
    if (_cnt != 0) {
      task_ctrl->Register(_cpuid, &_task);
    }
  }
}

void Callout::SetHandler(uint32_t us) {
  SetHandler(cpu_ctrl->GetCpuId(), us);
}

void Callout::SetHandler(CpuId cpuid, int us) {
  Locker locker(_lock);
  _time = timer->GetCntAfterPeriod(timer->ReadMainCnt(), us);
  _pending = true;
  _cpuid = cpuid;
  task_ctrl->RegisterCallout(this);
}

void Callout::Cancel() {
  Locker locker(_lock);
  _pending = false;
  task_ctrl->CancelCallout(this);
}

void Callout::HandleSub(void *) {
  if (timer->IsTimePassed(_time)) {
    _pending = false;
    _state = CalloutState::kHandling;
    _func.Execute();
    _state = CalloutState::kStopped;
  } else {
    task_ctrl->Register(cpu_ctrl->GetCpuId(), &_task);
  }
}

TaskCtrl::TaskStruct::TaskStruct() {
  Task *t = new Task;
  t->_status = Task::Status::kGuard;
  t->_next = nullptr;
  t->_prev = nullptr;

  top = t;
  bottom = t;

  t = new Task;
  t->_status = Task::Status::kGuard;
  t->_next = nullptr;
  t->_prev = nullptr;

  top_sub = t;
  bottom_sub = t;

  state = TaskQueueState::kNotStarted;

  Callout *dt = new Callout;
  dt->_next = nullptr;
  dtop = dt;
}
