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

class SimpleSpinLockX {
public:
  SimpleSpinLockX() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    int i = 1;
    while(_flag == 1 || !__sync_bool_compare_and_swap(&_flag, 0, 1)) {
      for (int j = 0; j < i; j++) {
        asm volatile("pause;");
      }
      i *= 2;
    }
  }
  void Unlock() {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
  }
private:
  volatile unsigned int _flag = 0;
};

class SimpleSpinLockY {
public:
  SimpleSpinLockY() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint64_t x = __sync_fetch_and_add(&_cnt, 1);
    while(_flag != x) {
    }
  }
  void Unlock() {
    __sync_fetch_and_add(&_flag, 1);
  }
private:
  uint64_t _flag = 0;
  uint64_t _cnt = 0;
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
