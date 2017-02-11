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

#include <stdint.h>

// SimpleSpinLock　単純
// McSpinLock1 1階層ロック
// McSpinLock2 2階層ロック
// SimpleSpinLockR 単純Spin On Read
// McSpinLock1R 1階層 Spin On Read
// McSpinLock2R 2階層 Spin On Read




// 単純
class SimpleSpinLock {
public:
  SimpleSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {
    }
  }
  void Unlock() {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
  }
private:
  volatile unsigned int _flag = 0;
};

template<class T>
void spin_lock_tts(T &flag) {
  while(flag == 1 || !__sync_bool_compare_and_swap(&flag, 0, 1)) {
  }
}

template<class T>
void spin_lock_tts_b(T &flag) {
  uint16_t delay = 1;
  while(flag == 1 || !__sync_bool_compare_and_swap(&flag, 0, 1)) {
    while(flag == 1) {
    }
    for (int j = 0; j < delay; j++) {
      asm volatile("pause;");
    }
    delay *= 2;
  }
}

template<class T>
void unlock(T &flag) {
  assert(__sync_lock_test_and_set(&flag, 0) == 1);
}

class TtsSpinLock {
public:
  TtsSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock(uint32_t apicid) {
    spin_lock_tts_b(_flag);
  }
  void Unlock(uint32_t apicid) {
    unlock(_flag);
  }
  bool IsNoOneWaiting(uint32_t apicid) {
    return false;
  }
private:
  volatile unsigned int _flag = 0;
};

class TicketSpinLock {
public:
  TicketSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock(uint32_t apicid) {
    uint64_t x = __sync_fetch_and_add(&_cnt, 1);
    while(_flag != x) {
      for (uint64_t j = 0; j < ((x < _flag) ? ((0xffffffffffffffff - _flag) + x): (x - _flag)); j++) {
        asm volatile("pause;");
      }
    }
  }
  void Unlock(uint32_t apicid) {
    __sync_fetch_and_add(&_flag, 1);
  }
private:
  uint64_t _flag = 0;
  uint64_t _cnt = 0;
};

class ClhSpinLock {
public:
  ClhSpinLock() {
    _tail = &_tmp;
    for (int i = 0; i < 37 * 8; i++) {
      _node[i] = &_nodes[i];
    }
  }
  void Lock(uint32_t apicid) {
    auto qnode = _node[apicid];
    qnode->locked = true;
    _pred[apicid] = __sync_lock_test_and_set(&_tail, qnode);
    while(_pred[apicid]->locked == true) {
    }
  }
  void Unlock(uint32_t apicid) {
    _node[apicid]->locked = false;
    _node[apicid] = _pred[apicid];
  }
  bool IsNoOneWaiting(uint32_t apicid) {
    return _tail == _node[apicid];
  }
private:
  struct QueueNode {
    int locked = false;
  } __attribute__ ((aligned (64)));
  QueueNode *_tail;
  QueueNode _tmp;
  QueueNode _nodes[37*8];
  QueueNode *_node[37*8];
  QueueNode *_pred[37*8];
};

template<int align, int arraysize>
class AndersonSpinLock {
public:
  AndersonSpinLock() {
    _flag[0] = 1;
    for (int i = 1; i < arraysize; i++) {
      _flag[i*align] = 0;
    }
  }
  bool TryLock() {
    assert(false);
  }
  void Lock(uint32_t apicid) {
    uint64_t cur = __sync_fetch_and_add(&_last, 1) % arraysize;
    while (_flag[cur*align] == 0) {
    }
    _flag[cur*align] = 0;
    _cur = cur;
  }
  void Unlock(uint32_t apicid) {
    _flag[((_cur + 1) % arraysize) * align] = 1;
  }
  bool IsNoOneWaiting(uint32_t apicid) {
    return (_cur + 1) % arraysize == _last % arraysize;
  }
 private:
  int _flag[arraysize*align];
  uint64_t _last = 0;
  uint64_t _cur = 0;
};

class McsSpinLock {
public:
  McsSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock(uint32_t apicid) {
    auto qnode = &_node[apicid];
    qnode->_next = nullptr;
    auto pred = __sync_lock_test_and_set(&_tail, qnode);
    if (pred != nullptr) {
      qnode->_spin = 1;
      pred->_next = qnode;
      while(qnode->_spin == 1) {
      }
    }
  }
  void Unlock(uint32_t apicid) {
    auto qnode = &_node[apicid];
    if (qnode->_next == nullptr) {
      if (__sync_bool_compare_and_swap(&_tail, qnode, nullptr)) {
        return;
      }
      while(qnode->_next == nullptr) {
      }
    }
    qnode->_next->_spin = 0;
  }
  bool IsNoOneWaiting(uint32_t apicid) {
    auto qnode = &_node[apicid];
    return qnode->_next == nullptr;
  }
private:
  struct QueueNode {
    QueueNode() {
      _next = nullptr;
      _spin = 0;
    }
    QueueNode *_next;
    int _spin;
  } __attribute__ ((aligned (64))) *_tail = nullptr;
  QueueNode _node[37 * 8];
};

// 単純 Spin On Read
class SimpleSpinLockR {
public:
  SimpleSpinLockR() {
  }
  bool TryLock() {
    if (_flag == 1) {
      return false;
    }
    return __sync_bool_compare_and_swap(&_flag, 0, 1);
  }
  void Lock(uint32_t apicid) {
    while (true) {
      while(_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock(uint32_t apicid) {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
  }
private:
  volatile unsigned int _flag = 0;
};

template<class L1, class L2>
class ExpSpinLock10 {
public:
  ExpSpinLock10() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock(uint32_t apicid) {
    uint32_t tileid = apicid / 8;
    uint32_t threadid = apicid % 8;

    _second_lock[tileid].Lock(threadid);
    if (_top_locked[tileid].i == 0) {
      _top_lock.Lock(tileid);
      _top_locked[tileid].i = kMax;
    }
  }
  void Unlock(uint32_t apicid) {
    uint32_t tileid = apicid / 8;
    uint32_t threadid = apicid % 8;

    _top_locked[tileid].i--;
    if (_top_locked[tileid].i == 0 && !_second_lock[tileid].IsNoOneWaiting(threadid) && _top_lock.IsNoOneWaiting(tileid)) {
      _top_locked[tileid].i = kMax;
    } else if (_top_locked[tileid].i == 0 || _second_lock[tileid].IsNoOneWaiting(threadid)) {
      _top_lock.Unlock(tileid);
      _top_locked[tileid].i = 0;
    }
    _second_lock[tileid].Unlock(threadid);
  }
private:
  static const int kSubStructNum = 37;
  L1 _top_lock;
  L2 _second_lock[kSubStructNum];
  static const int kMax = 300;
  struct Status {
    int i = 0;
  } __attribute__ ((aligned (64)));
  Status _top_locked[kSubStructNum];
};

