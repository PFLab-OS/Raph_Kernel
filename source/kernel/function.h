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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#pragma once

#include <raph.h>
#include <global.h>
#include <mem/virtmem.h>
#include <ptr.h>

template <class R, class... Args>
class GenericFunctionX {
 public:
  GenericFunctionX() {}
  virtual ~GenericFunctionX() {}
  virtual R Execute(Args... args) = 0;
};

template <class... Args>
class GenericFunctionX<void, Args...> {
 public:
  GenericFunctionX() {}
  virtual ~GenericFunctionX() {}
  virtual void Execute(Args... args) {}
};

template <class... Args>
using GenericFunction = GenericFunctionX<void, Args...>;

template <class R, class T, class... Args>
class FunctionX : public GenericFunctionX<R, Args...> {
 public:
  FunctionX(R (*func)(T, Args...), T arg) : _func(func), _arg(arg) {}
  virtual ~FunctionX() { kassert(_ref == 0); }
  FunctionX(const FunctionX &obj) : _func(obj._func), _arg(obj._arg) {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = _func(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  FunctionX();
  R (*_func)(T, Args...);
  T _arg;
  int _ref = 0;
};

template <class T, class... Args>
class FunctionX<void, T, Args...> : public GenericFunctionX<void, Args...> {
 public:
  FunctionX(void (*func)(T, Args...), T arg) : _func(func), _arg(arg) {}
  virtual ~FunctionX() { kassert(_ref == 0); }
  FunctionX(const FunctionX &obj) : _func(obj._func), _arg(obj._arg) {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    _func(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  FunctionX();
  void (*_func)(T, Args...);
  T _arg;
  int _ref = 0;
};

template <class T, class... Args>
using Function = FunctionX<void, T, Args...>;

template <class R, class T1, class T2, class... Args>
class FunctionX2 : public GenericFunctionX<R, Args...> {
 public:
  FunctionX2(R (*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _func(func), _arg1(arg1), _arg2(arg2) {}
  virtual ~FunctionX2() { kassert(_ref == 0); }
  FunctionX2(const FunctionX2 &obj)
      : _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = _func(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  FunctionX2();
  R (*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class T1, class T2, class... Args>
class FunctionX2<void, T1, T2, Args...>
    : public GenericFunctionX<void, Args...> {
 public:
  FunctionX2(void (*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _func(func), _arg1(arg1), _arg2(arg2) {}
  virtual ~FunctionX2() { kassert(_ref == 0); }
  FunctionX2(const FunctionX2 &obj)
      : _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    _func(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  FunctionX2();
  void (*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class T1, class T2, class... Args>
using Function2 = FunctionX2<void, T1, T2, Args...>;

template <class R, class T1, class T2, class T3, class... Args>
class FunctionX3 : public GenericFunctionX<R, Args...> {
 public:
  FunctionX3(R (*func)(T1, T2, T3, Args...), T1 arg1, T2 arg2, T3 arg3)
      : _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  virtual ~FunctionX3() { kassert(_ref == 0); }
  FunctionX3(const FunctionX3 &obj)
      : _func(obj._func),
        _arg1(obj._arg1),
        _arg2(obj._arg2),
        _arg3(obj._arg3) {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = _func(_arg1, _arg2, _arg3, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  FunctionX3();
  R (*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

template <class T1, class T2, class T3, class... Args>
class FunctionX3<void, T1, T2, T3, Args...>
    : public GenericFunctionX<void, Args...> {
 public:
  FunctionX3(void (*func)(T1, T2, T3, Args...), T1 arg1, T2 arg2, T3 arg3)
      : _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  virtual ~FunctionX3() { kassert(_ref == 0); }
  FunctionX3(const FunctionX3 &obj)
      : _func(obj._func),
        _arg1(obj._arg1),
        _arg2(obj._arg2),
        _arg3(obj._arg3) {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    _func(_arg1, _arg2, _arg3, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  FunctionX3();
  void (*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

template <class T1, class T2, class T3, class... Args>
using Function3 = FunctionX3<void, T1, T2, T3, Args...>;

// Do not define Function4!
// use struct as an alternative argument.

template <class R, class C, class T1, class... Args>
class ClassFunctionX : public GenericFunctionX<R, Args...> {
 public:
  ClassFunctionX(C *c, R (C::*func)(T1, Args...), T1 arg)
      : _c(c), _func(func), _arg(arg) {}
  ClassFunctionX(const ClassFunctionX &obj)
      : _c(obj._c), _func(obj._func), _arg(obj._arg) {}
  virtual ~ClassFunctionX() { kassert(_ref == 0); }
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunctionX();
  C *_c;
  R (C::*_func)(T1, Args...);
  T1 _arg;
  int _ref = 0;
};

template <class C, class T1, class... Args>
class ClassFunctionX<void, C, T1, Args...>
    : public GenericFunctionX<void, Args...> {
 public:
  ClassFunctionX(C *c, void (C::*func)(T1, Args...), T1 arg)
      : _c(c), _func(func), _arg(arg) {}
  ClassFunctionX(const ClassFunctionX &obj)
      : _c(obj._c), _func(obj._func), _arg(obj._arg) {}
  virtual ~ClassFunctionX() { kassert(_ref == 0); }
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunctionX();
  C *_c;
  void (C::*_func)(T1, Args...);
  T1 _arg;
  int _ref = 0;
};

template <class C, class T, class... Args>
using ClassFunction = ClassFunctionX<void, C, T, Args...>;

template <class R, class C, class T1, class T2, class... Args>
class ClassFunctionX2 : public GenericFunctionX<R, Args...> {
 public:
  ClassFunctionX2(C *c, R (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2) {}
  ClassFunctionX2(const ClassFunctionX2 &obj)
      : _c(obj._c), _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual ~ClassFunctionX2() {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunctionX2();
  C *_c;
  R (C::*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class C, class T1, class T2, class... Args>
class ClassFunctionX2<void, C, T1, T2, Args...>
    : public GenericFunctionX<void, Args...> {
 public:
  ClassFunctionX2(C *c, void (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2) {}
  ClassFunctionX2(const ClassFunctionX2 &obj)
      : _c(obj._c), _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual ~ClassFunctionX2() {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunctionX2();
  C *_c;
  void (C::*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class C, class T1, class T2, class... Args>
using ClassFunction2 = ClassFunctionX2<void, C, T1, T2, Args...>;

template <class R, class C, class T1, class T2, class T3, class... Args>
class ClassFunctionX3 : public GenericFunctionX<R, Args...> {
 public:
  ClassFunctionX3(C *c, R (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2,
                  T3 arg3)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  ClassFunctionX3(const ClassFunctionX3 &obj)
      : _c(obj._c),
        _func(obj._func),
        _arg1(obj._arg1),
        _arg2(obj._arg2),
        _arg3(obj._arg3) {}
  virtual ~ClassFunctionX3() { kassert(_ref == 0); }
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(_arg1, _arg2, _arg3, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunctionX3();
  C *_c;
  R (C::*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

template <class C, class T1, class T2, class T3, class... Args>
class ClassFunctionX3<void, C, T1, T2, T3, Args...>
    : public GenericFunctionX<void, Args...> {
 public:
  ClassFunctionX3(C *c, void (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2,
                  T3 arg3)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  ClassFunctionX3(const ClassFunctionX3 &obj)
      : _c(obj._c),
        _func(obj._func),
        _arg1(obj._arg1),
        _arg2(obj._arg2),
        _arg3(obj._arg3) {}
  virtual ~ClassFunctionX3() { kassert(_ref == 0); }
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(_arg1, _arg2, _arg3, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunctionX3();
  C *_c;
  void (C::*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

template <class C, class T1, class T2, class T3, class... Args>
using ClassFunction3 = ClassFunctionX3<void, C, T1, T2, T3, Args...>;

// Do not define ClassFunction4!
// use struct as an alternative argument.
