/*
 *
 * Copyright (c) 2016 Raphine Project
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

#ifndef __RAPH_LIBRARY_PTR_H__
#define __RAPH_LIBRARY_PTR_H__

#include <raph.h>

template<class T>
class uptr {
public:
  uptr(uptr &p) {
    _obj = p._obj;
    p._obj = nullptr;
  }
  uptr() {
    _obj = nullptr;
  }
  // to manage raw ptr
  // ptr 'obj' must be allocated by 'new'
  uptr(T *obj) {
    _obj = obj;
  }
  template <class ...Args>
  void Init(Args ...args) {
    kassert(_obj == nullptr);
    _obj = new T(args...);
  }
  ~uptr() {
    delete _obj;
  }
  T *operator&();
  T *operator*();
  T *operator->() {
    if (_obj == nullptr) {
      kassert(false);
    }
    return _obj;
  }
  T *GetRawPtr() {
    if (_obj == nullptr) {
      kassert(false);
    }
    return _obj;
  }
private:
  T *_obj;
};

template<class T>
class auptr {
public:
  auptr(auptr &p) {
    _obj = p._obj;
    _len = p._len;
    p._obj = nullptr;
  }
  auptr(T *obj, int len) {
    _obj = obj;
    _len = len;
  }
  auptr() {
    _len = 0;
    _obj = nullptr;
  }
  void Init(int len) {
    kassert(_obj == nullptr);
    _len = len;
    _obj = new T[_len];
  }
  ~auptr() {
    delete[] _obj;
  }
  T *operator&();
  T *operator*();
  T *operator->() {
    if (_obj == nullptr) {
      kassert(false);
    }
    return _obj;
  }
  T & operator [](int n) {
    kassert(0 <= n && n < _len);
    return _obj[n];
  }
  T *GetRawPtr() {
    if (_obj == nullptr) {
      kassert(false);
    }
    return _obj;
  }
  int GetLen() {
    return _len;
  }
private:
  T *_obj;
  int _len;
};

template<class T>
class sptr {
public:
  sptr(const sptr &p) {
    _obj = p._obj;
    _ref_cnt = p._ref_cnt;
    while(!__sync_bool_compare_and_swap(_ref_cnt, *_ref_cnt, *_ref_cnt + 1)) {
    }
  }
  sptr(T *obj) {
    _obj = obj;
    _ref_cnt = new int(1);
  }
  sptr() {
    _obj = nullptr;
  }
  template <class ...Args>
  void Init(Args ...args) {
    kassert(_obj == nullptr);
    _obj = new T(args...);
    _ref_cnt = new int(1);
  }
  ~sptr() {
    while(!__sync_bool_compare_and_swap(_ref_cnt, *_ref_cnt, *_ref_cnt - 1)) {
    }
    if (*_ref_cnt == 0) {
      delete _obj;
      delete _ref_cnt;
    }
  }
  T *operator&();
  T *operator*();
  T *operator->() {
    return _obj;
  }
  T *GetRawPtr() {
    if (_obj == nullptr) {
      kassert(false);
    }
    return _obj;
  }
private:
  T *_obj;
  int *_ref_cnt;
};

#endif // __RAPH_LIBRARY_PTR_H__
