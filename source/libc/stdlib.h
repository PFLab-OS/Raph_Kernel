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
 * Author: Levelfour
 * 
 */

#ifndef __RAPH_LIB_STDLIB_H__
#define __RAPH_LIB_STDLIB_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  
  uint32_t rand();
  void abort();


#ifdef __KERNEL__
  void *malloc (size_t size) __attribute__((malloc));
  void *calloc (size_t n, size_t size) __attribute__((malloc));
  void free(void *ptr);
#endif /* __KERNEL__ */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __RAPH_LIB_STDLIB_H__
