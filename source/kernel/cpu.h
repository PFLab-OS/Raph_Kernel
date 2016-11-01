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
 * Author: Liva, hikalium
 * 
 */

#ifndef __RAPH_LIB_CPU_H__
#define __RAPH_LIB_CPU_H__

#include <raph.h>
#include <global.h>
#include <mem/kstack.h>
#include <cpu.h>
#include <apic.h>

enum class CpuPurpose {
  kNone = 0,
  kLowPriority,
  kGeneralPurpose,
  kHighPerformance,
  kCpuPurposesNum
};

class CpuCtrlInterface {
public:
  const int kCpuIdNotFound = -1;
  const int kCpuIdBootProcessor = 0;
  
  virtual ~CpuCtrlInterface() {
  }
  virtual void Init() = 0;
  virtual bool IsInitialized() = 0;
  virtual CpuId GetCpuId() = 0;
  virtual int GetHowManyCpus() = 0;
  virtual CpuId RetainCpuIdForPurpose(CpuPurpose p) = 0;
  virtual void ReleaseCpuId(CpuId cpuid) = 0;
  virtual void AssignCpusNotAssignedToGeneralPurpose() = 0;
};

#ifdef __KERNEL__
#include <mem/kstack.h>

class CpuCtrl : public CpuCtrlInterface {
public:
  CpuCtrl() {
  }
  virtual void Init() override;
  virtual bool IsInitialized() override {
    return _is_initialized;
  }
  virtual CpuId GetCpuId() override {
    if (!KernelStackCtrl::IsInitialized()) {
      kassert(apic_ctrl->GetCpuId() == 0);
      CpuId cpuid(0);
      return cpuid;
    }
    return KernelStackCtrl::GetCtrl().GetCpuId();
  }
  virtual int GetHowManyCpus() override {
    return apic_ctrl->GetHowManyCpus();
  }
  virtual CpuId RetainCpuIdForPurpose(CpuPurpose p) override; 
  virtual void ReleaseCpuId(CpuId cpuid) override {
    int raw_cpu_id = cpuid.GetRawId();
    if(_cpu_purpose_count[raw_cpu_id] > 0) _cpu_purpose_count[raw_cpu_id]--;
    if(_cpu_purpose_count[raw_cpu_id] == 0) {
      _cpu_purpose_map[raw_cpu_id] = CpuPurpose::kNone;
    }
  }
  virtual void AssignCpusNotAssignedToGeneralPurpose() override {
    int len = GetHowManyCpus();
    for(int i = 0; i < len; i++) {
      if(_cpu_purpose_map[i] == CpuPurpose::kNone) {
        RetainCpuId(i, CpuPurpose::kGeneralPurpose);
      }
    }
  }
private:
  bool _is_initialized = false;
  CpuPurpose *_cpu_purpose_map;
  int *_cpu_purpose_count;
  // do not count for cpuid:0 (boot processor is always assigned to kLowPriority)
  int GetCpuIdNotAssigned(){
    int len = GetHowManyCpus();
    for(int i = 0; i < len; i++){
      if(_cpu_purpose_map[i] == CpuPurpose::kNone) return i;
    }
    return kCpuIdNotFound;
  }
  int GetCpuIdLessAssignedFor(CpuPurpose p) {
    int minCount = -1, minId = kCpuIdNotFound;
    int len = GetHowManyCpus();
    for(int i = 0; i < len; i++) {
      if(_cpu_purpose_map[i] == p &&
         (minCount == -1 || _cpu_purpose_count[i] < minCount)){
        minCount = _cpu_purpose_count[i];
        minId = i;
      }
    }
    return minId;
  }
  void RetainCpuId(int cpuid, CpuPurpose p) {
    if(_cpu_purpose_map[cpuid] != p) {
      _cpu_purpose_map[cpuid] = p;
      _cpu_purpose_count[cpuid] = 0;
    }
    _cpu_purpose_count[cpuid]++;
  }
};

inline bool CpuId::IsValid() {
  return _rawid >= 0 && (!cpu_ctrl->IsInitialized() || _rawid < cpu_ctrl->GetHowManyCpus());
}

inline void CpuId::CheckIfValid() {
  if (!IsValid()) {
    kernel_panic("CpuId", "Invalid ID");
  }
}

inline uint8_t CpuId::GetApicId() {
  return apic_ctrl->GetApicIdFromCpuId(*this);
}

#else
#include <thread.h>
#endif /* __KERNEL__ */

#endif /* __RAPH_LIB_CPU_H__ */
