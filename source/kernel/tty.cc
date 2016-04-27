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

#include <tty.h>
#include <mem/tmpmem.h>

void Tty::PrintInt(String &str, const char *arg1, const int arg2) {
  if (!strcmp(arg1, "d")) {
    if (arg2 < 0) {
      str.Write('-');
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
      str.Write(l + '0');
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
        str.Write(l + '0');
      } else if (l < 16) {
        str.Write(l - 10 + 'A');
      }
      _arg2 -= l * i;
    }
  } else {
    Printf_sub2(str, "s", "(invalid format)");
  }
} 

void Tty::PrintString(String *str) {
  for(int i = 0; i < String::length; i++) {
    if (str->str[i] == '\0') {
      return;
    }
    Write(str->str[i]);
  }
  if (str->next != nullptr) {
    PrintString(str->next);
  }
}

Tty::String *Tty::String::New() {
  String *str = reinterpret_cast<String *>(tmpmem_ctrl->Alloc(sizeof(String)));
  str->Init();
  return str;
}

void Tty::String::Write(uint8_t c) {
  kassert(offset <= length);
  if (offset == length) {
    if (next == nullptr) {
      if (type == Type::kQueue) {
        String *s = New();
        next = s;
        next->Write(c);
      }
    } else {
      next->Write(c);
    }
  } else {
    str[offset] = c;
    offset++;
  }
}
