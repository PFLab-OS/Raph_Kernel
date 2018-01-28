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

#include "_queue.h"
#include <functional.h>
#include <ptr.h>

template <class T>
class FunctionalQueue final : public Functional {
 public:
  FunctionalQueue() {}
  ~FunctionalQueue() {}
  void Push(T *data) {
    _queue.Push(data);
    WakeupFunction();
  }
  bool Pop(T *&data) { return _queue.Pop(data); }
  bool IsEmpty() { return _queue.IsEmpty(); }

 private:
  virtual bool ShouldFunc() override { return !_queue.IsEmpty(); }
  Queue<T> _queue;
};

template <class U>
class FunctionalQueue<uptr<U>> final : public Functional {
 public:
  FunctionalQueue() {}
  ~FunctionalQueue() {}
  void Push(uptr<U> data) {
    _queue.Push(data);
    WakeupFunction();
  }
  uptr<U> Pop() { return _queue.Pop(); }
  bool IsEmpty() { return _queue.IsEmpty(); }

 private:
  virtual bool ShouldFunc() override { return !_queue.IsEmpty(); }
  Queue<uptr<U>> _queue;
};
