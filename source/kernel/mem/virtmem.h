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

#ifndef __RAPH_KERNEL_MEM_VIRTMEM_H__
#define __RAPH_KERNEL_MEM_VIRTMEM_H__

#ifndef ASM_FILE

#include <spinlock.h>
#include <raph.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

typedef uint64_t virt_addr;
template <typename ptr> inline virt_addr ptr2virtaddr(ptr *addr) {
  return reinterpret_cast<virt_addr>(addr);
}

class VirtmemCtrl final {
public:
  VirtmemCtrl();
  ~VirtmemCtrl();
  // 新規に仮想メモリ領域を確保する。
  // 物理メモリが割り当てられていない領域の場合は物理メモリを割り当てる
  virt_addr Alloc(size_t size);
  // ０初期化版
  virt_addr AllocZ(size_t size) {
    virt_addr addr = Alloc(size);
    bzero(reinterpret_cast<void *>(addr), size);
    return addr;
  }
  template <class T, class... Y>
    T *New(const Y& ...args) {
    virt_addr addr = Alloc(sizeof(T));
    T *t = reinterpret_cast<T *>(addr);
    return new(t) T(args...);
  }
  template <class T>
    void Delete(T *c) {
    c->~T();
    Free(reinterpret_cast<virt_addr>(c));
  }
  // 仮想メモリ領域を解放するが、物理メモリは解放しない
  void Free(virt_addr addr);
#ifndef __UNIT_TEST__
private:
#endif // __UNIT_TEST__
  // AreaManagerはAreaManagerの実体とその直後のメモリ空間を管理する
  class AreaManager {
  public:
    AreaManager(AreaManager *next, AreaManager *prev, size_t size) {
      kassert(size == align(size, 8));
      _next = next;
      _prev = prev;
      _size = size;
      _is_allocated = false;
      kassert(size >= kSizeMin);
    }
    AreaManager *GetNext() {
      return _next;
    }
    AreaManager *GetPrev() {
      return _prev;
    }
    size_t GetSize() {
      // AreaManagerを含まないサイズ
      return _size - GetAreaManagerSize();
    }
    bool IsAllocated() {
      return _is_allocated;
    }
    void Allocate() {
      _is_allocated = true;
    }
    void Free() {
      _is_allocated = false;
    }
    bool UnifyWithNext() {
      if (_is_allocated) {
        return false;
      }
      if ((!_next->_is_allocated) && (GetAddr() + _size == _next->GetAddr())) {
        _size += _next->_size;
        _next = _next->_next;
        _next->_prev = this;
        return true;
      }
      return false;
    }
    AreaManager *Divide(size_t size) {
      // sizeはAreaManagerを含まないサイズ!!!
      kassert(size == align(size, 8));
      size_t esize = size + AreaManager::GetAreaManagerSize();
      kassert(esize <= _size);
      if (esize + AreaManager::GetAreaManagerSize() + kSizeMin > _size) {
        // divideできなかった
        return nullptr;
      }
      kassert(_size == align(_size, 8));
      AreaManager *next = new(reinterpret_cast<AreaManager *>(reinterpret_cast<size_t>(this) + esize)) AreaManager(_next, this, _size - esize);
      _size = esize;
      _next = next;
      next->_next->_prev = next;
      return _next;
    }
    // areaは未初期化でOK
    void Append(AreaManager *area, size_t size) {
      area = new(area) AreaManager(this->_next, this, size);
      this->_next->_prev = area;
      this->_next = area;
    }
    virt_addr GetAreaStartAddr() {
      return static_cast<virt_addr>(reinterpret_cast<size_t>(this) + GetAreaManagerSize());
    }
    static size_t GetAreaManagerSize() {
      return alignUp(sizeof(AreaManager), 8);
    }
    size_t GetAddr() {
      return reinterpret_cast<size_t>(this);
    }
#ifndef __UNIT_TEST__
private:
#endif // __UNIT_TEST__
    AreaManager *_next;
    AreaManager *_prev;
    size_t _size;   // AreaManagerを含んだサイズ
    bool _is_allocated;
    static const size_t kSizeMin = 16;
  };
  AreaManager *_first;
  AreaManager *_list;
  AreaManager *_last_freed = nullptr;
  virt_addr _heap_end;
  static AreaManager *GetAreaManager(virt_addr addr) {
    return reinterpret_cast<AreaManager *>(addr - AreaManager::GetAreaManagerSize());
  }
  SpinLock _lock;
};

#endif // ! ASM_FILE

#endif // __RAPH_KERNEL_MEM_VIRTMEM_H__
