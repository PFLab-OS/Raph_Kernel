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

class Function {
 public:
  Function() {
  }
  Function(const Function &func) {
    _func = func._func;
    _arg = func._arg;
  }
  void Init(void (*func)(void *), void *arg) {
    _func = func;
    _arg = arg;
  }
  void Execute() {
    if (_func != nullptr) {
      _func(_arg);
    }
  }
  volatile bool CanExecute() {
    return (_func != nullptr);
  }
  void Clear() {
    _func = nullptr;
  }
  bool Equal(const Function &func) {
    return (_func == func._func) && (_arg == func._arg);
  }
 private:
  void (*_func)(void *) = nullptr;
  void *_arg;
};

class Functional {
 public:
  enum class FunctionState {
    kFunctioning,
    kNotFunctioning,
  };
  Functional() {
  }
  virtual ~Functional() {
  }
  void SetFunction(int apicid, const Function &func);
 protected:
  void WakeupFunction();
  // 設定された関数を呼び出すべきかどうかを返す
  virtual bool ShouldFunc() = 0;
 private:
  static void Handle(void *p);
  Function _func;
  int _apicid = 0;
  SpinLock _lock;
  FunctionState _state = FunctionState::kNotFunctioning;
};

#endif // __RAPH_KERNEL_FUNCTIONAL_H__
