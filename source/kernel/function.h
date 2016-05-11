/*
 *
 * Copyright (c) 2016 Project Raphine
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
    _obj = virtmem_ctrl->New<FunctionBaseObj>();
  }
  virtual ~FunctionBase() {
    virtmem_ctrl->Delete<FunctionBaseObj>(_obj);
  }
  void Copy(const GenericFunction &obj) {
    // TODO Lockをかけるべき？
    virtmem_ctrl->Delete<FunctionBaseObj>(_obj);
    _obj = obj.GetObj()->Duplicate();
  }
private:
  FunctionBase(const FunctionBase &obj);
  virtual FunctionBaseObj *GetObj() const override {
    return _obj;
  }
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

#endif /* __RAPH_KERNEL_FUNCTION_H__ */
