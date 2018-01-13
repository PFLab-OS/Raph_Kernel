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

#include <_buf.h>
#include "test.h"
#include <iostream>
#include <thread>

class RingBuffer2_EmptyPop : public Tester {
public:
  virtual bool Test() override {
    int i;
    kassert(!_buf.Pop(i));
    return true;
  }
private:
  RingBuffer2<int, 2> _buf;
} static OBJ(__LINE__);

class RingBuffer2_OnePush : public Tester {
public:
  virtual bool Test() override {
    int i = 0;
    kassert(_buf.Push(i));
    return true;
  }
private:
  RingBuffer2<int, 2> _buf;
} static OBJ(__LINE__);

class RingBuffer2_FullPush : public Tester {
public:
  virtual bool Test() override {
    int i = 0;
    kassert(_buf.Push(i));
    kassert(_buf.Push(i));
    kassert(!_buf.Push(i));
    return true;
  }
private:
  RingBuffer2<int, 2> _buf;
} static OBJ(__LINE__);

class RingBuffer2_SinglePushPop : public Tester {
public:
  virtual bool Test() override {
    for (int i = 0; i < 10; i++) {
      kassert(_buf.Push(i));
    }
    for (int i = 0; i < 10; i++) {
      int j;
      kassert(_buf.Pop(j));
      kassert(i == j);
    }
    kassert(_buf.Empty());
    return true;
  }
private:
  RingBuffer2<int, 10> _buf;
} static OBJ(__LINE__);

class RingBuffer2_ParallelPop : public Tester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum];
    std::exception_ptr ep[kThreadNum];

    for (int i = 0; i < kElementNum; i++) {
      kassert(_buf.Push(i));
    }
    
    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&RingBuffer2_ParallelPop::Consumer, this, ep[i], i);
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
    for (int i = 0; i < kElementNum; i++) {
      kassert(_popped[i]);
    }
    kassert(_buf.Empty());
    return rval;
  }
private:
  void Consumer(std::exception_ptr ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum) {
        asm volatile("":::"memory");
      }
      for (int i = 0; i < kElementNum; i++) {
        int j;
        if (!_buf.Pop(j)) {
          break;
        }
        kassert(j >= 0 && j < kElementNum);
        kassert(!_popped[j]);
        _popped[j] = true;
      }
    } catch (...) {
      ep = std::current_exception();
    }
  }
  static const int kElementNum = 1000;
  static const int kThreadNum = 1000;
  RingBuffer2<int, kElementNum> _buf;
  bool _popped[kElementNum];
  int _flag1 = 0;
} static OBJ(__LINE__);

class RingBuffer2_ParallelPushPop : public ThreadTester {
public:
  virtual bool Test() override {
    std::thread threads[kThreadNum * 2];
    std::exception_ptr ep[kThreadNum * 2];

    for (int i = 0; i < kThreadNum; i++) {
      _popped[i] = false;
      _pushed[i] = false;
    }

    for (int i = 0; i < kThreadNum; i++) {
      std::thread th(&RingBuffer2_ParallelPushPop::Producer, this, ep[i], i);
      threads[i].swap(th);
    }

    for (int i = kThreadNum; i < kThreadNum * 2; i++) {
      std::thread th(&RingBuffer2_ParallelPushPop::Consumer, this, ep[i], i);
      threads[i].swap(th);
    }
    
    bool rval = true;
    for (int i = 0; i < kThreadNum * 2; i++) {
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
    if (_err) {
      return false;
    }
    for (int i = 0; i < kElementNum * kThreadNum; i++) {
      kassert(_popped[i]);
    }
    kassert(_buf.Empty());
    return rval;
  }
private:
  void Consumer(std::exception_ptr ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum * 2) {
        asm volatile("":::"memory");
      }
      while(!_err) {
        int j;
        if (!_buf.Pop(j)) {
          if (_flag2 == kThreadNum) {
            break;
          } else {
            continue;
          }
        }
        kassert(j >= 0 && j < kElementNum * kThreadNum);
        kassert(_popped[j] == 0);
        _popped[j] = true;
      }
      return;
    } catch (ExceptionAssertionFailure t) {
      t.Show();
    } catch (...) {
      ep = std::current_exception();
    }
    _err = true;
  }
  void Producer(std::exception_ptr ep, int id) {
    try {
      __sync_fetch_and_add(&_flag1, 1);
      while(_flag1 != kThreadNum * 2) {
        asm volatile("":::"memory");
      }
      for (int i = 0; i < kElementNum; i++) {
        if (_err) {
          return;
        }
        int j = i + id * kElementNum;
        while(!_buf.Push(j)) {
        }
        kassert(j >= 0 && j < kElementNum * kThreadNum);
        kassert(!_pushed[j]);
        _pushed[j] = true;
      }
      __sync_fetch_and_add(&_flag2, 1);
      return;
    } catch (ExceptionAssertionFailure t) {
      t.Show();
    } catch (...) {
      ep = std::current_exception();
    }
    _err = true;
  }
  static const int kElementNum = 1000;
  static const int kThreadNum = 200;
  RingBuffer2<int, 100> _buf;
  uint64_t _popped[kElementNum * kThreadNum];
  uint64_t _pushed[kElementNum * kThreadNum];
  int _flag1 = 0;
  int _flag2 = 0;
  bool _err = false;
} static OBJ(__LINE__);
