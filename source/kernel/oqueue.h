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
#include <spinlock2.h>

// !!!Important!!!
// Popping from multiple threads is prohibited!
// do not use inside interrupt handlers
// TODO assert these restrictions
template<class T, class U>
class OrderedQueue {
 public:
  class Container {
  public:
    // obj must be set master class
    Container(T *obj) {
      _obj = obj;
    }
    Container() = delete;
  private:
    friend OrderedQueue<T, U>;
    Container *_next;
    T *_obj;
    U _order;
    enum class Status {
      kQueued,
      kOutOfQueue,
    } _status = Status::kOutOfQueue;
  };
  OrderedQueue() {
  }
  virtual ~OrderedQueue() {
  }
  void Push(T *data, U order);
  // return false when the queue is empty
  bool Pop(T *&data) __attribute__((warn_unused_result));
  bool IsEmpty() {
    return _first == nullptr;
  }
  // should not call when empty
  U GetTopOrder() {
    Container *c = _first;
    return (c == nullptr) ? 0 : c->_order;
  }
private:
  Container *_first = nullptr;
  SpinLock2 _lock;
};

template <class T, class U>
void OrderedQueue<T, U>::Push(T *data, U order) {
  Container *c = data;
  kassert(c->_status == Container::Status::kOutOfQueue);
  c->_status = Container::Status::kQueued;
  c->_order = order;

  Locker locker(_lock);
  if (_first == nullptr) {
    c->_next = nullptr;
    _first = c;
    return;
  }
  if (_first->_order > order) {
    c->_next = _first;
    _first = c;
    return;
  }
  Container *d = _first;
  while(d->_next != nullptr && d->_next->_order <= order) {
    d = d->_next;
  }
  c->_next = d->_next;
  d->_next = c;
}

template<class T, class U>
bool OrderedQueue<T, U>::Pop(T *&data) {
  Locker locker(_lock);
  Container *c = _first;
  if (c == nullptr) {
    return false;
  }
  _first = c->_next;
  
  kassert(c->_status == Container::Status::kQueued);
  c->_status = Container::Status::kOutOfQueue;
  data = c->_obj;
  return true;
}
