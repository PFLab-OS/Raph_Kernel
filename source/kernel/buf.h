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

#include <_buf.h>
#include <functional.h>

template <class T, int S>
class FunctionalRingBuffer final : public Functional {
 public:
  FunctionalRingBuffer() {}
  ~FunctionalRingBuffer() {}
  bool Push(T data) __attribute__((warn_unused_result)) {
    bool flag = _buf.Push(data);
    Functional::WakeupFunction();
    return flag;
  }
  bool Pop(T &data) __attribute__((warn_unused_result)) {
    return _buf.Pop(data);
  }
  bool IsFull() { return _buf.IsFull(); }
  bool IsEmpty() { return _buf.IsEmpty(); }

 private:
  virtual bool ShouldFunc() override { return !_buf.IsEmpty(); }
  RingBuffer<T, S> _buf;
};
