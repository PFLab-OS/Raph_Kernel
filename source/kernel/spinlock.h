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
#include "raph.h"

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
  SpinLock() : _flag(0) {}
  volatile unsigned int GetFlag() {
    return _flag;
  }
  // WriteLockを通じてのみアクセスする事!!!!!
  bool SetFlag(unsigned int old_flag, unsigned int new_flag) {
    return __sync_bool_compare_and_swap(&_flag, old_flag, new_flag);
  }
  virtual SpinLockId getLockId() {
    return SpinLockId::kNull;
  }
  void Lock();
  void Unlock();
  int Trylock();
  bool IsLocked() {
    return ((_flag % 2) == 1);
  }
private:
  volatile unsigned int _flag;
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
  UTEST_VIRTUAL SpinLockId getCurrentLock() {
    return flags[0];
  }
  UTEST_VIRTUAL void setCurrentLock(SpinLockId spid) {
    flags[0] = spid;
  }
#ifndef __UNIT_TEST__
 private:
#endif
  SpinLockId flags[16];
};

#ifdef __UNIT_TEST__

#include <future>
#include <unistd.h>
#include <map>
#include <iostream>
#include "raph.h"

class SpinLockCtrlTest : public SpinLockCtrl {
public:
  SpinLockCtrlTest() {}
  UTEST_VIRTUAL SpinLockId getCurrentLock() {
    return _map.Get(std::this_thread::get_id(), static_cast<SpinLockId>(0xffff));
  }
  UTEST_VIRTUAL void setCurrentLock(SpinLockId spid) {
    _map.Set(std::this_thread::get_id(), spid);
  }
private:
  StdMap<std::thread::id, SpinLockId> _map;
};

#undef assert
#define assert(flag) if (!(flag)) {throw #flag;}

#endif // __UNIT_TEST__

/* class ReadLock { */
/* public: */
/*   // 最適化されるとバグりやすいので、インライン化しない方が良い */
/*   ReadLock(SpinLock &lock); */
/*   bool Check(); */
/*   ~ReadLock(); */
/* private: */
/*   void Wait() { */
/*     _flag = _lock.GetFlag(); */
/*     while((_flag % 2) == 1) { */
/*       _flag = _lock.GetFlag(); */
/*     } */
/*   } */
/*   SpinLock &_lock; */
/*   SpinLockId _tmp_spid; */
/*   volatile unsigned int _flag; */
/*   int _tried; */
/* }; */

/* class WriteLock { */
/* public: */
/*   // 最適化されるとバグりやすい、インライン化しない方が良い */
/*   WriteLock(SpinLock &lock); */
/*   bool Check() { */
/*     bool failed = !_tried; */
/*     _tried = true; */
/*     return failed; */
/*   } */
/*   ~WriteLock(); */
/* private: */
/*   SpinLock &_lock; */
/*   SpinLockId _tmp_spid; */
/*   volatile unsigned int _flag; */
/*   bool _tried; */
/* }; */

/* class ReadWriteLock { */
/* public: */
/*   enum class LockStatus { */
/*     kInit, */
/*     kReading, */
/*     kReadEnd, */
/*     kWriting, */
/*   }; */
/*   LockStatus _status; */
/*   ReadWriteLock (SpinLock &lock); */
/*   bool Check(); */
/*   ~ReadWriteLock(); */
/* private: */
/*   LockStatus _lstatus; */
/*   SpinLock &_lock; */
/*   SpinLockId _tmp_spid; */
/*   volatile unsigned int _flag; */
/* }; */

/* #define __READ_LOCK(lock, l) for(ReadLock __autogened_tmp_lock##l(lock); __autogened_tmp_lock##l.Check();) */
/* #define _READ_LOCK(lock, l) __READ_LOCK(lock, l) */
/* #define READ_LOCK(lock) _READ_LOCK(lock, __LINE__) */

/* #define __WRITE_LOCK(lock, l) for(WriteLock __autogened_tmp_lock##l(lock); __autogened_tmp_lock##l.Check();) */
/* #define _WRITE_LOCK(lock, l) __WRITE_LOCK(lock, l) */
/* #define WRITE_LOCK(lock) _WRITE_LOCK(lock, __LINE__) */

/* #define __RW_LOCK(lock, l) for(ReadWriteLock __autogened_tmp_lock##l(lock); __autogened_tmp_lock##l.Check();) switch(__autogened_tmp_lock##l._status) */
/* #define _RW_LOCK(lock, l) __RW_LOCK(lock, l) */
/* #define RW_LOCK(lock) _RW_LOCK(lock, __LINE__) */

#endif // __RAPH_KERNEL_SPINLOCK_H__
