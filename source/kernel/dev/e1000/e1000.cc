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

#include <stdint.h>
#include "e1000.h"
#include "../../timer.h"
#include "../../mem/paging.h"
#include "../../mem/virtmem.h"
#include "../../tty.h"
#include "../../global.h"

#include "./e1000_raph.h"

driver_t em_driver;

void E1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  BsdDriver bsd;
  E1000 tmp_e1000 = E1000(bus, device, mf);
  bsd.parent = &tmp_e1000;
  bsd.driver = &em_driver;
  if (em_driver.methods[device_probe](&bsd) == BUS_PROBE_DEFAULT) {
    E1000 *addr = reinterpret_cast<E1000 *>(virtmem_ctrl->Alloc(sizeof(E1000)));
    addr = new(addr) E1000(bus, device, mf);
    addr->bsd.parent = addr;
    addr->bsd.driver = &em_driver;
    addr->bsd.driver->methods[device_probe](&addr->bsd);
    kassert(addr->bsd.driver->methods[device_attach](&addr->bsd) == 0);
  }
}


void E1000::Handle() {
}
