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

#include <errno.h>
#include <stdlib.h>

extern "C" {
#ifdef __KERNEL__
  int errno = 0;
#endif // __KERNEL__
}

#ifdef __KERNEL__
#include <mem/virtmem.h>
#include <libglobal.h>

void *malloc (size_t size) {
  return reinterpret_cast<void *>(virtmem_ctrl->Alloc(size));
}

void *calloc (size_t n, size_t size) {
  return reinterpret_cast<void *>(virtmem_ctrl->AllocZ(n * size));
}

void free (void *ptr) {
  virtmem_ctrl->Free(reinterpret_cast<virt_addr>(ptr));
}

#endif // __KERNEL__
