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
  Function(void (*func)(T *), T *arg) {
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
  void (*_func)(T *);
  T *_arg;
};

template<class T>
class FunctionU : public GenericFunction {
public:
  FunctionU(void (*func)(uptr<T>), uptr<T> arg) {
    _func = func;
    _arg = arg;
  }
  virtual ~FunctionU() {
  }
  FunctionU(const FunctionU &obj) {
    _func = obj._func;
    _arg = obj._arg;
  }
  virtual void Execute() override {
    _func(_arg);
  }
private:
  FunctionU();
  void (*_func)(uptr<T>);
  uptr<T> _arg;
};

template<class T>
class FunctionS : public GenericFunction {
public:
  FunctionS(void (*func)(sptr<T>), sptr<T> arg) {
    _func = func;
    _arg = arg;
  }
  virtual ~FunctionS() {
  }
  FunctionS(const FunctionS &obj) {
    _func = obj._func;
    _arg = obj._arg;
  }
  virtual void Execute() override {
    _func(_arg);
  }
private:
  FunctionS();
  void (*_func)(sptr<T>);
  sptr<T> _arg;
};

template <class T>
class ClassFunction : public GenericFunction {
public:
  ClassFunction(T *c, void (T::*func)(void *), void *arg) {
    _c = c;
    _func = func;
    _arg = arg;
  }
  ClassFunction(const ClassFunction<T> &obj) {
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
  T *_c;
  void (T::*_func)(void *);
  void *_arg;
};






template<class C>
class FunctionBaseObj2 {
public:
  FunctionBaseObj2() {
  }
  FunctionBaseObj2(const FunctionBaseObj2 &f) {
  }
  virtual ~FunctionBaseObj2() {
  }
  void Execute(C *that) {
    if (CanExecute()) {
      ExecuteSub(that);
    }
  }
  virtual bool CanExecute() {
    return false;
  }
  virtual void Clear() {
  }
  virtual FunctionBaseObj2<C> *Duplicate() const {
    return virtmem_ctrl->New<FunctionBaseObj2<C>>(*this);
  }
protected:
  virtual void ExecuteSub(C *that) {
  }
};

template<class C>
class FunctionObj2 : public FunctionBaseObj2<C> {
public:
  FunctionObj2() {
  }
  FunctionObj2(const FunctionObj2 &f) {
    _func = f._func;
    _arg = f._arg;
  }
  virtual ~FunctionObj2() {
  }
  void Init(void (*func)(C *, void *), void *arg) {
    _func = func;
    _arg = arg;
  }
  virtual bool CanExecute() override {
    return (_func != nullptr);
  }
  virtual void Clear() override {
    _func = nullptr;
  }
  virtual FunctionBaseObj2<C> *Duplicate() const override {
    return virtmem_ctrl->New<FunctionObj2<C>>(*this);
  }
private:
  virtual void ExecuteSub(C *that) override {
    _func(that, _arg);
  }
  void (*_func)(C *, void *) = nullptr;
  void *_arg;
  C *_class;
};

template <class T, class C>
class ClassFunctionObj2 : public FunctionBaseObj2<C> {
public:
  ClassFunctionObj2() {
  }
  ClassFunctionObj2(const ClassFunctionObj2<T, C> &f) {
    _c = f._c;
    _func = f._func;
    _arg = f._arg;
  }
  virtual ~ClassFunctionObj2() {
  }
  void Init(T *c, void (T::*func)(C *, void *), void *arg) {
    _c = c;
    _func = func;
    _arg = arg;
  }
  virtual bool CanExecute() override {
    return (_func != nullptr);
  }
  virtual void Clear() override {
    _func = nullptr;
  }
  virtual FunctionBaseObj2<C> *Duplicate() const override {
    return virtmem_ctrl->New<ClassFunctionObj2<T, C>>(*this);
  }
private:
  virtual void ExecuteSub(C *that) override {
    (_c->*_func)(that, _arg);
  }
  T *_c;
  void (T::*_func)(C *, void *) = nullptr;
  void *_arg;
};

template<class C>
class GenericFunction2 {
public:
  GenericFunction2() {
  }
  virtual ~GenericFunction2() {
  }
  void Execute(C *c) {
    GetObj()->Execute(c);
  }
  bool CanExecute() {
    return GetObj()->CanExecute();
  }
  void Clear() {
    GetObj()->Clear();
  }
  virtual FunctionBaseObj2<C> *GetObj() const = 0;
};
template<class C>
class FunctionBase2 : public GenericFunction2<C> {
public:
  FunctionBase2() {
    _obj = &dummy;
  }
  virtual ~FunctionBase2() {
    if (_obj != &dummy) {
      delete(_obj);
    }
  }
  void Copy(const GenericFunction2<C> &obj) {
    // TODO Lockをかけるべき？
    if (_obj != &dummy) {
      delete(_obj);
    }
    _obj = obj.GetObj()->Duplicate();
  }
private:
  FunctionBase2(const FunctionBase2<C> &obj);
  virtual FunctionBaseObj2<C> *GetObj() const override {
    return _obj;
  }
  FunctionBaseObj2<C> dummy;
  FunctionBaseObj2<C> *_obj;
};
template<class C>
class Function2 : public GenericFunction2<C> {
public:
  Function2() {
    _obj = virtmem_ctrl->New<FunctionObj2<C>>();
  }
  virtual ~Function2() {
    virtmem_ctrl->Delete<FunctionObj2<C>>(_obj);
  }
  void Init(void (*func)(C *, void *), void *arg) {
    _obj->Init(func, arg);
  }
private:
  Function2(const Function2 &obj);
  virtual FunctionBaseObj2<C> *GetObj() const override {
    return _obj;
  }
  FunctionObj2<C> *_obj;
};
template <class T, class C>
class ClassFunction2 : public GenericFunction2<C> {
public:
  ClassFunction2() {
    _obj = virtmem_ctrl->New<ClassFunctionObj2<T, C>>();
  }
  virtual ~ClassFunction2() {
    virtmem_ctrl->Delete<ClassFunctionObj2<T, C>>(_obj);
  }
  void Init(T *c, void (T::*func)(C *, void *), void *arg) {
    this->_obj->Init(c, func, arg);
  }
private:
  ClassFunction2(const ClassFunction2<T, C> &obj);
  virtual FunctionBaseObj2<C> *GetObj() const override {
    return _obj;
  }
  ClassFunctionObj2<T, C> *_obj;
};

