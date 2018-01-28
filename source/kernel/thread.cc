/*
 *
 * Copyright (c) 2017 Raphine Project
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#include "thread.h"
#include <cpu.h>
#include <setjmp.h>
#include <apic.h>
#include <tty.h>
#include <mem/kstack.h>

ThreadCtrl *ThreadCtrl::_thread_ctrl;
bool ThreadCtrl::_is_initialized = false;

void Thread::Init(Thread::StackState sstate, const char *file, int line) {
  _stack_state = sstate;
  kassert(_op_obj._cnt == 0);
  kassert(_waiting_thread == nullptr);
  if (_stack_state == StackState::kIndependent) {
    _stack = KernelStackCtrl::GetCtrl().AllocThreadStack(cpu_ctrl->GetCpuId());
    AllocReturnBuf();
    if (setjmp(_return_buf) == 0) {
      asm volatile(
          "movq %0, %%rsp;"
          "call *%1;" ::"r"(_stack),
          "r"(&Thread::InitBufferSub), "D"(this));
      kernel_panic("Thread", "unexpectedly ended.");
    } else {
      // return from Thread::InitBuffer()
    }
  }
  _file = file;
  _line = line;
}

void Thread::InitBuffer() {
  kassert(_stack_state == StackState::kIndependent);
  if (setjmp(_buf) == 0) {
    ReleaseReturnBuf();
    longjmp(_return_buf, 1);
    // return back to Thread::Init()
  } else {
    // call from Thread::SwitchTo()
    do {
      // increment op count during executing the function
      // the purpose of this is not to return back to Join()
      // while the function is running.
      auto dummy_op =
          CreateOperator();
      _func->Execute();
    } while (0);
    ReleaseReturnBuf();
    longjmp(_return_buf, 1);
  }
  kernel_panic("Thread", "unexpectedly ended.");
}

void Thread::Execute() {
  if (_stack_state == StackState::kIndependent) {
    SwitchTo();
  } else {
    _func->Execute();
  }
  int cnt = __sync_fetch_and_sub(&_op_obj._cnt, 1);
  kassert(cnt >= 1);
  if (cnt == 1 && _waiting_thread != nullptr) {
    // no operator reference to this thread.
    // return back to Join
    if (GetState() != Thread::State::kRunning) {
      ShowErr("no operator reference but the thread is queued. the bug of Thread?");
    }
    _waiting_thread->Schedule();
    _waiting_thread = nullptr;
  }
}

void Thread::Join() {
  kassert(_waiting_thread == nullptr);
  _waiting_thread = ThreadCtrl::GetCurrentThread();
  kassert(ThreadCtrl::GetCurrentThread()->SetState(State::kOutOfQueue) ==
          State::kRunning);
  ThreadCtrl::WaitCurrentThread();
}

Thread::State Thread::Stop() {
  while (true) {
    auto state = _state;
    switch (state) {
      case State::kWaitingInQueue:
        if (CompareAndSetState(state, State::kStopping)) {
          kassert(__sync_fetch_and_sub(&_op_obj._cnt, 1) >= 1);
          return state;
        }
        break;
      case State::kRunning:
      case State::kOutOfQueue:
      case State::kStopping:
        return state;
      default:
        kassert(false);
        break;
    }
  }
}

// TODO show warning if no one refers this thread
void Thread::Wait() {
  if (_stack_state != StackState::kIndependent) {
    ShowErr("The StackState of a thread which calls Wait()/Join() must be 'Independent'.");
  }
  if (setjmp(_buf) == 0) {
    ReleaseReturnBuf();
    longjmp(_return_buf, 1);
  }
}

void Thread::Delete() {
  if (_state == State::kRunning || _state == State::kWaitingInQueue) {
    Stop();
  }
  if (_op_obj._cnt != 0) {
    ShowErr("tried to delete a thread unless Thread Operators remain.");
  }
  while (true) {
    switch (_state) {
      case State::kStopping:
        if (CompareAndSetState(State::kStopping, State::kWaitingToDelete)) {
          return;
        }
        break;
      case State::kOutOfQueue:
        CompareAndSetState(State::kOutOfQueue, State::kDeleting);
        _ctrl->PushToIdle(this);
        return;
      case State::kRunning:
      case State::kWaitingInQueue:
      case State::kWaitingToDelete:
      case State::kDeleting:
      case State::kIdle:
        kassert(false);
    }
  }
}

void Thread::ShowErr(const char *str) {
  gtty->DisablePrint();
  gtty->ErrPrintf("Thread Info: allocated at %s:%d\n", _file, _line);
  kernel_panic("Thread", str);
}

int Thread::SwitchTo() {
  kassert(_stack_state == StackState::kIndependent);
  AllocReturnBuf();
  int sig;
  if ((sig = setjmp(_return_buf)) == 0) {
    longjmp(_buf, 1);
    kernel_panic("Thread", "unexpectedly ended.");
  } else {
    return sig;
  }
}

void Thread::Release() {
  if (_stack_state == StackState::kIndependent) {
    _stack_state = StackState::kShared;
    _using_return_buf = 0;
    KernelStackCtrl::GetCtrl().FreeThreadStack(_stack);
  }
  kassert(_op_obj._cnt == 0);
  kassert(_waiting_thread == nullptr);
}

ThreadCtrl::~ThreadCtrl() {
  for (int i = 0; i < kMaxThreadNum; i++) {
    delete _threads[i];
  }
  delete _threads;
}

void ThreadCtrl::Init() {
  _thread_ctrl = new ThreadCtrl[cpu_ctrl->GetHowManyCpus()];
  for (int i = 0; i < cpu_ctrl->GetHowManyCpus(); i++) {
    CpuId id(i);
    _thread_ctrl[i].InitSub(id);
  }
  _is_initialized = true;
}

void ThreadCtrl::InitSub(CpuId id) {
  _cpuid = id;
  _idle_threads.Init();
  _threads = new Thread *[kMaxThreadNum];
  for (int i = 0; i < kMaxThreadNum; i++) {
    _threads[i] = new Thread(this);
    _idle_threads.Push(_threads[i]);
  }
}

uptr<Thread> ThreadCtrl::AllocNewThread_(Thread::StackState sstate, const char *file, int line) {
  Thread *t;
  _idle_threads.Pop(t);
  t->Init(sstate, file ,line);
  return make_uptr(t);
}

void ThreadCtrl::IdleThreads::Push(Thread *t) {
  kassert(t->SetState(Thread::State::kIdle) == Thread::State::kDeleting);
  kassert(_buf->Push(t));
}

void ThreadCtrl::IdleThreads::Pop(Thread *&t) {
  if (!_buf->Pop(t)) {
    kernel_panic("ThreadCtrl", "not enough resources");
  }
  kassert(t->SetState(Thread::State::kOutOfQueue) == Thread::State::kIdle);
}

bool ThreadCtrl::ScheduleRunQueue(Thread *thread) {
  auto state = thread->SetState(Thread::State::kWaitingInQueue);
  switch (state) {
    case Thread::State::kOutOfQueue:
    case Thread::State::kRunning:
      _run_queue.Push(thread);
      return true;
    case Thread::State::kWaitingInQueue:
    case Thread::State::kStopping:
      return false;
    default:
      kassert(false);
  }
}

bool ThreadCtrl::ScheduleWaitQueue(Thread *thread, int us) {
  if (us == 0) {
      thread->ShowErr("us of ScheduleWaitQueue() must be greater than zero.");
  }
  auto state = thread->SetState(Thread::State::kWaitingInQueue);
  switch (state) {
    case Thread::State::kOutOfQueue:
    case Thread::State::kRunning:
      _wait_queue.Push(&thread->GetWqElement(), timer->ReadTime() + us);
      return true;
    case Thread::State::kWaitingInQueue:
    case Thread::State::kStopping:
      return false;
    default:
      kassert(false);
  }
}

void ThreadCtrl::Run() {
  apic_ctrl->SetupTimer(kExecutionInterval);

  while (true) {
    Thread *cur;
    bool wait_queue_empty = true;
    Time t;

    _state = QueueState::kRunning;

    while (true) {
      if (wait_queue_empty) {
        wait_queue_empty = _wait_queue.IsEmpty();
        if (!wait_queue_empty) {
          t = _wait_queue.GetTopOrder();
        }
      }
      if (!wait_queue_empty && t < timer->ReadTime()) {
        // pop from wait queue and push to run queue
        Thread::WaitQueueElement *wq_ele;
        kassert(_wait_queue.Pop(wq_ele));
        Thread *wthread = wq_ele->GetThread();
        Thread::State state = wthread->GetState();
        kassert(state == Thread::State::kWaitingInQueue ||
                state == Thread::State::kStopping ||
                state == Thread::State::kWaitingToDelete);
        _run_queue.Push(wthread);
        wait_queue_empty = true;
      }
      if (!_run_queue.Pop(cur)) {
        // no more threads on run queue
        break;
      }
      if (cur->CompareAndSetState(Thread::State::kStopping,
                                  Thread::State::kOutOfQueue)) {
      } else {
        if (cur->CompareAndSetState(Thread::State::kWaitingInQueue,
                                    Thread::State::kRunning)) {
          kassert(_current_thread == nullptr);
          _current_thread = cur;
          cur->Execute();
          _current_thread = nullptr;
          cur->CompareAndSetState(Thread::State::kRunning,
                                  Thread::State::kOutOfQueue);
        }
        for (bool flag = false; !flag;) {
          auto state = cur->GetState();
          switch (state) {
            case Thread::State::kWaitingToDelete:
              kassert(cur->SetState(Thread::State::kDeleting) ==
                      Thread::State::kWaitingToDelete);
              PushToIdle(cur);
              flag = true;
              break;
            case Thread::State::kStopping:
              if (cur->CompareAndSetState(state, Thread::State::kOutOfQueue)) {
                flag = true;
              }
              break;
            case Thread::State::kDeleting:
            case Thread::State::kRunning:
              kassert(false);
              break;
            default:
              flag = true;
              break;
          }
        }
      }
    }

    _state = QueueState::kSleeping;

    apic_ctrl->StartTimer();
    asm volatile("hlt");
  }
}
