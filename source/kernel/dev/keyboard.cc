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

#include "keyboard.h"
#include <ptr.h>
#include <function.h>
#include <shell.h>
#include <global.h>

void Keyboard::Setup() {
  _buf.SetFunction(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority),
                   make_uptr(new ClassFunction1<void, Keyboard, void *>(
                       this, &Keyboard::Handle, nullptr)));
  SetupSub();
}

void Keyboard::Handle(void *) {
  KeyInfo ki;
  if (!_buf.Pop(ki)) {
    return;
  }
  for (int i = 0; i < kKeyBufSize; i++) {
    if (ki.c[i] == static_cast<char>(SpecialKey::kShift)) {
      for (int j = 0; j < kKeyBufSize; j++) {
        if ('!' <= ki.c[j] && ki.c[j] <= '~') {
          ki.c[j] = kUpperChar[static_cast<int>(ki.c[j])];
        }
      }
      break;
    }
  }
  for (int i = 0; i < kKeyBufSize; i++) {
    switch (ki.c[i]) {
      case static_cast<char>(SpecialKey::kNull):
      case static_cast<char>(SpecialKey::kShift):
      case static_cast<char>(SpecialKey::kAlt):
      case static_cast<char>(SpecialKey::kCtrl):
        break;
      default:
        shell->PushCh(ki.c[i]);
    }
  }
}

const char Keyboard::kUpperChar[128] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, ' ', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, '\"', 0x0, 0x0, 0x0, 0x0, '<',
    '_', '>', '?', '(', '!', '@', '#', '$', '%', '^',  '&', '*', ')', 0x0, ':',
    0x0, '+', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,  0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, '{', '|', '}', 0x0, 0x0, '~', 'A', 'B', 'C',  'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',  'S', 'T', 'U', 'V', 'W',
    'X', 'Y', 'Z', 0x0, 0x0, 0x0, 0x0, 0x0,
};
