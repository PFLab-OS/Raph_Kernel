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

#ifndef __RAPH_KERNEL_FUNCTIONAL_H__
#define __RAPH_KERNEL_FUNCTIONAL_H__

#include <spinlock.h>
#include <task.h>

class Functional {
 public:
  enum class FunctionState {
    kFunctioning,
    kNotFunctioning,
  };
  Functional() {
    Function func;
    func.Init(Handle, reinterpret_cast<void *>(this));
    _task.SetFunc(func);
  }
  virtual ~Functional() {
  }
  void SetFunction(int cpuid, const GenericFunction &func);
 protected:
  void WakeupFunction();
  // 設定された関数を呼び出すべきかどうかを返す
  virtual bool ShouldFunc() = 0;
 private:
  static void Handle(void *p);
  FunctionBase _func;
  Task _task;
  int _cpuid = 0;
  SpinLock _lock;
  FunctionState _state = FunctionState::kNotFunctioning;
};

#endif // __RAPH_KERNEL_FUNCTIONAL_H__
