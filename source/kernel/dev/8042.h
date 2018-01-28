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
 * Author: Yuchiki, LEDiA
 *
 */

#pragma once

#include <string.h>
#include <dev/device.h>
#include <dev/keyboard.h>

class LegacyKeyboard : Keyboard {
 public:
  LegacyKeyboard() { memset(_pushed_keys, 0, kKeyBufSize); }
  static void Attach();
  virtual ~LegacyKeyboard() {}

 private:
  static const char kScanCode[128];
  static const int kDataPort = 0x60;
  void SetupSub() override;
  void Write(uint8_t &code) {}
  static void Handler(Regs *reg, void *arg);
  char _pushed_keys[kKeyBufSize];
};
