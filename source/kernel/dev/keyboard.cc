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
 * Author: Yuchiki
 * 
 */

#include <stdint.h>
#include <cpu.h>

#include <global.h>
#include <apic.h>
#include <idt.h>
#include <buf.h>
#include <dev/keyboard.h>

void Keyboard::Setup(const GenericFunction &func) {
  kassert(apic_ctrl != nullptr);
  kassert(idt != nullptr);
  CpuId cpuid = cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority);
  int vector = idt->SetIntCallback(cpuid, Keyboard::Handler, nullptr);
  apic_ctrl->SetupIoInt(ApicCtrl::kIrqKeyboard, cpuid.GetApicId(), vector);
  _buf.SetFunction(cpuid, func);
}

void Keyboard::Handler(Regs *reg, void *arg) {
  uint8_t data = inb(kDataPort);
  if (data < (1 << 7)) keyboard->Write(data);
}

const char Keyboard::kScanCode[256] = {
    '!','!','1','2','3','4','5','6',
    '7','8','9','0','-','=','\b','\t',
    'q','w','e','r','t','y','u','i',
    'o','p','[',']','\n','!','a','s',
    'd','f','g','h','j','k','l',';',
    '"','^','!','\\','z','x','c','v',
    'b','n','m',',','.','/','!','!',
    '!',' ','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
    '!','!','!','!','!','!','!','!',
};
