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

#include <idt.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <global.h>

extern "C" void dummy_int(Regs *rs) {
  if (gtty != nullptr) {
    gtty->Printf("s","\n[kernel] error: unimplemented interrupt #","d",rs->n);
  }
  asm volatile("hlt; nop; nop; hlt; nop; hlt;"::"a"(rs->n));
}

extern void (*vectors[256])(Regs *rs);
extern void (*idt_vectors[256])(Regs *rs);

struct idt_entity {
  uint32_t entry[4];
} __attribute__((aligned(8))) idt_def[256];

void Idt::Setup() {
  for (int i = 0; i < 256; i++) {
    SetGate(idt_vectors[i], i, 0, false);
  }
  static volatile uint16_t idtr[5];
  virt_addr idt_addr = reinterpret_cast<virt_addr>(idt_def);
  idtr[0] = 8*256-1;
  idtr[1] = idt_addr & 0xffff;
  idtr[2] = (idt_addr >> 16) & 0xffff;
  idtr[3] = (idt_addr >> 32) & 0xffff;
  idtr[4] = (idt_addr >> 48) & 0xffff;
  asm volatile ("lidt (%0)"::"r"(idtr));
}

void Idt::SetGate(void (*gate)(Regs *rs), int n, uint8_t dpl, bool trap) {
  virt_addr vaddr = reinterpret_cast<virt_addr>(gate);
  phys_addr addr = vaddr;
  uint32_t type = trap ? 0xF : 0xE;
  idt_def[n].entry[0] = (addr & 0xFFFF) | (KERNEL_CS << 16);
  idt_def[n].entry[1] = (addr & 0xFFFF0000) | (type << 8) | ((dpl & 0x3) << 13) | kIdtPresent;
  idt_def[n].entry[2] = addr >> 32;
  idt_def[n].entry[3] = 0;
}
