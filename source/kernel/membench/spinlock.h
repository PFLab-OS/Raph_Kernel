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
#include <global.h>
#include <tty.h>

static const int kFastMeasurement = true;

#define check_align(ptr) assert(reinterpret_cast<size_t>(ptr) % 64 == 0)

template<class T>
static inline void check_type_align() {
  assert(sizeof(T) % 64 == 0);
}

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
} __attribute__ ((aligned (64)));

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
} __attribute__ ((aligned (64)));

class TicketSpinLock {
public:
  TicketSpinLock() {
    check_align(this);
    check_align(&_flag);
    check_align(&_cnt);
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
  bool IsNoOneWaiting(uint32_t apicid) {
    return _flag + 1 == _cnt;
  }
private:
  uint64_t _flag __attribute__ ((aligned (64))) = 0;
  uint64_t _cnt __attribute__ ((aligned (64))) = 0;
} __attribute__ ((aligned (64)));

class ClhSpinLock {
public:
  ClhSpinLock() {
    check_align(this);
    check_type_align<QueueNode>();
    check_align(&_tail);
    check_align(&_tmp);
    check_align(_nodes);
    check_align(_node);
    check_align(_pred);
    _tail = &_tmp;
    assert(_tail->locked == false);
    for (int i = 0; i < 37 * 8; i++) {
      _node[i] = &_nodes[i];
      _pred[i] = nullptr;
    }
  }
  void Lock(uint32_t apicid) {
    auto qnode = _node[apicid];
    qnode->locked = 1;
    _pred[apicid] = __sync_lock_test_and_set(&_tail, qnode);
    while(_pred[apicid]->locked == 1) {
    }
  }
  void Unlock(uint32_t apicid) {
    _node[apicid]->locked = 0;
    _node[apicid] = _pred[apicid];
  }
  bool IsNoOneWaiting(uint32_t apicid) {
    return _tail == _node[apicid];
  }
private:
  struct QueueNode {
    int locked = 0;
  } __attribute__ ((aligned (64)));
  QueueNode *_tail;
  QueueNode _tmp;
  QueueNode _nodes[37*8];
  QueueNode *_node[37*8];
  QueueNode *_pred[37*8];
} __attribute__ ((aligned (64)));

class HClhSpinLock {
public:
  HClhSpinLock() {
    check_align(this);
    check_type_align<QueueNode>();
    check_align(_nodes);
    check_align(_node);
    check_align(_pred);
    check_align(&_global_tmp);
    check_align(_local_queue);
    check_align(&_global_queue);
    for (int i = 0; i < 37 * 8; i++) {
      _node[i] = &_nodes[i];
      _pred[i] = nullptr;
    }
    _global_tmp.tail_when_spliced = 1;
    _global_queue = &_global_tmp;
    for (int i = 0; i < 37; i++) {
      _local_queue[i] = nullptr;
    }
  }
  void Lock(uint32_t apicid) {
    uint32_t tileid = apicid / 8;
    auto qnode = _node[apicid];
    qnode->tileid = tileid;
    qnode->locked = 1;
    qnode->tail_when_spliced = 0;
    
    do {
      _pred[apicid] = _local_queue[tileid];
    } while(!__sync_bool_compare_and_swap(&_local_queue[tileid], _pred[apicid], qnode));

    if (_pred[apicid] != nullptr) {
      while(true) {
        if (_pred[apicid]->tileid != tileid) {
          break;
        }
        if (_pred[apicid]->tail_when_spliced == 1) {
          break;
        }
        if (_pred[apicid]->tileid == tileid && _pred[apicid]->locked == 0 && _pred[apicid]->tail_when_spliced == 0) {
          return;
        }
      }
    }

    for (int i = 0; i < 10; i++) {
      asm volatile("pause");
    }

    QueueNode *local_tail;
    do {
      _pred[apicid] = _global_queue;
      local_tail = _local_queue[tileid];
    } while(!__sync_bool_compare_and_swap(&_global_queue, _pred[apicid], local_tail));
    
    local_tail->tail_when_spliced = 1;

    while(_pred[apicid]->locked == 1) {
    }
  }
  void Unlock(uint32_t apicid) {
    _node[apicid]->locked = 0;
    _node[apicid] = _pred[apicid];
  }
private:
  struct QueueNode {
    int locked = 0;
    int tail_when_spliced = 0;
    uint32_t tileid = 0xFFFFFFFF;
  } __attribute__ ((aligned (64)));
  QueueNode _nodes[37*8];
  QueueNode *_node[37*8];
  QueueNode *_pred[37*8];
  QueueNode _global_tmp;
  QueueNode __attribute__ ((aligned (64))) *_local_queue[37];
  QueueNode *__attribute__ ((aligned (64))) _global_queue;
} __attribute__ ((aligned (64)));

template<int align, int arraysize>
class AndersonSpinLock {
public:
  AndersonSpinLock() {
    check_align(this);
    if (align == 64) {
      check_type_align<Flag>();
      check_align(_flag); 
    }
    check_align(&_last);
    check_align(&_cur);
   _flag[0].i = 1;
    for (int i = 1; i < arraysize; i++) {
      _flag[i].i = 0;
    }
  }
  bool TryLock() {
    assert(false);
  }
  void Lock(uint32_t apicid) {
    uint64_t cur = __sync_fetch_and_add(&_last, 1) % arraysize;
    while (_flag[cur].i == 0) {
    }
    _flag[cur].i = 0;
    _cur = cur;
  }
  void Unlock(uint32_t apicid) {
    _flag[(_cur + 1) % arraysize].i = 1;
  }
  bool IsNoOneWaiting(uint32_t apicid) {
    return (_cur + 1) % arraysize == _last % arraysize;
  }
 private:
  struct Flag {
    int i;
  } __attribute__ ((aligned (align)));
  uint64_t _last __attribute__ ((aligned (64))) = 0;
  uint64_t _cur __attribute__ ((aligned (64))) = 0;
  Flag _flag[arraysize];
} __attribute__ ((aligned (64)));

template<int align>
class McsSpinLock {
public:
  McsSpinLock() {
    check_align(this);
    if (align == 64) {
      check_type_align<QueueNode>();
    }
    check_align(_node);
    check_align(&_tail);
  }
  bool TryLock(uint32_t apicid) {
    auto qnode = &_node[apicid];
    qnode->_next = nullptr;
    return __sync_bool_compare_and_swap(&_tail, nullptr, qnode);
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
  } __attribute__ ((aligned (align)));
  QueueNode *__attribute__ ((aligned (64))) _tail = nullptr;
  QueueNode _node[37 * 8];
} __attribute__ ((aligned (64)));

// 単純 Spin On Read
class SimpleSpinLockR {
public:
  SimpleSpinLockR() {
    check_align(this);
    check_align(&_flag);
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
} __attribute__ ((aligned (64)));

template<class L1, class L2, int kMax>
class ExpSpinLock10 {
public:
  ExpSpinLock10() {
    check_align(this);
    check_align(&_top_lock);
    check_align(_second_lock);
    check_type_align<L2>();
    check_align(_top_locked);
    check_type_align<Status>();
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
  struct Status {
    int i = 0;
  } __attribute__ ((aligned (64)));
  Status _top_locked[kSubStructNum];
} __attribute__ ((aligned (64)));
