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

#ifndef __RAPH_LIB_CTYPE_H__
#define __RAPH_LIB_CTYPE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  static inline int isdigit(int c) {
    return (c >= '0' && c <= '9') ? 1 : 0;
  }

  static inline int isxdigit(int c) {
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) ? 1 : 0;
  }

  static inline int isprint(int c) {
    return (c >= 0x20 && c <= 0x7E) ? 1 : 0;
  }

  static inline int isspace(int c) {
    switch(c) {
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      return 1;
    default:
      return 0;
    }
  }

  static inline int isupper(int c) {
    return (c >= 'A' && c <= 'Z') ? 1 : 0;
  }

  static inline int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
      return c - 'A' + 'a';
    }
    return c;
  }

  static inline int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
      return c - 'a' + 'A';
    }
    return c;
  }
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RAPH_LIB_CTYPE_H__ */
