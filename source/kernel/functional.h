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

#pragma once

#include <thread.h>
#include <raph.h>
#include <_cpu.h>

class Functional {
 public:
  enum class FunctionState {
    kFunctioning,
    kNotFunctioning,
  };
  Functional() {}
  virtual ~Functional() {}
  void SetFunction(CpuId cpuid, uptr<GenericFunction<>> func) {
    _thread =
        ThreadCtrl::GetCtrl(cpuid).AllocNewThread(Thread::StackState::kShared);
    _thread->CreateOperator().SetFunc(
        make_uptr(new ClassFunction<Functional, void *>(
            this, &Functional::Handle, nullptr)));
    _func = func;
  }
  void Block() {
    kassert(!_block_flag);
    _block_flag = true;
  }
  void UnBlock() {
    kassert(_block_flag);
    _block_flag = false;
    if (ShouldFunc()) {
      WakeupFunction();
    }
  }

 protected:
  void WakeupFunction();
  // check whether Functional needs to process function
  virtual bool ShouldFunc() = 0;

 private:
  void Handle(void *);
  uptr<GenericFunction<>> _func;
  uptr<Thread> _thread;
  FunctionState _state = FunctionState::kNotFunctioning;
  bool _block_flag = false;
};

inline void Functional::WakeupFunction() {
  if (!_thread.IsNull()) {
    if (__sync_lock_test_and_set(&_state, FunctionState::kFunctioning) ==
        FunctionState::kNotFunctioning) {
      _thread->CreateOperator().Schedule();
    }
  }
}

inline void Functional::Handle(void *) {
  for (int i = 0; i < 10; i++) {
    // not to loop infidentry
    // it will lock cpu and block other tasks

    if (!ShouldFunc() || _block_flag) {
      break;
    }
    _func->Execute();
  }

  kassert(_state == FunctionState::kFunctioning);

  if (ShouldFunc() && !_block_flag) {
    _thread->CreateOperator().Schedule();
  } else {
    kassert(__sync_lock_test_and_set(&_state, FunctionState::kNotFunctioning) ==
            FunctionState::kFunctioning);
  }
}
