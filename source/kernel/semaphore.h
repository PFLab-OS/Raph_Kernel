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

#ifndef __RAPH_KERNEL_SEMAPHORE_H__
#define __RAPH_KERNEL_SEMAPHORE_H__

#include <raph.h>
#include <spinlock.h>

class Semaphore {
public:
  Semaphore(int max) : _max(max) {
    _cur = _max;
  }
  void Acquire() {
    while(true) {
      Locker locker(_lock);
      if (_cur > 0) {
        _cur--;
        return;
      }
    }
  }
  int Tryacquire() {
    Locker locker(_lock);
    if (_cur == 0) {
      return -1;
    } else {
      _cur--;
      return 0;
    }
  }
  void Release() {
    Locker locker(_lock);
    kassert(_cur < _max);
    _cur++;
  }
  virtual ~Semaphore() {
  }
private:
  Semaphore();
  SpinLock _lock;
  const int _max;
  int _cur;
};

#endif /* __RAPH_KERNEL_SEMAPHORE_H__ */
