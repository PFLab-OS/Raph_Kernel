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
 * see IA-PC HPET Specification
 *
 */

#ifndef __RAPH_KERNEL_DEV_HPET_H__
#define __RAPH_KERNEL_DEV_HPET_H__

#include <stdint.h>
#include "../mem/physmem.h"
#include "../acpi.h"
#include "../timer.h"
#include "../global.h"

struct HPETDT {
  ACPISDTHeader header;
  uint32_t EventTimerBlockID;
  uint8_t BaseAddrSpaceID;
  uint8_t BaseAddrRegisterBitWidth;
  uint8_t BaseAddrRegisterBitOffset;
  uint8_t BaseAddrReserved;
  uint64_t BaseAddr;
  uint8_t SeqNum;
  uint16_t MinumClockTick;
  uint8_t PageProtectionAndOEMAttribute;
} __attribute__ ((packed));

class Hpet : public Timer {
 public:
  virtual bool Setup() override {
    _dt = acpi_ctrl->GetHPETDT();
    if (_dt == nullptr) {
      return false;
    }
    phys_addr pbase = _dt->BaseAddr;
    _reg = reinterpret_cast<uint64_t *>(p2v(pbase));

    _cnt_clk_period = (_reg[kRegGenCapabilities] >> 32) / (1000 * 1000);
  
    // Enable Timer
    _reg[kRegGenConfig] |= kRegGenConfigFlagEnable;
    return true;
  }
  virtual volatile uint32_t ReadMainCnt() override {
    return _reg[kRegMainCnt];
  }
 private:
  HPETDT *_dt;
  uint64_t *_reg;
  
  // see manual 2.3
  static const int kRegGenCapabilities = 0x0 / sizeof(uint64_t);
  static const int kRegGenConfig = 0x10 / sizeof(uint64_t);
  static const int kRegMainCnt = 0xF0 / sizeof(uint64_t);

  static const uint64_t kRegGenConfigFlagEnable = 1 << 0;
};

#endif /* __RAPH_KERNEL_DEV_HPET_H__ */
