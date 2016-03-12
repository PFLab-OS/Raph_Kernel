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
#include "../../callout.h"
#include "../../freebsd/sys/errno.h"
#include "../../freebsd/sys/bus_dma.h"
#include "e1000.h"

#define NULL nullptr

typedef bool boolean_t;

static inline int sprintf(const char *s, const char *format, ...) {
  return 0;
}

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

#define BUS_PROBE_SPECIFIC  0 /* Only I can use this device */
#define BUS_PROBE_VENDOR  (-10) /* Vendor supplied driver */
#define BUS_PROBE_DEFAULT (-20) /* Base OS default driver */
#define BUS_PROBE_LOW_PRIORITY  (-40) /* Older, less desirable drivers */
#define BUS_PROBE_GENERIC (-100)  /* generic driver for dev */
#define BUS_PROBE_HOOVER  (-1000000) /* Driver for any dev on bus */
#define BUS_PROBE_NOWILDCARD  (-2000000000) /* No wildcard device matches */

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

struct BsdDriver;
typedef BsdDriver *device_t;

static inline void pci_write_config(device_t dev, int reg, uint32_t val, int width) {
  kassert(width == 2);
  dev->parent->WriteReg<uint16_t>(static_cast<uint16_t>(reg), static_cast<uint16_t>(val));
}

static inline uint32_t pci_read_config(device_t dev, int reg, int width) {
  kassert(width == 2);
  return dev->parent->ReadReg<uint16_t>(static_cast<uint16_t>(reg));
}

#define device_printf(...)

static inline void device_set_desc_copy(device_t dev, const char* desc) {
}

uint16_t pci_get_vendor(device_t dev);
uint16_t pci_get_device(device_t dev);
uint16_t pci_get_subvendor(device_t dev);
uint16_t pci_get_subdevice(device_t dev);

void *device_get_softc(device_t dev);

static const int TRUE = 1;
static const int FALSE = 0;

enum device_method {
  device_probe = 0,
  device_attach,
  device_detach,
  device_shutdown,
  device_suspend,
  device_resume,
  device_end
};

#define DEVMETHOD(method, func) [method] = func
#define DEVMETHOD_END [device_end] = nullptr

#define DRIVER_MODULE(...) 
#define MODULE_DEPEND(...)
#define TUNABLE_INT(...)

typedef int (*device_method_t)(device_t);

typedef struct driver {
  const char *name;
  const device_method_t *methods;
  int size;
} driver_t;

typedef struct devclass {
  driver_t *driver;
} devclass_t;

typedef unsigned long u_long;
typedef unsigned int u_int;

typedef int64_t intmax_t;

#define SLIST_HEAD(name, type)            \
  struct name {               \
  struct type *slh_first; /* first element */     \
  }

#define SLIST_ENTRY(type)           \
  struct {                \
  struct type *sle_next;  /* next element */      \
  }


SLIST_HEAD(sysctl_oid_list, sysctl_oid);

#define SYSCTL_HANDLER_ARGS struct sysctl_oid *oidp, void *arg1,  \
    intmax_t arg2, struct sysctl_req *req

struct sysctl_oid {
  struct sysctl_oid_list oid_children;
  struct sysctl_oid_list *oid_parent;
  SLIST_ENTRY(sysctl_oid) oid_link;
  int    oid_number;
  u_int    oid_kind;
  void    *oid_arg1;
  intmax_t   oid_arg2;
  const char  *oid_name;
  int   (*oid_handler)(SYSCTL_HANDLER_ARGS);
  const char  *oid_fmt;
  int    oid_refcnt;
  u_int    oid_running;
  const char  *oid_descr;
};

#define SYSCTL_ADD_PROC(...)

#define hz 1

typedef void timeout_t (void *);

struct callout {
  Callout callout;
};

int callout_stop(struct callout *c) {
  bool flag = c->callout.CanExecute();
  c->callout.Cancel();
  return flag ? 1 : 0;
}

int callout_drain(struct callout *c) {
  int r = callout_stop(c);
  while(true) {
    volatile bool flag = c->callout.IsHandling();
    if (!flag) {
      break;
    }
  }
  return r;
}

void callout_reset(struct callout *c, int ticks, timeout_t *func, void *arg) {
  c->callout.Cancel();
  if (ticks < 0) {
    ticks = 1;
  }
  c->callout.Init(func, arg);
  c->callout.SetHandler(static_cast<uint32_t>(ticks) * 1000 * 1000 / hz);
}

#define ETHER_ADDR_LEN    6 /* length of an Ethernet address */
#define ETHER_TYPE_LEN    2 /* length of the Ethernet type field */
#define ETHER_CRC_LEN   4 /* length of the Ethernet CRC */
#define ETHER_HDR_LEN   (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)
#define ETHER_MIN_LEN   64  /* minimum frame len, including CRC */
#define ETHER_MAX_LEN   1518  /* maximum frame len, including CRC */
#define ETHER_MAX_LEN_JUMBO 9018  /* max jumbo frame len, including CRC */

#define ETHERMTU  (ETHER_MAX_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#define ETHERMIN  (ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#define ETHERMTU_JUMBO  (ETHER_MAX_LEN_JUMBO - ETHER_HDR_LEN - ETHER_CRC_LEN)

#define EVENTHANDLER_REGISTER(...)
#define EVENTHANDLER_DEREGISTER(...)

#endif /* __RAPH_KERNEL_E1000_RAPH_H__ */
