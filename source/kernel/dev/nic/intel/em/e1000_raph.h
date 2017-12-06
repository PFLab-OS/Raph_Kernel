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

typedef char *caddr_t;

struct mbuf {};

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

#define KASSERT(cmp, comment) kassert(cmp)

#define __NO_STRICT_ALIGNMENT
#define DEVICE_POLLING

#endif /* __RAPH_KERNEL_E1000_RAPH_H__ */
