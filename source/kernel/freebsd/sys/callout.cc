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

extern "C" {

  void callout_init_mtx(struct callout *c, struct mtx *mutex, int flags) {
    c->callout = new LckCallout();
    c->callout->SetLock(mutex->lock_object.lock);
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

  void callout_reset(struct callout *c, int ticks, void func(void *), void *arg) {
    c->callout->Cancel();
    if (ticks < 0) {
      ticks = 1;
    }
    Function f;
    f.Init(func, arg);
    c->callout->Init(f);
    //TODO cpuid
    c->callout->SetHandler(1, static_cast<uint32_t>(ticks) * reciprocal_of_hz);
  }
    
}
