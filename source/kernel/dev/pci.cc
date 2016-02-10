/*
 *
 * Copyright (c) 2015 Raphine Project
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

#include "../acpi.h"
#include "../mem/physmem.h"
#include "../global.h"
#include "../tty.h"
#include "pci.h"

void PCICtrl::Init() {
  if (_mcfg == nullptr) {
    gtty->Printf("s", "could not find MCFG table");
    return;
  }
  gtty->Printf("d", -12340, "s", " ", "x", 0xABC0123, "s", "\n\n");
  for (int i = 0; i * sizeof(MCFGSt) < _mcfg->header.Length - sizeof(ACPISDTHeader); i++) {
    if (i == 1) {
      gtty->Printf("s", "multiple MCFG tables.");
      break;
    }
    if (_mcfg->list[i].ecam_base >= 0x100000000) {
      gtty->Printf("s", "ECAM base addr is not exist in low 4GB of memory");
      continue;
    }
    _base_addr = p2v(_mcfg->list[i].ecam_base);
    for (int j = _mcfg->list[i].pci_bus_start; j <= _mcfg->list[i].pci_bus_end; j++) {
      for (int k = 0; k < 32; k++) {
	uint16_t vid = ReadReg<uint16_t>(GetVaddr(j, k, 0, kVendorIDReg));
	if (vid == 0xffff) {
	  continue;
	}
	gtty->Printf("x", vid, "s", " ");
	uint16_t did = ReadReg<uint16_t>(GetVaddr(j, k, 0, kDeviceIDReg));
	gtty->Printf("x", did, "s", " ");

	uint32_t base_addr = ReadReg<uint32_t>(GetVaddr(j, k, 0, kBassAddress0));
	gtty->Printf("s", "bus:", "x", j, "s", " dev:", "x", k, "s", " ");
	gtty->Printf("s", "BAR:", "x", base_addr, "s", " ");
	bool mf = ReadReg<uint8_t>(GetVaddr(j, k, 0, kHeaderTypeReg)) & kHeaderTypeMultiFunction;
	if (mf) {
	  gtty->Printf("s", "mf");
	}
	gtty->Printf("s", "\n");

	for (int l = 0; l < _device_register_num; l++) {
	  _device_map[l]->CheckInit(vid, did, j, k, mf);
	}
      }
    }
  }
}
