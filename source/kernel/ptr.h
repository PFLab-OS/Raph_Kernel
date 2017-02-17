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

#pragma once

#include <assert.h>

template<class T>
class uptr {
public:
  template <class A>
  uptr(const uptr<A> &p) {
    _obj = p._obj;
    uptr<A> *p_ = const_cast<uptr<A> *>(&p);
    p_->_obj = nullptr;
  }
  uptr(const uptr &p) {
    _obj = p._obj;
    uptr *p_ = const_cast<uptr *>(&p);
    p_->_obj = nullptr;
  }
  uptr() {
    _obj = nullptr;
  }
  // to manage raw ptr
  // ptr 'obj' must be allocated by 'new'
  explicit uptr(T *obj) {
    assert(obj != nullptr);
    _obj = obj;
  }
  uptr &operator=(const uptr &p) {
    *this = const_cast<uptr &>(p);
    return *this;
  }
  uptr &operator=(uptr &p) {
    delete _obj;

    if (p._obj != nullptr) {
      _obj = p._obj;
      p._obj = nullptr;
    } else {
      _obj = nullptr;
    }

    return (*this);
  }
  ~uptr() {
    delete _obj;
  }
  T *operator&();
  T &operator*() {
    return *_obj;
  }
  T *operator->() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  T *GetRawPtr() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  bool IsNull() {
    return _obj == nullptr;
  }
private:
  template <typename A>
  friend class uptr;
  T *_obj;
} __attribute__((__packed__));

template<class Array>
class uptr<Array []> {
public:
  template <class A>
  uptr(const uptr<A> &p) {
    _obj = p._obj;
    uptr<A> *p_ = const_cast<uptr<A> *>(&p);
    p_->_obj = nullptr;
  }
  uptr(const uptr &p) {
    _obj = p._obj;
    uptr *p_ = const_cast<uptr *>(&p);
    p_->_obj = nullptr;
  }
  uptr() {
    _obj = nullptr;
  }
  // to manage raw ptr
  // ptr 'obj' must be allocated by 'new'
  explicit uptr(Array *obj) {
    _obj = obj;
  }
  uptr &operator=(const uptr &p) = delete;
  uptr &operator=(uptr &p) {
    delete [] _obj;

    if (p._obj != nullptr) {
      _obj = p._obj;
      uptr *p_ = const_cast<uptr *>(&p);
      p_->_obj = nullptr;
    } else {
      _obj = nullptr;
    }

    return (*this);
  }
  ~uptr() {
    delete [] _obj;
  }
  Array *operator&();
  Array *operator*();
  Array *operator->() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  Array operator [](int n) {
    return _obj[n];
  }
  Array *GetRawPtr() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  bool IsNull() {
    return _obj == nullptr;
  }
private:
  template <class A>
  friend class uptr;
  Array *_obj;
} __attribute__((__packed__));

template <class T>
inline uptr<T> make_uptr(T *ptr) {
  uptr<T> p(ptr);
  return p;
}

template <class T>
inline uptr<T> make_uptr() {
  uptr<T> p;
  return p;
}

class counter_helper {
private:
  template<class A>
  friend class sptr;
  template<class A>
  friend class wptr;
  counter_helper() {
    weak_cnt = 1;
    shared_cnt = 1;
  }
  int weak_cnt;
  int shared_cnt;
} __attribute__((__packed__));

template<class T>
class wptr;

template<class T>
class sptr {
public:
  template<class A>
  sptr(const sptr<A> &p) {
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp = _helper->shared_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->shared_cnt, tmp, tmp + 1)) {
        tmp = _helper->shared_cnt;
      }
    }
  }
  sptr(const sptr &p) {
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp = _helper->shared_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->shared_cnt, tmp, tmp + 1)) {
        tmp = _helper->shared_cnt;
      }
    }
  }
  explicit sptr(wptr<T> &p) {
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp = _helper->shared_cnt;
      while(tmp != 0 && !__sync_bool_compare_and_swap(&_helper->shared_cnt, tmp, tmp + 1)) {
        tmp = _helper->shared_cnt;
      }
      if (tmp == 0) {
        _obj = nullptr;
        _helper = nullptr;
        return;
      }
    }
  }
  explicit sptr(T *obj) {
    assert(obj != nullptr);
    _obj = obj;
    _helper = new counter_helper();
  }
  sptr() {
    _obj = nullptr;
  }
  sptr &operator=(const sptr &p) {
    *this = const_cast<sptr &>(p);
    return *this;
  }
  sptr &operator=(sptr &p) {
    release();
    
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp = _helper->shared_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->shared_cnt, tmp, tmp + 1)) {
        tmp = _helper->shared_cnt;
      }
    }

    return (*this);
  }
  ~sptr() {
    release();
  }
  T *operator&();
  T &operator*();
  T *operator->() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  bool operator==(const sptr& rhs) const {
    return _obj == rhs._obj;
  }
  bool operator!=(const sptr& rhs) const {
    return _obj != rhs._obj;
  }
  T *GetRawPtr() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  bool IsNull() {
    return _obj == nullptr;
  }
  int GetCnt() {
    return _helper->shared_cnt;
  }
private:
  void release() {
    if (_obj != nullptr) {
      int tmp = _helper->shared_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->shared_cnt, tmp, tmp - 1)) {
        tmp = _helper->shared_cnt;
      }
      if (tmp == 1) {
        delete _obj;
        int tmp2 = _helper->weak_cnt;
        while(!__sync_bool_compare_and_swap(&_helper->weak_cnt, tmp2, tmp2 - 1)) {
          tmp2 = _helper->weak_cnt;
        }
        if (tmp2 == 1) {
          delete _helper;
        }
      }
    }
  }
  template <class A>
  friend class sptr;
  template <class A>
  friend class wptr;
  T *_obj;
  counter_helper *_helper;
} __attribute__((__packed__));

template<class T>
class wptr {
public:
  template<class A>
  wptr(const wptr<A> &p) {
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp2 = _helper->weak_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->weak_cnt, tmp2, tmp2 + 1)) {
        tmp2 = _helper->weak_cnt;
      }
    }
  }
  wptr(const wptr &p) {
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp2 = _helper->weak_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->weak_cnt, tmp2, tmp2 + 1)) {
        tmp2 = _helper->weak_cnt;
      }
    }
  }
  wptr() {
    _obj = nullptr;
  }
  explicit wptr(sptr<T> &p) {
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp2 = _helper->weak_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->weak_cnt, tmp2, tmp2 + 1)) {
        tmp2 = _helper->weak_cnt;
      }
    }
  }
  wptr &operator=(const wptr &p) {
    *this = const_cast<wptr &>(p);
    return *this;
  }
  wptr &operator=(wptr &p) {
    release();
    
    if (p._obj == nullptr) {
      _obj = nullptr;
      _helper = nullptr;
    } else {
      _obj = p._obj;
      _helper = p._helper;
      int tmp2 = _helper->weak_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->weak_cnt, tmp2, tmp2 + 1)) {
        tmp2 = _helper->weak_cnt;
      }
    }

    return (*this);
  }
  ~wptr() {
    release();
  }
  T *operator&();
  T &operator*() {
    return *_obj;
  }
  T *operator->() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  bool operator==(const wptr& rhs) const {
    return _obj == rhs._obj;
  }
  bool operator!=(const wptr& rhs) const {
    return _obj != rhs._obj;
  }
  T *GetRawPtr() {
    if (_obj == nullptr) {
      assert(false);
    }
    return _obj;
  }
  bool IsNull() {
    return _obj == nullptr;
  }
  bool IsObjReleased() {
    if (_helper == nullptr) {
      return true;
    }
    return _helper->shared_cnt == 0;
  }
private:
  void release() {
    if (_obj != nullptr) {
      int tmp2 = _helper->weak_cnt;
      while(!__sync_bool_compare_and_swap(&_helper->weak_cnt, tmp2, tmp2 - 1)) {
        tmp2 = _helper->weak_cnt;
      }
      if (tmp2 == 1) {
        delete _helper;
      }
    }
  }
  template <typename A>
  friend class wptr;
  template <typename A>
  friend class sptr;
  T *_obj;
  counter_helper *_helper;
} __attribute__((__packed__));
template <class T>
inline sptr<T> make_sptr(T *ptr) {
  sptr<T> p(ptr);
  return p;
}
template <class T>
inline sptr<T> make_sptr(wptr<T> &ptr) {
  sptr<T> p(ptr);
  return p;
}
template<class T>
inline sptr<T> make_sptr() {
  sptr<T> p;
  return p;
}
template <class T>
inline wptr<T> make_wptr(sptr<T> &ptr) {
  wptr<T> p(ptr);
  return p;
}
