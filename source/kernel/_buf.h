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
// !!!Important!!!
// pushing from multiple threads is prohibited!
// popping from interrupt handlers is prohibited!
// TODO assert these restrictions
template<class T, int S>
class RingBuffer2 {
public:
  RingBuffer2() {
  }
  virtual ~RingBuffer2() {
  }
  // return false if full
  bool Push(T data) __attribute__((warn_unused_result)) {
    if (_tail + S == _head) {
      return false;
    }
    kassert(_head < _tail + S);
    uint64_t index = _head;
    _buffer[index % S] = data;
    kassert(index == __sync_fetch_and_add(&_head, 1));
    return true;
  }
  // return false if empty
  bool Pop(T &data) __attribute__((warn_unused_result)) {
    uint64_t index;
    bool iflag;
    while(true) {
      index = _pop_ticket;

      if (_pop_ticket == _head) {
        return false;
      }
      
      if (_pop_ticket == index && __sync_bool_compare_and_swap(&_pop_ticket, index, index + 1)) {
        iflag = disable_interrupt();
        break;
      }
    }
    data = _buffer[index % S];
    while(_tail != index) {
      asm volatile("":::"memory");
    }
    _tail++;
    enable_interrupt(iflag);
    return true;
  }
  bool Empty() {
    return _head == _tail;
  }
private:
  T _buffer[S];
  uint64_t _head = 0;
  uint64_t _pop_ticket = 0;
  uint64_t _tail = 0;
};
