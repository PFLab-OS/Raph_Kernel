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
 * Author: Levelfour
 * 
 */

#ifndef __RAPH_LIB_MEM_UVIRTMEM_H__
#define __RAPH_LIB_MEM_UVIRTMEM_H__

#ifndef __KERNEL__

#include <stdlib.h>
#include <stdint.h>
#include <mem/virtmem.h>

class UVirtmemCtrl : public VirtmemCtrl {
public:
  virtual virt_addr Alloc(size_t size) override;
  virtual void Free(virt_addr addr) override;
  virtual virt_addr Sbrk(int64_t increment) override {
    abort();
  }
};

#endif // !__KERNEL__
#endif // __RAPH_LIB_MEM_UVIRTMEM_H__
