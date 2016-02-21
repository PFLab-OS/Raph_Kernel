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

#ifndef __RAPH_KERNEL_HPET_H__
#define __RAPH_KERNEL_HPET_H__

#include <stdint.h>
#include "../mem/physmem.h"
#include "../acpi.h"

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

class Hpet {
 public:
  void Setup();
  volatile uint64_t ReadMainCnt() {
    return _reg[kRegMainCnt];
  }
  // us秒後のカウントを取得する
  volatile uint64_t GetCntAfterPeriod(uint64_t us) {
    volatile uint64_t cur = ReadMainCnt();
    return cur + (us * 1000000000) / _cnt_clk_period;
  }
  volatile bool IsTimePassed(volatile uint64_t time) {
    volatile uint64_t cur = ReadMainCnt();
    return cur >= time;
  }
  // ビジーループタイムアウト
  // 最適化回避のためにできるだけこの関数を使うべき
  void BusyUwait(uint64_t us) {
    volatile uint64_t t = GetCntAfterPeriod(1000*1000);
    while(true) {
      volatile bool flag = IsTimePassed(t);
      if (flag) {
        break;
      }
    }
  }
  uint32_t _cnt_clk_period = 0;
 private:
  HPETDT *_dt;
  uint64_t *_reg;
  
  // see manual 2.3
  static const int kRegGenCapabilities = 0x0 / sizeof(uint64_t);
  static const int kRegGenConfig = 0x10 / sizeof(uint64_t);
  static const int kRegMainCnt = 0xF0 / sizeof(uint64_t);

  static const uint64_t kRegGenConfigFlagEnable = 1 << 0;
};

#endif /* __RAPH_KERNEL_HPET_H__ */
