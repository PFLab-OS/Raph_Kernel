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

#include "tmpmem.h"
#include <mem/virtmem.h>

void TmpmemCtrl::Init() {
  Locker locker(_lock);
  AllocNewFrame();
}

virt_addr TmpmemCtrl::Alloc(size_t size) {
  size = ((size + 7) / 8) * 8;
  kassert(size < kMaxMemSize);
  Locker locker(_lock);
  if (_frame_offset + 8 + size > kFrameSize) {
    AllocNewFrame();
    kassert(_frame_offset + 8 + size <= kFrameSize);
  }
  uint64_t *cnt = reinterpret_cast<uint64_t *>(_frame);
  *cnt = *cnt + 1;
  virt_addr addr = reinterpret_cast<virt_addr>(_frame) + _frame_offset + 8;
  uint8_t **ptr = reinterpret_cast<uint8_t **>(_frame + _frame_offset);
  *ptr = _frame;
  _frame_offset += 8 + size;
  return addr;
}

void TmpmemCtrl::Free(virt_addr addr) {
  uint8_t *frame;
  uint64_t *cnt;
  {
    Locker locker(_lock);
    uint8_t **ptr = reinterpret_cast<uint8_t **>(addr - 8);
    frame = *ptr;
    cnt = reinterpret_cast<uint64_t *>(_frame);
    *cnt = *cnt - 1;
  }  
  if (*cnt == 0) {
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(frame));
  }
}

void TmpmemCtrl::AllocNewFrame() {
  kassert(_lock.IsLocked());
  _frame = reinterpret_cast<uint8_t *>(virtmem_ctrl->Alloc(kFrameSize));
  uint64_t *cnt = reinterpret_cast<uint64_t *>(_frame);
  *cnt = 0;
  _frame_offset = 8;
}
