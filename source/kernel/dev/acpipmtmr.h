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

#ifndef __RAPH_KERNEL_DEV_ACPIPMTMR_H__
#define __RAPH_KERNEL_DEV_ACPIPMTMR_H__

#include <stdint.h>
#include "../acpi.h"
#include "../timer.h"
#include "../global.h"

class AcpiPmTimer : public Timer {
 public:
  virtual bool Setup() override {
    kassert(acpi_ctrl->GetFADT() != nullptr);
    _port = acpi_ctrl->GetFADT()->PmTmrBlk;

    _cnt_clk_period = 279 >> (32 - 24);
    return true;
  }
  virtual volatile uint32_t ReadMainCnt() override {
    uint32_t val;
    asm volatile("inl %%dx, %%eax":"=a"(val): "d"(_port));
    return val << (32 - 24);
  }
 private:
  uint32_t _port;
};

#endif /* __RAPH_KERNEL_DEV_ACPIPMTMR_H__ */
