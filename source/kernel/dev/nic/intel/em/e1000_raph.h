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
#include <stddef.h>
#include <raph.h>
#include <task.h>
#include <dev/pci.h>
#include <buf.h>
#include <mem/virtmem.h>
#include <global.h>
#include <freebsd/sys/errno.h>
#include <freebsd/sys/param.h>
#include <freebsd/sys/bus-raph.h>
#include <freebsd/sys/types.h>
#include <freebsd/sys/bus_dma.h>
#include <freebsd/sys/endian.h>
#include <freebsd/dev/pci/pcireg.h>
#include <freebsd/net/if.h>
#include <freebsd/net/if_var-raph.h>
#include <freebsd/net/if_types.h>

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

struct ifmedia {};

struct mbuf {};

typedef struct {} eventhandler_tag;

static inline void pci_write_config(device_t dev, int reg, uint32_t val, int width) {
  switch (width) {
  case 1:
    dev->GetPciClass()->WriteReg<uint8_t>(static_cast<uint16_t>(reg), static_cast<uint8_t>(val));
    return;
  case 2:
    dev->GetPciClass()->WriteReg<uint16_t>(static_cast<uint16_t>(reg), static_cast<uint16_t>(val));
    return;
  case 4:
    dev->GetPciClass()->WriteReg<uint32_t>(static_cast<uint16_t>(reg), static_cast<uint32_t>(val));
    return;
  default:
    kassert(false);
    return;
  }
}

static inline uint32_t pci_read_config(device_t dev, int reg, int width) {
  switch (width) {
  case 1:
    return dev->GetPciClass()->ReadReg<uint8_t>(static_cast<uint16_t>(reg));
  case 2:
    return dev->GetPciClass()->ReadReg<uint16_t>(static_cast<uint16_t>(reg));
  case 4:
    return dev->GetPciClass()->ReadReg<uint32_t>(static_cast<uint16_t>(reg));
  default:
    kassert(false);
    return 0;
  };
}

static inline int pci_enable_busmaster(device_t dev) {
  dev->GetPciClass()->WriteReg<uint16_t>(PciCtrl::kCommandReg, dev->GetPciClass()->ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);
  return 0;
}

static inline int pci_find_cap(device_t dev, int capability, int *capreg)
{
  PciCtrl::CapabilityId id;
  switch(capability) {
  case PCIY_EXPRESS:
    id = PciCtrl::CapabilityId::kPcie;
    break;
  default:
    kassert(false);
  }
  uint16_t cap;
  if ((cap = dev->GetPciClass()->FindCapability(id)) != 0) {
    *capreg = cap;
    return 0;
  } else {
    return -1;
  }
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

#define EVENTHANDLER_REGISTER(...)
#define EVENTHANDLER_DEREGISTER(...)

#define bootverbose 0

#define KASSERT(cmp, comment) kassert(cmp)

#define __NO_STRICT_ALIGNMENT
#define DEVICE_POLLING

#endif /* __RAPH_KERNEL_E1000_RAPH_H__ */
