/*
 *
 * Copyright (c) 2015 Raphine Project
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

#ifndef __RAPH_KERNEL_MULTIBOOT_H__
#define __RAPH_KERNEL_MULTIBOOT_H__

#include <raph.h>
#include <global.h>
#include <tty.h>
#include <boot/multiboot2.h>
#include <mem/physmem.h>
#include <mem/paging.h>
#include <elf.h>
#include <ptr.h>
#include <array.h>

class FrameBufferInfo;
extern uint32_t multiboot_info;

class MultibootCtrl {
public:
  MultibootCtrl() {
    // because multiboot_info is defined on boot/boot.S, it is placed at lower memory
    extern uint32_t multiboot_info;
    extern int kLinearAddrOffset;
    _multiboot_info = reinterpret_cast<uint32_t *>(reinterpret_cast<virt_addr>(&multiboot_info) + reinterpret_cast<virt_addr>(&kLinearAddrOffset));
    kassert(align(*_multiboot_info, 8) == *_multiboot_info);
  }
  void Setup();
  void SetupFrameBuffer(FrameBufferInfo *fb_info);
  phys_addr GetAcpiRoot();
  void ShowMemoryInfo();
  void ShowModuleInfo();
  void ShowBuildTimeStamp();
  uptr<Array<uint8_t>> LoadFile(const char *str);
  phys_addr GetPhysMemoryEnd() {
    return _phys_memory_end;
  }
private:
  uint32_t *_multiboot_info;
  PhysAddr _info_addr;
  phys_addr _phys_memory_end = 0;
};

#endif /* __RAPH_KERNEL_MULTIBOOT_H__ */
