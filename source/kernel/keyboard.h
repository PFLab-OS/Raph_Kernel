/*
 *
 * Copyright (c) 2015 Project Raphine
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



#ifndef __RAPH_KERNEL_KEYBOARD_H__
#define __RAPH_KERNEL_KEYBOARD_H__

#include <global.h>
#include <apic.h>

// this file should be put in dev/...?
class Keyboard {
 public:
  void Setup();
  void Write(uint8_t code);
  uint8_t Read();
  char Getch();
  bool Overflow();
  bool Underflow();
  int Count();
  void Reset();

  static void intKeyboard (Regs *reg);
 private:
  static const int kbufSize=100;
  int _count=0; 
  uint8_t _buf[kbufSize];
  bool _overflow=false;
  bool _underflow=false;
  int _next_w=0;
  int _next_r=0;
  
  static const char kScanCode[256];
};

#endif // __RAPH_KERNEL_KEYBOARD_H__
