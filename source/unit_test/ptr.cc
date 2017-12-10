/*
 *
 * Copyright (c) 2017 Raphine Project
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

#include <ptr.h>
#include "test.h"
#include <iostream>

class PtrTestObject1 {
public:
  PtrTestObject1() = delete;
  PtrTestObject1(int &cnt) : _cnt(cnt) {
    __sync_fetch_and_add(&_cnt, 1);
  }
  ~PtrTestObject1() {
    __sync_fetch_and_sub(&_cnt, 1);
  }
  void Func(){
  }
private:
  int &_cnt;
};

class PtrTestObject2 final : public CustomPtrObjInterface {
public:
  PtrTestObject2() = delete;
  PtrTestObject2(int &cnt1, int &cnt2) : _cnt1(cnt1), _cnt2(cnt2) {
    __sync_fetch_and_add(&_cnt1, 1);
  }
  ~PtrTestObject2() {
    __sync_fetch_and_sub(&_cnt1, 1);
  }
  void Func(){
  }
  virtual void Delete() override {
    __sync_fetch_and_add(&_cnt2, 1);
  }
private:
  int &_cnt1;
  int &_cnt2;
};

class PtrTester_UptrBasic : public Tester {
public:
  virtual bool Test() override {
    int cnt = 0;
    do {
      uptr<PtrTestObject1> p(new PtrTestObject1(cnt));
      kassert(cnt == 1);
      p->Func();
    } while(0);
    kassert(cnt == 0);
    return true;
  }
private:
} static OBJ(__LINE__);

class PtrTester_UptrMove : public Tester {
public:
  virtual bool Test() override {
    int cnt = 0;
    do {
      uptr<PtrTestObject1> p1(new PtrTestObject1(cnt));
      uptr<PtrTestObject1> p2 = p1;
      kassert(cnt == 1);
      p2->Func();
      try {
        p1->Func();
        kassert(false);
      } catch (ExceptionAssertionFailure t) {
      };
    } while(0);
    kassert(cnt == 0);
    return true;
  }
private:
} static OBJ(__LINE__);

class PtrTester_CustomUptr : public Tester {
public:
  virtual bool Test() override {
    int cnt1 = 0;
    int cnt2 = 0;
    auto ptr = new PtrTestObject2(cnt1, cnt2);
    kassert(cnt1 == 1);
    do {
      uptr<PtrTestObject2> p1(ptr);
      kassert(cnt1 == 1);
      p1->Func();
    } while(0);
    kassert(cnt1 == 1);
    kassert(cnt2 == 1);
    delete ptr;
    return true;
  }
private:
} static OBJ(__LINE__);

