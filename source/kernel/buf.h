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

#ifndef __RAPH_KERNEL_BUF_H__
#define __RAPH_KERNEL_BUF_H__
  
#include <stddef.h>
#include <raph.h>
#include <global.h>
#include <spinlock.h>
#include <task.h>
#include <functional.h>

// The characteristic of RingBuffer is it doesn't allocate memory dynamically.
//
// T should be primitive data type(int or pointer).
// If you want to contain struct (or class) in RingBuffer,
// you should allocate struct array to different place and
// manage this array by RingBuffer. In this case, T must be
// a pointer to the struct.
template<class T, int S>
class RingBuffer {
 public:
  RingBuffer() {
    _head = 0;
    _tail = 0;
  }
  virtual ~RingBuffer() {
  }
  // 満杯の時は何もせず、falseを返す
  bool Push(T data) {
    Locker locker(_lock);
    int ntail = (_tail + 1) % (S + 1);
    if (ntail != _head) {
      _buffer[_tail] = data;
      _tail = ntail;
      return true;
    } else {
      return false;
    }
  }
  // 空の時は何もせず、falseを返す
  bool Pop(T &data) {
    Locker locker(_lock);
    if (_head != _tail) {
      data = _buffer[_head];
      _head = (_head + 1) % (S + 1);
      return true;
    } else {
      return false;
    }
  }
  bool IsFull() {
    Locker locker(_lock);
    int ntail = (_tail + 1) % (S + 1);
    return (ntail == _head);
  }
  bool IsEmpty() {
    Locker locker(_lock);
    return (_tail == _head);
  }
  constexpr static int GetBufSize() {
    return S;
  }
 private:
  T _buffer[S + 1];
  int _head;
  int _tail;
  SpinLock _lock;
};

template<class T, int S>
  class FunctionalRingBuffer final : public Functional {
 public:
  FunctionalRingBuffer() {
  }
  ~FunctionalRingBuffer() {
  }
  bool Push(T data) {
    bool flag = _buf.Push(data);
    Functional::WakeupFunction();
    return flag;
  }
  bool Pop(T &data) {
    return _buf.Pop(data);
  }
  bool IsFull() {
    return _buf.IsFull();
  }
  bool IsEmpty() {
    return _buf.IsEmpty();
  }
 private:
  virtual bool ShouldFunc() override {
    return !_buf.IsEmpty();
  }
  RingBuffer<T, S> _buf;
};

#endif /* __RAPH_KERNEL_BUF_H__ */
