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

template<class... Args>
class GenericFunction {
public:
  GenericFunction() {
  }
  virtual ~GenericFunction() {
  }
  virtual void Execute(Args... args) {
  }
};

template<class T, class... Args>
class Function : public GenericFunction<Args...> {
public:
  Function(void (*func)(T, Args...), T arg) {
    _func = func;
    _arg = arg;
  }
  virtual ~Function() {
  }
  Function(const Function &obj) {
    _func = obj._func;
    _arg = obj._arg;
  }
  virtual void Execute(Args... args) override {
    _func(_arg, args...);
  }
private:
  Function();
  void (*_func)(T, Args...);
  T _arg;
};

template<class T1, class T2, class... Args>
class Function2 : public GenericFunction<Args...> {
public:
  Function2(void (*func)(T1, T2, Args...), T1 arg1, T2 arg2) {
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
  virtual void Execute(Args... args) override {
    _func(_arg1, _arg2, args...);
  }
private:
  Function2();
  void (*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
};

template <class C, class T1, class... Args>
class ClassFunction : public GenericFunction<Args...> {
public:
  ClassFunction(C *c, void (C::*func)(T1, Args... ), T1 arg) {
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
  virtual void Execute(Args... args) override {
    (_c->*_func)(_arg, args...);
  }
private:
  ClassFunction();
  C *_c;
  void (C::*_func)(T1, Args...);
  T1 _arg;
};

template <class C, class T1, class T2, class... Args>
class ClassFunction2 : public GenericFunction<Args...> {
public:
  ClassFunction2(C *c, void (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2) {
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
  virtual void Execute(Args... args) override {
    (_c->*_func)(_arg1, _arg2, args...);
  }
private:
  ClassFunction2();
  C *_c;
  void (C::*_func)(T1, T2, Args...);
  T1 _arg1;
  T2 _arg2;
};

template <class C, class T1, class T2, class T3, class... Args>
class ClassFunction3 : public GenericFunction<Args...> {
public:
  ClassFunction3(C *c, void (C::*func)(T1, T2, Args...), T1 arg1, T2 arg2, T3 arg3) {
    _c = c;
    _func = func;
    _arg1 = arg1;
    _arg2 = arg2;
    _arg3 = arg3;
  }
  ClassFunction3(const ClassFunction3 &obj) {
    _c = obj._c;
    _func = obj._func;
    _arg1 = obj._arg1;
    _arg2 = obj._arg2;
    _arg3 = obj._arg3;
  }
  virtual ~ClassFunction3() {
  }
  virtual void Execute(Args... args) override {
    (_c->*_func)(_arg1, _arg2, _arg3, args...);
  }
private:
  ClassFunction3();
  C *_c;
  void (C::*_func)(T1, T2, T3, Args...);
  T1 _arg1;
  T2 _arg2;
  T3 _arg3;
};

