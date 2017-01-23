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

#include <ptr.h>
#include <dev/usb/usb.h>
#include <dev/keyboard.h>

class DevUsbKeyboard : public DevUsb {
public:
  DevUsbKeyboard(DevUsbController *controller, int addr) : DevUsb(controller, addr) {
  }
  virtual ~DevUsbKeyboard() {
  }
  static DevUsb *InitUsb(DevUsbController *controller, int addr);
  virtual void InitSub() override;
private:
  class KeyboardSub : public Keyboard {
  public:
    void Push(char c) {
      _buf.Push(c);
    }
  } _dev;
  static const int kTdNum = 32; // TODO is this ok?
  static const int kMaxPacketSize = 8;
  static const char kScanCode[256];
  char _prev_buf[8];
  struct Buffer {
    uint8_t buf[kMaxPacketSize];
  };
  Buffer _buffer[kTdNum];
  DevUsbKeyboard() = delete;
  void Handle(void *, uptr<Array<uint8_t>>);
};
