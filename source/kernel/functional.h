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

#include <task.h>
#include <functional.h>
#include <raph.h>
#include <_cpu.h>

class Functional {
 public:
  enum class FunctionState {
    kFunctioning,
    kNotFunctioning,
  };
  Functional() : _task(new Task) {
    _task->SetFunc(make_uptr(new ClassFunction<Functional, void *>(this, &Functional::Handle, nullptr)));
  }
  virtual ~Functional() {
  }
  void SetFunction(CpuId cpuid, uptr<GenericFunction<>> func);
 protected:
  void WakeupFunction();
  // check whether Functional needs to process function
  virtual bool ShouldFunc() = 0;
private:
  void Handle(void *);
  uptr<GenericFunction<>> _func;
  sptr<Task> _task;
  CpuId _cpuid;
  SpinLock _lock;
  FunctionState _state = FunctionState::kNotFunctioning;
};

inline void Functional::WakeupFunction() {
  if (!_cpuid.IsValid()) {
    // not initialized
    return;
  }

  bool register_flag = false;
  
  {
    Locker locker(_lock);
    if (_state == FunctionState::kNotFunctioning) {
      register_flag = true;
      _state = FunctionState::kFunctioning;
    }
  }

  if (register_flag) {
    task_ctrl->Register(_cpuid, _task);
  }
}

inline void Functional::Handle(void *) {
  for (int i = 0; i < 10; i++) {
    // not to loop infidentry
    // it will lock cpu and inhibit other tasks
    
    if (!ShouldFunc()) {
      break;
    }
    _func->Execute();
  }

  assert(_state == FunctionState::kFunctioning);

  bool register_flag = false;
  
  {
    Locker locker(_lock);
    if (ShouldFunc()) {
      register_flag = true;
      _state = FunctionState::kFunctioning;
    } else {
      _state = FunctionState::kNotFunctioning;
    }
  }

  if (register_flag) {
    task_ctrl->Register(_cpuid, _task);
  }
}

inline void Functional::SetFunction(CpuId cpuid, uptr<GenericFunction<>> func) {
  _cpuid = cpuid;
  _func = func;
}

