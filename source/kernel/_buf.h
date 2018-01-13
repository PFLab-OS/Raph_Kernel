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

// replace RingBuffer to this
template<class T, int S>
class RingBuffer2 {
public:
  RingBuffer2() {
    for (int i = 0; i < S; i++) {
      _push_flags[i] = 0;
      _pop_flags[i] = 0;
    }
  }
  virtual ~RingBuffer2() {
  }
  // return false if full
  bool Push(T data) __attribute__((warn_unused_result)) {
    if (!CheckCnt(_push_resource_cnt)) {
      return false;
    }

    kassert(_head < _tail + S);
    uint64_t index = __sync_fetch_and_add(&_head, 1);
    _buffer[GetArrayIndex(index)] = data;

    kassert(_phead <= index);
    if (_phead < index) {
      if (_push_flags[GetArrayIndex(index)] == 0 &&
          __sync_lock_test_and_set(&_push_flags[GetArrayIndex(index)], 1) == 0) {
        return true;
      }
      while(_phead != index) {
        asm volatile("":::"memory");
      }
    }
    kassert(_phead == index);
    _push_flags[GetArrayIndex(index)] = 0;
    
    uint64_t cindex;
    for (cindex = index + 1; cindex < index + S; cindex++) {
      if (_push_flags[GetArrayIndex(cindex)] == 0 &&
          __sync_lock_test_and_set(&_push_flags[GetArrayIndex(cindex)], 1) == 0) {
        break;
      }
      _push_flags[GetArrayIndex(cindex)] = 0;
    }
    kassert(__sync_add_and_fetch(&_pop_resource_cnt, cindex - index) <= S);
    
    _phead = cindex;
    return true;
  }
  // return false if empty
  bool Pop(T &data) __attribute__((warn_unused_result)) {
    if (!CheckCnt(_pop_resource_cnt)) {
      return false;
    }

    kassert(_tail < _head);
    uint64_t index = __sync_fetch_and_add(&_tail, 1);
    data = _buffer[GetArrayIndex(index)];

    kassert(_ptail <= index);
    if (_ptail < index) {
      if (_pop_flags[GetArrayIndex(index)] == 0 &&
          __sync_lock_test_and_set(&_pop_flags[GetArrayIndex(index)], 1) == 0) {
        return true;
      }
      while(_ptail != index) {
        asm volatile("":::"memory");
      }
    }
    kassert(_ptail == index);
    _pop_flags[GetArrayIndex(index)] = 0;
    
    uint64_t cindex;
    for (cindex = index + 1; cindex < index + S; cindex++) {
      if (_pop_flags[GetArrayIndex(cindex)] == 0 &&
          __sync_lock_test_and_set(&_pop_flags[GetArrayIndex(cindex)], 1) == 0) {
        break;
      }
      _pop_flags[GetArrayIndex(cindex)] = 0;
    }
    kassert(__sync_add_and_fetch(&_push_resource_cnt, cindex - index) <= S);
    
    _ptail = cindex;
    return true;
  }
  bool Empty() {
    return _head == _tail;
  }
private:
  bool CheckCnt(int &cnt) {
    while(true) {
      int x = cnt;
      if (x <= 0) {
        return false;
      }
      if (cnt == x && __sync_bool_compare_and_swap(&cnt, x, x - 1)) {
        return true;
      }
    }
  }
  static uint64_t GetArrayIndex(uint64_t index) {
    return index % S;
  }

  static_assert(S > 0, "");
  T _buffer[S];
  int _push_flags[S];
  int _pop_flags[S];
  uint64_t _head = 0;
  uint64_t _phead = 0;
  uint64_t _tail = 0;
  uint64_t _ptail = 0;
  int _pop_resource_cnt = 0;
  int _push_resource_cnt = S;
};
