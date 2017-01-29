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

#include <apic.h>
#include <mem/kstack.h>

void TaskCtrl::Setup() {
  kassert(apic_ctrl->DidSetup());
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
  apic_ctrl->SetupTimer(kTaskExecutionInterval);
  while(true) {
    TaskQueueState oldstate;
    {
      Locker locker(ts->lock);
      oldstate = ts->state;
      if (oldstate == TaskQueueState::kNotRunning) {
        apic_ctrl->StopTimer();
      }
      kassert(oldstate == TaskQueueState::kNotRunning
              || oldstate == TaskQueueState::kSlept);
      ts->state = TaskQueueState::kRunning;
    }
    if (oldstate == TaskQueueState::kNotRunning) {
      uint64_t time = timer->GetCntAfterPeriod(timer->ReadMainCnt(), kTaskExecutionInterval);
      
      Locker locker1(ts->dlock);
      sptr<Callout> dt = ts->dtop;
      while(true) {
        sptr<Callout> dtt;
        dtt = dt->_next;
        if (dtt.IsNull()) {
          break;
        }
        if (timer->IsGreater(dtt->_time, time)) {
          break;
        }
        {
          Locker locker2(dtt->_lock);
          dt->_next = dtt->_next;
          dtt->_next = make_sptr<Callout>();
          dtt->_state = Callout::CalloutState::kTaskQueue;
          Register(cpuid, dtt->_task);
        }
      }
    }
    while(true) {
      while(true) {
        sptr<Task> t;
        {
          Locker locker(ts->lock);
          sptr<Task> tt = ts->top;
          t = tt->_next;
          if (t.IsNull()) {
            kassert(tt == ts->bottom);
            break;
          }
          tt->_next = t->_next;
          if (t->_next.IsNull()) {
            kassert(ts->bottom == t);
            ts->bottom = tt;
          } else {
            t->_next->_prev = tt;
          }
          kassert(t->_status == Task::Status::kWaitingInQueue);
          t->_status = Task::Status::kRunning;
          t->_next = make_sptr<Task>();
          t->_prev = make_sptr<Task>();
        }

        t->Execute();

        {
          Locker locker(ts->lock);
          if (t->_status == Task::Status::kRunning) {
            t->_status = Task::Status::kOutOfQueue;
          }
        }
      }
      {
        Locker locker(ts->lock);

        if (ts->top->_next.IsNull() &&
            ts->top_sub->_next.IsNull()) {
          ts->state = TaskQueueState::kSlept;
          break;
        }
        sptr<Task> tmp;
        tmp = ts->top;
        ts->top = ts->top_sub;
        ts->top_sub = tmp;

        tmp = ts->bottom;
        ts->bottom = ts->bottom_sub;
        ts->bottom_sub = tmp;
      }

      volatile uint64_t time = timer->GetCntAfterPeriod(timer->ReadMainCnt(), kTaskExecutionInterval);
      {
        Locker locker(ts->dlock);
        sptr<Callout> dtt = ts->dtop->_next;
        if (!dtt.IsNull() && timer->IsGreater(time, dtt->_time)) {
          ts->state = TaskQueueState::kNotRunning;
          break;
        }
      }
    }
    
    kassert(ts->state == TaskQueueState::kSlept || ts->state == TaskQueueState::kNotRunning);

    if (ts->state == TaskQueueState::kSlept) {
      {
        Locker locker(ts->dlock);
        if (!ts->dtop->_next.IsNull()) {
          ts->state = TaskQueueState::kNotRunning;
        }
      }
      apic_ctrl->StartTimer();
      asm volatile("hlt");
    }
  }
}

void TaskCtrl::Register(CpuId cpuid, sptr<Task> task) {
  if (task->IsRegistered()) {
    kernel_panic("TaskCtrl", "unable to register queued Task.");
  }
  int raw_cpuid = cpuid.GetRawId();
  // TODO should'nt we add SpinLock to Task?
  Locker locker(_task_struct[raw_cpuid].lock);
  if (task->_status == Task::Status::kWaitingInQueue) {
    return;
  }
  task->_next = make_sptr<Task>();
  task->_status = Task::Status::kWaitingInQueue;
  task->_cpuid = cpuid;
  _task_struct[raw_cpuid].bottom_sub->_next = task;
  task->_prev = _task_struct[raw_cpuid].bottom_sub;
  _task_struct[raw_cpuid].bottom_sub = task;
  
  ForceWakeup(cpuid);
}

void TaskCtrl::Remove(sptr<Task> task) {
  kassert(task->_status != Task::Status::kGuard);
  if (task->_status != Task::Status::kOutOfQueue) {
    int cpuid = cpu_ctrl->GetCpuId().GetRawId();
    Locker locker(_task_struct[cpuid].lock);
    switch(task->_status) {
    case Task::Status::kWaitingInQueue: {
      sptr<Task> next = task->_next;
      sptr<Task> prev = task->_prev;

      task->_next = make_sptr<Task>();
      task->_prev = make_sptr<Task>();

      kassert(!prev.IsNull());
      prev->_next = next;

      if (next.IsNull()) {
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

extern "C" {
  void initialize_TaskThread(TaskWithStack::TaskThread *t) {
    t->InitBuffer();
  }
}

void TaskWithStack::TaskThread::Init() {
  _stack = KernelStackCtrl::GetCtrl().AllocThreadStack(_cpuid);
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
    assert(_cpuid == cpu_ctrl->GetCpuId());
    _func->Execute();
    longjmp(_return_buf, 1);
  }
  kernel_panic("TaskThread", "unexpectedly ended.");
}

void TaskWithStack::TaskThread::Wait() {
  if (setjmp(_buf) == 0) {
    _state = State::kWaiting;
    longjmp(_return_buf, 1);
  }
  assert(_cpuid == cpu_ctrl->GetCpuId());
}

void TaskWithStack::TaskThread::SwitchTo() {
  assert(_cpuid == cpu_ctrl->GetCpuId());
  assert(_state == State::kWaiting);
  _state = State::kRunning;
  if (setjmp(_return_buf) == 0) {
    longjmp(_buf, 1);
  }
}

void TaskCtrl::RegisterCallout(sptr<Callout> task, CpuId cpuid, int us) {
  assert(task->_state == Callout::CalloutState::kHandling || task->_state == Callout::CalloutState::kStopped);
  task->SetHandler(task, cpuid, us);
  TaskStruct *ts = &_task_struct[task->_cpuid.GetRawId()];
  {
    Locker locker(ts->dlock);
    sptr<Callout> dt = ts->dtop;
    while(true) {
      sptr<Callout> dtt = dt->_next;
      if (dtt.IsNull()) {
        task->_state = Callout::CalloutState::kCalloutQueue;
      	task->_next = make_sptr<Callout>();
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

void TaskCtrl::RegisterCallout2(sptr<Callout> task, int us) {
  assert(task->_state == Callout::CalloutState::kHandling || task->_state == Callout::CalloutState::kStopped);
  CpuId cpuid = cpu_ctrl->GetCpuId();
  task->SetHandler(task, cpuid, us);
  TaskStruct *ts = &_task_struct[task->_cpuid.GetRawId()];
  {
    Locker locker(ts->dlock);
    sptr<Callout> dt = ts->dtop;
    while(true) {
      sptr<Callout> dtt = dt->_next;
      if (dtt.IsNull()) {
        task->_state = Callout::CalloutState::kCalloutQueue;
      	task->_next = make_sptr<Callout>();
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

bool TaskCtrl::CancelCallout(sptr<Callout> task) {
  task->Cancel();
  bool flag = false;
  switch(task->_state) {
  case Callout::CalloutState::kCalloutQueue: {
    int cpuid = task->_cpuid.GetRawId();
    Locker locker(_task_struct[cpuid].dlock);
    sptr<Callout> dt = _task_struct[cpuid].dtop;
    while(!dt->_next.IsNull()) {
      sptr<Callout> dtt = dt->_next;
      if (dtt == task) {
        dt->_next = dtt->_next;
        break;
      }
      dt = dtt;
    }
    task->_next = make_sptr<Callout>();
    flag = true;
    break;
  }
  case Callout::CalloutState::kTaskQueue: {
    Remove(task->_task);
    flag = true;
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
  return flag;
}

void TaskCtrl::ForceWakeup(CpuId cpuid) {
  int raw_cpuid = cpuid.GetApicId();
  if (_task_struct[raw_cpuid].state == TaskQueueState::kSlept) {
    if (cpu_ctrl->GetCpuId().GetRawId() != raw_cpuid) {
      apic_ctrl->SendIpi(raw_cpuid);
    }
  }
}

void CountableTask::Inc() {
  //TODO CASを使って高速化
  Locker locker(_lock);
  _cnt++;
  if (_cnt == 1) {
    task_ctrl->Register(_cpuid, _task);
  }
}

void CountableTask::HandleSub(void *) {
  _func->Execute();
  {
    Locker locker(_lock);
    _cnt--;
    if (_cnt != 0) {
      task_ctrl->Register(_cpuid, _task);
    }
  }
}

void Callout::SetHandler(sptr<Callout> callout, CpuId cpuid, int us) {
  Locker locker(_lock);
  _time = timer->GetCntAfterPeriod(timer->ReadMainCnt(), us);
  _pending = true;
  _cpuid = cpuid;
  _task->SetFunc(make_uptr(new ClassFunction<Callout, sptr<Callout>>(this, &Callout::HandleSub, callout)));
}

void Callout::Cancel() {
  Locker locker(_lock);
  _pending = false;
}

void Callout::HandleSub2(sptr<Callout> callout) {
  if (timer->IsTimePassed(_time)) {
    {
      Locker locker(_lock);
      _pending = false;
      _state = CalloutState::kHandling;
      _task->SetFunc(make_uptr<GenericFunction<>>());
    }
    _func->Execute();
    {
      // TODO CASにする
      Locker locker(_lock);
      if (_state == CalloutState::kHandling) {
        _state = CalloutState::kStopped;
      }
    }
  } else {
    task_ctrl->Register(cpu_ctrl->GetCpuId(), _task);
  }
}

TaskCtrl::TaskStruct::TaskStruct() {
  sptr<Task> t = make_sptr(new Task);
  t->_status = Task::Status::kGuard;
  t->_next = make_sptr<Task>();
  t->_prev = make_sptr<Task>();

  top = t;
  bottom = t;

  t = make_sptr(new Task);
  t->_status = Task::Status::kGuard;
  t->_next = make_sptr<Task>();
  t->_prev = make_sptr<Task>();

  top_sub = t;
  bottom_sub = t;

  state = TaskQueueState::kNotStarted;

  sptr<Callout> dt = make_sptr(new Callout);
  dt->_next = make_sptr<Callout>();
  dtop = dt;
}
