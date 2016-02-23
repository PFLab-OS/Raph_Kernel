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

#ifndef __RAPH_KERNEL_RAPH_H__
#define __RAPH_KERNEL_RAPH_H__

#include "global.h"
#include "tty.h"

void kernel_panic(char *class_name, char *err_str);

template<class T>
static inline T align(T val, int base) {
  return (val / base) * base;
}

template<class T>
static inline T alignUp(T val, int base) {
  return align(val + base - 1, base);
}

static inline void outb(uint16_t pin, uint8_t data) {
  asm volatile("outb %%al, %%dx;"::"d"(pin),"a"(data));
}

static inline void outw(uint16_t pin, uint16_t data) {
  asm volatile("outw %%ax, %%dx;"::"d"(pin),"a"(data));
}

static inline void outl(uint16_t pin, uint32_t data) {
  asm volatile("outl %%eax, %%dx;"::"d"(pin),"a"(data));
}

static inline uint8_t inb(uint16_t pin) {
  uint8_t data;
  asm volatile("inb %%dx, %%al;":"=a"(data):"d"(pin));
  return data;
}

static inline uint16_t inw(uint16_t pin) {
  uint16_t data;
  asm volatile("inw %%dx, %%ax;":"=a"(data):"d"(pin));
  return data;
}

static inline uint32_t inl(uint16_t pin) {
  uint32_t data;
  asm volatile("inl %%dx, %%eax;":"=a"(data):"d"(pin));
  return data;
}

#ifdef __UNIT_TEST__
#define UTEST_VIRTUAL virtual
#define kassert(flag) if (!(flag)) {throw #flag;}

#include <map>
template <class K, class V>
class StdMap {
public:
  StdMap() : _flag(0) {}
  void Set(K key, V value) {
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {}
    _map[key] = value;
    __sync_bool_compare_and_swap(&_flag, 1, 0);
  }
  // keyが存在しない場合は、initを返す
  V Get (K key, V init) {
    V value = init;
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {}
    if (_map.find(key) == _map.end()) {
      _map.insert(make_pair(key, init));
      value = init;
    } else {
      value = _map.at(key);
    }
    __sync_bool_compare_and_swap(&_flag, 1, 0);
    return value;
  }
  bool Exist(K key) {
    bool flag = false;
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {}
    if (_map.find(key) != _map.end()) {
      flag = true;
    }
    __sync_bool_compare_and_swap(&_flag, 1, 0);
    return flag;
  }
  // mapへの直接アクセス
  // 並列実行しないように
  std::map<K, V> &Map() {
    return _map;
  }
private:
  std::map<K, V> _map;
  volatile int _flag;
};
#else
#define UTEST_VIRTUAL
#undef kassert
#define kassert(flag) if (!(flag)) {if (gtty != nullptr) {gtty->Printf("s", "assertion failed at ", "s", __FILE__, "s", " l.", "d", __LINE__, "s", " (", "s", __func__, "s", ") Kernel stopped!");} while(true){asm volatile("hlt");}}

inline void *operator new(size_t, void *p)     throw() { return p; }
inline void *operator new[](size_t, void *p)   throw() { return p; }
inline void  operator delete  (void *, void *) throw() { };
inline void  operator delete[](void *, void *) throw() { };
#endif // __UNIT_TEST__

#endif // __RAPH_KERNEL_RAPH_H__