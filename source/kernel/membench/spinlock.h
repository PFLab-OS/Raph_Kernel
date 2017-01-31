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
  assert(__sync_bool_compare_and_swap(&flag, 1, 0));
}

class TtsSpinLock {
public:
  TtsSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    spin_lock_tts_b(_flag);
  }
  void Unlock() {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
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
  void Lock() {
    uint64_t x = __sync_fetch_and_add(&_cnt, 1);
    while(_flag != x) {
      for (uint64_t j = 0; j < ((x < _flag) ? ((0xffffffffffffffff - _flag) + x): (x - _flag)); j++) {
        asm volatile("pause;");
      }
    }
  }
  void Unlock() {
    __sync_fetch_and_add(&_flag, 1);
  }
private:
  uint64_t _flag = 0;
  uint64_t _cnt = 0;
};

class McsSpinLock {
public:
  McsSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    auto qnode = &_node[cpu_ctrl->GetCpuId().GetApicId()];
    qnode->_next = nullptr;
    auto pred = xchg(&_tail, qnode);
    if (pred != nullptr) {
      qnode->_spin = 1;
      pred->_next = qnode;
      while(qnode->_spin == 1) {
      }
    }
  }
  void Unlock() {
    auto qnode = &_node[cpu_ctrl->GetCpuId().GetApicId()];
    if (qnode->_next == nullptr) {
      if (__sync_bool_compare_and_swap(&_tail, qnode, nullptr)) {
        return;
      }
      while(qnode->_next == nullptr) {
      }
    }
    qnode->_next->_spin = 0;
  }
private:
  struct QueueNode {
    QueueNode() {
      _next = nullptr;
      _spin = 0;
    }
    QueueNode *_next;
    int _spin;
  } *_tail = nullptr;
  QueueNode *xchg(QueueNode **addr, QueueNode *newval)
  {   
    QueueNode *result = newval;

    asm volatile("lock xchgq %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
  QueueNode _node[37 * 8];
};

class McsSpinLock2 {
public:
  struct QueueNode {
    QueueNode() {
      _next = nullptr;
      _spin = 0;
    }
    QueueNode *_next;
    int _spin;
  };
  McsSpinLock2() {
  }
  bool TryLock() {
    assert(false);
  }
  QueueNode *Lock() {
    auto qnode = &_node[cpu_ctrl->GetCpuId().GetApicId()];
    qnode->_next = nullptr;
    auto pred = xchg(&_tail, qnode);
    if (pred != nullptr) {
      qnode->_spin = 1;
      pred->_next = qnode;
      while(qnode->_spin == 1) {
      }
    }
    return qnode;
  }
  void Unlock(QueueNode *qnode) {
    if (qnode->_next == nullptr) {
      if (__sync_bool_compare_and_swap(&_tail, qnode, nullptr)) {
        return;
      }
      while(qnode->_next == nullptr) {
      }
    }
    qnode->_next->_spin = 0;
  }
private:
  QueueNode *_tail = nullptr;
  QueueNode *xchg(QueueNode **addr, QueueNode *newval)
  {   
    QueueNode *result = newval;

    asm volatile("lock xchgq %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
  QueueNode _node[37 * 8];
};

// 1階層ロック
template <int kSubStructNum>
class McSpinLock1 {
public:
  McSpinLock1() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(!__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
    }
    while(!__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
  }
private:
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
};

// 2階層ロック
template <int kSubStructNum>
class McSpinLock2 {
public:
  McSpinLock2() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(!__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 0, 1)) {
    }
    while(!__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
    }
    while(!__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    // _third_flag[apicid / 4] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
    assert(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 1, 0));
  }
private:
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
  volatile unsigned int _third_flag[kSubStructNum*2] = {0};
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
  void Lock() {
    while (true) {
      while(_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
  }
private:
  volatile unsigned int _flag = 0;
};

// 1階層 Spin On Read
template <int kSubStructNum>
class McSpinLock1R {
public:
  McSpinLock1R() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(true) {
      while(_second_flag[apicid / 8] == 1) {
      }
      if(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
        break;
      }
    }
    while (true) {
      while(_top_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
  }
private:
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
};

// 2階層 Spin On Read
class McSpinLock2R {
public:
  McSpinLock2R() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(true) {
      while(_third_flag[apicid / 4] == 1) {
      }
      if(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 0, 1)) {
        break;
      }
    }
    while(true) {
      while(_second_flag[apicid / 8] != 0) {
      }
      if(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
        break;
      }
    }
    while (true) {
      while(_top_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    // _third_flag[apicid / 4] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
    assert(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 1, 0));
  }
private:
  static const int kSubStructNum = 37;
  static const int kPhysAvailableCoreNum = 32;
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
  volatile unsigned int _third_flag[kSubStructNum*2] = {0};
};

template<class L1, class L2>
class ExpSpinLock1 {
public:
  ExpSpinLock1() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    _second_lock[apicid / 8].Lock();
    _top_lock.Lock();
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    _top_lock.Unlock();
    _second_lock[apicid / 8].Unlock();
  }
private:
  static const int kSubStructNum = 37;
  L1 _top_lock;
  L2 _second_lock[kSubStructNum];
};

template<class L1, class L2>
class ExpSpinLock2 {
public:
  ExpSpinLock2() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    int *state1 = &_state1[apicid / 8];
    int prev;
    while (true) {
      int tmp = *state1;
      if (tmp < 100 && __sync_bool_compare_and_swap(state1, tmp, tmp + 1)) {
        prev = tmp;
        break;
      }
    }
    if (prev == 0) {
      _top_lock.Lock();
      _second_lock[apicid / 8].Lock();
      _state2[apicid / 8] = xchg(&_state1[apicid / 8], 100) - 1;
    } else {
      while(*state1 != 100) {
      }
      _second_lock[apicid / 8].Lock();
      _state2[apicid / 8]--;
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    if (_state2[apicid / 8] == 0) {
      _top_lock.Unlock();
      _state1[apicid / 8] = 0;
    }
    _second_lock[apicid / 8].Unlock();
  }
private:
  static const int kSubStructNum = 37;
  L1 _top_lock;
  L2 _second_lock[kSubStructNum];
  int _state1[kSubStructNum] = {0};
  int _state2[kSubStructNum] = {0};
  int xchg(int *addr, int newval)
  {   
    int result = newval;

    asm volatile("lock xchgl %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
};


template<class L1>
class ExpSpinLock4 {
public:
  ExpSpinLock4() {
    for (int i = 0; i < kSubStructNum * 2; i++) {
      _state1[i] = -1;
    }
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    int *state1 = &_state1[apicid / 4];

    uint8_t delay1 = 1;
    
    int state = *state1;

    while(true) {
      if (state == -1) {
        if (__sync_bool_compare_and_swap(state1, -1, 0)) {
          state = 0;
          _top_lock.Lock();
          state = __sync_fetch_and_add(state1, 100);
          state += 100;
          while(true) {
            if (state == 100) {
              *state1 = 200;
              return;
            }
            for (int j = 0; j < state - 100; j++) {
              asm volatile("pause;");
            }
            state = *state1;
          }
        }
      } else if (state >= 0 && state < 100) {
        if (__sync_bool_compare_and_swap(state1, state, state + 1)) {
          uint8_t delay2 = 1;
          while(true) {
            state = *state1;
            if (state >= 100 && state < 200 && __sync_bool_compare_and_swap(state1, state, state + 100)) {
              assert(state != 100);
              return;
            }
            for (int j = 0; j < delay2; j++) {
              asm volatile("pause;");
            }
            delay2 *= 2;
          }
        }
      } else {
        for (int j = 0; j < delay1; j++) {
          asm volatile("pause;");
        }
        delay1 *= 2;
      }
      state = *state1;
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    int *state1 = &_state1[apicid / 4];
    if (*state1 == 200) {
      _top_lock.Unlock();
      *state1 = -1;
    } else {
      __sync_fetch_and_sub(state1, 101);
    }
  }
private:
  static const int kSubStructNum = 37 * 2;
  L1 _top_lock;
  int _state1[kSubStructNum*2];
  int xchg(int *addr, int newval)
  {   
    int result = newval;

    asm volatile("lock xchgl %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
};

template<class L1>
class ExpSpinLock5 {
public:
  ExpSpinLock5() {
    for (int i = 0; i < kSubStructNum * 2; i++) {
      _state1[i] = -1;
    }
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    int *state1 = &_state1[apicid / 4];

    int state = *state1;

    uint8_t delay1 = 1;
    while(true) {
      if (state == -1) {
        if (__sync_bool_compare_and_swap(state1, -1, 0)) {
          state = 0;
          _top_lock.Lock();
          state = __sync_fetch_and_add(state1, 100);
          state += 100;
          while(true) {
            if (state == 100) {
              *state1 = 200;
              return;
            }
            for (int j = 0; j < state - 100; j++) {
              asm volatile("pause;");
            }
            state = *state1;
          }
        }
      } else if (state >= 0 && state < 100) {
        if (__sync_bool_compare_and_swap(state1, state, state + 1)) {
          uint8_t delay2 = 1;
          while(true) {
            state = *state1;
            if (state >= 100 && state < 200 && __sync_bool_compare_and_swap(state1, state, state + 100)) {
              assert(state != 100);
              return;
            }
            if (state < 100) {
              for (int j = 0; j < delay2; j++) {
                asm volatile("pause;");
              }
              delay2 *= 2;
            }
          }
        }
      } else {
        for (int j = 0; j < delay1; j++) {
          asm volatile("pause;");
        }
        delay1 *= 2;
      }
      state = *state1;
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    int *state1 = &_state1[apicid / 4];
    if (*state1 == 200) {
      _top_lock.Unlock();
      *state1 = -1;
    } else {
      __sync_fetch_and_sub(state1, 101);
    }
  }
private:
  static const int kSubStructNum = 37 * 2;
  L1 _top_lock;
  int _state1[kSubStructNum*2];
  int xchg(int *addr, int newval)
  {   
    int result = newval;

    asm volatile("lock xchgl %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
};

class ExpSpinLock8 {
public:
  ExpSpinLock8() {
    for (int i = 0; i < kSubStructNum; i++) {
      _cnt[i] = 0;
      _flag[i] = 0;
      _lock_cnt[i] = 0;
    }
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    uint32_t tileid = apicid / 8;
    uint64_t *cnt = &_cnt[tileid];
    uint64_t *flag = &_flag[tileid];

    uint64_t x = __sync_fetch_and_add(cnt, 1);
    while(*flag != x) {
      for (uint64_t j = 0; j < ((x < *flag) ? ((0xffffffffffffffff - *flag) + x): (x - *flag)); j++) {
        asm volatile("pause;");
      }
    }

    if (__sync_bool_compare_and_swap(&_lock_cnt[tileid], *flag, *flag)) {
      _qnode[tileid] = _top_lock.Lock();
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    uint32_t tileid = apicid / 8;
    uint64_t *flag = &_flag[tileid];
    uint64_t *cnt = &_cnt[tileid];

    uint64_t tmp = *flag + 1;
    if (__sync_bool_compare_and_swap(cnt, tmp, tmp)) {
      _top_lock.Unlock(_qnode[tileid]);
      xchg(&_lock_cnt[tileid], tmp);
    }
    __sync_fetch_and_add(flag, 1);
  }
private:
  static const int kSubStructNum = 37;
  McsSpinLock2 _top_lock;
  uint64_t _cnt[kSubStructNum];
  uint64_t _flag[kSubStructNum];
  uint64_t _lock_cnt[kSubStructNum];
  McsSpinLock2::QueueNode *_qnode[kSubStructNum];
  uint64_t xchg(uint64_t *addr, uint64_t newval)
  {   
    uint64_t result = newval;

    asm volatile("lock xchgq %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
};

class ExpSpinLock82 {
public:
  ExpSpinLock82() {
    for (int i = 0; i < kSubStructNum; i++) {
      _cnt[i] = 0;
      _flag[i] = 0;
      _lock_cnt[i] = 0;
    }
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    uint32_t tileid = apicid / 8;
    uint64_t *cnt = &_cnt[tileid];
    uint64_t *flag = &_flag[tileid];

    uint64_t x = __sync_fetch_and_add(cnt, 1);
    while(*flag != x) {
      for (uint64_t j = 0; j < ((x < *flag) ? ((0xffffffffffffffff - *flag) + x): (x - *flag)); j++) {
        asm volatile("pause;");
      }
    }

    if (__sync_bool_compare_and_swap(&_lock_cnt[tileid], *flag, *flag)) {
      _qnode[tileid] = _top_lock.Lock();
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    uint32_t tileid = apicid / 8;
    uint64_t *flag = &_flag[tileid];
    uint64_t *cnt = &_cnt[tileid];

    uint64_t tmp = *flag + 1;
    if (_lock_cnt[tileid] + 1000 == tmp || __sync_bool_compare_and_swap(cnt, tmp, tmp)) {
    // if (__sync_bool_compare_and_swap(&_unlock_cnt[tileid], *flag, *flag)) {
      _top_lock.Unlock(_qnode[tileid]);
      xchg(&_lock_cnt[tileid], tmp);
    }
    __sync_fetch_and_add(flag, 1);
  }
private:
  static const int kSubStructNum = 37;
  McsSpinLock2 _top_lock;
  uint64_t _cnt[kSubStructNum];
  uint64_t _flag[kSubStructNum];
  uint64_t _lock_cnt[kSubStructNum];
  McsSpinLock2::QueueNode *_qnode[kSubStructNum];
  uint64_t xchg(uint64_t *addr, uint64_t newval)
  {   
    uint64_t result = newval;

    asm volatile("lock xchgq %0, %1"
                 :"+r"(result), "+m"(*addr)
                 ::"memory","cc");
    return result;
  }
};
