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

#include <mem/paging.h>
#include "multiboot.h"

void MultibootCtrl::Setup() {
  kassert(align(multiboot_info, 8) == multiboot_info);
  virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
  addr += 8;
  multiboot_tag *tag;

  // load memory info
  for (tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
    switch(tag->type) {
    case MULTIBOOT_TAG_TYPE_MMAP: {
      gtty->Cprintf("mmap from grub\n");
      multiboot_memory_map_t *mmap;
      int available_entry_num = 0;
      for (mmap = ((struct multiboot_tag_mmap *) tag)->entries;
           (multiboot_uint8_t *) mmap 
             < (multiboot_uint8_t *) tag + tag->size;
           mmap = (multiboot_memory_map_t *) 
             ((unsigned long) mmap
              + ((struct multiboot_tag_mmap *) tag)->entry_size)) {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
          available_entry_num++;
        }
      }
      multiboot_memory_map_t entries[available_entry_num];
      int cnt = 0;
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
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
          entries[cnt] = *mmap;
          cnt++;
        }
      }
      // sort
      for (int i = 0; i < available_entry_num - 1; i++) {
        for (int j = 0; j < available_entry_num - i - 1; j++) {
          if (entries[j + 1].addr < entries[j].addr) {
            multiboot_memory_map_t tmp = entries[j + 1];
            entries[j + 1] = entries[j];
            entries[j] = tmp;
          }
        }
      }
      _phys_memory_end = entries[available_entry_num - 1].addr + entries[available_entry_num - 1].len;
      phys_addr cur = 0;
      for (int i = 0; i < available_entry_num; i++) {
        phys_addr start = PagingCtrl::RoundUpAddrOnPageBoundary(entries[i].addr);
        phys_addr end = PagingCtrl::RoundAddrOnPageBoundary(entries[i].addr + entries[i].len);
        if (cur != start) {
          physmem_ctrl->Reserve(cur, start - cur);
        }
        cur = end;
      }
      break;
    }
    default:
      break;
    }
  }
}

void MultibootCtrl::ShowModuleInfo() {
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

void MultibootCtrl::ShowBuildTimeStamp() {
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
    gtty->Cprintf("No build information\n");
  } else {
    gtty->Cprintf("%s\n", str);
  }
}

uptr<Array<uint8_t>> MultibootCtrl::LoadFile(const char *str) {
  kassert(align(multiboot_info, 8) == multiboot_info);
  virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
  addr += 8;
  for (multiboot_tag *tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
    switch(tag->type) {
    case MULTIBOOT_TAG_TYPE_MODULE: {
      multiboot_tag_module* info = (struct multiboot_tag_module *) tag;
      if (strcmp(info->cmdline, str) == 0) {
        size_t len = info->mod_end - info->mod_start;
        auto array = make_uptr(new Array<uint8_t>(len));
        memcpy(array->GetRawPtr(), reinterpret_cast<uint8_t *>(info->mod_start), len);
        return array;
      }
      break;
    }
    default:
      break;
    }
  }
  return make_uptr<Array<uint8_t>>();
}
