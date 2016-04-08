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
 * Author: Liva
 * 
 */

// 一時メモリ管理

#ifndef __RAPH_KERNEL_MEM_TMPMEM_H__
#define __RAPH_KERNEL_MEM_TMPMEM_H__

#include <string.h>
#include <mem/virtmem.h>

class TmpmemCtrl {
 public:
  TmpmemCtrl();
  virt_addr Alloc(size_t size);
  virt_addr AllocZ(size_t size) {
    virt_addr addr = Alloc(size);
    bzero(reinterpret_cast<void *>(addr), size);
    return addr;
  }
  // 仮想メモリ領域を開放するが、物理メモリ領域は解放しない
  void Free(virt_addr addr);
 private:
};

#endif // __RAPH_KERNEL_MEM_TMPMEM_H__

