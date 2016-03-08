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

#ifndef __RAPH_KERNEL_TIMER_H__
#define __RAPH_KERNEL_TIMER_H__

#include <stdint.h>

class Timer {
 public:
  virtual bool Setup() = 0;
  virtual volatile uint32_t ReadMainCnt() = 0;
  // us秒後のカウントを取得する
  volatile uint32_t GetCntAfterPeriod(volatile uint32_t cur, uint32_t us) {
    return cur + (us * 1000) / _cnt_clk_period;
  }
  volatile bool IsTimePassed(volatile uint32_t time) {
    volatile uint32_t cur = ReadMainCnt();
    return cur >= time;
  }
  // ビジーループタイムアウト
  // 最適化回避のためにできるだけこの関数を使うべき
  void BusyUwait(uint32_t us) {
    volatile uint32_t cur = ReadMainCnt();
    volatile uint32_t end = GetCntAfterPeriod(cur, us);
    if (cur > end) {
      for (volatile uint32_t tmp = ReadMainCnt(); cur <= tmp; tmp = ReadMainCnt()) {
      }
    }
    while(true) {
      volatile bool flag = IsTimePassed(end);
      if (flag) {
        break;
      }
    }
  }
  int64_t GetUsecFromCnt(int32_t cnt) {
    return (static_cast<int64_t>(cnt) * _cnt_clk_period) / 1000;
  }
 protected:
  // １カウントが何ナノ秒か
  uint32_t _cnt_clk_period = 1;
};

#endif /* __RAPH_KERNEL_TIMER_H__ */
