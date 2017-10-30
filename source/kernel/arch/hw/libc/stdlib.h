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
 * Author: Liva, Levelfour
 * 
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
  
  uint32_t rand();
  void abort();


  void *malloc (size_t size) __attribute__((malloc));
  void *calloc (size_t n, size_t size) __attribute__((malloc));
  void free(void *ptr);

  int atexit(void (*function)(void));


  // originally from musl-libc
  // Copyright © 2005-2014 Rich Felker, et al.
  // Released under the MIT license
  // https://opensource.org/licenses/mit-license.php
  static int atoi(const char *s)
  {
    int n=0, neg=0;
    while (isspace(*s)) s++;
    switch (*s) {
    case '-': neg=1;
    case '+': s++;
    }
    /* Compute n as a negative number to avoid overflow on INT_MIN */
    while (isdigit(*s))
      n = 10*n - (*s++ - '0');
    return neg ? n : -n;
  }
#ifdef __cplusplus
}
#endif /* __cplusplus */
