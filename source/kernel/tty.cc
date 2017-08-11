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

#include <tty.h>
#include <mem/virtmem.h>
#include <cpu.h>
#include <global.h>
#include <task.h>

void Tty::Setup() {
  _cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
  _queue.SetFunction(_cpuid, make_uptr(new ClassFunction<Tty, void *>(this, &Tty::Handle, nullptr)));
  _Setup();
  String::Init(_str_buffer);
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

void Tty::PrintErrString(String *str) {
  for(int i = 0; i < String::length; i++) {
    if (str->str[i] == '\0') {
      return;
    }
    WriteErr(str->str[i]);
  }
  if (str->next != nullptr) {
    PrintErrString(str->next);
  }
}

void Tty::DoString(String *str) {
  if (_cpuid.IsValid() && task_ctrl->GetState(_cpuid) != TaskCtrl::TaskQueueState::kNotStarted) {
    _queue.Push(str);
  } else {
    Locker locker(_lock);
    PrintString(str);
  }
}

Tty::String *Tty::String::New() {
  String *str = new String;
  str->type = Type::kQueue;
  str->offset = 0;
  str->next = nullptr;
  return str;
}

void Tty::String::Init(StringBuffer &buf) {
  while(!buf.IsFull()) {
    String *str = new String;
    str->type = Type::kBuffered;
    str->offset = 0;
    str->next = nullptr;
    buf.Push(str);
  }
}

void Tty::String::Delete(StringBuffer &buf) {
  if (next != nullptr) {
    next->Delete(buf);
  }
  switch(type) {
  case Type::kSingle:
    assert(false);
    break;
  case Type::kQueue:
    delete this;
    break;
  case Type::kBuffered:
    offset = 0;
    next = nullptr;
    assert(buf.Push(this));
    break;
  }
}

void Tty::Vprintf_sub(String *str, const char *fmt, va_list args) {
  while(*fmt != '\0') {
    switch(*fmt) {
    case '%': {
      fmt++;
      switch(*fmt) {
      case '+':
      case '-':
      case '#': {
        fmt++;
        break;
      }
      }
      while(isdigit(*fmt)) {
        fmt++;
      }
      switch(*fmt) {
      case '.': {
        fmt++;
        break;
      }
      }
      int accuracy = 0;
      while(isdigit(*fmt)) {
        accuracy *= 10;
        accuracy += *fmt - '0';
        fmt++;
      }
      // switch(*fmt) {
      // case 'h':
      // case 'l':
      // case 'L': {
      //   fmt++;
      //   break;
      // }
      // }
      switch(*fmt) {
      case '\0': {
        return;
      }
      case 'c': {
        str->Write(static_cast<char>(va_arg(args, int)), _str_buffer);
        break;
      }
      case 's': {
        const char *s = reinterpret_cast<const char *>(va_arg(args, const char *));
        if (accuracy == 0) {
          while(*s) {
            str->Write(*s, _str_buffer);
            s++;
          }
        } else {
          for (; accuracy > 0; accuracy--, s++) { 
            if (*s == '\0') {
              break;
            }
            str->Write(*s, _str_buffer);
          }
        }
        break;
      }
      case 'u': {
        unsigned int num = reinterpret_cast<unsigned int>(va_arg(args, unsigned int));
        unsigned int _num = num;
        unsigned int i = _num;
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
          unsigned int l = _num / i;
          str->Write(l + '0', _str_buffer);
          _num -= l * i;
        }
        break;
      }
      case 'd': {
        int num = reinterpret_cast<int>(va_arg(args, int));
        if (num < 0) {
          str->Write('-', _str_buffer);
        }
        unsigned int _num = (num < 0) ? -num : num;
        unsigned int i = _num;
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
          unsigned int l = _num / i;
          str->Write(l + '0', _str_buffer);
          _num -= l * i;
        }
        break;
      }
      case 'p':
      case 'X':
      case 'x': {
        int num = reinterpret_cast<int>(va_arg(args, int));
        unsigned int _num = num;
        unsigned int i = _num;
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
          unsigned int l = _num / i;
          if (l < 10) {
            str->Write(l + '0', _str_buffer);
          } else if (l < 16) {
            str->Write(l - 10 + 'A', _str_buffer);
          }
          _num -= l * i;
        }
        break;
      }
      case 'l': {
        fmt++;
        switch(*fmt) {
        case 'l': {
          fmt++;
          switch(*fmt) {
          case 'd': {
            int64_t num = reinterpret_cast<int64_t>(va_arg(args, int64_t));
            if (num < 0) {
              str->Write('-', _str_buffer);
            }
            uint64_t _num = (num < 0) ? -num : num;
            uint64_t i = _num;
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
              unsigned int l = _num / i;
              str->Write(l + '0', _str_buffer);
              _num -= l * i;
            }
            break;
          }
          case 'u': {
            uint64_t num = reinterpret_cast<uint64_t>(va_arg(args, uint64_t));
            if (num < 0) {
              str->Write('-', _str_buffer);
            }
            uint64_t _num = num;
            uint64_t i = _num;
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
              unsigned int l = _num / i;
              str->Write(l + '0', _str_buffer);
              _num -= l * i;
            }
            break;
          }
          case 'x': {
            uint64_t num = reinterpret_cast<uint64_t>(va_arg(args, uint64_t));
            uint64_t _num = num;
            uint64_t i = _num;
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
              unsigned int l = _num / i;
              if (l < 10) {
                str->Write(l + '0', _str_buffer);
              } else if (l < 16) {
                str->Write(l - 10 + 'A', _str_buffer);
              }
              _num -= l * i;
            }
            break;
          }
          default: {
            str->Write('%', _str_buffer);
            str->Write('l', _str_buffer);
            str->Write('l', _str_buffer);
            str->Write(*fmt, _str_buffer);
          }
          }
          break;
        }
        default: {
          str->Write('%', _str_buffer);
          str->Write('l', _str_buffer);
          str->Write(*fmt, _str_buffer);
        }
        }
        break;
      }
      default: {
        str->Write('%', _str_buffer);
        str->Write(*fmt, _str_buffer);
      }
      }
      break;
    }
    default: {
      str->Write(*fmt, _str_buffer);
    }
    }
    fmt++;
  }
}
