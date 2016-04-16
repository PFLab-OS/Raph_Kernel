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

#ifndef __RAPH_KERNEL_SPINLOCK_H__
#define __RAPH_KERNEL_SPINLOCK_H__

#include <stdint.h>
#include <assert.h>

enum class SpinLockId : int {
  kNull = -1,
  kEnd,
};

//
// 同じロックに対してreadlockしてからwritelockするとデッドロックする
// そういう場合は最初からwritelockするか、readfreeしてからwritelockすべし
//
// Read引数のcallbackは何度も実行される可能性がある事に注意
//
// SpinLockを定義する場合は、継承して、getLockId()を書き換える事

class SpinLock {
public:
  SpinLock() {}
  virtual ~SpinLock() {}
  volatile unsigned int GetFlag() {
    return _flag;
  }
  volatile int GetProcId() {
    return _id;
  }
  // WriteLockを通じてのみアクセスする事!!!!!
  bool SetFlag(unsigned int old_flag, unsigned int new_flag) {
    return __sync_bool_compare_and_swap(&_flag, old_flag, new_flag);
  }
  virtual SpinLockId getLockId() {
    return SpinLockId::kNull;
  }
  virtual void Lock();
  void Unlock();
  int Trylock();
  bool IsLocked() {
    return ((_flag % 2) == 1);
  }
protected:
  volatile unsigned int _flag = 0;
  volatile int _id;
};

class DebugSpinLock : public SpinLock {
public:
  DebugSpinLock() {
    _key = kKey;
  }
  virtual ~DebugSpinLock() {}
  virtual void Lock() override;
private:
  int _key;
  static const int kKey = 0x13572468;
};

// コンストラクタ、デストラクタでlock,unlockができるラッパー
// 関数からreturnする際に必ずunlockできるので、unlock忘れを防止する
class Locker {
 public:
 Locker(SpinLock &lock) : _lock(lock) {
    _lock.Lock();
  }
  ~Locker() {
    _lock.Unlock();
  }
 private:
  SpinLock &_lock;
};

class SpinLockCtrl {
 public:
  SpinLockCtrl() {
    for(int i = 0; i < 16; i++) {
      flags[i] = SpinLockId::kEnd;
    }
  }
  SpinLockId getCurrentLock() {
    return flags[0];
  }
  void setCurrentLock(SpinLockId spid) {
    flags[0] = spid;
  }
 private:
  SpinLockId flags[16];
};

#endif // __RAPH_KERNEL_SPINLOCK_H__
