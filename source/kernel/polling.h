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

#ifndef __RAPH_KERNEL_DEV_POLLING_H__
#define __RAPH_KERNEL_DEV_POLLING_H__
#include <timer.h>
#include <global.h>
#include <raph.h>
#include <task.h>
#include <cpu.h>

class Polling {
 protected:
  enum class PollingState {
    kPolling,
    kStopped,
  };
  Polling() {
    ClassFunction<Polling> func;
    func.Init(this, &Polling::HandleSub, nullptr);
    _task.SetFunc(func);
  }
  void RegisterPolling(CpuId cpuid) {
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
    _state = PollingState::kStopped;
  }
  virtual void Handle() = 0;
 private:
  void HandleSub(void *) {
    if (_state == PollingState::kPolling) {
      Handle();
      task_ctrl->Register(_cpuid, &_task);
    } else {
      RemovePolling();
    }
  }
  PollingState _state = PollingState::kStopped;
  CpuId _cpuid;
  Task _task;
};

class PollingFunc : public Polling {
 public:
  void Register() {
    Register(cpu_ctrl->GetCpuId());
  }
  void Register(CpuId cpuid) {
    this->RegisterPolling(cpuid);
  }
  void Remove() {
    this->RemovePolling();
  }
  void Init(const GenericFunction &func) {
    _func.Copy(func);
  }
 private:
  virtual void Handle() override {
    _func.Execute();
  }
  FunctionBase _func;
};

#endif /* __RAPH_KERNEL_DEV_POLLING_H__ */
