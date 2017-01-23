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

#include <raph.h>
#include <spinlock.h>
#include <ptr.h>

// deplecated
template<class T>
class ObjectList {
public:
  struct Container {
    template<class... Arg>
    Container(Arg... args) : obj(args...) {
      next = nullptr;
    }
    Container *GetNext() {
      return next;
    }
    T *GetObject() {
      return &obj;
    }
  private:
    T obj;
    Container *next;
    friend ObjectList;
  };
  ObjectList() {
    _last = &_first;
    _first.obj = nullptr;
    _first.next = nullptr;
  }
  ~ObjectList() {
  }
  template<class... Arg>
  Container *PushBack(Arg... args) {
    Container *c = new Container(args...);
    Locker locker(_lock);
    kassert(_last->next == nullptr);
    _last->next = c;
    _last = c;
    return c;
  }
  // 帰るコンテナは番兵なので、オブジェクトを格納していない
  Container *GetBegin() {
    return &_first;
  }
  bool IsEmpty() {
    return &_first == _last;
  }
private:
  Container _first;
  Container *_last;
  SpinLock _lock;
};

template<class T>
class List {
public:
  class Container {
  public:
    ~Container() {
    }
    T &operator*() {
      return _obj;
    }
    T *operator&() {
      return &_obj;
    }
    T *operator->() {
      return &_obj;
    }
    wptr<Container> GetNext() {
      Locker locker(_lock);
      return make_wptr(_next);
    }
  private:
    template<class... Arg>
    Container(Arg... args) : _obj(args...) {
    }
    Container();
    T _obj;
    SpinLock _lock;
    sptr<Container> _next;
    sptr<Container> _prev;
    friend List;
  };
  List() {
  }
  ~List() {
    auto iter = GetBegin();
    while(!iter.IsNull()) {
      iter = Remove(iter);
    }
  }
  template<class... Arg>
  wptr<Container> PushBack(Arg... args) {
    auto c = make_sptr(new Container(args...));
    while(true) {
      // TODO this lock is conservative. fix it!
      trylock(_lock) {
        if (_first.IsNull()) {
          SetFirst(c);
          return make_wptr(c);
        } else {
          trylock(_last->_lock) {
            Link(_last, c);
            _last = c;
            return make_wptr(c);
          }
        }
      }
    }
  }
  wptr<Container> Remove(wptr<Container> c) {
    auto c_ = make_sptr(c);
    while(true) {
      // TODO this lock is conservative. fix it!
      trylock(_lock) {
        if (!c_->_lock.Trylock()) {
          break;
        }
        auto next = c_->_next;
        auto prev = c_->_prev;
        if (!next.IsNull() && !next->_lock.Trylock()) {
          break;
        }
        if (!prev.IsNull() && !prev->_lock.Trylock()) {
          break;
        }
        if (_first == c_) {
          if (next.IsNull()) {
            assert(_last == c_);
            _first = make_sptr<Container>();
            _last = make_sptr<Container>();
          } else {
            assert(_last != c_);
            _first = c_->_next;
          }
        } else if (_last == make_sptr(c)) {
          assert(!prev.IsNull());
          _last = prev;
        }
        RemoveSub(c_);
        auto ptr = make_wptr(next);
        if (!next.IsNull()) {
          next->_lock.Unlock();
        }
        if (!prev.IsNull()) {
          prev->_lock.Unlock();
        }
        c_->_lock.Unlock();
        return ptr;
      }
    }
  }
  wptr<Container> GetBegin() {
    Locker locker(_lock);
    return make_wptr(_first);
  }
  bool IsEmpty() {
    Locker locker(_lock);
    return _first.IsNull();
  }
private:
  void SetFirst(sptr<Container> c) {
    assert(_first.IsNull());
    assert(_last.IsNull());
    _first = c;
    _last = c;
  }
  void Link(sptr<Container> c1, sptr<Container> c2) {
    auto tmp = c1->_next;
    c1->_next = c2;
    c2->_next = tmp;
    c2->_prev = c1;
    if (!tmp.IsNull()) {
      assert(tmp->_prev == c1);
      tmp->_prev = c2;
    }
  }
  void RemoveSub(sptr<Container> c) {
    auto next = c->_next;
    auto prev = c->_prev;
    c->_next = make_sptr<Container>();
    c->_prev = make_sptr<Container>();
    if (!next.IsNull()) {
      next->_prev = prev;
    }
    if (!prev.IsNull()) {
      prev->_next = next;
    }
  }
  sptr<Container> _first;
  sptr<Container> _last;
  SpinLock _lock;
};

  
