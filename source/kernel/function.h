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
class GenericFunction {
 public:
  GenericFunction() {}
  virtual ~GenericFunction() {}
  virtual R Execute(Args... args) {
    // This Function must not be called.
    // Because IsBaseOf<S,T>'s T must be abstract class.
    // Following code call IsBaseOf.
    // uptr<GenericFunction<void>>
    kassert(false);
    R r;
    return r;
  };
};

template <class... Args>
class GenericFunction<void, Args...> {
 public:
  GenericFunction() {}
  virtual ~GenericFunction() {}
  virtual void Execute(Args... args) {}
};

template <class R, class... Args>
class Function0 : public GenericFunction<R, Args...> {
 public:
  Function0(R (*func)(Args...)) : _func(func) {}
  virtual ~Function0() { kassert(_ref == 0); }
  Function0(const Function0 &obj) : _func(obj._func) {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = _func(args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  Function0();
  R (*_func)(Args...);
  int _ref = 0;
};

template <class... Args>
class Function0<void, Args...> : public GenericFunction<void, Args...> {
 public:
  Function0(void (*func)(Args...)) : _func(func) {}
  virtual ~Function0() { kassert(_ref == 0); }
  Function0(const Function0 &obj) : _func(obj._func) {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    _func(args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  Function0();
  void (*_func)(Args...);
  int _ref = 0;
};

template <class R, class T, class... Args>
class Function1 : public GenericFunction<R, Args...> {
 public:
  Function1(R (*func)(T, Args...), T arg) : _func(func), _arg(arg) {}
  virtual ~Function1() { kassert(_ref == 0); }
  Function1(const Function1 &obj) : _func(obj._func), _arg(obj._arg) {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = _func(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  Function1();
  R (*_func)(T, Args...);
  T _arg;
  int _ref = 0;
};

template <class T, class... Args>
class Function1<void, T, Args...> : public GenericFunction<void, Args...> {
 public:
  Function1(void (*func)(T, Args...), T arg) : _func(func), _arg(arg) {}
  virtual ~Function1() { kassert(_ref == 0); }
  Function1(const Function1 &obj) : _func(obj._func), _arg(obj._arg) {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    _func(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  Function1();
  void (*_func)(T, Args...);
  T _arg;
  int _ref = 0;
};

template <class R, class T1, class T2, class... Args>
class Function2 : public GenericFunction<R, Args...> {
 public:
  Function2(R (*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _func(func), _arg1(arg1), _arg2(arg2) {}
  virtual ~Function2() { kassert(_ref == 0); }
  Function2(const Function2 &obj)
      : _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = _func(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  Function2();
  R (*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class T1, class T2, class... Args>
class Function2<void, T1, T2, Args...> : public GenericFunction<void, Args...> {
 public:
  Function2(void (*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _func(func), _arg1(arg1), _arg2(arg2) {}
  virtual ~Function2() { kassert(_ref == 0); }
  Function2(const Function2 &obj)
      : _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    _func(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  Function2();
  void (*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class R, class T1, class T2, class T3, class... Args>
class Function3 : public GenericFunction<R, Args...> {
 public:
  Function3(R (*func)(T1, T2, T3, Args...), T1 arg1, T2 arg2, T3 arg3)
      : _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  virtual ~Function3() { kassert(_ref == 0); }
  Function3(const Function3 &obj)
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
  Function3();
  R (*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

template <class T1, class T2, class T3, class... Args>
class Function3<void, T1, T2, T3, Args...>
    : public GenericFunction<void, Args...> {
 public:
  Function3(void (*func)(T1, T2, T3, Args...), T1 arg1, T2 arg2, T3 arg3)
      : _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  virtual ~Function3() { kassert(_ref == 0); }
  Function3(const Function3 &obj)
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
  Function3();
  void (*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

// Do not define Function4!
// use struct as an alternative argument.

template <class R, class C, class... Args>
class ClassFunction0 : public GenericFunction<R, Args...> {
 public:
  ClassFunction0(C *c, R (C::*func)(Args...)) : _c(c), _func(func) {}
  ClassFunction0(const ClassFunction0 &obj) : _c(obj._c), _func(obj._func) {}
  virtual ~ClassFunction0() { kassert(_ref == 0); }
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunction0();
  C *_c;
  R (C::*_func)(Args...);
  int _ref = 0;
};

template <class C, class... Args>
class ClassFunction0<void, C, Args...> : public GenericFunction<void, Args...> {
 public:
  ClassFunction0(C *c, void (C::*func)(Args...)) : _c(c), _func(func) {}
  ClassFunction0(const ClassFunction0 &obj) : _c(obj._c), _func(obj._func) {}
  virtual ~ClassFunction0() { kassert(_ref == 0); }
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunction0();
  C *_c;
  void (C::*_func)(Args...);
  int _ref = 0;
};

template <class R, class C, class T1, class... Args>
class ClassFunction1 : public GenericFunction<R, Args...> {
 public:
  ClassFunction1(C *c, R (C::*func)(T1, Args...), T1 arg)
      : _c(c), _func(func), _arg(arg) {}
  ClassFunction1(const ClassFunction1 &obj)
      : _c(obj._c), _func(obj._func), _arg(obj._arg) {}
  virtual ~ClassFunction1() { kassert(_ref == 0); }
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunction1();
  C *_c;
  R (C::*_func)(T1, Args...);
  T1 _arg;
  int _ref = 0;
};

template <class C, class T1, class... Args>
class ClassFunction1<void, C, T1, Args...>
    : public GenericFunction<void, Args...> {
 public:
  ClassFunction1(C *c, void (C::*func)(T1, Args...), T1 arg)
      : _c(c), _func(func), _arg(arg) {}
  ClassFunction1(const ClassFunction1 &obj)
      : _c(obj._c), _func(obj._func), _arg(obj._arg) {}
  virtual ~ClassFunction1() { kassert(_ref == 0); }
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(_arg, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunction1();
  C *_c;
  void (C::*_func)(T1, Args...);
  T1 _arg;
  int _ref = 0;
};

template <class R, class C, class T1, class T2, class... Args>
class ClassFunction2 : public GenericFunction<R, Args...> {
 public:
  ClassFunction2(C *c, R (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2) {}
  ClassFunction2(const ClassFunction2 &obj)
      : _c(obj._c), _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual ~ClassFunction2() {}
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunction2();
  C *_c;
  R (C::*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class C, class T1, class T2, class... Args>
class ClassFunction2<void, C, T1, T2, Args...>
    : public GenericFunction<void, Args...> {
 public:
  ClassFunction2(C *c, void (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2) {}
  ClassFunction2(const ClassFunction2 &obj)
      : _c(obj._c), _func(obj._func), _arg1(obj._arg1), _arg2(obj._arg2) {}
  virtual ~ClassFunction2() {}
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(_arg1, _arg2, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunction2();
  C *_c;
  void (C::*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
  int _ref = 0;
};

template <class R, class C, class T1, class T2, class T3, class... Args>
class ClassFunction3 : public GenericFunction<R, Args...> {
 public:
  ClassFunction3(C *c, R (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2, T3 arg3)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  ClassFunction3(const ClassFunction3 &obj)
      : _c(obj._c),
        _func(obj._func),
        _arg1(obj._arg1),
        _arg2(obj._arg2),
        _arg3(obj._arg3) {}
  virtual ~ClassFunction3() { kassert(_ref == 0); }
  virtual R Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    R res = (_c->*_func)(_arg1, _arg2, _arg3, args...);
    __sync_fetch_and_sub(&_ref, 1);
    return res;
  }

 private:
  ClassFunction3();
  C *_c;
  R (C::*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

template <class C, class T1, class T2, class T3, class... Args>
class ClassFunction3<void, C, T1, T2, T3, Args...>
    : public GenericFunction<void, Args...> {
 public:
  ClassFunction3(C *c, void (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2,
                 T3 arg3)
      : _c(c), _func(func), _arg1(arg1), _arg2(arg2), _arg3(arg3) {}
  ClassFunction3(const ClassFunction3 &obj)
      : _c(obj._c),
        _func(obj._func),
        _arg1(obj._arg1),
        _arg2(obj._arg2),
        _arg3(obj._arg3) {}
  virtual ~ClassFunction3() { kassert(_ref == 0); }
  virtual void Execute(Args... args) override {
    __sync_fetch_and_add(&_ref, 1);
    (_c->*_func)(_arg1, _arg2, _arg3, args...);
    __sync_fetch_and_sub(&_ref, 1);
  }

 private:
  ClassFunction3();
  C *_c;
  void (C::*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
  int _ref = 0;
};

// Do not define ClassFunction4!
// use struct as an alternative argument.
