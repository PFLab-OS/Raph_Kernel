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

extern uint32_t multiboot_info;

class MultibootCtrl {
public:
  MultibootCtrl() {
  }
  void Setup() {
    GetMemoryInfo();
  }
  void GetMemoryInfo() {
    kassert(align(multiboot_info, 8) == multiboot_info);
    virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
    addr += 8;
    multiboot_tag *tag;
    for (tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
      switch(tag->type) {
      case MULTIBOOT_TAG_TYPE_MMAP: {
        gtty->Cprintf("mmap from grub\n");
        multiboot_memory_map_t *mmap;
        for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
             (multiboot_uint8_t *) mmap 
               < (multiboot_uint8_t *) tag + tag->size;
             mmap = (multiboot_memory_map_t *) 
               ((unsigned long) mmap
                + ((struct multiboot_tag_mmap *) tag)->entry_size)) {
          gtty->Cprintf (" base_addr = 0x%llx,"
                         " length = 0x%llx, type = 0x%x\n",
                         mmap->addr,
                         mmap->len,
                         (unsigned) mmap->type);
          phys_addr memory_end = mmap->addr + mmap->len;
          if (_phys_memory_end < memory_end) {
            _phys_memory_end = memory_end;
          }
        }
        break;
      }
      default:
        break;
      }
    }
  }
  void ShowModuleInfo() {
    kassert(align(multiboot_info, 8) == multiboot_info);
    virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
    addr += 8;
    multiboot_tag *tag;
    for (tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
      switch(tag->type) {
      case MULTIBOOT_TAG_TYPE_MODULE: {
        multiboot_tag_module* info = (struct multiboot_tag_module *) tag;
        gtty->CprintfRaw("module %x %x\n", info->mod_start, info->mod_end);
	readElfTest(info);
        break;
      }
      default:
        break;
      }
    }
  }
  phys_addr GetPhysMemoryEnd() {
    return _phys_memory_end;
  }
private:
  PhysAddr _info_addr;
  phys_addr _phys_memory_end = 0;
};

#endif /* __RAPH_KERNEL_MULTIBOOT_H__ */
