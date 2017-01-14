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
#include <machine/resource.h>
// #include <machine/_limits.h>
#include <machine/_bus.h>
#include <sys/_bus_dma.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define	FILTER_STRAY		0x01
#define	FILTER_HANDLED		0x02
#define	FILTER_SCHEDULE_THREAD	0x04


  /**
   * @brief State of the device.
   */
  typedef enum device_state {
    DS_NOTPRESENT = 10,		/**< @brief not probed or probe failed */
    DS_ALIVE = 20,			/**< @brief probe succeeded */
    DS_ATTACHING = 25,		/**< @brief currently attaching */
    DS_ATTACHED = 30,		/**< @brief attach method called */
    DS_BUSY = 40			/**< @brief device is open */
  } device_state_t;

  enum {
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

  struct devclass {
  };
  typedef struct devclass		*devclass_t;

  struct kobj_class {
  };
  typedef struct kobj_class	driver_t;


  uint8_t bus_space_read_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset);
  uint16_t bus_space_read_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset);
  uint32_t bus_space_read_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset);
  void bus_space_write_1(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint8_t value);
  void bus_space_write_2(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint16_t value);
  void bus_space_write_4(bus_space_tag_t space, bus_space_handle_t handle, bus_size_t offset, uint32_t value);

  struct resource;
  int bus_generic_attach(device_t dev);
  struct resource *bus_alloc_resource_any(device_t dev, int type, int *rid, u_int flags);
  int bus_release_resource(device_t dev, int type, int rid, struct resource *r);
  int bus_setup_intr(device_t dev, struct resource *r, int flags, driver_filter_t filter, driver_intr_t ithread, void *arg, void **cookiep);
  int     bus_teardown_intr(device_t dev, struct resource *r, void *cookie);
  int     bus_bind_intr(device_t dev, struct resource *r, int cpu);
  int device_printf(device_t dev, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));;

  bus_dma_tag_t bus_get_dma_tag(device_t dev);

  /*
   * Access functions for device.
   */
  device_t	device_add_child(device_t dev, const char *name, int unit);
  device_t	device_add_child_ordered(device_t dev, u_int order,
                                     const char *name, int unit);
  void	device_busy(device_t dev);
  int	device_delete_child(device_t dev, device_t child);
  int	device_delete_children(device_t dev);
  int	device_attach(device_t dev);
  int	device_detach(device_t dev);
  void	device_disable(device_t dev);
  void	device_enable(device_t dev);
  device_t	device_find_child(device_t dev, const char *classname,
                              int unit);
  const char	*device_get_desc(device_t dev);
  devclass_t	device_get_devclass(device_t dev);
  driver_t	*device_get_driver(device_t dev);
  u_int32_t	device_get_flags(device_t dev);
  device_t	device_get_parent(device_t dev);
  int	device_get_children(device_t dev, device_t **listp, int *countp);
  void	*device_get_ivars(device_t dev);
  void	device_set_ivars(device_t dev, void *ivars);
  const	char *device_get_name(device_t dev);
  const	char *device_get_nameunit(device_t dev);
  void	*device_get_softc(device_t dev);
  device_state_t	device_get_state(device_t dev);
  int	device_get_unit(device_t dev);
  struct sysctl_ctx_list *device_get_sysctl_ctx(device_t dev);
  struct sysctl_oid *device_get_sysctl_tree(device_t dev);
  int	device_is_alive(device_t dev);	/* did probe succeed? */
  int	device_is_attached(device_t dev);	/* did attach succeed? */
  int	device_is_enabled(device_t dev);
  int	device_is_suspended(device_t dev);
  int	device_is_quiet(device_t dev);
  device_t device_lookup_by_name(const char *name);
  int	device_print_prettyname(device_t dev);
  int	device_printf(device_t dev, const char *, ...) __printflike(2, 3);
  int	device_probe(device_t dev);
  int	device_probe_and_attach(device_t dev);
  int	device_probe_child(device_t bus, device_t dev);
  int	device_quiesce(device_t dev);
  void	device_quiet(device_t dev);
  void	device_set_desc(device_t dev, const char* desc);
  void	device_set_desc_copy(device_t dev, const char* desc);
  int	device_set_devclass(device_t dev, const char *classname);
  int	device_set_devclass_fixed(device_t dev, const char *classname);
  int	device_set_driver(device_t dev, driver_t *driver);
  void	device_set_flags(device_t dev, u_int32_t flags);
  void	device_set_softc(device_t dev, void *softc);
  void	device_free_softc(void *softc);
  void	device_claim_softc(device_t dev);
  int	device_set_unit(device_t dev, int unit);	/* XXX DONT USE XXX */
  int	device_shutdown(device_t dev);
  void	device_unbusy(device_t dev);
  void	device_verbose(device_t dev);
  
  /*
 * Access functions for device resources.
 */

int	resource_int_value(const char *name, int unit, const char *resname,
			   int *result);

  /**
 * Some convenience defines for probe routines to return.  These are just
 * suggested values, and there's nothing magical about them.
 * BUS_PROBE_SPECIFIC is for devices that cannot be reprobed, and that no
 * possible other driver may exist (typically legacy drivers who don't fallow
 * all the rules, or special needs drivers).  BUS_PROBE_VENDOR is the
 * suggested value that vendor supplied drivers use.  This is for source or
 * binary drivers that are not yet integrated into the FreeBSD tree.  Its use
 * in the base OS is prohibited.  BUS_PROBE_DEFAULT is the normal return value
 * for drivers to use.  It is intended that nearly all of the drivers in the
 * tree should return this value.  BUS_PROBE_LOW_PRIORITY are for drivers that
 * have special requirements like when there are two drivers that support
 * overlapping series of hardware devices.  In this case the one that supports
 * the older part of the line would return this value, while the one that
 * supports the newer ones would return BUS_PROBE_DEFAULT.  BUS_PROBE_GENERIC
 * is for drivers that wish to have a generic form and a specialized form,
 * like is done with the pci bus and the acpi pci bus.  BUS_PROBE_HOOVER is
 * for those busses that implement a generic device place-holder for devices on
 * the bus that have no more specific driver for them (aka ugen).
 * BUS_PROBE_NOWILDCARD or lower means that the device isn't really bidding
 * for a device node, but accepts only devices that its parent has told it
 * use this driver.
 */
#define BUS_PROBE_SPECIFIC	0	/* Only I can use this device */
#define BUS_PROBE_VENDOR	(-10)	/* Vendor supplied driver */
#define BUS_PROBE_DEFAULT	(-20)	/* Base OS default driver */
#define BUS_PROBE_LOW_PRIORITY	(-40)	/* Older, less desirable drivers */
#define BUS_PROBE_GENERIC	(-100)	/* generic driver for dev */
#define BUS_PROBE_HOOVER	(-1000000) /* Driver for any dev on bus */
#define BUS_PROBE_NOWILDCARD	(-2000000000) /* No wildcard device matches */

/**
 * During boot, the device tree is scanned multiple times.  Each scan,
 * or pass, drivers may be attached to devices.  Each driver
 * attachment is assigned a pass number.  Drivers may only probe and
 * attach to devices if their pass number is less than or equal to the
 * current system-wide pass number.  The default pass is the last pass
 * and is used by most drivers.  Drivers needed by the scheduler are
 * probed in earlier passes.
 */
#define	BUS_PASS_ROOT		0	/* Used to attach root0. */
#define	BUS_PASS_BUS		10	/* Busses and bridges. */
#define	BUS_PASS_CPU		20	/* CPU devices. */
#define	BUS_PASS_RESOURCE	30	/* Resource discovery. */
#define	BUS_PASS_INTERRUPT	40	/* Interrupt controllers. */
#define	BUS_PASS_TIMER		50	/* Timers and clocks. */
#define	BUS_PASS_SCHEDULER	60	/* Start scheduler. */
#define	BUS_PASS_DEFAULT	__INT_MAX /* Everything else. */

#define	BUS_PASS_ORDER_FIRST	0
#define	BUS_PASS_ORDER_EARLY	2
#define	BUS_PASS_ORDER_MIDDLE	5
#define	BUS_PASS_ORDER_LATE	7
#define	BUS_PASS_ORDER_LAST	9

#define bus_read_1(r, o) \
  bus_space_read_1(rman_get_bustag(r), rman_get_bushandle(r), (o))
#define bus_write_1(r, o, v)                                          \
  bus_space_write_1(rman_get_bustag(r), rman_get_bushandle(r), (o), (v))
#define bus_read_2(r, o) \
  bus_space_read_2(rman_get_bustag(r), rman_get_bushandle(r), (o))
#define bus_write_2(r, o, v)                                          \
  bus_space_write_2(rman_get_bustag(r), rman_get_bushandle(r), (o), (v))
#define bus_read_4(r, o) \
  bus_space_read_4(rman_get_bustag(r), rman_get_bushandle(r), (o))
#define bus_write_4(r, o, v)                                          \
  bus_space_write_4(rman_get_bustag(r), rman_get_bushandle(r), (o), (v))
  
#ifdef __cplusplus
}
#endif /* __cplusplus */
  
#endif /* _FREEBSD_BUS_H_ */
