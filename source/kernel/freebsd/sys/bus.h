/*-
 * Copyright (c) 1997,1998,2003 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _FREEBSD_BUS_H_
#define _FREEBSD_BUS_H_

#include <stdint.h>
#include <raph.h>

typedef enum {
  BUS_SPACE_MEMIO,
  BUS_SPACE_PIO
} bus_space_tag_t;

typedef uint64_t bus_space_handle_t;

typedef void *bus_addr_t;
typedef int bus_size_t;

static inline uint8_t bus_space_read_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset) {
  switch(space) {
  case BUS_SPACE_MEMIO:
    return reinterpret_cast<volatile uint8_t *>(handle)[offset / sizeof(uint8_t)];
  case BUS_SPACE_PIO:
    return inb(handle + offset);
  default:
    kassert(false);
  }
}

static inline uint16_t bus_space_read_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset) {
  switch(space) {
  case BUS_SPACE_MEMIO:
    return reinterpret_cast<volatile uint16_t *>(handle)[offset / sizeof(uint16_t)];
  case BUS_SPACE_PIO:
    return inw(handle + offset);
  default:
    kassert(false);
  }
}

static inline uint32_t bus_space_read_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset) {
  switch(space) {
  case BUS_SPACE_MEMIO:
    return reinterpret_cast<volatile uint32_t *>(handle)[offset / sizeof(uint32_t)];
  case BUS_SPACE_PIO:
    return inl(handle + offset);
  default:
    kassert(false);
  }
}

static inline void bus_space_write_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint8_t value) {
  switch(space) {
  case BUS_SPACE_MEMIO:
    reinterpret_cast<volatile uint8_t *>(handle)[offset / sizeof(uint8_t)] = value;
    return;
  case BUS_SPACE_PIO:
    outb(handle + offset, value);
    return;
  default:
    kassert(false);
  }
}

static inline void bus_space_write_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint16_t value) {
  switch(space) {
  case BUS_SPACE_MEMIO:
    reinterpret_cast<volatile uint16_t *>(handle)[offset / sizeof(uint16_t)] = value;
    return;
  case BUS_SPACE_PIO:
    outw(handle + offset, value);
    return;
  default:
    kassert(false);
  }
}

static inline void bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint32_t value) {
  switch(space) {
  case BUS_SPACE_MEMIO:
    reinterpret_cast<volatile uint32_t *>(handle)[offset / sizeof(uint32_t)] = value;
    return;
  case BUS_SPACE_PIO:
    outl(handle + offset, value);
    return;
  default:
    kassert(false);
  }
}


#endif /* _FREEBSD_BUS_H_ */
