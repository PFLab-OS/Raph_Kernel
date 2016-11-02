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

#ifndef __RAPH_KERNEL_FUNCTION_H__
#define __RAPH_KERNEL_FUNCTION_H__

#include <global.h>
#include <mem/virtmem.h>

class FunctionBaseObj {
public:
  FunctionBaseObj() {
  }
  FunctionBaseObj(const FunctionBaseObj &f) {
  }
  virtual ~FunctionBaseObj() {
  }
  void Execute() {
    if (CanExecute()) {
      ExecuteSub();
    }
  }
  virtual bool CanExecute() {
    return false;
  }
  virtual void Clear() {
  }
  virtual FunctionBaseObj *Duplicate() const {
    return virtmem_ctrl->New<FunctionBaseObj>(*this);
  }
protected:
  virtual void ExecuteSub() {
  }
};

class FunctionObj : public FunctionBaseObj {
public:
  FunctionObj() {
  }
  FunctionObj(const FunctionObj &f) {
    _func = f._func;
    _arg = f._arg;
  }
  virtual ~FunctionObj() {
  }
  void Init(void (*func)(void *), void *arg) {
    _func = func;
    _arg = arg;
  }
  virtual bool CanExecute() override {
    return (_func != nullptr);
  }
  virtual void Clear() override {
    _func = nullptr;
  }
  virtual FunctionBaseObj *Duplicate() const override {
    return virtmem_ctrl->New<FunctionObj>(*this);
  }
private:
  virtual void ExecuteSub() override {
    _func(_arg);
  }
  void (*_func)(void *) = nullptr;
  void *_arg;
};

template <class T>
class ClassFunctionObj : public FunctionBaseObj {
public:
  ClassFunctionObj() {
  }
  ClassFunctionObj(const ClassFunctionObj<T> &f) {
    _c = f._c;
    _func = f._func;
    _arg = f._arg;
  }
  virtual ~ClassFunctionObj() {
  }
  void Init(T *c, void (T::*func)(void *), void *arg) {
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
  virtual FunctionBaseObj *Duplicate() const override {
    return virtmem_ctrl->New<ClassFunctionObj<T>>(*this);
  }
private:
  virtual void ExecuteSub() override {
    (_c->*_func)(_arg);
  }
  T *_c;
  void (T::*_func)(void *) = nullptr;
  void *_arg;
};

class GenericFunction {
public:
  GenericFunction() {
  }
  virtual ~GenericFunction() {
  }
  void Execute() {
    GetObj()->Execute();
  }
  bool CanExecute() {
    return GetObj()->CanExecute();
  }
  void Clear() {
    GetObj()->Clear();
  }
  virtual FunctionBaseObj *GetObj() const = 0;
};
class FunctionBase : public GenericFunction {
public:
  FunctionBase() {
    _obj = &dummy;
  }
  virtual ~FunctionBase() {
    if (_obj != &dummy) {
      delete(_obj);
    }
  }
  void Copy(const GenericFunction &obj) {
    // TODO Lockをかけるべき？
    if (_obj != &dummy) {
      delete(_obj);
    }
    _obj = obj.GetObj()->Duplicate();
  }
private:
  FunctionBase(const FunctionBase &obj);
  virtual FunctionBaseObj *GetObj() const override {
    return _obj;
  }
  FunctionBaseObj dummy;
  FunctionBaseObj *_obj;
};
class Function : public GenericFunction {
public:
  Function() {
    _obj = virtmem_ctrl->New<FunctionObj>();
  }
  virtual ~Function() {
    virtmem_ctrl->Delete<FunctionObj>(_obj);
  }
  void Init(void (*func)(void *), void *arg) {
    _obj->Init(func, arg);
  }
private:
  Function(const Function &obj);
  virtual FunctionBaseObj *GetObj() const override {
    return _obj;
  }
  FunctionObj *_obj;
};
template <class T>
class ClassFunction : public GenericFunction {
public:
  ClassFunction() {
    _obj = virtmem_ctrl->New<ClassFunctionObj<T>>();
  }
  virtual ~ClassFunction() {
    virtmem_ctrl->Delete<ClassFunctionObj<T>>(_obj);
  }
  void Init(T *c, void (T::*func)(void *), void *arg) {
    this->_obj->Init(c, func, arg);
  }
private:
  ClassFunction(const ClassFunction<T> &obj);
  virtual FunctionBaseObj *GetObj() const override {
    return _obj;
  }
  ClassFunctionObj<T> *_obj;
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

#endif /* __RAPH_KERNEL_FUNCTION_H__ */
