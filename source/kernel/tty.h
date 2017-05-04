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

/*
 * How to print strings
 *
 * In many cases, you can use Printf(). However, it takes time from when this function is
 * executed until it is drawn on the screen. If you want to output to the screen instantly,
 * you have to call Flush().
 *
 * ErrPrintf() is used when a serious error occurs in the system. Since this function does
 * not alloc memory internally, there is a high possibility that it will be executed normally
 * even if there is a problem in the system. In order to prevent Printf() from overwriting the
 * screen while this function is drawing, DisablePrint() should be called beforehand.
 */
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
  virtual void Init() {
  }
  void Setup();
  void Printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Vprintf(fmt, args);
    va_end(args);
  }
  void ErrPrintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    ErrVprintf(fmt, args);
    va_end(args);
  }
  void Vprintf(const char *fmt, va_list args) {
    String *str = String::Get(_str_buffer);
    Vprintf_sub(str, fmt, args);
    str->Exit(_str_buffer);
    DoString(str);
  }
  void ErrVprintf(const char *fmt, va_list args) {
    String str;
    Vprintf_sub(&str, fmt, args);
    str.Exit(_str_buffer);
    PrintErrString(&str);
  }
  virtual void PrintShell(const char *str) = 0;
  void SetColor(Color c) {
    _color = c;
  }
  void ResetColor() {
    _color = Color::kWhite;
  }
  virtual int GetRow() = 0;
  virtual int GetColumn() = 0;
  virtual void Flush() = 0;
  void DisablePrint() {
    _disable_flag = true;
  }
 protected:
  virtual void _Setup() {
  }
  virtual void Write(char c) = 0;
  virtual void WriteErr(char c) {
    Write(c);
  }
  int _cx = 0;
  int _cy = 0;
  Color _color;
  bool _disable_flag = false;
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
    void Write(const char c, StringBuffer &buf) {
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
  void Vprintf_sub(String *str, const char *fmt, va_list args);
  void PrintString(String *str);
  void PrintErrString(String *str);
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
  virtual void Write(char c) {
    assert(_offset + 1 < _buffer_size);
    _buf[_offset] = c;
    _buf[_offset + 1] = '\0';
    _offset++;
  }
  virtual int GetRow() override {
    return 1;
  }
  virtual int GetColumn() override {
    return _buffer_size;
  };
  virtual void Flush() override {
  }
  char *_buf;
  int _offset;
  const int _buffer_size;
};

#endif // __RAPH_KERNEL_TTY_H__
