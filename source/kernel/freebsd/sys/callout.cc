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
#include <ptr.h>

extern "C" {

  struct LckCalloutContainer {
    sptr<LckCallout> callout;
    LckCalloutContainer() : callout(make_sptr(new LckCallout)) {
    }
  };
  
  void _callout_init_lock(struct callout *c, struct lock_object *lock, int flags) {
    c->callout_container = new LckCalloutContainer();
    c->callout_container->callout->SetLock(lock->lock);
  }

  int callout_stop(struct callout *c) {
    return task_ctrl->CancelCallout(c->callout_container->callout);
  }

  int callout_drain(struct callout *c) {
    int r = callout_stop(c);
    while(true) {
      volatile bool flag = c->callout_container->callout->IsHandling();
      if (!flag) {
        break;
      }
    }
    return r;
  }

  int callout_reset(struct callout *c, int ticks, void func(void *), void *arg) {
    bool pending = c->callout_container->callout->IsPending();
    
    task_ctrl->CancelCallout(c->callout_container->callout);
    if (ticks < 0) {
      ticks = 1;
    }
    c->callout_container->callout->Init(make_uptr(new Function<void *>(func, arg)));
    task_ctrl->RegisterCallout(c->callout_container->callout, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), static_cast<uint32_t>(ticks) * reciprocal_of_hz);

    return pending ? 1 : 0;
  }

  int callout_reset_sbt(struct callout *c, sbintime_t sbt, sbintime_t precision, void ftn(void *), void *arg, int flags) {
    bool pending = c->callout_container->callout->IsPending();
    
    task_ctrl->CancelCallout(c->callout_container->callout);
    if (sbt < 0) {
      sbt = 1;
    }
    c->callout_container->callout->Init(make_uptr(new Function<void *>(ftn, arg)));
    task_ctrl->RegisterCallout(c->callout_container->callout, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), sbt * static_cast<sbintime_t>(1000000) / SBT_1S);

    return pending ? 1 : 0;
  }

  int	callout_schedule(struct callout *c, int ticks) {
    bool pending = c->callout_container->callout->IsPending();
    
    task_ctrl->CancelCallout(c->callout_container->callout);
    if (ticks < 0) {
      ticks = 1;
    }
    task_ctrl->RegisterCallout(c->callout_container->callout, cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), static_cast<uint32_t>(ticks) * reciprocal_of_hz);

    return pending ? 1 : 0;
  }
    
}
