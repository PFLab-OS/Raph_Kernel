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

// 仮想メモリ関連のコード

#ifndef __RAPH_KERNEL_MEM_KVIRTMEM_H__
#define __RAPH_KERNEL_MEM_KVIRTMEM_H__

#include <assert.h>
#include <spinlock.h>
#include <raph.h>
#include <mem/virtmem.h>

class KVirtmemCtrl : public VirtmemCtrl {
public:
  KVirtmemCtrl();
  virtual ~KVirtmemCtrl() {
  }
  // 新規に仮想メモリ領域を確保する。
  // 物理メモリが割り当てられていない領域の場合は物理メモリを割り当てる
  virtual virt_addr Alloc(size_t size) override;
  // 仮想メモリ領域を解放するが、物理メモリは解放しない
  virtual void Free(virt_addr addr) override;
  virtual virt_addr Sbrk(int64_t increment) override;
private:
  virt_addr _heap_allocated_end;
  virt_addr _brk_end;
  virt_addr _heap_limit;
  SpinLock _lock;
};

#endif // __RAPH_KERNEL_MEM_KVIRTMEM_H__
