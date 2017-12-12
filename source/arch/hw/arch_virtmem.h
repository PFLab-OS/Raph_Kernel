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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#ifndef __RAPH_LIB_MEM_VIRTMEM_H__
#error "invalid inclusion"
#endif /* __RAPH_LIB_MEM_VIRTMEM_H__ */

class PagingCtrl;

class VirtmemCtrlInterface {
public:
  virtual virt_addr Sbrk(int64_t increment) = 0;
  virtual void Switch() = 0;
};

// In this class, paging_ctrl's cr3 Register variable is no meaning.
// Because All processes share the same kernel memory space.
class KernelVirtmemCtrl final : public VirtmemCtrlInterface {
 public:
  KernelVirtmemCtrl();
  ~KernelVirtmemCtrl() {
  }

  // 新規に仮想メモリ領域を確保する。
  // 物理メモリが割り当てられていない領域の場合は物理メモリを割り当てる
  virt_addr Alloc(size_t size);
  // This function releases virtual memory but does not release physical memory.
  void Free(virt_addr addr);

  //TODO: rename
  void Init();
  void ReleaseLowMemory();

  void Switch();

  // For Initialization 
  virt_addr AllocZ(size_t size) {
    virt_addr addr = Alloc(size);
    bzero(reinterpret_cast<void *>(addr), size);
    return addr;
  }
  virt_addr Sbrk(int64_t increment);
  virt_addr GetHeapEnd() { return _brk_end; }

  //make protected
  PagingCtrl *paging_ctrl;
 private:
  //TODO: Check if this lock need on many core machine.
  SpinLock _lock;
  // For treating heap memory
  virt_addr _heap_allocated_end;
  virt_addr _brk_end;
  virt_addr _heap_limit;
};

class VirtmemCtrl : public VirtmemCtrlInterface {
public:
  VirtmemCtrl();
  VirtmemCtrl(const VirtmemCtrl* vmc);
  virtual ~VirtmemCtrl() {
  }

  void Init() {
  }

  void Switch();
  
  //TODO:To be implemetation
  virt_addr Sbrk(int64_t increment) {
    return 0;
  }
  //make protected
  PagingCtrl *paging_ctrl;
};
