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
#include <apic.h>

class Polling {
 protected:
  enum class PollingState {
    kPolling,
    kStopped,
  };
  void RegisterPolling() {
    if (_state == PollingState::kPolling) {
      return;
    }
    Function func;
    func.Init(HandleSub, reinterpret_cast<void *>(this));
    _state = PollingState::kPolling;
    task_ctrl->RegisterPolling(_apicid, func);
  }
  // pollingを登録したprocessorで実行する事
  void RemovePolling() {
    if (_state == PollingState::kStopped) {
      return;
    }
    Function func;
    func.Init(HandleSub, reinterpret_cast<void *>(this));
    _state = PollingState::kStopped;
    task_ctrl->Remove(_apicid, func);
  }
  virtual void Handle() = 0;
  int _apicid = 0;
 private:
  static void HandleSub(void *p) {
    Polling *that = reinterpret_cast<Polling *>(p);
    if (that->_state == PollingState::kPolling) {
      that->Handle();
    } else {
      that->RemovePolling();
    }
  }
  PollingState _state = PollingState::kStopped;
};

class PollingFunc : public Polling {
 public:
  void Register() {
    this->RegisterPolling();
  }
  void Remove() {
    this->RemovePolling();
  }
  void Init(void (*func)(void *), void *arg) {
    _apicid = apic_ctrl->GetApicId();
    _func.Init(func, arg);
  }
 private:
  virtual void Handle() override {
    _func.Execute();
  }
  Function _func;
};

#endif /* __RAPH_KERNEL_DEV_POLLING_H__ */
