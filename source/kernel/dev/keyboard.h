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
 * Author: Yuchiki, LEDiA, hikalium
 * 
 */

#ifndef __RAPH_KERNEL_DEV_KEYBOARD_H__
#define __RAPH_KERNEL_DEV_KEYBOARD_H__

#include <global.h>
#include <apic.h>
#include <buf.h>

class Keyboard {
 public:
  void Setup(const GenericFunction &func);
  bool Read(uint8_t &data) {
    return _buf.Pop(data);
  }
  static char Interpret(const uint8_t &code) {
    return kScanCode[code];
  }

 private:
  void Write(uint8_t &code) {
    _buf.Push(code);
  }
  static void Handler (Regs *reg, void *arg);
  static const int kBufSize = 100;
  static const char kScanCode[256];
  static const int kDataPort = 0x60;
  IntFunctionalRingBuffer<uint8_t, kBufSize> _buf;
};

#endif // __RAPH_KERNEL_DEV_KEYBOARD_H__
