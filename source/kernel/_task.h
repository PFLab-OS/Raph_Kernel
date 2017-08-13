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

#pragma once

#include <raph.h>
#include <global.h>
#include <function.h>
#include <spinlock.h>
#include <timer.h>
#include <ptr.h>

class Task {
public:
  enum class Status {
    kRunning,
    kWaitingInQueue,
    kOutOfQueue,
    kGuard,
  };
  Task() {
  }
  virtual ~Task() {
    kassert(_status == Status::kOutOfQueue);
  }
  virtual void SetFunc(uptr<GenericFunction<>> func) {
    _func = func;
  }
  Status GetStatus() {
    return _status;
  }
  virtual void Execute() {
    _func->Execute();
  }
  virtual void Wait(int sig) {
    kernel_panic("Task", "unable to use blocking operation(use TaskWithStack instead)");
  }
  virtual void Kill() {
    kernel_panic("Task", "unable to kill normal Task");
  }
  bool IsRegistered() {
    return _status == Task::Status::kWaitingInQueue;
  }
private:
  uptr<GenericFunction<>> _func;
  sptr<Task> _next;
  sptr<Task> _prev;
  CpuId _cpuid;
  Status _status = Status::kOutOfQueue;
  friend TaskCtrl;  // TODO should be removed
};

// Taskがキューに積まれている間にインクリメント可能
// 割り込み内からも呼び出せる
// ただし、一定時間後に立ち上げる事や割り当てcpuidを変える事はできない
class CountableTask {
public:
  CountableTask() : _task(new Task) {
    _cnt = 0;
    _task->SetFunc(make_uptr(new ClassFunction<CountableTask, void *>(this, &CountableTask::HandleSub, nullptr)));
  }
  virtual ~CountableTask() {
  }
  void SetFunc(CpuId cpuid, uptr<GenericFunction<>> func) {
    _cpuid = cpuid;
    _func = func;
  }
  Task::Status GetStatus() {
    return _task->GetStatus();
  }
  sptr<Task> GetTask() {
    return _task;
  }
  void Inc();
  int GetCnt() {
    return _cnt;
  }
private:
  void HandleSub(void *);
  sptr<Task> _task;
  SpinLock _lock;
  uptr<GenericFunction<>> _func;
  int _cnt;
  CpuId _cpuid;
};

// 遅延実行されるタスク 
// 一度登録すると、実行されるかキャンセルするまでは再登録はできない
// 割り込み内からも呼び出し可能
class Callout {
public:
  enum class CalloutState {
    kCalloutQueue,
    kTaskQueue,
    kHandling,
    kStopped,
  };
  Callout() : _task(new Task) {
  }
  virtual ~Callout() {
  }
  void Init(uptr<GenericFunction<>> func) {
    _func = func;
  }
  volatile bool IsHandling() {
    return (_state == CalloutState::kHandling);
  }
  bool IsPending() {
    return _pending;
  }
  bool IsRegistered() {
    return _task->IsRegistered() || _state == CalloutState::kCalloutQueue || _state == CalloutState::kTaskQueue;
  }
protected:
  void HandleSub2(sptr<Callout> callout);
private:
  friend TaskCtrl;
  void SetHandler(sptr<Callout> callout, CpuId cpuid, int us);
  void Cancel();
  virtual void HandleSub(sptr<Callout> callout) {
    HandleSub2(callout);
  }
  CpuId _cpuid;
  sptr<Task> _task;
  uint64_t _time;
  sptr<Callout> _next;
  uptr<GenericFunction<>> _func;
  SpinLock _lock;
  bool _pending = false;
  CalloutState _state = CalloutState::kStopped;
};

class LckCallout : public Callout {
 public:
  LckCallout() {
  }
  virtual ~LckCallout() {
  }
  void SetLock(SpinLock *lock) {
    _lock = lock;
  }
 private:
  virtual void HandleSub(sptr<Callout> callout) {
    if (_lock != nullptr) {
      _lock->Lock();
    }
    HandleSub2(callout);
    if (_lock != nullptr) {
      _lock->Unlock();
    }
  }
  SpinLock *_lock = nullptr;
};
