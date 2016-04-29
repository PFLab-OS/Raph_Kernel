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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  static inline size_t strlen(const char *str) {
    size_t len = 0;
    for(; *str != '\0'; str++, len++) {
    }
    return len;
  } 

  static inline int strncmp(const char *s1, const char *s2, size_t n) {
    for (; n > 0; n--, s1++, s2++) {
      if (*s1 != *s2) {
        return *s1 - *s2;
      }
    }
    return 0;
  }

  static inline int strcmp(const char *s1, const char *s2) {
    while((*s1 != '\0') && (*s1 == *s2)) {
      s1++;
      s2++;
    }
    return *s1 - *s2;
  }

  static inline char *strncpy(char *s1, const char *s2, size_t n) {
    char *s = s1;
    for(; n > 0; n--, s++)  {
      if (*s2 == '\0') {
        *s = '\0';
      } else {
        *s = *s2;
        s2++;
      }
    }
    return s1;
  }

  static inline char *strcpy(char *s1, const char *s2) {
    char *s = s1;
    while((*s1 = *s2) != '\0') {
      s1++;
      s2++;
    }
    return s;
  }

  static inline void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for(; n > 0; n--, d++, s++) {
      *d = *s;
    }
    return dest;
  }

  static void *memset(void *dest, uint8_t c, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    for(; n > 0; n--, d++) {
      *d = c;
    }
    return dest;
  }

  static int memcmp(const void *s1, const void *s2, size_t n) { 
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;
    for(; n > 0; n--, p1++, p2++) {
      int i = *p1 - *p2;
      if (i != 0) {
        return i;
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

  static inline char *strcat(char *s1, const char *s2) {
    char *s = s1;
    while(*s != '\0') {
      s++;
    }
    while(*s2 != '\0') {
      *s = *s2;
      s++;
      s2++;
    }
    *s = '\0';
    return s1;
  }
  
  static inline unsigned long strtoul(const char *nptr, char **endptr, int base) {
    int val = 0;
    int sign = 1;
    switch(*nptr) {
    case '+': {
      nptr++;
      break;
    }
    case '-': {
      sign = -1;
      nptr++;
      break;
    }
    }
    if (base == 0) {
      base = 10;
      if (*nptr == '0') {
        base = 8;
        nptr++;
        if (*nptr == 'x' || *nptr == 'X') {
          base = 16;
          nptr++;
        }
      }
    } else if (base == 16) {
      if (*nptr && (nptr[1] == 'x' || nptr[1] == 'X')) {
        nptr+=2;
      }
    }
    while (*nptr != '\0') {
      val *= base;
      if (base <= 10) {
        if (*nptr >= '0' && *nptr < '0' + base) {
          val += *nptr - '0';
        } else {
          *endptr = (char *)nptr;
          break;
        }
      } else {
        if (*nptr >= '0' && *nptr <= '9') {
          val += *nptr - '0';
        } else if (*nptr >= 'A' && *nptr < 'A' + base - 10) {
          val += *nptr - 'A' + 10;
        } else if (*nptr >= 'a' && *nptr < 'a' + base - 10) {
          val += *nptr - 'a' + 10;
        } else {
          *endptr = (char *)nptr;
          break;
        }
      }
      nptr++;
    }
    return val * sign;
  }

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __RAPH_LIB_STRING_H__
