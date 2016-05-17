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

#include "raph.h"
#include "global.h"
#include "boot/multiboot2.h"
#include "mem/physmem.h"
#include "mem/paging.h"

extern uint32_t multiboot_info;

class MultibootCtrl {
public:
  MultibootCtrl() {
  }
  void Setup() {
    SetupAcpi();
  }
private:
  void SetupAcpi() {
    kassert(align(multiboot_info, 8) == multiboot_info);
    virt_addr addr = p2v(static_cast<phys_addr>(multiboot_info));
    addr += 8;
    multiboot_tag *tag;
    for (tag = reinterpret_cast<multiboot_tag *>(addr); tag->type != MULTIBOOT_TAG_TYPE_END; addr = alignUp(addr + tag->size, 8), tag = reinterpret_cast<multiboot_tag *>(addr)) {
      switch(tag->type) {
        // no need anymore
        // case MULTIBOOT_TAG_TYPE_ACPI_OLD:
        //   {
        //     RSDPDescriptor *table = reinterpret_cast<RSDPDescriptor *>(tag + 1);
        //     acpi_ctrl->Setup(table);
        //   }
        //   break;
      default:
        break;
      }
    }
  }
  PhysAddr _info_addr;
};

#endif /* __RAPH_KERNEL_MULTIBOOT_H__ */
