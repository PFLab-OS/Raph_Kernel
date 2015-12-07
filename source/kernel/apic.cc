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

#include <assert.h>
#include "acpi.h"
#include "apic.h"

void ApicCtrl::Setup() {
  if (_madt) {
    int ncpu = 0;
    for(uint32_t offset = 0; offset < _madt->header.Length - sizeof(MADT);) {
      virt_addr ptr = ptr2virtaddr(_madt->table) + offset;
      MADTSt *madtSt = reinterpret_cast<MADTSt *>(ptr);
      switch (madtSt->type) {
      case MADTStType::kLocalAPIC:
	{
	  MADTStLAPIC *madtStLAPIC = reinterpret_cast<MADTStLAPIC *>(ptr);
	  ncpu++;
	}
	break;
      case MADTStType::kIOAPIC:
	{
	  MADTStIOAPIC *madtStIOAPIC = reinterpret_cast<MADTStIOAPIC *>(ptr);
	}
	break;
      default:
	break;
      }
      offset += madtSt->length;
    }
    if (ncpu > 0) {
      _lapicCtrlAddr = reinterpret_cast<uint32_t *>(p2v(_madt->lapicCtrlAddr));
      SetupLapic();
    }
  }
}

void ApicCtrl::SetupLapic() {
  assert(_lapicCtrlAddr != nullptr);

  uint32_t msr;
  // check if local apic enabled
  // see intel64 manual vol3 10.4.3 (Enabling or Disabling the Local APIC)
  asm volatile("rdmsr;":"=a"(msr):"c"(kIa32ApicBaseMsr));
  if (!(msr & kApicGlobalEnableFlag)) {
    return;
  }

  // see intel64 manual vol3 8.4.4 (MP Initialization Example)
  _lapicCtrlAddr[kLapicRegSvr] |= kLapicRegSvrApicEnableFlag;
  _lapicCtrlAddr[kLapicRegLvtErr] = 32 + 19; // TODO
  

  

  asm volatile("hlt; nop;"::"a"(_madt->lapicCtrlAddr));
}
