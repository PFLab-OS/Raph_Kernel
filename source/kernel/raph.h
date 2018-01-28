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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <libglobal.h>

#ifdef __cplusplus
template <class T, class U>
static inline T align(T val, U base) {
  return (val / base) * base;
}

template <class T, class U>
static inline T alignUp(T val, U base) {
  return align(val + base - 1, base);
}

// https://www.ruche-home.net/boyaki/2011-09-14/b361cc6d
template <class TBase, class TDerived>
class IsBaseOf {
 private:
  typedef char Yes;
  typedef struct {
    char v[2];
  } No;

  static Yes check(const TBase &);
  static No check(...);

  static TDerived d;

 public:
  static const bool value = (sizeof(check(d)) == sizeof(Yes));
};

#ifdef __NO_LIBC__
inline void *operator new(size_t, void *p) throw() { return p; }
inline void *operator new[](size_t, void *p) throw() { return p; }
inline void operator delete(void *, void *)throw(){};
inline void operator delete[](void *, void *) throw(){};
#endif  // __NO_LIBC__

#endif /* __cplusplus */

#undef kassert

#ifndef __TEST__

extern "C"[[noreturn]] void _kassert(const char *file, int line,
                                     const char *func);
#define kassert(flag)                       \
  if (!(flag)) {                            \
    while (gtty == nullptr) {               \
      asm volatile("cli; nop; hlt;");       \
    }                                       \
    _kassert(__FILE__, __LINE__, __func__); \
  }

#else /* __TEST__ */

#include <iostream>
struct ExceptionAssertionFailure {
  const char *file;
  int line;
  const char *func;
  void Show() {
    std::cout << "\x1b[31mAssertion failure at " << file << ":" << line << "("
              << func << ")\x1b[0m" << std::endl;
  }
};
static inline void _kassert(const char *file, int line, const char *func) {
  ExceptionAssertionFailure t;
  t.file = file;
  t.line = line;
  t.func = func;
  throw t;
}
#define kassert(flag)                       \
  if (!(flag)) {                            \
    _kassert(__FILE__, __LINE__, __func__); \
  }

#endif /* __TEST__ */

#ifdef __cplusplus
extern "C" {
#endif

#define MASK(val, ebit, sbit) \
  ((val) & (((1 << ((ebit) - (sbit) + 1)) - 1) << (sbit)))

[[noreturn]] void _kernel_panic(const char *class_name, const char *err_str);
#define kernel_panic(...)             \
  do {                                \
    while (gtty == nullptr) {         \
      asm volatile("cli; nop; hlt;"); \
    }                                 \
    _kernel_panic(__VA_ARGS__);       \
  } while (true)

void checkpoint(int id, const char *str);
void _checkpoint(const char *func, const int line);
#define CHECKPOINT _checkpoint(__func__, __LINE__)

void show_backtrace(size_t *rbp);

enum class ReturnState {
  kOk,
  kError,
};

static inline void outb(uint16_t pin, uint8_t data) {
  __asm__ volatile("outb %%al, %%dx;" ::"d"(pin), "a"(data));
}

static inline void outw(uint16_t pin, uint16_t data) {
  __asm__ volatile("outw %%ax, %%dx;" ::"d"(pin), "a"(data));
}

static inline void outl(uint16_t pin, uint32_t data) {
  __asm__ volatile("outl %%eax, %%dx;" ::"d"(pin), "a"(data));
}

static inline uint8_t inb(uint16_t pin) {
  uint8_t data;
  __asm__ volatile("inb %%dx, %%al;" : "=a"(data) : "d"(pin));
  return data;
}

static inline uint16_t inw(uint16_t pin) {
  uint16_t data;
  __asm__ volatile("inw %%dx, %%ax;" : "=a"(data) : "d"(pin));
  return data;
}

static inline uint32_t inl(uint16_t pin) {
  uint32_t data;
  __asm__ volatile("inl %%dx, %%eax;" : "=a"(data) : "d"(pin));
  return data;
}

#ifdef __KERNEL__
static inline bool disable_interrupt() {
  uint64_t if_flag;
  asm volatile("pushfq; popq %0; andq $0x200, %0;" : "=r"(if_flag));
  bool did_stop_interrupt = (if_flag != 0);
  asm volatile("cli;");
  return did_stop_interrupt;
}
static inline void enable_interrupt(bool flag) {
  if (flag) {
    asm volatile("sti");
  }
}
#else  /* __KERNEL__ */
static inline bool disable_interrupt() { return true; }
static inline void enable_interrupt(bool flag) {}
#endif /* __KERNEL__ */

#ifdef __cplusplus
}
#endif
