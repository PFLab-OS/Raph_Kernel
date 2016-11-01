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

#ifndef __RAPH_LIB_ASSERT_H__
#define __RAPH_LIB_ASSERT_H__

#undef assert
#ifdef __KERNEL__
#include <raph.h>
#define assert(flag) kassert(flag)
#else
#define assert(flag)  #define assert(x)   do {          \
    if (!(x)) {                                         \
      printf("Assertion failed: %s, file %s, line %d\n" \
             , #x, __FILE__, __LINE__);                 \
      exit(1);                                          \
    }                                                   \
  } while (0) 
#endif /* __KERNEL__ */

#endif // __RAPH_LIB_ASSERT_H__
