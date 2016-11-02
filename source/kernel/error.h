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

#ifndef __RAPH_LIB_ERROR_H__
#define __RAPH_LIB_ERROR_H__

#include <raph.h>

template<class T>
class ErrorContainer {
public:
  ErrorContainer(T &data) {
    _data = data;
    _error = false;
  }
  ErrorContainer() {
    _error = true;
  }
  T &GetValue() {
    if (_error) {
      kernel_panic("ErrorContainer", "failed to get value.");
    }
    return _data;
  }
  bool IsError() {
    return _error;
  }
private:
  T _data;
  bool _error;
};

template<class T>
class ErrorContainer<T *> {
public:
  ErrorContainer(T *data) {
    _data = data;
  }
  ErrorContainer() {
    _data = nullptr;
  }
  T *GetValue() {
    if (_data == nullptr) {
      kernel_panic("ErrorContainer", "failed to get value.");
    }
    return _data;
  }
  bool IsError() {
    return (_data == nullptr);
  }
private:
  T *_data;
};

#endif /* __RAPH_LIB_ERROR_H__ */
