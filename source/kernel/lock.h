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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Liva
 * 
 */

#pragma once

#include <raph.h>

class LockInterface {
public:
  LockInterface() {}
  virtual ~LockInterface() {}
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual ReturnState Trylock() = 0;
  virtual bool IsLocked() = 0;
};

// wrapper of LockInterface
// This class will eliminate forgotten Unlock().
class Locker {
 public:
  Locker(LockInterface &lock) : _lock(lock) {
    _lock.Lock();
  }
  ~Locker() {
    _lock.Unlock();
  }
 private:
  LockInterface &_lock;
};

// use this with trylock macro
class TryLocker {
public:
  TryLocker(LockInterface &lock) : _lock(lock) {
    switch (lock.Trylock()) {
    case ReturnState::kOk:
      _flag = true;
      break;
    case ReturnState::kError:
      _flag = false;
      break;
    };
  }
  ~TryLocker() {
    if (_flag) {
      _lock.Unlock();
    }
  }
  bool Do() {
    return _flag;
  }
  void Unlock() {
    _lock.Unlock();
    _flag = false; 
  }
private:
  bool _flag;
  LockInterface &_lock;
};

#define trylock__(lock, l) for (TryLocker locker##l(lock); locker##l.Do(); locker##l.Unlock())
#define trylock_(lock, l) trylock__(lock, l)
#define trylock(lock) trylock_(lock, __LINE__)
