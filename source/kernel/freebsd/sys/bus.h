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
#include <stddef.h>
#include <sys/types.h>
#include <machine/_bus.h>
#include <sys/_bus_dma.h>
#include <machine/resource.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define	FILTER_STRAY		0x01
#define	FILTER_HANDLED		0x02
#define	FILTER_SCHEDULE_THREAD	0x04

typedef enum {
  BUS_SPACE_MEMIO,
  BUS_SPACE_PIO
};

typedef uint64_t bus_space_handle_t;

typedef int (*driver_filter_t)(void*);
typedef void (*driver_intr_t)(void*);

enum intr_type {
	INTR_TYPE_TTY = 1,
	INTR_TYPE_BIO = 2,
	INTR_TYPE_NET = 4,
	INTR_TYPE_CAM = 8,
	INTR_TYPE_MISC = 16,
	INTR_TYPE_CLK = 32,
	INTR_TYPE_AV = 64,
	INTR_EXCL = 256,		/* exclusive interrupt */
	INTR_MPSAFE = 512,		/* this interrupt is SMP safe */
	INTR_ENTROPY = 1024,		/* this interrupt provides entropy */
	INTR_MD1 = 4096,		/* flag reserved for MD use */
	INTR_MD2 = 8192,		/* flag reserved for MD use */
	INTR_MD3 = 16384,		/* flag reserved for MD use */
	INTR_MD4 = 32768		/* flag reserved for MD use */
};

enum intr_trigger {
	INTR_TRIGGER_CONFORM = 0,
	INTR_TRIGGER_EDGE = 1,
	INTR_TRIGGER_LEVEL = 2
};

enum intr_polarity {
	INTR_POLARITY_CONFORM = 0,
	INTR_POLARITY_HIGH = 1,
	INTR_POLARITY_LOW = 2
};

  uint8_t bus_space_read_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset);
  uint16_t bus_space_read_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset);
  uint32_t bus_space_read_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset);
  void bus_space_write_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint8_t value);
  void bus_space_write_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint16_t value);
  void bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint32_t value);

  struct resource;
  struct resource *bus_alloc_resource_any(device_t dev, int type, int *rid, u_int flags);
  int bus_setup_intr(device_t dev, struct resource *r, int flags, driver_filter_t filter, driver_intr_t ithread, void *arg, void **cookiep);
  int device_printf(device_t dev, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));;

  bus_dma_tag_t bus_get_dma_tag(device_t dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */
  
#endif /* _FREEBSD_BUS_H_ */
