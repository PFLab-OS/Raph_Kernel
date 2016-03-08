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


void E1000::InitPCI(uint16_t vid, uint16_t did, uint8_t bus, uint8_t device, bool mf) {
  DevPCI pci(bus, device, mf);
  if (lem_probe(&pci) == BUS_PROBE_DEFAULT) {
    E1000 *addr = reinterpret_cast<E1000 *>(virtmem_ctrl->Alloc(sizeof(E1000)));
    e1000 = new(addr) E1000(bus, device, mf);
    kassert(lem_attach(e1000) == 0);
  }
}


void E1000::Handle() {
}
