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
#include <spinlock.h>
#include <queue.h>

class Tty {
 public:
  Tty() {
  }
  void Init() {
    Function func;
    func.Init(Handle, reinterpret_cast<void *>(this));
    _queue.SetFunction(1, func);
  }
  void Printf() {
  }
  template<class... T>
    void Printf(const T& ...args) {
    String *str = String::New();
    Printf_sub1(*str, args...);
    str->Exit();
    _queue.Push(str);
  }
  // use to print error message
  template<class... T>
    void PrintfRaw(const T& ...args) {
    String str;
    str.Init();
    str.type = String::Type::kSingle;
    Printf_sub1(str, args...);
    str.Exit();
    Locker locker(_lock);
    int tx = _cx;
    int ty = _cy;
    _cx = _rcx;
    _cy = _rcy;
    PrintString(&str);
    _rcx = _cx;
    _rcy = _cy;
    _cx = tx;
    _cy = ty;    
  }
 protected:
  virtual void Write(uint8_t c) = 0;
  int _cx = 0;
  int _cy = 0;
  int _rcx = 0;
  int _rcy = 0;
 private:
  struct String {
    enum class Type {
      kSingle,
      kQueue,
    } type;
    static const int length = 100;
    uint8_t str[length];
    int offset;
    String *next;
    static String *New();
    void Init() {
      type = Type::kQueue;
      offset = 0;
      next = nullptr;
    }
    void Write(uint8_t c);
    void Exit() {
      Write('\0');
    }
  };
  static void Handle(void *tty){
    Tty *that = reinterpret_cast<Tty *>(tty);
    void *str;
    while(that->_queue.Pop(str)) {
      Locker locker(that->_lock);
      that->PrintString(reinterpret_cast<String *>(str));
    }
  }
  void Printf_sub1(String &str) {
  }
  template<class T>
    void Printf_sub1(String &str, T /* arg */) {
    Printf_sub2(str, "s", "(invalid format)");
  }
  template<class T1, class T2, class... T>
    void Printf_sub1(String &str, const T1 &arg1, const T2 &arg2, const T& ...args) {
    Printf_sub2(str, arg1, arg2);
    Printf_sub1(str, args...);
  }
  
  void Printf_sub2(String &str, const char *arg1, const char arg2) {
    if (strcmp(arg1, "c")) {
      Printf_sub2(str, "s", "(invalid format)");
    } else {
      str.Write(arg2);
    }
  }
  void Printf_sub2(String &str, const char* arg1, const char *arg2) {
    if (strcmp(arg1, "s")) {
      Printf_sub2(str, "s", "(invalid format)");
    } else {
      while(*arg2) {
        str.Write(*arg2);
        arg2++;
      }
    }
  }
  void Printf_sub2(String &str, const char *arg1, const int8_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const int16_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const int32_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const int64_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const uint8_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const uint16_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const uint32_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  void Printf_sub2(String &str, const char *arg1, const uint64_t arg2) {
    PrintInt(str, arg1, arg2);
  }
  template<class T1>
    void Printf_sub2(String &str, const char *arg1, const T1& /*arg2*/) {
    Printf_sub2(str, "s", "(unknown)");
  }
  template<class T1, class T2>
    void Printf_sub2(String &str, const T1& /*arg1*/, const T2& /*arg2*/) {
    Printf_sub2(str, "s", "(invalid format)");
  }
  void PrintInt(String &str, const char *arg1, const int arg2);
  void PrintString(String *str);
  FunctionalQueue _queue;
  SpinLock _lock;
};

#endif // __RAPH_KERNEL_TTY_H__
