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

#include <buf.h>
#include <dev/device.h>
#include <thread.h>

class Keyboard : public Device {
 public:
  Keyboard() {}
  void Setup();
  static const int kKeyBufSize = 8;
  struct KeyInfo {
    static const int kBufSize = 8;
    char c[kBufSize];
  };
  enum class SpecialKey : char {
    kNull = 0,
    kShift = 1,
    kCtrl = 2,
    kAlt = 3,
    kBackSpace = 8,
    kTab = 9,
    kEnter = 12,
    kDelete = 127,
  };

 protected:
  FunctionalRingBuffer<KeyInfo, kKeyBufSize> _buf;
  static const char kUpperChar[128];
  virtual void SetupSub() {}
  void Handle(void *);
};
