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

#include <thread.h>
#include <spinlock.h>

// do not use inside interrupt handlers
class Mutex : public LockInterface final {
 public:
  Mutex() {}
  virtual ~Mutex() {}
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
  virtual ReturnState Trylock() = 0;
  virtual bool IsLocked() override { return ((_flag % 2) == 1); }

 private:
  volatile unsigned int _flag = 0;
};
