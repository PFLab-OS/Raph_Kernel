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

#ifndef __RAPH_KERNEL_DEV_POLLING_H__
#define __RAPH_KERNEL_DEV_POLLING_H__
#include <timer.h>
#include <global.h>
#include <raph.h>
#include <task.h>

class Polling {
 protected:
  void RegisterPolling() {
    Function func;
    func.Init(HandleSub, reinterpret_cast<void *>(this));
    _poll = true;
    task_ctrl->Register(func);
  }
  // pollingを登録したprocessorで実行する事
  void RemovePolling() {
    Function func;
    func.Init(HandleSub, reinterpret_cast<void *>(this));
    _poll = false;
    task_ctrl->Remove(func);
  }
  virtual void Handle() = 0;
 private:
  static void HandleSub(void *p) {
    Polling *that = reinterpret_cast<Polling *>(p);
    that->Handle();
    if (that->_poll) {
      Function func;
      func.Init(HandleSub, p);
      task_ctrl->Register(func);
    }
  }
  bool _poll = false;
};

class PollingFunc : public Polling {
 public:
  void RegisterFunc(void (*f)()) {
    _f = f;
    this->RegisterPolling();
  }
  void RemoveFunc() {
    this->RemovePolling();
  }
 private:
  virtual void Handle() override {
    if (_f != nullptr) {
      _f();
    }
  }
  void (*_f)() = nullptr;
};

#endif /* __RAPH_KERNEL_DEV_POLLING_H__ */
