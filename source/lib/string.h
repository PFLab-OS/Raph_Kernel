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

#ifndef __RAPH_LIB_STRING_H__
#define __RAPH_LIB_STRING_H__

#include <stdint.h>

static inline size_t strlen(const char *str) {
  const char *s;
  for (s = str; *s; ++s);
  return(s - str);
} 

static inline int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++, s1++, s2++) {
    if (*s1 != *s2) {
      return *s1 - *s2;
    }
  }
  return 0;
}

static inline int strcmp(const char *s1, const char *s2) {
  while(!*s1 && *s1 != *s2) {
    s1++;
    s2++;
  }
  return *s1 - *s2;
}

static inline char *strncpy(char *s1, const char *s2, size_t n) {
  char *s = s1;
  while (n > 0 && *s2 != '\0') { *s++ = *s2++; --n; }
  while (n > 0) { *s++ = '\0'; --n; }
  return s1;
}

static void * memcpy(void *dest, const void *src, size_t n) {
  uint8_t *d = reinterpret_cast<uint8_t *>(dest);
  const uint8_t *s = reinterpret_cast<const uint8_t *>(src);
  while(n--) *(d++) = *(s++);
  return dest;
}

static void *memset(void *dest, uint8_t c, size_t n) {
  if(n) {
    uint8_t *d = reinterpret_cast<uint8_t *>(dest);
    do { *d++ = c; } while (--n);
  }
  return dest;
}

#endif // __RAPH_LIB_STRING_H__
