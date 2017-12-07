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

  //make protected
  PagingCtrl *paging_ctrl;
protected:
  // For treating heap memory
  virt_addr _heap_allocated_end;
  virt_addr _brk_end;
  virt_addr _heap_limit;
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
  virt_addr KernelHeapAlloc(size_t size);
  // This function releases virtual memory but does not release physical memory.
  void KernelHeapFree(virt_addr addr);

  //TODO: rename
  void Init1();
  void Init2();

  void Switch();

  // For Initialization 
  virt_addr KernelHeapAllocZ(size_t size) {
    virt_addr addr = KernelHeapAlloc(size);
    bzero(reinterpret_cast<void *>(addr), size);
    return addr;
  }
  virt_addr Sbrk(int64_t increment);
  virt_addr GetHeapEnd() { return _brk_end; }

 private:
  SpinLock _lock;
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
private:
  SpinLock _lock;
};
