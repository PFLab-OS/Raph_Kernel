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

#ifndef __RAPH_KERNEL_RAPHQUEUE_H__
#define __RAPH_KERNEL_RAPHQUEUE_H__

#include <functional.h>

class Queue {
 public:
  Queue() {
    _last = &_first;
    _first.data = nullptr;
    _first.next = nullptr;
  }
  virtual ~Queue() {
  }
  void Push(void *data);
  // 空の時はfalseが帰る
  bool Pop(void *&data);
  bool IsEmpty() {
    return &_first == _last;
  }
 private:
  struct Container {
    void *data;
    Container *next;
  };
  Container _first;
  Container *_last;
  SpinLock _lock;
};

class FunctionalQueue final : public Functional {
 public:
  FunctionalQueue() {
  }
  ~FunctionalQueue() {
  }
  void Push(void *data) {
    _queue.Push(data);
    WakeupFunction();
  }
  bool Pop(void *&data) {
    return _queue.Pop(data);
  }
  bool IsEmpty() {
    return _queue.IsEmpty();
  }
 private:
  virtual bool ShouldFunc() override {
    return !_queue.IsEmpty();
  }
  Queue _queue;
};

#endif // __RAPH_KERNEL_RAPHQUEUE_H__
