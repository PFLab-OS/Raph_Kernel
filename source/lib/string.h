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
#include <stddef.h>

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

static inline void *memcpy_b(uint8_t *d, const uint8_t *s, size_t n) {
  while(n--) *(d++) = *(s++);
  return d;
}

static inline void *memcpy_q(uint64_t *d, const uint64_t *s, size_t n) {
  while(n--) *(d++) = *(s++);
  return d;
}

static inline void *memcpy(void *dest, const void *src, size_t n) {
  size_t d = reinterpret_cast<size_t>(dest);
  size_t s = reinterpret_cast<size_t>(src);
  /* if ((d % 8) == (s % 8)) { */
  /*   size_t d2 = (d + 7) / 8 * 8; */
  /*   size_t s2 = (s + 7) / 8 * 8; */
  /*   size_t d3 = (d + n) / 8 * 8; */
  /*   size_t s3 = (s + n) / 8 * 8; */
  /*   memcpy_b(reinterpret_cast<uint8_t *>(dest), reinterpret_cast<const uint8_t *>(src), d2 - d); */
  /*   memcpy_q(reinterpret_cast<uint64_t *>(d2), reinterpret_cast<const uint64_t *>(s2), (d3 - d2) / 8); */
  /*   memcpy_b(reinterpret_cast<uint8_t *>(d3), reinterpret_cast<const uint8_t *>(s3), (d + n - d3)); */
  /* } else { */
    memcpy_b(reinterpret_cast<uint8_t *>(dest), reinterpret_cast<const uint8_t *>(src), n);
  /* } */
  return dest;
}

static inline void *memset(void *buf, int ch, size_t n) {
  for (size_t i = 0; i < n; i++) {
    reinterpret_cast<uint8_t *>(buf)[i] = ch;
  }
  return buf;
}

static inline int memcmp(const void *buf1, const void *buf2, size_t n) {
  const unsigned char *b1 = reinterpret_cast<const unsigned char *>(buf1);
  const unsigned char *b2 = reinterpret_cast<const unsigned char *>(buf2);
  for (size_t i = 0; i < n; i++, b1++, b2++) {
    int j;
    if ((j = *b1 - *b2) != 0) {
      return j;
    }
  }
  return 0;
}

static inline int bcmp(const void *s1, const void *s2, size_t n) {
  return memcmp(s1, s2, n);
}

static inline void bzero(void *s, size_t n) {
  memset(s, 0, n);
}


#endif // __RAPH_LIB_STRING_H__
