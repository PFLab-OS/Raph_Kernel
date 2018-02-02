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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#ifndef __RAPH_KERNEL_SPINLOCK_H__
#define __RAPH_KERNEL_SPINLOCK_H__
#ifdef __KERNEL__

#include <stdint.h>
#include <stdlib.h>
#include <_cpu.h>
#include <raph.h>
#include <lock.h>

class SpinLock final : public LockInterface {
 public:
  SpinLock() {}
  virtual ~SpinLock() {}
  virtual void Lock() override;
  virtual void Unlock() override;
  virtual ReturnState Trylock() override;
  virtual bool IsLocked() override { return ((_flag % 2) == 1); }
  static bool _spinlock_timeout;

 protected:
  bool SetFlag(unsigned int old_flag, unsigned int new_flag) {
    return __sync_bool_compare_and_swap(&_flag, old_flag, new_flag);
  }
  volatile unsigned int _flag = 0;
  CpuId _cpuid;
  size_t _rip[3];
  bool _did_stop_interrupt = false;
};
#else
#include <raph.h>
#include <lock.h>

class SpinLock final : public LockInterface {
 public:
  SpinLock() {}
  virtual ~SpinLock() {}
  virtual void Lock() {
    while (!SetFlag(0, 1)) {
    }
  }
  virtual void Unlock() { _flag = 0; }
  virtual ReturnState Trylock() override {
    kassert(false);  // TODO impl
    return ReturnState::kOk;
  }
  virtual bool IsLocked() override { return _flag == 1; }

 protected:
  bool SetFlag(unsigned int old_flag, unsigned int new_flag) {
    return __sync_bool_compare_and_swap(&_flag, old_flag, new_flag);
  }
  volatile unsigned int _flag = 0;
};
#endif  // __KERNEL__

#endif  // __RAPH_KERNEL_SPINLOCK_H__
