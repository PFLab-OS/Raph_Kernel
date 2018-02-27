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

#include <timer.h>
#include <global.h>
#include <raph.h>
#include <thread.h>
#include <cpu.h>

class Polling {
 protected:
  enum class PollingState {
    kPolling,
    kStopped,
  };
  Polling() {}
  ~Polling() { RemovePolling(); }
  void RegisterPolling(CpuId cpuid) {
    if (_state == PollingState::kPolling) {
      return;
    }
    _state = PollingState::kPolling;
    _thread = ThreadCtrl::GetCtrl(cpuid).AllocNewThread(
        Thread::StackState::kIndependent);
    _thread->CreateOperator().SetFunc(
        make_uptr(new ClassFunctionX1<void, Polling, void *>(
            this, &Polling::HandleSub, nullptr)));
    _thread->CreateOperator().Schedule();
  }
  void RemovePolling() {
    if (_state == PollingState::kStopped) {
      return;
    }
    _state = PollingState::kStopped;
    uptr<Thread> null_thread;
    _thread = null_thread;
  }
  virtual void Handle() = 0;

 private:
  void HandleSub(void *) {
    if (_state == PollingState::kPolling) {
      Handle();
      _thread->CreateOperator().Schedule();
    } else {
      RemovePolling();
    }
  }
  PollingState _state = PollingState::kStopped;
  uptr<Thread> _thread;
};

class PollingFunc : public Polling {
 public:
  void Register() { Register(cpu_ctrl->GetCpuId()); }
  void Register(CpuId cpuid) { this->RegisterPolling(cpuid); }
  void Remove() { this->RemovePolling(); }
  void Init(uptr<GenericFunction<>> func) { _func = func; }

 private:
  virtual void Handle() override { _func->Execute(); }
  uptr<GenericFunction<>> _func;
};
