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

#pragma once

#include <raph.h>

// TODO replace Queue & Queue2 to IntQueue
// class T must contain Container & inherit ContainerInterface
// !!!Important!!!
// Popping from multiple threads is prohibited!
// Popping from interrupt handlers is prohibited!
// TODO assert these restrictions
template<class T>
class IntQueue {
 public:
  class Container {
  public:
    // obj must be set master class
    Container(T *obj) {
      _obj = obj;
    }
    Container() = delete;
  private:
    friend IntQueue<T>;
    Container *_next;
    T *_obj;
    enum class Status {
      kQueued,
      kOutOfQueue,
    } _status = Status::kOutOfQueue;
  };
  class ContainerInterface {
  public:
    virtual Container *GetContainer() = 0;
  };
  IntQueue() {
  }
  virtual ~IntQueue() {
  }
  void Push(T *data);
  // return false when the queue is empty
  bool Pop(T *&data) __attribute__((warn_unused_result));
  bool IsEmpty() {
    return _first == nullptr;
  }
private:
  Container *_first = nullptr;
  Container *_last = nullptr;
};

template <class T>
void IntQueue<T>::Push(T *data) {
  ContainerInterface *i = data;
  Container *c = i->GetContainer();
  kassert(c->_status == Container::Status::kOutOfQueue);
  c->_status = Container::Status::kQueued;
  c->_next = nullptr;

  Container *pred = __sync_lock_test_and_set(&_last, c);
  bool iflag = disable_interrupt();
  if (pred == nullptr) {
    while(_first != nullptr) {
      asm volatile("":::"memory");
    }
    _first = c;
  } else {
    pred->_next = c;
  }
  enable_interrupt(iflag);
}

template<class T>
bool IntQueue<T>::Pop(T *&data) {
  Container *c = _first;
  if (c == nullptr) {
    return false;
  }

  bool iflag = disable_interrupt();
  if (c->_next == nullptr && _last == c && __sync_bool_compare_and_swap(&_last, c, nullptr)) {
    _first = nullptr;
  } else {
    while(c->_next == nullptr) {
      asm volatile("":::"memory");
    }
    _first = c->_next;
  }
  enable_interrupt(iflag);
  
  kassert(c->_status == Container::Status::kQueued);
  c->_status = Container::Status::kOutOfQueue;
  data = c->_obj;
  return true;
}
