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

#include <oqueue.h>
#include "test.h"
#include <iostream>
#include <pthread.h>
#include <thread>
#include <exception>
#include <stdexcept>
using namespace std;

class OqElement : public OrderedQueueContainer<OqElement, int> {
public:
  OqElement() : OrderedQueueContainer<OqElement, int>(this) {
  }
private:
};

class OrderedQueueTester_EmptyPop : public Tester {
public:
  virtual bool Test() override {
    OqElement *ele;
    kassert(_queue.Pop(ele) == false);
    return true;
  }
private:
  OrderedQueue<OqElement, int> _queue;
} static OBJ(__LINE__);

class OrderedQueueTester_SinglePushPop : public Tester {
public:
  virtual bool Test() override {
    OqElement ele1;
    _queue.Push(&ele1, 0);
    OqElement *ele = nullptr;
    kassert(_queue.GetTopOrder() == 0);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele1);
    kassert(!_queue.Pop(ele));
    return true;
  }
private:
  OrderedQueue<OqElement, int> _queue;
} static OBJ(__LINE__);

class OrderedQueueTester_OrderedMultiPushPop : public Tester {
public:
  virtual bool Test() override {
    OqElement ele1, ele2, ele3;
    _queue.Push(&ele1, 0);
    _queue.Push(&ele2, 1);
    _queue.Push(&ele3, 2);
    OqElement *ele;
    kassert(_queue.GetTopOrder() == 0);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele1);
    kassert(_queue.GetTopOrder() == 1);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele2);
    kassert(_queue.GetTopOrder() == 2);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele3);
    kassert(!_queue.Pop(ele));
    return true;
  }
private:
  OrderedQueue<OqElement, int> _queue;
} static OBJ(__LINE__);

class OrderedQueueTester_NonOrderedMultiPushPop : public Tester {
public:
  virtual bool Test() override {
    OqElement ele1, ele2, ele3, ele4, ele5;
    _queue.Push(&ele1, 3);
    _queue.Push(&ele2, 2);
    _queue.Push(&ele3, 0);
    _queue.Push(&ele4, 1);
    _queue.Push(&ele5, 4);
    OqElement *ele;
    kassert(_queue.GetTopOrder() == 0);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele3);
    kassert(_queue.GetTopOrder() == 1);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele4);
    kassert(_queue.GetTopOrder() == 2);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele2);
    kassert(_queue.GetTopOrder() == 3);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele1);
    kassert(_queue.GetTopOrder() == 4);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele5);
    kassert(!_queue.Pop(ele));
    return true;
  }
private:
  OrderedQueue<OqElement, int> _queue;
} static OBJ(__LINE__);

class OrderedQueueTester_ParallelPop : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum + 1];
    std::exception_ptr ep[kThreadNum + 1];
    _ele = new OqElement[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&OrderedQueueTester_ParallelPop::Producer, this, &ep[i], i);
      threads[i].swap(th);
    }
    std::thread th(&OrderedQueueTester_ParallelPop::Consumer, this, &ep[kThreadNum]);
    threads[kThreadNum].swap(th);
    
    bool rval = true;
    for (int i = 0; i < kThreadNum; i++) {
      threads[i].join();
      try {
        if (ep[i]) {
          _error = true;
          std::rethrow_exception(ep[i]);
        }
      } catch (ExceptionAssertionFailure t) {
        t.Show();
        rval = false;
      };
    }

    asm volatile("":::"memory");
    
    threads[kThreadNum].join();
    try {
      if (ep[kThreadNum]) {
        _error = true;
        std::rethrow_exception(ep[kThreadNum]);
      }
    } catch (ExceptionAssertionFailure t) {
      t.Show();
      rval = false;
    };
    kassert(_queue.IsEmpty());
    kassert(_push_cnt == kThreadNum * kElementNum);
    kassert(_pop_cnt == kThreadNum * kElementNum);
    delete[] _ele;
    return rval;
  }
private:
  void Consumer(std::exception_ptr *ep) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum + 1) {
        asm volatile("":::"memory");
      }
      for(int i = 0; i < kElementNum; i++) {
        for(int j = 0; j < kThreadNum; j++) {
          OqElement *ele;
          while(_queue.IsEmpty()) {
          }
          kassert(_queue.Pop(ele));
          __sync_fetch_and_add(&_pop_cnt, 1);
          if (_error) {
            break;
          }
        }
        if (_error) {
          break;
        }
        _flag2++;
      }
    } catch (...) {
      *ep = std::current_exception();
    }
  }
  void Producer(std::exception_ptr *ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum + 1) {
        asm volatile("":::"memory");
      }
      for (int i = 0; i < kElementNum; i++) {
        while(_flag2 < i) {
          asm volatile("":::"memory");
        }
        _queue.Push(&_ele[id * kElementNum + i], id * kElementNum + i);
        __sync_fetch_and_add(&_push_cnt, 1);
      }
    } catch (...) {
      *ep = std::current_exception();
    }
  }
  
  OrderedQueue<OqElement, int> _queue;
  OqElement *_ele;
  int _push_cnt = 0;
  int _pop_cnt = 0;
  bool _error = false;
  int _flag1 = 0;
  int _flag2 = 0;
  static const int kElementNum = 10;
  static const int kThreadNum = 50;
} static OBJ(__LINE__);
