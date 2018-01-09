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

#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

extern "C" {
int errno = 0;

// originally from musl-libc
// Copyright © 2005-2014 Rich Felker, et al.
// Released under the MIT license
// https://opensource.org/licenses/mit-license.php
int atoi(const char *s) {
  int n = 0, neg = 0;
  while (isspace(*s)) s++;
  switch (*s) {
    case '-':
      neg = 1;
    case '+':
      s++;
  }
  /* Compute n as a negative number to avoid overflow on INT_MIN */
  while (isdigit(*s)) n = 10 * n - (*s++ - '0');
  return neg ? n : -n;
}
}

#include <mem/virtmem.h>
#include <libglobal.h>

void *malloc(size_t size) {
  return reinterpret_cast<void *>(system_memory_space->GetKernelVirtmemCtrl()->Alloc(size));
}

void *calloc(size_t n, size_t size) {
  return reinterpret_cast<void *>(system_memory_space->GetKernelVirtmemCtrl()->AllocZ(n * size));
}

void free(void *ptr) { system_memory_space->GetKernelVirtmemCtrl()->Free(reinterpret_cast<virt_addr>(ptr)); }

int atexit(void (*function)(void)) {
  // DUMMY
  return 0;
}

