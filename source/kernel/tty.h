/*
 *
 * Copyright (c) 2015 Project Raphine
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

#ifndef __RAPH_KERNEL_TTY_H__
#define __RAPH_KERNEL_TTY_H__

#include <stdint.h>
#include <assert.h>

class Tty {
 public:
  Tty() {
  }
  void Print() {
  }
  template<class... T2>
    void Print(const char arg1, const T2& ...args) {
    Write(arg1);
    Print(args...);
  }
  template<class... T2>
    void Print(const char* arg1, const T2& ...args) {
    if (arg1[0] == 0) {
      return;
    }
    Write(arg1[0]);
    Print(arg1 + 1, args...);
  }
  template<class... T2>
    void Print(const int arg1, const T2& ...args) {
    if (arg1 >= 0 && arg1 < 10) {
      Write(arg1 + '0');
      Print(args...);
    } else if (arg1 < 0) {
      Write('-');
      Print(-arg1, args...);
    } else {
      int i = arg1;
      int j = 1;
      while(i >= 10) {
	i /= 10;
	j *= 10;
      }
      Write(i + '0');
      Print(arg1 - i * j, args...);
    }
  } 
  template<class T1, class... T2>
    void Print(const T1& /*arg1*/, const T2& ...args) {
    Print("(unknown)", args...);
  }
 private:
  virtual void Write(uint8_t c) {
    assert(false);
  }
};

#endif // __RAPH_KERNEL_TTY_H__
