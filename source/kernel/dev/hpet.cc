/*
 *
 * Copyright (c) 2016 Project Raphine
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

#include "hpet.h"
#include "../acpi.h"

void Hpet::Setup() {
  _dt = acpi_ctrl->GetHPETDT();
  kassert(_dt != nullptr);
  phys_addr pbase = _dt->BaseAddr;
  _reg = reinterpret_cast<uint64_t *>(p2v(pbase));

  _cnt_clk_period = _reg[kRegGenCapabilities] >> 32;
  
  // Enable Timer
  _reg[kRegGenConfig] |= kRegGenConfigFlagEnable;
}
