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

#include <stdint.h>
#include <raph.h>
#include <callout.h>
#include <dev/pci.h>
#include <buf.h>
#include <mem/virtmem.h>
#include <global.h>
#include <freebsd/sys/errno.h>
#include <freebsd/sys/param.h>
#include <freebsd/sys/bus.h>
#include <freebsd/sys/bus_dma.h>
#include <freebsd/sys/endian.h>
#include <freebsd/dev/pci/pcireg.h>
#include <freebsd/net/if.h>
#include <freebsd/net/if_var.h>
#include <freebsd/net/if_types.h>
#include "bem.h"

#define NULL nullptr

typedef bool boolean_t;

static inline int sprintf(const char *s, const char *format, ...) {
  return 0;
}

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

struct ifmedia {};

struct mbuf {};

typedef struct {} eventhandler_tag;

static inline void pci_write_config(device_t dev, int reg, uint32_t val, int width) {
  switch (width) {
  case 1:
    dev->parent->WriteReg<uint8_t>(static_cast<uint16_t>(reg), static_cast<uint8_t>(val));
    return;
  case 2:
    dev->parent->WriteReg<uint16_t>(static_cast<uint16_t>(reg), static_cast<uint16_t>(val));
    return;
  case 4:
    dev->parent->WriteReg<uint32_t>(static_cast<uint16_t>(reg), static_cast<uint32_t>(val));
    return;
  default:
    kassert(false);
    return;
  }
}

static inline uint32_t pci_read_config(device_t dev, int reg, int width) {
  switch (width) {
  case 1:
    return dev->parent->ReadReg<uint8_t>(static_cast<uint16_t>(reg));
  case 2:
    return dev->parent->ReadReg<uint16_t>(static_cast<uint16_t>(reg));
  case 4:
    return dev->parent->ReadReg<uint32_t>(static_cast<uint16_t>(reg));
  default:
    kassert(false);
    return 0;
  };
}

static inline int pci_enable_busmaster(device_t dev) {
  dev->parent->WriteReg<uint16_t>(PCICtrl::kCommandReg, dev->parent->ReadReg<uint16_t>(PCICtrl::kCommandReg) | PCICtrl::kCommandRegBusMasterEnableFlag);
  return 0;
}

static inline int pci_find_cap(device_t dev, int capability, int *capreg)
{
  PCICtrl::CapabilityId id;
  switch(capability) {
  case PCIY_EXPRESS:
    id = PCICtrl::CapabilityId::kPcie;
    break;
  default:
    kassert(false);
  }
  uint16_t cap;
  if ((cap = dev->parent->FindCapability(id)) != 0) {
    *capreg = cap;
    return 0;
  } else {
    return -1;
  }
}

#define device_printf(...)

static inline void device_set_desc_copy(device_t dev, const char* desc) {
}

uint16_t pci_get_vendor(device_t dev);
uint16_t pci_get_device(device_t dev);
uint16_t pci_get_subvendor(device_t dev);
uint16_t pci_get_subdevice(device_t dev);

static const int TRUE = 1;
static const int FALSE = 0;

#define DRIVER_MODULE(...) 
#define MODULE_DEPEND(...)
#define TUNABLE_INT(...)

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


//SLIST_HEAD(sysctl_oid_list, sysctl_oid);

#define SYSCTL_HANDLER_ARGS struct sysctl_oid *oidp, void *arg1,  \
    intmax_t arg2, struct sysctl_req *req
/*
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
  };*/

#define SYSCTL_ADD_PROC(...)

#define hz 1000

typedef void timeout_t (void *);

struct callout {
  LckCallout callout;
};

struct mtx {
  SpinLock lock;
};

static inline void callout_init_mtx(struct callout *c, struct mtx *mtx, int flags) {
  new(&c->callout) LckCallout;
  c->callout.SetLock(&mtx->lock);
}

static inline int callout_stop(struct callout *c) {
  bool flag = c->callout.CanExecute();
  c->callout.Cancel();
  return flag ? 1 : 0;
}

static inline int callout_drain(struct callout *c) {
  int r = callout_stop(c);
  while(true) {
    volatile bool flag = c->callout.IsHandling();
    if (!flag) {
      break;
    }
  }
  return r;
}

static inline void callout_reset(struct callout *c, int ticks, timeout_t *func, void *arg) {
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

struct resource {
  phys_addr addr;
  bus_space_tag_t type;
  union {
    struct {
      bool is_prefetchable;
    } mem;
  } data;
};

struct resource *bus_alloc_resource_from_bar(device_t dev, int bar);
#define bus_alloc_resource_any(dev, dummy1, bar, dummy2) bus_alloc_resource_from_bar(dev, *(bar))

static inline bus_space_tag_t rman_get_bustag(struct resource *r) {
  return r->type;
}

static inline bus_space_handle_t rman_get_bushandle(struct resource *r) {
  bus_space_handle_t h;
  switch(r->type) {
  case BUS_SPACE_PIO:
  case BUS_SPACE_MEMIO:
    h = r->addr;
    break;
  default:
    kassert(false);
  }
  return h;
}

struct adapter *device_get_softc(device_t dev);

#define bootverbose 0

static inline int if_getmtu(if_t ifp) {
  return ETHERMTU;
}

// replacement of ticks
static inline int get_ticks() {
  return (timer->GetUsecFromCnt(timer->ReadMainCnt()) * hz) / 1000000;
}

#define KASSERT(cmp, comment) kassert(cmp)

#define __NO_STRICT_ALIGNMENT
#define DEVICE_POLLING

#endif /* __RAPH_KERNEL_E1000_RAPH_H__ */
