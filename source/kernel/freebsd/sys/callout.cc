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
#include <task.h>
#include <cpu.h>

extern "C" {

  void _callout_init_lock(struct callout *c, struct lock_object *lock, int flags) {
    c->callout = new LckCallout();
    c->callout->SetLock(lock->lock);
  }

  int callout_stop(struct callout *c) {
    bool flag = c->callout->CanExecute();
    c->callout->Cancel();
    return flag ? 1 : 0;
  }

  int callout_drain(struct callout *c) {
    int r = callout_stop(c);
    while(true) {
      volatile bool flag = c->callout->IsHandling();
      if (!flag) {
        break;
      }
    }
    return r;
  }

  int callout_reset(struct callout *c, int ticks, void func(void *), void *arg) {
    bool pending = c->callout->IsPending();
    
    c->callout->Cancel();
    if (ticks < 0) {
      ticks = 1;
    }
    Function<void> f;
    f.Init(func, arg);
    c->callout->Init(f);
    c->callout->SetHandler(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), static_cast<uint32_t>(ticks) * reciprocal_of_hz);

    return pending ? 1 : 0;
  }

  int callout_reset_sbt(struct callout *c, sbintime_t sbt, sbintime_t precision, void ftn(void *), void *arg, int flags) {
    bool pending = c->callout->IsPending();
    
    c->callout->Cancel();
    if (sbt < 0) {
      sbt = 1;
    }
    Function<void> f;
    f.Init(ftn, arg);
    c->callout->Init(f);
    c->callout->SetHandler(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), sbt * static_cast<sbintime_t>(1000000) / SBT_1S);

    return pending ? 1 : 0;
  }

  int	callout_schedule(struct callout *c, int ticks) {
    bool pending = c->callout->IsPending();
    
    c->callout->Cancel();
    if (ticks < 0) {
      ticks = 1;
    }
    c->callout->SetHandler(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), static_cast<uint32_t>(ticks) * reciprocal_of_hz);

    return pending ? 1 : 0;
  }
    
}
