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
 * Author: Levelfour
 * 
 */

#ifndef __KERNEL__

#include <thread.h>
#include <task.h>
#include <global.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

PthreadCtrl::~PthreadCtrl() {
  for(thread_pool_t::size_type i = 0; i < _thread_pool.size(); i++) {
    _thread_pool[i]->detach();
  }
}

void PthreadCtrl::Setup() {
  // main thread id
  _thread_ids[0] = GetThreadId();

  for(thread_pool_t::size_type i = 0; i < _thread_pool.size(); i++) {
    std::unique_ptr<std::thread> th(new std::thread ([this, i]{
        // sub thread id
        _thread_ids[i+1] = GetThreadId();

        task_ctrl->Run();
    }));
    _thread_pool[i] = std::move(th);
  }
}

volatile int PthreadCtrl::GetId() {
  for(int i = 0; i < _cpu_nums; i++) {
    if(_thread_ids[i] == GetThreadId()) {
      return i;
    }
  }
  return 0;
}

int PthreadCtrl::GetThreadId() {
  return syscall(SYS_gettid);
}

#endif // !__KERNEL__
