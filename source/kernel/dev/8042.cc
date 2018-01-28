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
 * Author: Yuchiki
 *
 */

#include <cpu.h>
#include <global.h>
#include <apic.h>
#include <idt.h>
#include <buf.h>
#include "8042.h"

void LegacyKeyboard::Attach() {
  LegacyKeyboard *keyboard = new LegacyKeyboard;
  keyboard->Setup();
}

void LegacyKeyboard::SetupSub() {
  CpuId cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
  int vector =
      idt->SetIntCallback(cpuid, LegacyKeyboard::Handler,
                          reinterpret_cast<void *>(this), Idt::EoiType::kLapic);
  apic_ctrl->SetupIoInt(ApicCtrl::kIrqKeyboard, cpuid.GetApicId(), vector, true,
                        true);
}

void LegacyKeyboard::Handler(Regs *reg, void *arg) {
  auto that = reinterpret_cast<LegacyKeyboard *>(arg);
  uint8_t data = inb(kDataPort);
  if (data < 0x80) {
    data = kScanCode[data];
    int i;
    bool pushed = false;
    switch (data) {
      case static_cast<const char>(SpecialKey::kShift):
      case static_cast<const char>(SpecialKey::kCtrl):
      case static_cast<const char>(SpecialKey::kAlt): {
        for (i = 0; i < kKeyBufSize; i++) {
          if (that->_pushed_keys[i] == data) {
            break;
          }
        }
        if (i == kKeyBufSize) {
          for (i = 0; i < kKeyBufSize; i++) {
            if (that->_pushed_keys[i] == 0) {
              break;
            }
          }
          if (i != kKeyBufSize) {
            that->_pushed_keys[i] = data;
            pushed = true;
          }
        }
      }
    }
    KeyInfo ki;
    memcpy(ki.c, that->_pushed_keys, kKeyBufSize);
    if (!pushed) {
      for (i = 0; i < kKeyBufSize; i++) {
        if (ki.c[i] == 0) {
          ki.c[i] = data;
          break;
        }
      }
    }
    if (!that->_buf.Push(ki)) {
      // TODO show warning
    }
  } else {
    data -= 0x80;
    data = kScanCode[data];
    for (int i = 0; i < kKeyBufSize; i++) {
      if (that->_pushed_keys[i] == data) {
        that->_pushed_keys[i] = 0;
      }
    }
  }
}

const char LegacyKeyboard::kScanCode[128] = {
    0x0,
    0x0,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    static_cast<const char>(SpecialKey::kBackSpace),
    static_cast<const char>(SpecialKey::kTab),
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    static_cast<const char>(SpecialKey::kEnter),
    0x0,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    static_cast<const char>(SpecialKey::kShift),
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    static_cast<const char>(SpecialKey::kShift),
    0x0,
    0x0,
    ' ',
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
    0x0,
};
