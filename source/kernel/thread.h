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
 * Author: Levelfour
 * 
 * ApicCtrl class for test_main.cc
 *
 *  16/04/15 created
 *
 */

#ifndef __RAPH_KERNEL_THREAD_H__
#define __RAPH_KERNEL_THREAD_H__

#ifdef __UNIT_TEST__

#include <stdint.h>
#include <apic.h>

class PthreadCtrl : public ApicCtrlInterface {
public:
  PthreadCtrl() {}
  virtual void Setup() override {
  }
  virtual volatile uint8_t GetApicId() override {
    return 0;
  }
  virtual int GetHowManyCpus() override {
    return _cpu_nums;
  }
  virtual void SetupTimer(uint32_t irq) override {
  }
  virtual void StartTimer() override {
  }
  virtual void StopTimer() override {
  }

private:
  int _cpu_nums = 2;
};

#endif // __UNIT_TEST__

#endif /* __RAPH_KERNEL_THREAD_H__ */
