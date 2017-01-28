/*
 *
 * Copyright (c) 2015 Raphine Project
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
#include <stdarg.h>
#include <ctype.h>
#include <spinlock.h>
#include <queue.h>
#include <global.h>
#include <buf.h>

class Tty {
 public:
  enum class Color : uint8_t {
    kBlack,
    kBlue,
    kGreen,
    kCyan,
    kRed,
    kMagenta,
    kBrown,
    kLightGray,
    kDarkGray,
    kLightBlue,
    kLightGreen,
    kLightCyan,
    kLightRed,
    kLightMagenta,
    kLightBrown,
    kWhite
  };
  Tty() {
  }
  void Init();
  void Cprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Cvprintf(fmt, args);
    va_end(args);
  }
  void CprintfRaw(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    CvprintfRaw(fmt, args);
    va_end(args);
  }
  void Cvprintf(const char *fmt, va_list args) {
    String *str = String::Get(_str_buffer);
    Cvprintf_sub(str, fmt, args);
    str->Exit(_str_buffer);
    DoString(str);
  }
  void CvprintfRaw(const char *fmt, va_list args) {
    String str;
    Cvprintf_sub(&str, fmt, args);
    str.Exit(_str_buffer);
    Locker locker(_lock);
    PrintString(&str);
  }
  virtual void PrintShell(const char *str) = 0;
  virtual void SetColor(Color) = 0;
  virtual void ResetColor() = 0;
 protected:
  virtual void _Init() {
  }
  virtual void Write(uint8_t c) = 0;
  int _cx = 0;
  int _cy = 0;
 private:
  class String;
  using StringBuffer = RingBuffer<String *, 64>;
  class String {
  public:
    enum class Type {
      kSingle,
      kQueue,
      kBuffered,
    } type;
    String() {
      type = Type::kSingle;
      offset = 0;
      next = nullptr;
    } 
    static String *New();
    static void Init(StringBuffer &buf);
    static String *Get(StringBuffer &buf) {
      String *s;
      if (buf.Pop(s)) {
        return s;
      } else {
        String *s_ = New();
        return s_;
      }
    }
    void Delete(StringBuffer &buf);
    void Write(const uint8_t c, StringBuffer &buf) {
      if (offset == length) {
        if (next == nullptr) {
          if (type == Type::kQueue) {
            String *s = New();
            next = s;
            next->Write(c, buf);
          } else if (type == Type::kBuffered) {
            String *s = Get(buf);
            next = s;
            next->Write(c, buf);
          }
        } else {
          next->Write(c, buf);
        }
      } else {
        str[offset] = c;
        offset++;
      }
    }
    void Exit(StringBuffer &buf) {
      Write('\0', buf);
    }
    static const int length = 100;
    uint8_t str[length];
    int offset;
    String *next;
  };
  void Handle(void *){
    void *s;
    while(_queue.Pop(s)) {
      String *str = reinterpret_cast<String *>(s);
      {
        Locker locker(_lock);
        PrintString(str);
      }
      str->Delete(_str_buffer);
    }
  }
  void Cvprintf_sub(String *str, const char *fmt, va_list args);
  void PrintString(String *str);
  void DoString(String *str);
  FunctionalQueue _queue;
  SpinLock _lock;
  CpuId _cpuid;
  StringBuffer _str_buffer;
};

class StringTty : public Tty {
public:
  StringTty() = delete;
  StringTty(int buffer_size) : _buffer_size(buffer_size) {
    _offset = 0;
    _buf = new char[buffer_size];
  }
  ~StringTty() {
    delete[] _buf;
  }
  char *GetRawPtr() {
    return _buf;
  }
private:
  virtual void PrintShell(const char *str) override {
    kassert(false);
  }
  virtual void SetColor(Color) override {
  }
  virtual void ResetColor() override {
  }
  virtual void Write(uint8_t c) {
    assert(_offset + 1 < _buffer_size);
    _buf[_offset] = c;
    _buf[_offset + 1] = '\0';
    _offset++;
  }
  char *_buf;
  int _offset;
  const int _buffer_size;
};

#endif // __RAPH_KERNEL_TTY_H__
