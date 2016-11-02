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
 * Author: Levelfour
 * 
 */

#ifndef __RAPH_LIB_THREAD_H__
#define __RAPH_LIB_THREAD_H__

#ifndef __KERNEL__

#include <vector>
#include <memory>
#include <thread>
#include <stdint.h>
#include <cpu.h>

class PthreadCtrl : public CpuCtrlInterface {
public:
  PthreadCtrl() : _thread_pool(0) {}
  PthreadCtrl(int num_threads) : _cpu_nums(num_threads), _thread_pool(num_threads-1) {}
  ~PthreadCtrl();
  void Setup();
  virtual volatile int GetId() override;
  virtual int GetHowManyCpus() override {
    return _cpu_nums;
  }

private:
  static const uint8_t kMaxThreadsNumber = 128;

  int _cpu_nums = 1;

  typedef std::vector<std::unique_ptr<std::thread>> thread_pool_t;
  thread_pool_t _thread_pool;

  int _thread_ids[kMaxThreadsNumber];

  int GetThreadId();
};

#endif // !__KERNEL__

#endif /* __RAPH_LIB_THREAD_H__ */
