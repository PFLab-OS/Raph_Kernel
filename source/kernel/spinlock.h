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

#ifndef __RAPH_KERNEL_SPINLOCK_H__
#define __RAPH_KERNEL_SPINLOCK_H__

#include <stdint.h>
#include <_cpu.h>

class SpinLockInterface {
public:
  SpinLockInterface() {}
  virtual ~SpinLockInterface() {}
  virtual volatile unsigned int GetFlag() = 0;
  virtual CpuId GetProcId() = 0;
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual int Trylock() = 0;
  virtual bool IsLocked() = 0;
};


#ifdef __KERNEL__
// 割り込みハンドラ内でも使えるSpinLock
class IntSpinLock : public SpinLockInterface {
public:
  IntSpinLock() {}
  virtual ~IntSpinLock() {}
  virtual volatile unsigned int GetFlag() override {
    return _flag;
  }
  virtual CpuId GetProcId() override {
    return _cpuid;
  }
  virtual void Lock() override;
  virtual void Unlock() override;
  virtual int Trylock() override;
  virtual bool IsLocked() override {
    return ((_flag % 2) == 1);
  }
protected:
  bool SetFlag(unsigned int old_flag, unsigned int new_flag) {
    return __sync_bool_compare_and_swap(&_flag, old_flag, new_flag);
  }
  volatile unsigned int _flag = 0;
  CpuId _cpuid;
  bool _did_stop_interrupt = false;
};

using SpinLock = IntSpinLock;

// コンストラクタ、デストラクタでlock,unlockができるラッパー
// 関数からreturnする際に必ずunlockできるので、unlock忘れを防止する
class Locker {
 public:
  Locker(SpinLockInterface &lock) : _lock(lock) {
    _lock.Lock();
  }
  ~Locker() {
    _lock.Unlock();
  }
 private:
  SpinLockInterface &_lock;
};

#endif // __KERNEL__

#endif // __RAPH_KERNEL_SPINLOCK_H__
