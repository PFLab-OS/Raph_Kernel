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

#include <string.h>
#include <stdint.h>
#include <assert.h>

class Tty {
 public:
  Tty() {
  }
  void Printf() {
  }
  template<class T>
    void Printf(T /* arg */) {
  }
  template<class... T2>
    void Printf(const char *arg1, const char arg2, const T2& ...args) {
    if (strcmp(arg1, "c")) {
      Printf("s", "(invalid format)", args...);
    } else {
      Write(arg2);
      Printf(args...);
    }
  }
  template<class... T2>
    void Printf(const char* arg1, const char *arg2, const T2& ...args) {
    if (strcmp(arg1, "s")) {
      Printf("s", "(invalid format)");
    } else {
      if (arg2[0] != 0) {
	Write(arg2[0]);
	Printf("s", arg2 + 1);
      }
    }
    Printf(args...);
  }
  template<class... T2>
    void PrintInt(const char *arg1, const int arg2, const T2& ...args) {
    if (!strcmp(arg1, "d")) {
      if (arg2 < 0) {
	Write('-');
      }
      unsigned int _arg2 = (arg2 < 0) ? -arg2 : arg2;
      unsigned int i = _arg2;
      int digit = 0;
      while (i >= 10) {
	i /= 10;
	digit++;
      }
      for (int j = digit; j >= 0; j--) {
	i = 1;
	for (int k = 0; k < j; k++) {
	  i *= 10;
	}
	unsigned int l = _arg2 / i;
	Write(l + '0');
	_arg2 -= l * i;
      }
    } else if (!strcmp(arg1, "x")) {
      unsigned int _arg2 = arg2;
      unsigned int i = _arg2;
      int digit = 0;
      while (i >= 16) {
	i /= 16;
	digit++;
      }
      for (int j = digit; j >= 0; j--) {
	i = 1;
	for (int k = 0; k < j; k++) {
	  i *= 16;
	}
	unsigned int l = _arg2 / i;
	if (l < 10) {
	  Write(l + '0');
	} else if (l < 16) {
	  Write(l - 10 + 'A');
	}
	_arg2 -= l * i;
      }
    } else {
      Printf("s", "(invalid format)");
    }
    Printf(args...);
  } 
  template<class... T2>
    void Printf(const char *arg1, const int arg2, const T2& ...args) {
      PrintInt(arg1, arg2, args...);
    }
  template<class... T2>
    void Printf(const char *arg1, const unsigned int arg2, const T2& ...args) {
      PrintInt(arg1, arg2, args...);
    }
  template<class T1, class... T2>
    void Print(const char *arg1, const T1& /*arg2*/, const T2& ...args) {
    Printf("s", "(unknown)", args...);
  }
  template<class T1, class T2, class... T3>
    void Print(const T1& /*arg1*/, const T2& /*arg2*/, const T3& ...args) {
    Printf("s", "(invalid format)", args...);
  }
 private:
  virtual void Write(uint8_t c) {
    assert(false);
  }
};

#endif // __RAPH_KERNEL_TTY_H__
