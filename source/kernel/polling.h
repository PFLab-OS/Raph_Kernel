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
  Polling() {
    Function func;
    func.Init(HandleSub, reinterpret_cast<void *>(this));
    _task.SetFunc(func);
  }
  void RegisterPolling(int cpuid) {
    if (_state == PollingState::kPolling) {
      return;
    }
    _cpuid = cpuid;
    _state = PollingState::kPolling;
    task_ctrl->Register(_cpuid, &_task);
  }
  void RemovePolling() {
    if (_state == PollingState::kStopped) {
      return;
    }
    Function func;
    func.Init(HandleSub, reinterpret_cast<void *>(this));
    _state = PollingState::kStopped;
    // task_ctrl->Remove(_cpuid, &_task);
  }
  virtual void Handle() = 0;
 private:
  static void HandleSub(void *p) {
    Polling *that = reinterpret_cast<Polling *>(p);
    if (that->_state == PollingState::kPolling) {
      that->Handle();
      task_ctrl->Register(that->_cpuid, &that->_task);
    } else {
      that->RemovePolling();
    }
  }
  PollingState _state = PollingState::kStopped;
  int _cpuid = -1;
  Task _task;
};

class PollingFunc : public Polling {
 public:
  void Register() {
    Register(apic_ctrl->GetCpuId());
  }
  void Register(int cpuid) {
    this->RegisterPolling(cpuid);
  }
  void Remove() {
    this->RemovePolling();
  }
  void Init(const Function &func) {
    _func = func;
  }
 private:
  virtual void Handle() override {
    _func.Execute();
  }
  Function _func;
};

#endif /* __RAPH_KERNEL_DEV_POLLING_H__ */
