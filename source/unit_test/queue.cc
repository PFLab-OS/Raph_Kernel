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

#include <stdio.h>
#include <_queue.h>
#include "test.h"
#include <iostream>
#include <pthread.h>
#include <thread>
#include <exception>
#include <stdexcept>
using namespace std;

class QElement : public QueueContainer<QElement> {
public:
  QElement() : QueueContainer<QElement>(this) {
  }
private:
};

class QueueTester_EmptyPop : public Tester {
public:
  virtual bool Test() override {
    QElement *ele;
    kassert(_queue.Pop(ele) == false);
    return true;
  }
private:
  Queue<QElement> _queue;
} static OBJ(__LINE__);

class QueueTester_SinglePushPop : public Tester {
public:
  virtual bool Test() override {
    QElement ele1;
    _queue.Push(&ele1);
    QElement *ele = nullptr;
    kassert(_queue.Pop(ele));
    kassert(ele == &ele1);
    kassert(!_queue.Pop(ele));
    return true;
  }
private:
  Queue<QElement> _queue;
} static OBJ(__LINE__);

class QueueTester_TriplePushPop : public Tester {
public:
  virtual bool Test() override {
    QElement ele1, ele2, ele3;
    _queue.Push(&ele1);
    _queue.Push(&ele2);
    _queue.Push(&ele3);
    QElement *ele;
    kassert(_queue.Pop(ele));
    kassert(ele == &ele1);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele2);
    kassert(_queue.Pop(ele));
    kassert(ele == &ele3);
    kassert(!_queue.Pop(ele));
    return true;
  }
private:
  Queue<QElement> _queue;
} static OBJ(__LINE__);

class QueueTester_ParallelPush : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum];
    std::exception_ptr ep[kThreadNum];
    _ele = new QElement[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&QueueTester_ParallelPush::Producer, this, ep[i], i);
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
      QElement *ele;
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
  
  Queue<QElement> _queue;
  QElement *_ele;
  int _flag1 = 0;
  static const int kElementNum = 10000;
  static const int kThreadNum = 50;
} static OBJ(__LINE__);

class QueueTester_ParallelPop : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum];
    std::exception_ptr ep[kThreadNum];
    _ele = new QElement[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum * kElementNum; i++) {
      _queue.Push(&_ele[i]);
    }

    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&QueueTester_ParallelPop::Consumer, this, &ep[i]);
      threads[i].swap(th);
    }
    
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
    kassert(_pop_cnt == kThreadNum * kElementNum);
    kassert(_queue.IsEmpty());
    delete[] _ele;
    return rval;
  }
private:
  void Consumer(std::exception_ptr *ep) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum) {
        asm volatile("":::"memory");
      }
      int cnt = 0;
      while (cnt < kElementNum && !_error) {
        QElement *ele;
        kassert(_queue.Pop(ele));
        cnt++;
        __sync_fetch_and_add(&_pop_cnt, 1);
      }
    } catch (...) {
      *ep = std::current_exception();
    }
  }
  
  Queue<QElement> _queue;
  QElement *_ele;
  int _pop_cnt = 0;
  bool _error = false;
  int _flag1 = 0;
  static const int kElementNum = 10000;
  static const int kThreadNum = 50;
} static OBJ(__LINE__);

class QueueTester_ParallelPushPop : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum * 2];
    std::exception_ptr ep[kThreadNum * 2];
    _ele = new QElement[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&QueueTester_ParallelPushPop::Producer, this, &ep[i], i);
      threads[i].swap(th);
    }
    for (int i = kThreadNum; i < kThreadNum * 2; i++) {
      std::thread th(&QueueTester_ParallelPushPop::Consumer, this, &ep[i], i);
      threads[i].swap(th);
    }
    
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
    for (int i = kThreadNum; i < kThreadNum * 2; i++) {
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

    kassert(_queue.IsEmpty());
    kassert(_push_cnt == kThreadNum * kElementNum);
    kassert(_pop_cnt == kThreadNum * kElementNum);
    delete[] _ele;
    return rval;
  }
private:
  void Consumer(std::exception_ptr *ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum * 2) {
        asm volatile("":::"memory");
      }
      int cnt = 0;
      while (cnt < kElementNum && !_error) {
        QElement *ele;
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
      while(_flag1 != kThreadNum * 2) {
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
  
  Queue<QElement> _queue;
  QElement *_ele;
  int _push_cnt = 0;
  int _pop_cnt = 0;
  bool _error = false;
  bool _no_more_produce = false;
  int _flag1 = 0;
  static const int kElementNum = 1000; // TODO 10000
  static const int kThreadNum = 50;
} static OBJ(__LINE__);

class QueueTester_ReusePoppedQElement : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum * 2];
    std::exception_ptr ep[kThreadNum * 2];
    _ele = new QElement[kThreadNum * kElementNum];
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&QueueTester_ReusePoppedQElement::Producer, this, &ep[i], i);
      threads[i].swap(th);
    }
    for (int i = kThreadNum; i < kThreadNum * 2; i++) {
      std::thread th(&QueueTester_ReusePoppedQElement::Consumer, this, &ep[i]);
      threads[i].swap(th);
    }
    
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
    for (int i = kThreadNum; i < kThreadNum * 2; i++) {
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
    for (int i = 0; i < kThreadNum * kElementNum; i++) {
      QElement *ele;
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
      while(_flag1 != kThreadNum * 2) {
        asm volatile("":::"memory");
      }
      int cnt = 0;
      while (cnt < kElementNum && !_error) {
        QElement *ele;
        if (_queue.Pop(ele)) {
          cnt++;
          __sync_fetch_and_add(&_pop_cnt, 1);
          _queue.Push(ele);
        }
      }
    } catch (...) {
      *ep = std::current_exception();
    }
  }
  void Producer(std::exception_ptr *ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum * 2) {
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
  
  Queue<QElement> _queue;
  QElement *_ele;
  int _push_cnt = 0;
  int _pop_cnt = 0;
  bool _error = false;
  int _flag1 = 0;
  static const int kElementNum = 1000; // TODO 10000
  static const int kThreadNum = 50;
} static OBJ(__LINE__);


