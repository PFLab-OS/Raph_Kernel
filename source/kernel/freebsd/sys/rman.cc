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

#include <sys/kernel.h>
#include <sys/types.h>
#include <sys/rman.h>
#include <sys/bus-raph.h>

extern "C" {
  struct resource_i {
    struct resource         r_r;
    TAILQ_ENTRY(resource_i) r_link;
    LIST_ENTRY(resource_i)  r_sharelink;
    LIST_HEAD(, resource_i) *r_sharehead;
    rman_res_t      r_start;        /* index of the first entry in this resource */
    rman_res_t      r_end;          /* index of the last entry (inclusive) */
    u_int   r_flags;
    void    *r_virtual;     /* virtual address of this resource */
    device_t r_dev;   /* device which has allocated this resource */
    struct rman *r_rm;      /* resource manager from whence this came */
    int     r_rid;          /* optional rid for this resource. */
  };

  static struct resource_i *int_alloc_resource() {
    return new resource_i;
  }

  struct resource *rman_reserve_resource(struct rman *rm, rman_res_t start,
                                         rman_res_t end, rman_res_t count,
                                         u_int flags, device_t dev) {
    struct resource *r = new resource;
    r->__r_i = int_alloc_resource();
    r->__r_i->r_start = start;
    r->__r_i->r_end = end;
    r->__r_i->r_flags = flags;
    r->__r_i->r_dev = dev;
    r->__r_i->r_rm = rm;
    return r;
  }

  int	rman_release_resource(struct resource *r) {
    delete r;
    return 0;
  }

  void rman_set_bustag(struct resource *_r, bus_space_tag_t _t){
    _r->r_bustag = _t;
  }
  
  bus_space_tag_t rman_get_bustag(struct resource *r) {
    return r->r_bustag;
  }

  void rman_set_bushandle(struct resource *_r, bus_space_handle_t _h) {
    _r->r_bushandle = _h;
  }

  bus_space_handle_t rman_get_bushandle(struct resource *r) {
    return r->r_bushandle;
  }

  int rman_fini(struct rman *rm) {
    return 0;
  }
  
  int rman_init(struct rman *rm) {
    return 0;
  }

  rman_res_t rman_get_end(struct resource *r) {
    return r->__r_i->r_end;
  }

  rman_res_t rman_get_start(struct resource *r) {
    return r->__r_i->r_start;
  }

  int rman_manage_region(struct rman *rm, rman_res_t start, rman_res_t end) {
    return 0;
  }
}
