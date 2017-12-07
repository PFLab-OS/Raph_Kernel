/*
 *
 * Copyright (c) 2015 Raphine Project
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
 * 動的データ確保アロケータ
 * ある程度の数の同種のオブジェクトを、頻繁に確保、開放する時に使う
 * 領域が足りなくなりそうになったら足りなくなる前に自動で伸ばす
 * 
 */

#ifndef __RAPH_KERNEL_ALLOCATOR_H__
#define __RAPH_KERNEL_ALLOCATOR_H__

#include <spinlock.h>
#include <raph.h>
#include <global.h>
#include <mem/virtmem.h>

template <typename T>
class Allocator {
public:
  Allocator();
  T *Alloc();
  void Free(T *data);
private:
  T *Extend(T *entry);
  class Container {
  public:
    Container() : _next(nullptr), _flag(0) {}
    T _entry[64];
    Container *_next;
    uint64_t _flag;
  } _first;
  Container *_list;
  SpinLock _lock;
};

template <typename T>
Allocator<T>::Allocator() : _list(&_first) {}

template <typename T>
void Allocator<T>::Free(T *data) {
  Locker locker(_lock);
  Container *before = nullptr;
  Container *cur = _list;
  while(cur != nullptr) {
    if (cur->_entry <= data && data <= cur->_entry + 63) {
      if (~cur->_flag == 0 && before != nullptr) {
        before->_next = cur->_next;
        cur->_next = _list;
        _list = cur;
      }
      assert(((reinterpret_cast<size_t>(data) - reinterpret_cast<size_t>(cur->_entry)) % sizeof(T)) == 0);
      cur->_flag &= ~(1 << (data - cur->_entry));
      return;
    }
    before = cur;
    cur = cur->_next;
  }
  assert(false);
}

template <typename T>
T *Allocator<T>::Alloc() {
  bool extend = false;
  T* rval = nullptr;
  while(true) {
    // 最適化回避
    volatile size_t *_list_ptr = const_cast<volatile size_t *>(reinterpret_cast<size_t *>(&this->_list));
    while(~reinterpret_cast<Container *>(*_list_ptr)->_flag == 0) {
      _list_ptr = const_cast<volatile size_t *>(reinterpret_cast<size_t *>(&this->_list));
      asm volatile ("nop");
    }
    {
      Locker locker(_lock);
      int i;
      for (i = 0; i < 64; i++) {
        uint64_t bit = static_cast<uint64_t>(1) << i;
        if ((_list->_flag & bit) == 0) {
          break;
        }
      }
      if (i == 64) {
        break;
      }
      rval = &_list->_entry[i];
      _list->_flag |= static_cast<uint64_t>(1) << i;
      if (~_list->_flag == 0) {
        if (_list->_next == nullptr || ~_list->_next->_flag == 0) {
          extend = true;
          break;
        }
        Container *tmp = _list;
        _list = _list->_next;
        Container *cur = _list;
        while(cur->_next != nullptr && ~cur->_next->_flag != 0) {
          cur = cur->_next;
        }
        tmp->_next = cur->_next;
        cur->_next = tmp;
      }
    }
    if (rval != nullptr) {
      break;
    }
  }
  if (extend) {
    return Extend(rval);
  }
  return rval;
}

template <typename T>
T *Allocator<T>::Extend(T *entry) {
  Container *tmp = nullptr;
  kassert(kernel_virtmem_ctrl != nullptr);
  tmp = reinterpret_cast<Container *>(kernel_virtmem_ctrl->KernelHeapAlloc(sizeof(Container)));
  tmp = new(tmp) Container;
  Locker locker(_lock);
  tmp->_next = _list;
  _list = tmp;
  return entry;
}

#endif // __RAPH_KERNEL_ALLOCATOR_H__
