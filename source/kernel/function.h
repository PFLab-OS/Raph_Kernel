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

#include <global.h>
#include <mem/virtmem.h>
#include <ptr.h>

class GenericFunction {
public:
  GenericFunction() {
  }
  virtual ~GenericFunction() {
  }
  virtual void Execute() {
  }
};

template<class T>
class Function : public GenericFunction {
public:
  Function(void (*func)(T), T arg) {
    _func = func;
    _arg = arg;
  }
  virtual ~Function() {
  }
  Function(const Function &obj) {
    _func = obj._func;
    _arg = obj._arg;
  }
  virtual void Execute() override {
    _func(_arg);
  }
private:
  Function();
  void (*_func)(T);
  T _arg;
};

template<class T1, class T2>
class Function2 : public GenericFunction {
public:
  Function2(void (*func)(T1, T2), T1 arg1, T2 arg2) {
    _func = func;
    _arg1 = arg1;
    _arg2 = arg2;
  }
  virtual ~Function2() {
  }
  Function2(const Function2 &obj) {
    _func = obj._func;
    _arg1 = obj._arg1;
    _arg2 = obj._arg2;
  }
  virtual void Execute() override {
    _func(_arg1, _arg2);
  }
private:
  Function2();
  void (*_func)(T1, T2);
  T1 _arg1;
  T2 _arg2;
};

template <class C, class T1>
class ClassFunction : public GenericFunction {
public:
  ClassFunction(C *c, void (C::*func)(T1), T1 arg) {
    _c = c;
    _func = func;
    _arg = arg;
  }
  ClassFunction(const ClassFunction &obj) {
    _c = obj._c;
    _func = obj._func;
    _arg = obj._arg;
  }
  virtual ~ClassFunction() {
  }
  virtual void Execute() override {
     (_c->*_func)(_arg);
  }
private:
  ClassFunction();
  C *_c;
  void (C::*_func)(T1);
  T1 _arg;
};

template <class C, class T1, class T2>
class ClassFunction2 : public GenericFunction {
public:
  ClassFunction2(C *c, void (C::*func)(T1, T2), T1 arg1, T2 arg2) {
    _c = c;
    _func = func;
    _arg1 = arg1;
    _arg2 = arg2;
  }
  ClassFunction2(const ClassFunction2 &obj) {
    _c = obj._c;
    _func = obj._func;
    _arg1 = obj._arg1;
    _arg2 = obj._arg2;
  }
  virtual ~ClassFunction2() {
  }
  virtual void Execute() override {
    (_c->*_func)(_arg1, _arg2);
  }
private:
  ClassFunction2();
  C *_c;
  void (C::*_func)(T1, T2);
  T1 _arg1;
  T2 _arg2;
};




// template<class C>
// class GenericFunction2 {
// public:
//   GenericFunction2() {
//   }
//   virtual ~GenericFunction2() {
//   }
//   virtual void Execute(C) {
//   }
// };

// template<class C, class T>
// class Function2 : public GenericFunction2<C> {
// public:
//   Function2(void (*func)(C, T *), T *arg) {
//     _func = func;
//     _arg = arg;
//   }
//   virtual ~Function2() {
//   }
//   Function2(const Function2 &obj) {
//     _func = obj._func;
//     _arg = obj._arg;
//   }
//   virtual void Execute(C c) override {
//     _func(c, _arg);
//   }
// private:
//   Function2();
//   void (*_func)(C, T *);
//   T *_arg;
// };

// template<class C, class T>
// class FunctionU2 : public GenericFunction2<C> {
// public:
//   FunctionU2(void (*func)(C, uptr<T>), uptr<T> arg) {
//     _func = func;
//     _arg = arg;
//   }
//   virtual ~FunctionU2() {
//   }
//   FunctionU2(const FunctionU2 &obj) {
//     _func = obj._func;
//     _arg = obj._arg;
//   }
//   virtual void Execute(C c) override {
//     _func(c, _arg);
//   }
// private:
//   FunctionU2();
//   void (*_func)(C, uptr<T>);
//   uptr<T> _arg;
// };

// template<class C, class T>
// class FunctionS2 : public GenericFunction2<C> {
// public:
//   FunctionS2(void (*func)(C, sptr<T>), sptr<T> arg) {
//     _func = func;
//     _arg = arg;
//   }
//   virtual ~FunctionS2() {
//   }
//   FunctionS2(const FunctionS2 &obj) {
//     _func = obj._func;
//     _arg = obj._arg;
//   }
//   virtual void Execute(C c) override {
//     _func(c, _arg);
//   }
// private:
//   FunctionS2();
//   void (*_func)(C, sptr<T>);
//   sptr<T> _arg;
// };

// template <class C, class T>
// class ClassFunction2 : public GenericFunction2<C> {
// public:
//   ClassFunction2(T *c, void (T::*func)(C, void *), void *arg) {
//     _c = c;
//     _func = func;
//     _arg = arg;
//   }
//   ClassFunction2(const ClassFunction<T> &obj) {
//     _c = obj._c;
//     _func = obj._func;
//     _arg = obj._arg;
//   }
//   virtual ~ClassFunction2() {
//   }
//   virtual void Execute(C c) override {
//     (_c->*_func)(c, _arg);
//   }
// private:
//   ClassFunction2();
//   T *_c;
//   void (T::*_func)(C, void *);
//   void *_arg;
// };


