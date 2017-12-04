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

#include <timer.h>
#include "test.h"
#include <raph.h>

class TimeTester_Order : public Tester {
public:
  virtual bool Test() override {
    Time t1(100);
    Time t2(200);
    Time t3(200);
    kassert(t1 < t2);
    kassert(t2 > t1);
    kassert(t1 <= t2);
    kassert(t2 >= t1);
    kassert(t2 == t3);
    kassert(t1 != t2);
    return true;
  }
private:
} static OBJ(__LINE__);

class TimeTester_Arith : public Tester {
public:
  virtual bool Test() override {
    Time t1(100);
    Time t2(200);
    Time t3(200);
    kassert(t1 + 200 > t2);
    kassert(t1 + 10 > t1 - 10);
    kassert(t2 + 100 == t3 + 100);
    return true;
  }
private:
} static OBJ(__LINE__);
