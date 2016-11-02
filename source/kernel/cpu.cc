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
 * Author: hikalium
 * 
 */

#ifdef __KERNEL__
#include <apic.h>
#include <global.h>
#include <cpu.h>

//
// CpuCtrl
//


void CpuCtrl::Init() {
  _cpu_purpose_map = new CpuPurpose[apic_ctrl->GetHowManyCpus()];
  _cpu_purpose_count = new int[apic_ctrl->GetHowManyCpus()];
  for (int i = 0; i < apic_ctrl->GetHowManyCpus(); i++) {
    if (i == 0) {
      _cpu_purpose_map[0] = CpuPurpose::kLowPriority;
    } else {
      _cpu_purpose_map[i] = CpuPurpose::kNone;
    }
    _cpu_purpose_count[i] = 0;
  }
  _is_initialized = true;
}

CpuId CpuCtrl::RetainCpuIdForPurpose(CpuPurpose p) {
  // Returns valid CpuId all time.
  // boot processor is always assigned to kLowPriority
  if(p == CpuPurpose::kLowPriority) return CpuId(CpuId::kCpuIdBootProcessor);
  int cpu_id;
  cpu_id = GetCpuIdNotAssigned();
  if(cpu_id != kCpuIdNotFound) {
    RetainCpuId(cpu_id, p);
    return CpuId(cpu_id);
  }
  cpu_id = GetCpuIdLessAssignedFor(p);
  if(cpu_id != kCpuIdNotFound) {
    RetainCpuId(cpu_id, p);
    return CpuId(cpu_id);
  }
  return CpuId(CpuId::kCpuIdBootProcessor);
}

#else
#include <thread.h>
#endif /* __KERNEL__ */

