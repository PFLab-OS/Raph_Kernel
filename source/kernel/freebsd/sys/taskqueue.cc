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

#include "taskqueue.h"
#include <raph.h>
#include <cpu.h>
#include <ptr.h>
#include "thread-raph.h"

extern "C" {

  struct taskqueue *taskqueue_fast = nullptr;
  
  static void __taskqueue_handle(struct task *t) {
    t->ta_func(t->ta_context, t->ta_pending);
    t->ta_pending++;
  }

  void _task_init(struct task *t, int priority, task_fn_t *func, void *context) {
    t->ta_thread = new CountableThread;
    t->ta_thread->Init(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority));
    t->ta_thread->SetFunc(make_uptr(new Function<struct task *>(__taskqueue_handle, t)));
    t->ta_pending = 0;
    t->ta_func = (func);
    t->ta_context = (context);
  }

  int taskqueue_enqueue(struct taskqueue *, struct task *task) {
    task->ta_thread->Inc();
    return 0;
  }

  void taskqueue_drain(struct taskqueue *, struct task *task) {
    while(task->ta_thread->GetState() == Thread::State::kRunning) {
      asm volatile("":::"memory");
    }
  }
}
