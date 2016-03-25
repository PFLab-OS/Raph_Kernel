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

#ifndef __RAPH_KERNEL_DEV_TASK_H__
#define __RAPH_KERNEL_DEV_POLLING_H__

#include <allocator.h>
#include <apic.h>
#include <mem/virtmem.h>
#include <global.h>

class Function {
 public:
  Function() {
  }
  void Init(void (*func)(void *), void *arg) {
    _func = func;
    _arg = arg;
  }
  void Execute() {
    if (_func != nullptr) {
      void (*func)(void *) = _func;
      _func = nullptr;
      func(_arg);
    }
  }
  volatile bool CanExecute() {
    return (_func != nullptr);
  }
  void Clear() {
    _func = nullptr;
  }
 private:
  void (*_func)(void *) = nullptr;
  void *_arg;
};

class TaskCtrl {
 public:
  TaskCtrl() {
    _task_list = nullptr;
  }
  void Setup() {
    int cpus = apic_ctrl->GetHowManyCpus();
    _task_list = reinterpret_cast<Allocator<Function> *>(virtmem_ctrl->Alloc(sizeof(Allocator<Function>) * cpus));
    for (int i = 0; i < cpus; i++) {
      new(&_task_list[i]) Allocator<Function>;
    }
  }
  void Run() {
  }
 private:
  Allocator<Function> *_task_list;
};


#endif /* __RAPH_KERNEL_DEV_TASK_H__ */
