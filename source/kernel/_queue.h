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
#include <ptr.h>

template<class T>
class QueueBase;
template<class T>
class QueueContainer;

template<class T>
class Queue : public QueueBase<T> {
public:
  void Push(T *data) {
    QueueContainer<T> *c = data;
    QueueBase<T>::Push(c);  
  }
  // return false when the queue is empty
  bool Pop(T *&data) __attribute__((warn_unused_result)) {
    QueueContainer<T> *ct;
    if (QueueBase<T>::Pop(ct)) {
      data = ct->_obj;
      return true;
    } else {
      return false;
    }
  }
private:
};

template<class U>
class Queue<uptr<U>> : public QueueBase<U> {
public:
  void Push(uptr<U> data) {
    QueueContainer<U> *c = data.GetRawPtr();
    data.Release();
    QueueBase<U>::Push(c);
  }
  // return false when the queue is empty
  uptr<U> Pop() {
    QueueContainer<U> *ct;
    if (QueueBase<U>::Pop(ct)) {
      return make_uptr(ct->_obj);
    } else {
      uptr<U> ptr;
      return ptr;
    }
  }
private:
};

template<class T>
class QueueContainer {
public:
  // obj must be set master class
  QueueContainer(T *obj) {
    _obj = obj;
  }
  QueueContainer() = delete;
private:
  friend Queue<T>;
  friend Queue<uptr<T>>;
  friend QueueBase<T>;
  QueueContainer *_next;
  T *_obj;
  enum class Status {
    kQueued,
    kOutOfQueue,
  } _status = Status::kOutOfQueue;
};

// class T must inherit QueueContainer
// !!!Important!!!
// Popping from interrupt handlers is prohibited!
// TODO assert this restrictions
template<class T>
class QueueBase {
public:
  static_assert(IsBaseOf<QueueContainer<T>, T>::value, "T of Queue<T> must be child of QueueContainer<T>");
  bool IsEmpty() {
    return _push_first == nullptr;
  }
protected:
  void Push(QueueContainer<T> *ct);
  // return false when the queue is empty
  bool Pop(QueueContainer<T> *&ct) __attribute__((warn_unused_result));
private:
  class PopQueueContainer {
  public:
    QueueContainer<T> *_ptr = nullptr;
    PopQueueContainer *_next = nullptr;
    int _flag = 0;
  };
  QueueContainer<T> *_push_first = nullptr;
  QueueContainer<T> *_push_last = nullptr;
  PopQueueContainer *_pop_last = nullptr;
  int _cnt = 0;
};

template<class T>
void QueueBase<T>::Push(QueueContainer<T> *ct) {
  kassert(ct->_status == QueueContainer<T>::Status::kOutOfQueue);
  ct->_status = QueueContainer<T>::Status::kQueued;
  ct->_next = nullptr;

  bool iflag = disable_interrupt();
  QueueContainer<T> *pred = __sync_lock_test_and_set(&_push_last, ct);
  if (pred == nullptr) {
    while(_push_first != nullptr) {
      asm volatile("":::"memory");
    }
    _push_first = ct;
  } else {
    pred->_next = ct;
  }
  enable_interrupt(iflag);
  
  __sync_fetch_and_add(&_cnt, 1);
}

template <class T>
bool QueueBase<T>::Pop(QueueContainer<T> *&ct) {
  if (_cnt <= 0) {
    return false;
  }

  if (__sync_fetch_and_sub(&_cnt, 1) <= 0) {
    __sync_fetch_and_add(&_cnt, 1);
    return false;
  }

  bool iflag = disable_interrupt();

  QueueContainer<T> *c;

  PopQueueContainer pc;
  PopQueueContainer *ppred = __sync_lock_test_and_set(&_pop_last, &pc);
  if (ppred != nullptr) {
    ppred->_next = &pc;
    while(pc._ptr == nullptr) {
      asm volatile("":::"memory");
    }
    c = pc._ptr;
  } else {
    c = _push_first;
  }

  kassert(c != nullptr);

  bool next_flag = false;
  
  if (pc._next != nullptr) {
    if (pc._flag == 0) {
      next_flag = true;
    }
  } else {
    kassert(pc._flag == 0);
    if (_push_last == c && __sync_bool_compare_and_swap(&_push_last, c, nullptr)) {
      kassert(c->_next == nullptr);
      _push_first = nullptr;
    } else {
      while(c->_next == nullptr) {
        asm volatile("":::"memory");
      }
      _push_first = c->_next;
    }

    if (_pop_last != &pc || !__sync_bool_compare_and_swap(&_pop_last, &pc, nullptr)) {
      while(pc._next == nullptr) {
        asm volatile("":::"memory");
      }
      next_flag = true;
    } else {
      kassert(pc._next == nullptr);
    }
  }

  if (next_flag) {
    PopQueueContainer *pc_cur = pc._next;
    QueueContainer<T> *c_cur = c->_next;
    while(pc_cur != nullptr) {
      kassert(c_cur != nullptr);
      PopQueueContainer *pc_tmp = pc_cur->_next;
      QueueContainer<T> *c_tmp = c_cur->_next;
      if (pc_tmp != nullptr) {
        pc_cur->_flag = 1;
      }
      pc_cur->_ptr = c_cur;
      pc_cur = pc_tmp;
      c_cur = c_tmp;
    }
  }

  enable_interrupt(iflag);
  
  kassert(c->_status == QueueContainer<T>::Status::kQueued);
  c->_status = QueueContainer<T>::Status::kOutOfQueue;
  ct = c;
  return true;
}

