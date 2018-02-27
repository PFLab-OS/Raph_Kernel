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

#include <sys/callout.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <thread.h>
#include <cpu.h>
#include <ptr.h>

extern "C" {

  // TODO release
  struct LckCalloutContainer {
    LckCalloutContainer() {
    }
    uptr<Thread> thread;
    SpinLock *lock;
  };

  static void _callout_handle(struct callout *c) {
    if (c->func != nullptr) {
      c->func(c->arg);
    }
  }
  
  void _callout_init_lock(struct callout *c, struct lock_object *lock, int flags) {
    c->callout_container = new LckCalloutContainer();
    c->callout_container->lock = lock->lock;
    c->callout_container->thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
    c->callout_container->thread->CreateOperator().SetFunc(make_uptr(new Function1<struct callout *>(_callout_handle, c)));
    c->func = nullptr;
  }

  int callout_stop(struct callout *c) {
    return (c->callout_container->thread->CreateOperator().Stop() == Thread::State::kWaitingInQueue) ? 1 : 0;
  }

  int callout_drain(struct callout *c) {
    int r = callout_stop(c);
    while(true) {
      if (c->callout_container->thread->CreateOperator().GetState() == Thread::State::kRunning) {
        break;
      }
      asm volatile("":::"memory");
    }
    return r;
  }

  int callout_reset(struct callout *c, int ticks, void func(void *), void *arg) {
    int r = (c->callout_container->thread->CreateOperator().Stop() == Thread::State::kWaitingInQueue) ? 1 : 0;
    if (ticks < 0) {
      ticks = 1;
    }
    c->func = func;
    c->arg = arg;
    c->callout_container->thread->CreateOperator().Schedule(static_cast<uint32_t>(ticks) * reciprocal_of_hz);

    return r;
  }

  int callout_reset_sbt(struct callout *c, sbintime_t sbt, sbintime_t precision, void ftn(void *), void *arg, int flags) {
    int r = (c->callout_container->thread->CreateOperator().Stop() == Thread::State::kWaitingInQueue) ? 1 : 0;
    
    if (sbt < 0) {
      sbt = 1;
    }
    c->func = ftn;
    c->arg = arg;
    c->callout_container->thread->CreateOperator().Schedule(sbt * static_cast<sbintime_t>(1000000) / SBT_1S);

    return r;
  }

  int	callout_schedule(struct callout *c, int ticks) {
    int r = (c->callout_container->thread->CreateOperator().Stop() == Thread::State::kWaitingInQueue) ? 1 : 0;
    
    if (ticks < 0) {
      ticks = 1;
    }
    c->callout_container->thread->CreateOperator().Schedule(static_cast<uint32_t>(ticks) * reciprocal_of_hz);

    return r;
  }
    
}
