/*
 *
 * Copyright (c) 2016 Project Raphine
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

#include <global.h>
#include <apic.h>
#include <idt.h>
#include <dev/keyboard.h>

void Keyboard::Setup(int cpuid) {
  kassert(apic_ctrl != nullptr);
  kassert(idt != nullptr);
  int vector = idt->SetIntCallback(cpuid, Keyboard::Handler, nullptr);
  apic_ctrl->SetupIoInt(ApicCtrl::Ioapic::kIrqKeyboard, apic_ctrl->GetApicIdFromCpuId(cpuid), vector);
}

void Keyboard::Write(uint8_t code){
  if (_next_w == _next_r+1) _overflow = true;
  _buf[_next_w] = code;
  _next_w++;
  _count++;
  _next_w %= kbufSize;
}
uint8_t Keyboard::Read() {
  uint8_t data = _buf[_next_r];
  if (_next_r == _next_w) _underflow = true;
  _next_r++;
  _count--;
  _next_r %= kbufSize;
  return data;
}

char Keyboard::GetCh() {
  uint8_t data = Read();
  return kScanCode[data];
}
bool Keyboard::Overflow() {
  return _overflow;
}
bool Keyboard::Underflow() {
  return _underflow;
}

int Keyboard::Count() {
  return _count;
}

void Keyboard::Reset() {
  _overflow = false;
  _underflow = false;
  _count=_next_w = 0;
  _next_r = 0;
}


void Keyboard::Handler(Regs *reg, void *arg) {
  uint8_t data;
  data = inb(kDataPort);
  if(data < (1 << 7))  keyboard->Write(data);
}

const char Keyboard::kScanCode[256] = {
    '!','!','1','2','3','4','5','6',
    '7','8','9','0','-','!','!','\t',
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
