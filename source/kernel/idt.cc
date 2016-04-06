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
#include <apic.h>
#include <gdt.h>
#include <global.h>

namespace C {
extern "C" void handle_int(Regs *rs) {
  // TODO 二重割り込みの際どうするか
  //      割り込みカウント方式にすべき？
  idt->_handling_cnt[apic_ctrl->GetApicId()]++;
  if (idt->_callback[apic_ctrl->GetApicId()][rs->n] == nullptr) {
    if (gtty != nullptr) {
      gtty->PrintfRaw("s","[kernel] error: unimplemented interrupt ", "d", rs->n, "s", " ");
    }
    asm volatile("cli; hlt; nop; nop; hlt; nop; hlt;"::"a"(rs->rip));
  } else {
    idt->_callback[apic_ctrl->GetApicId()][rs->n](rs);
  }
  idt->_handling_cnt[apic_ctrl->GetApicId()]--;
}
}

extern idt_callback vectors[256];
extern idt_callback idt_vectors[256];

struct idt_entity {
  uint32_t entry[4];
} __attribute__((aligned(8))) idt_def[256];

void Idt::SetupGeneric() {
  for (int i = 0; i < 256; i++) {
    uint8_t ist;
    switch (i) {
    case 8:
      ist = 1;
      break;
    case 2:
      ist = 2;
      break;
    case 1:
    case 3:
      ist = 3;
      break;
    case 18:
      ist = 4;
      break;
    default:
      ist = 5;
      break;
    };
    SetGate(idt_vectors[i], i, 0, false, ist);
  }
  virt_addr idt_addr = reinterpret_cast<virt_addr>(idt_def);
  _idtr[0] = 8*256-1;
  _idtr[1] = idt_addr & 0xffff;
  _idtr[2] = (idt_addr >> 16) & 0xffff;
  _idtr[3] = (idt_addr >> 32) & 0xffff;
  _idtr[4] = (idt_addr >> 48) & 0xffff;
  kassert(virtmem_ctrl != nullptr);
  kassert(apic_ctrl != nullptr);
  _callback = reinterpret_cast<idt_callback **>(virtmem_ctrl->Alloc(sizeof(idt_callback *) * apic_ctrl->GetHowManyCpus()));
  _handling_cnt = reinterpret_cast<int *>(virtmem_ctrl->Alloc(sizeof(int) * apic_ctrl->GetHowManyCpus()));
  for (int i = 0; i < apic_ctrl->GetHowManyCpus(); i++) {
    _callback[i] = reinterpret_cast<idt_callback *>(virtmem_ctrl->Alloc(sizeof(idt_callback) * 256));
    _handling_cnt[i] = 0;
    for (int j = 0; j < 256; j++) {
      _callback[i][j] = nullptr;
    }
  }
}

void Idt::SetupProc() {
  asm volatile ("lidt (%0)"::"r"(_idtr));
  asm volatile ("sti;");
}

void Idt::SetGate(idt_callback gate, int n, uint8_t dpl, bool trap, uint8_t ist) {
  virt_addr vaddr = reinterpret_cast<virt_addr>(gate);
  phys_addr addr = vaddr; // TODO やばくね？
  uint32_t type = trap ? 0xF : 0xE;
  idt_def[n].entry[0] = (addr & 0xFFFF) | (KERNEL_CS << 16);
  idt_def[n].entry[1] = (addr & 0xFFFF0000) | (type << 8) | ((dpl & 0x3) << 13) | kIdtPresent | ist;
  idt_def[n].entry[2] = addr >> 32;
  idt_def[n].entry[3] = 0;
}

void Idt::SetIntCallback(int n, idt_callback callback) {
  kassert(apic_ctrl != nullptr);
  _callback[apic_ctrl->GetApicId()][n] = callback;
}
