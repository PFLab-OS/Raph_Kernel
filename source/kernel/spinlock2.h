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

#include <lock.h>

// TODO replace Spinlock to Spinlock2
// do not use inside interrupt handlers
// do not lock huge area
class SpinLock2 final : public LockInterface {
public:
  SpinLock2() {}
  virtual ~SpinLock2() {}
  virtual void Lock() override {
    uint64_t my_ticket = __sync_fetch_and_add(&_next_ticket, 1);
    while(my_ticket != _cur_ticket) {
      asm volatile("":::"memory");
    }
    kassert(disable_interrupt());
  }
  virtual void Unlock() override {
    _cur_ticket++;
    enable_interrupt(true);
  }
  virtual ReturnState Trylock() override {
    uint64_t my_ticket = _cur_ticket;
    if (_next_ticket != _cur_ticket) {
      return ReturnState::kError;
    }
    return __sync_bool_compare_and_swap(&_next_ticket, my_ticket, my_ticket + 1) ? ReturnState::kOk : ReturnState::kError;
  }
  virtual bool IsLocked() override {
    return _cur_ticket != _next_ticket;
  }
private:
  uint64_t _cur_ticket;
  uint64_t _next_ticket;
};
