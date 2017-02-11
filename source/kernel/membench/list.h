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

#include "spinlock.h"

template <class L, int kListEntryNum>
class LinkedList {
public:
  LinkedList() {
    for (int j = 0; j < kListEntryNum; j++) {
      _first.i[j] = 0;
    }
    _first.next = nullptr;
    _last = &_first;
  }
  int Get() {
    Container *c = nullptr;
    _lock.Lock();
    if (_first.next != nullptr) {
      c = _first.next;
      _first.next = c->next;
      if (_last == c) {
        _last = &_first;
      }
    }
    _lock.Unlock();
    int i = 0;
    if (c != nullptr) {
      for (int l = 0; l < 1000; l++) {
        int k = 1;
        for (int j = 0; j < kListEntryNum; j++) {
          k *= c->i[j];
        }
        i += k;
      }
      delete c;
    }
    return i;
  }
  void Push(int i) {
    assert(i != 0);
    Container *c = new Container;
    for (int j = 0; j < kListEntryNum; j++) {
      c->i[j] = i;
    }
    c->next = nullptr;
    _lock.Lock();
    _last->next = c;
    _last = c;
    _lock.Unlock();
  }
  struct Container {
    Container *next;
    int i[kListEntryNum];
  };
private:
  L _lock;
  Container _first; // 番兵
  Container *_last;
};

template<int kListEntryNum>
struct Container {
  Container *next;
  int i[kListEntryNum];
  // char dummy[64 - kListEntryNum * 4 - 8];
  int Calc() {
    int x = 1;
    // if (c != nullptr) {
    //   for (int l = 0; l < 1000; l++) {
    //     int k = 1;
    //     for (int j = 0; j < kListEntryNum; j++) {
    //       k *= c->i[j];
    //     }
    //     i += k;
    //   }
    // }
    int k = 1;
    for (int l = 0; l < kListEntryNum; l++) {
      for (int j = 0; j < kListEntryNum; j++) {
        k += i[j] * i[l];
      }
      x *= k;
    }
    if (x == 0) {
      return 1;
    }
    return x;
  }
};

template <class L, int kListEntryNum>
class LinkedList2 {
public:
  LinkedList2() {
    for (int j = 0; j < kListEntryNum; j++) {
      _first.i[j] = 0;
    }
    _first.next = nullptr;
    _last = &_first;
  }
  Container<kListEntryNum> *Get() {
    Container<kListEntryNum> *c = nullptr;
    _lock.Lock();
    if (_first.next != nullptr) {
      c = _first.next;
      _first.next = c->next;
      if (_last == c) {
        _last = &_first;
      }
    }
    _lock.Unlock();
    return c;
  }
  void Push(Container<kListEntryNum> *c, int i) {
    assert(i != 0);
    for (int j = 0; j < kListEntryNum; j++) {
      c->i[j] = i;
    }
    _lock.Lock();
    PushSub(c);
    _lock.Unlock();
  }
  void PushSub(Container<kListEntryNum> *c) {
    c->next = nullptr;
    _last->next = c;
    _last = c;
  }
  bool Acquire() {
    return _lock.TryLock();
  }
  void Release() {
    _lock.Unlock();
  }
private:
  L _lock;
  Container<kListEntryNum> _first; // 番兵
  Container<kListEntryNum> *_last;
};

template <int kListEntryNum>
class LinkedList3 {
public:
  LinkedList3() {
    for (int j = 0; j < kListEntryNum; j++) {
      _first.i[j] = 0;
    }
    _first.next = nullptr;
    _last = &_first;
  }
  Container<kListEntryNum> *Get() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    Container<kListEntryNum> *c = nullptr;
    while (true) {
      c = _list[apicid / 8].Get();
      if (c != nullptr) {
        break;
      }
      if (_list[apicid / 8].Acquire()) {
        _lock.Lock();
        bool flag = false;
        for (int i = 0; i < 16; i++) {
          if (_first.next != nullptr) {
            flag = true;
            Container<kListEntryNum> *c2 = _first.next;
            _first.next = c2->next;
            if (_last == c2) {
              _last = &_first;
            }
            _list[apicid / 8].PushSub(c2);
          }
        }
        _lock.Unlock();
        _list[apicid / 8].Release();
        if (!flag) {
          return nullptr;
        }
      }
    }
    
    return c;
  }
  void Push(Container<kListEntryNum> *c, int i) {
    assert(i != 0);
    for (int j = 0; j < kListEntryNum; j++) {
      c->i[j] = i;
    }
    c->next = nullptr;
    _lock.Lock();
    _last->next = c;
    _last = c;
    _lock.Unlock();
  }
private:
  SimpleSpinLock _lock;
  LinkedList2<SimpleSpinLock, kListEntryNum> _list[37];
  Container<kListEntryNum> _first; // 番兵
  Container<kListEntryNum> *_last;
};
