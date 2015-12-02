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
 * リスト
 * 領域が足りなくなりそうになったら足りなくなる前に自動で伸ばす
 * 
 */

#ifndef __RAPH_KERNEL_LIST_H__
#define __RAPH_KERNEL_LIST_H__

#include "spinlock.h"

template <typename T>
class List {
public:
  List();
  T *Alloc();
  void Free(T *data);
private:
  T *Extend(T *entry);
  class Container {
  public:
    Container() : _next(nullptr), _flag(0) {}
    T _entry[64];
    Container *_next;
    uint64_t _flag;
  } _first;
  Container *_list;
  SpinLock _lock;
};

#ifndef __RAPH_KERNEL_MEM_VIRTMEM_H__

#include "list_def.h"

#endif // __RAPH_KERNEL_MEM_VIRTMEM_H__

#endif // __RAPH_KERNEL_LIST_H__
