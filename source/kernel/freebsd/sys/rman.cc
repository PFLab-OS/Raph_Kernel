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
#include <sys/rman.h>
#include <sys/bus-raph.h>
#include <sys/rman-raph.h>

bus_space_tag_t rman_get_bustag(struct resource *r) {
  return r->type;
}

bus_space_handle_t rman_get_bushandle(struct resource *r) {
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
