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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * 
 */

#pragma once

#include <stdint.h>
#include <raph.h>

// 固定長
template <class T>
class Array {
public:
  explicit Array(int len) {
    _array = new T[len];
    _len = len;
  }
  ~Array() {
    delete[] _array;
  }
  T& operator[](int n) {
    kassert(0 <= n && n < _len);
    return _array[n];
  }
  T& operator=(const T &array) {
    kassert(false);
  }
  T *GetRawPtr() {
    return _array;
  }
  size_t GetLen() {
    return _len;
  }
private:
  Array();
  T *_array;
  size_t _len;
};
