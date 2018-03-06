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
 * Author: mumumu
 *
 */

#include "set.h"
#include "test.h"
#include <iostream>
#include <pthread.h>
#include <exception>
#include <stdexcept>
#include <ptr.h>
using namespace std;

class SetTester_EmptyPop : public Tester {
public:
  virtual bool Test() override {
    sptr<int> r1 = _set.Pop();
    kassert(r1.IsNull());

    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_PushNull : public Tester {
public:
  virtual bool Test() override {
    sptr<int> r1;
    kassert(_set.Push(r1) == false);

    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_SinglePushPop : public Tester {
public:
  virtual bool Test() override {
    sptr<int> s1 = make_sptr(new int(1));
    _set.Push(s1);

    sptr<int> r1 = _set.Pop();
    kassert(r1.GetRawPtr() == s1.GetRawPtr());
    kassert(*(r1.GetRawPtr()) == *(s1.GetRawPtr()));

    sptr<int> r2 = _set.Pop();
    kassert(r2.IsNull());

    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_SinglePushPopWithCondition : public Tester {
public:
  virtual bool Test() override {
    sptr<int> s1 = make_sptr(new int(1));
    _set.Push(s1);

    sptr<int> r1 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 2){return true;}return false;})));
    kassert(r1.IsNull());

    sptr<int> r2 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 1){return true;}return false;})));
    kassert(r2.GetRawPtr() == s1.GetRawPtr());
    kassert(*(r2.GetRawPtr()) == *(s1.GetRawPtr()));

    sptr<int> r3 = _set.Pop();
    kassert(r3.IsNull());

    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_TriplePushPop : public Tester {
public:
  virtual bool Test() override {
    sptr<int> s1 = make_sptr(new int(1)),s2 = make_sptr(new int(2)),s3 = make_sptr(new int(3));
    _set.Push(s1);
    _set.Push(s2);
    _set.Push(s3);

    sptr<int> r1 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 4){return true;} return false;})));
    kassert(r1.IsNull());

    sptr<int> r2 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 3){return true;} return false;})));
    kassert(*(r2.GetRawPtr()) == 3);
    kassert(*(r2.GetRawPtr()) == *(s3.GetRawPtr()));

    sptr<int> r3 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 3){return true;} return false;})));
    kassert(r3.IsNull());

    sptr<int> r4 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 2){return true;} return false;})));
    kassert(*(r4.GetRawPtr()) == 2);
    kassert(*(r4.GetRawPtr()) == *(s2.GetRawPtr()));

    sptr<int> r5 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 1){return true;} return false;})));
    kassert(*(r5.GetRawPtr()) == 1);
    kassert(*(r5.GetRawPtr()) == *(s1.GetRawPtr()));

    kassert(_set.IsEmpty());
    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_SinglePushPopWithOtherInterface : public Tester {
public:
  virtual bool Test() override {
    sptr<int> s1 = make_sptr(new int(1));
    _set.Push(s1);

    sptr<int> r1;
    _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 2){return true;}return false;})),r1);
    kassert(r1.IsNull());

    sptr<int> r2;
    _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 1){return true;}return false;})),r2);
    kassert(r2.GetRawPtr() == s1.GetRawPtr());
    kassert(*(r2.GetRawPtr()) == *(s1.GetRawPtr()));

    sptr<int> r3;
    _set.Pop(r3);
    kassert(r3.IsNull());

    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_EmptyMap : public Tester {
public:
  virtual bool Test() override {
    _set.Map(make_uptr(new Function0<void,sptr<int>>([](sptr<int> s){})));
    kassert(_set.IsEmpty());
    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);

class SetTester_TriplePushAndMap : public Tester {
public:
  virtual bool Test() override {
    sptr<int> s1 = make_sptr(new int(1)),s2 = make_sptr(new int(2)),s3 = make_sptr(new int(3));
    _set.Push(s1);
    _set.Push(s2);
    _set.Push(s3);

    _set.Map(make_uptr(new Function0<void,sptr<int>>([](sptr<int> s){*(s.GetRawPtr()) = 5;})));

    sptr<int> r1 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 1){return true;} return false;})));
    kassert(r1.IsNull());

    sptr<int> r2 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 5){return true;} return false;})));
    kassert(*(r2.GetRawPtr()) == 5);

    r2 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 5){return true;} return false;})));
    kassert(*(r2.GetRawPtr()) == 5);

    r2 = _set.Pop(make_uptr(new Function0<bool,sptr<int>>([](sptr<int> s){ if(*(s.GetRawPtr()) == 5){return true;} return false;})));
    kassert(*(r2.GetRawPtr()) == 5);

    kassert(_set.IsEmpty());
    return true;
  }
private:
  Set<int> _set;
} static OBJ(__LINE__);
