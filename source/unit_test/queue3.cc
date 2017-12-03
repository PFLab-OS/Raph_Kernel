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

#include <_queue3.h>
#include "test.h"
#include <iostream>
#include <pthread.h>
#include <thread>
#include <exception>
#include <stdexcept>
using namespace std;

class Element : public IntQueue<Element>::ContainerInterface {
public:
  Element() : _container(this) {
  }
  IntQueue<Element>::Container *GetContainer() {
    return &_container;
  }
private:
  IntQueue<Element>::Container _container;
};

class IntQueueTester_EmptyPop : public Tester {
public:
  virtual bool Test() override {
    Element *ele;
    return _queue.Pop(ele) == false;
  }
private:
  IntQueue<Element> _queue;
} static OBJ(__LINE__);

class IntQueueTester_SinglePushPop : public Tester {
public:
  virtual bool Test() override {
    Element ele1;
    _queue.Push(&ele1);
    Element *ele = nullptr;
    return (_queue.Pop(ele) && ele == &ele1);
  }
private:
  IntQueue<Element> _queue;
} static OBJ(__LINE__);

class IntQueueTester_TriplePushPop : public Tester {
public:
  virtual bool Test() override {
    Element ele1, ele2, ele3;
    _queue.Push(&ele1);
    _queue.Push(&ele2);
    _queue.Push(&ele3);
    Element *ele;
    return (_queue.Pop(ele) && ele == &ele1 && _queue.Pop(ele) && ele == &ele2 && _queue.Pop(ele) && ele == &ele3);
  }
private:
  IntQueue<Element> _queue;
} static OBJ(__LINE__);

class IntQueueTester_PushPopThenEmptyPop : public Tester {
public:
  virtual bool Test() override {
    Element ele1, ele2, ele3;
    _queue.Push(&ele1);
    _queue.Push(&ele2);
    _queue.Push(&ele3);
    Element *ele;
    _queue.Pop(ele);
    _queue.Pop(ele);
    _queue.Pop(ele);
    return _queue.Pop(ele) == false;
  }
private:
  IntQueue<Element> _queue;
} static OBJ(__LINE__);

class IntQueueTester_ParallelPush : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum];
    std::exception_ptr ep[kThreadNum];
    _ele = new Element[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&IntQueueTester_ParallelPush::Producer, this, ep[i], i);
      threads[i].swap(th);
    }
    bool rval = true;
    for (int i = 0; i < kThreadNum; i++) {
      threads[i].join();
      try {
        if (ep[i]) {
          std::rethrow_exception(ep[i]);
        }
      } catch (ExceptionAssertionFailure t) {
        t.Show();
        rval = false;
      };
    }
    for (int i = 0; i < kThreadNum * kElementNum; i++) {
      Element *ele;
      kassert(_queue.Pop(ele));
    }
    kassert(_queue.IsEmpty());
    delete[] _ele;
    return rval;
  }
private:
  void Producer(std::exception_ptr ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum) {
        asm volatile("":::"memory");
      }
      for (int i = 0; i < kElementNum; i++) {
        _queue.Push(&_ele[id * kElementNum + i]);
      }
    } catch (...) {
      ep = std::current_exception();
    }
  }
  
  IntQueue<Element> _queue;
  Element *_ele;
  int _flag1;
  static const int kElementNum = 10000;
  static const int kThreadNum = 100;
} static OBJ(__LINE__);

class IntQueueTester_PopInParallel : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum + 1];
    std::exception_ptr ep[kThreadNum + 1];
    _ele = new Element[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&IntQueueTester_PopInParallel::Producer, this, &ep[i], i);
      threads[i].swap(th);
    }
    std::thread th(&IntQueueTester_PopInParallel::Consumer, this, &ep[kThreadNum]);
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
    _no_more_produce = true;
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
      int cnt = 0;
      while (cnt < kElementNum * kThreadNum && !_error) {
        Element *ele;
        if (_queue.Pop(ele)) {
          cnt++;
          __sync_fetch_and_add(&_pop_cnt, 1);
        } else {
          kassert(!_no_more_produce);
        }
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
        _queue.Push(&_ele[id * kElementNum + i]);
        __sync_fetch_and_add(&_push_cnt, 1);
      }
    } catch (...) {
      *ep = std::current_exception();
    }
  }
  
  IntQueue<Element> _queue;
  Element *_ele;
  int _push_cnt = 0;
  int _pop_cnt = 0;
  bool _error = false;
  bool _no_more_produce = false;
  int _flag1;
  static const int kElementNum = 10000;
  static const int kThreadNum = 100;
} static OBJ(__LINE__);

class IntQueueTester_ReusePoppedElement : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum + 1];
    std::exception_ptr ep[kThreadNum + 1];
    _ele = new Element[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&IntQueueTester_ReusePoppedElement::Producer, this, &ep[i], i);
      threads[i].swap(th);
    }
    std::thread th(&IntQueueTester_ReusePoppedElement::Consumer, this, &ep[kThreadNum]);
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
    _no_more_produce = true;
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
    for (int i = 0; i < kThreadNum * kElementNum; i++) {
      Element *ele;
      kassert(_queue.Pop(ele));
    }
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
      int cnt = 0;
      while (cnt < kElementNum * kThreadNum && !_error) {
        Element *ele;
        if (_queue.Pop(ele)) {
          cnt++;
          __sync_fetch_and_add(&_pop_cnt, 1);
          _queue.Push(ele);
        } else {
          kassert(!_no_more_produce);
        }
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
        _queue.Push(&_ele[id * kElementNum + i]);
        __sync_fetch_and_add(&_push_cnt, 1);
      }
    } catch (...) {
      *ep = std::current_exception();
    }
  }
  
  IntQueue<Element> _queue;
  Element *_ele;
  int _push_cnt = 0;
  int _pop_cnt = 0;
  bool _error = false;
  bool _no_more_produce = false;
  int _flag1;
  static const int kElementNum = 10000;
  static const int kThreadNum = 100;
} static OBJ(__LINE__);
