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

#ifndef __RAPH_KERNEL_E1000_RAPH_H__
#define __RAPH_KERNEL_E1000_RAPH_H__

#include "../../raph.h"
#include <stdint.h>

#define NULL nullptr

typedef bool boolean_t;

typedef enum {
  BUS_SPACE_MEMIO,
  BUS_SPACE_PIO
} bus_space_tag_t;

typedef union {
  uint32_t            ioport;
  volatile uint8_t *maddr;
} bus_space_handle_t;

typedef void *bus_addr_t;
typedef int bus_size_t;

typedef char *caddr_t;

struct ahd_linux_dma_tag {
  bus_size_t      alignment;
  bus_size_t      boundary;
  bus_size_t      maxsize;
};
typedef struct ahd_linux_dma_tag* bus_dma_tag_t;

typedef unsigned long long dma_addr_t;
typedef dma_addr_t bus_dmamap_t;

typedef struct bus_dma_segment
{
   dma_addr_t      ds_addr;
   bus_size_t      ds_len;
} bus_dma_segment_t;

struct task {};

struct inet {};
typedef struct ifnet * if_t;

struct ifmedia {};

struct callout {};

typedef struct {} eventhandler_tag;

static inline uint8_t bus_space_read_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset) {
  kassert(space == BUS_SPACE_MEMIO);
  return reinterpret_cast<volatile uint8_t *>(handle.maddr)[offset / sizeof(uint8_t)];
}

static inline uint16_t bus_space_read_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset) {
  kassert(space == BUS_SPACE_MEMIO);
  return reinterpret_cast<volatile uint16_t *>(handle.maddr)[offset / sizeof(uint32_t)];
}

static inline uint32_t bus_space_read_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset) {
  kassert(space == BUS_SPACE_MEMIO);
  return reinterpret_cast<volatile uint32_t *>(handle.maddr)[offset / sizeof(uint32_t)];
}

static inline void bus_space_write_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint8_t value) {
  kassert(space == BUS_SPACE_MEMIO);
  reinterpret_cast<volatile uint8_t *>(handle.maddr)[offset / sizeof(uint8_t)] = value;
}

static inline void bus_space_write_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint16_t value) {
  kassert(space == BUS_SPACE_MEMIO);
  reinterpret_cast<volatile uint16_t *>(handle.maddr)[offset / sizeof(uint16_t)] = value;
}

static inline void bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint32_t value) {
  kassert(space == BUS_SPACE_MEMIO);
  reinterpret_cast<volatile uint32_t *>(handle.maddr)[offset / sizeof(uint32_t)] = value;
}

class E1000;
static inline void device_printf(E1000 *dev, char *str) {
}

static const int ENXIO = 6;
static const int ENOMEM = 12;

static const int TRUE = 1;
static const int FALSE = 0;

#endif /* __RAPH_KERNEL_E1000_RAPH_H__ */
