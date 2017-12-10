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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * 
 */

#pragma once

#include <ptr.h>
#include <thread.h>

class CountableThread {
public:
  CountableThread() {
  }
  void Init(CpuId cpuid) {
    _thread = ThreadCtrl::GetCtrl(cpuid).AllocNewThread(Thread::StackState::kShared);
    _thread->CreateOperator().SetFunc(make_uptr(new ClassFunction<CountableThread, void *>(this, &CountableThread::Handle, nullptr)));
  }
  void SetFunc(uptr<GenericFunction<>> func) {
    _func = func;
  }
  void Inc() {
    if (__sync_fetch_and_add(&_count, 1) == 0) {
      _thread->CreateOperator().Schedule();
    }
  }
  Thread::State GetState() {
    return _thread->CreateOperator().GetState();
  }
  Thread::State Stop() {
    return _thread->CreateOperator().Stop();
  }
  int GetCnt() {
    return _count;
  }
private:
  void Handle(void *) {
    _func->Execute();
    if (__sync_fetch_and_sub(&_count, 1) > 1) {
      _thread->CreateOperator().Schedule();
    }
  }
  
  int _count = 0;
  uptr<Thread> _thread;
  uptr<GenericFunction<>> _func;
};
