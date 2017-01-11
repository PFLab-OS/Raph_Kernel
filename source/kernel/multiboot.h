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

extern uint32_t multiboot_info;

class MultibootCtrl {
public:
  MultibootCtrl() {
  }
  void Setup();
  void ShowModuleInfo() {
    kassert(align(multiboot_info, 8) == multiboot_info);
    virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
    addr += 8;
    gtty->CprintfRaw("\n");
    for (multiboot_tag *tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
      switch(tag->type) {
      case MULTIBOOT_TAG_TYPE_MODULE: {
        multiboot_tag_module* info = (struct multiboot_tag_module *) tag;
        gtty->CprintfRaw("module: %x %x\n", info->mod_start, info->mod_end);
        break;
      }
      default:
        break;
      }
    }
  }
  void ShowBuildTimeStamp() {
    virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
    addr += 8;
    multiboot_tag_module* info = nullptr;
    for (multiboot_tag *tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
      bool break_flag = false;
      switch(tag->type) {
      case MULTIBOOT_TAG_TYPE_MODULE: {
        info = (struct multiboot_tag_module *) tag;
        if (strcmp(info->cmdline, "time") == 0) {
          break_flag = true;
        }
        break;
      }
      default:
        break;
      }
      if (break_flag) {
        break;
      }
    }
    size_t len = info->mod_end - info->mod_start;
    char str[len + 1];
    for (size_t i = 0; i < len; i++) {
      str[i] = reinterpret_cast<uint8_t *>(info->mod_start)[i];
    }
    str[len] = '\0';
    if (info == nullptr) {
      gtty->Cprintf("[kernel] warning: No build information\n");
    } else {
      gtty->Cprintf("[kernel] info: Build Information as follows\n%s\n", str);
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
