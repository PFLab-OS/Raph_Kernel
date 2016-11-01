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

#ifndef __RAPH_LIB_ARPA_INET_H__
#define __RAPH_LIB_ARPA_INET_H__

#ifdef __cplusplus
extern "C" {
#endif

  static inline uint32_t inet_atoi(uint8_t ip[]) {
    return (ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | ip[3];
  } 

  static inline uint16_t htons(uint16_t n) {
    return (((n & 0xff) << 8) | ((n & 0xff00) >> 8));
  }

  static inline uint16_t ntohs(uint16_t n) {
    return (((n & 0xff) << 8) | ((n & 0xff00) >> 8));
  }

  static inline uint32_t htonl(uint32_t n) {
    return (((n & 0xff) << 24)
            |((n & 0xff00) << 8)
            |((n & 0xff0000) >> 8)
            |((n & 0xff000000) >> 24));
  }

  static inline uint32_t ntohl(uint32_t n) {
    return (((n & 0xff) << 24)
            |((n & 0xff00) << 8)
            |((n & 0xff0000) >> 8)
            |((n & 0xff000000) >> 24));
  }

#ifdef __cplusplus
}
#endif

#endif /* __RAPH_LIB_ARPA_INET_H__ */
