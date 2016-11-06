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

#ifndef __RAPH_LIB_MEM_VIRTMEM_H__
#define __RAPH_LIB_MEM_VIRTMEM_H__

#include <stdint.h>
#include <string.h>
#include <raph.h>

typedef uint64_t virt_addr;
template <typename ptr> inline virt_addr ptr2virtaddr(ptr *addr) {
  return reinterpret_cast<virt_addr>(addr);
}

class VirtmemCtrl {
public:
  VirtmemCtrl() {
  }
  virtual ~VirtmemCtrl() {
  }
  virtual virt_addr Alloc(size_t size) = 0;
  virtual void Free(virt_addr addr) = 0;
  // ０初期化版
  virt_addr AllocZ(size_t size) {
    virt_addr addr = Alloc(size);
    bzero(reinterpret_cast<void *>(addr), size);
    return addr;
  }
  template <class T, class... Y>
  [[deprecated]] T *New(const Y& ...args) {
    virt_addr addr = Alloc(sizeof(T));
    T *t = reinterpret_cast<T *>(addr);
    return new(t) T(args...);
  }
  template <class T>
  [[deprecated]] void Delete(T *c) {
    c->~T();
    Free(reinterpret_cast<virt_addr>(c));
  }
  virtual virt_addr Sbrk(int64_t increment) = 0;
private:
};

#endif // __RAPH_LIB_MEM_VIRTMEM_H__
