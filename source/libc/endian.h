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

#pragma once

#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

static inline uint16_t htole16(uint16_t v) {
  return v;
}
static inline uint32_t htole32(uint32_t v) {
  return v;
}
static inline uint64_t htole64(uint64_t v) {
  return v;
}

static inline uint16_t le16toh(uint16_t v) {
  return v;
}
static inline uint32_t le32toh(uint32_t v) {
  return v;
}
static inline uint64_t le64toh(uint64_t v) {
  return v;
}

static inline uint16_t htobe16(uint16_t v) {
  return __builtin_bswap16(v);
}
static inline uint32_t htobe32(uint32_t v) {
  return __builtin_bswap32(v);
}
static inline uint64_t htobe64(uint64_t v) {
  return __builtin_bswap64(v);
}

static inline uint16_t be16toh(uint16_t v) {
  return __builtin_bswap16(v);
}
static inline uint32_t be32toh(uint32_t v) {
  return __builtin_bswap32(v);
}
static inline uint64_t be64toh(uint64_t v) {
  return __builtin_bswap64(v);
}

#else
#error not supported
#endif

