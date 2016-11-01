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

#ifndef __RAPH_KERNEL_TIMER_H__
#define __RAPH_KERNEL_TIMER_H__

#include <stdint.h>

class Timer {
public:
  bool Setup() {
    bool b = SetupSub();
    _setup = true;
    return b;
  }
  bool DidSetup() {
    return _setup;
  }
  virtual volatile uint64_t ReadMainCnt() = 0;
  // us秒後のカウントを取得する
  volatile uint64_t GetCntAfterPeriod(volatile uint64_t cur, int us) {
    return cur + (static_cast<int64_t>(us) * 1000) / _cnt_clk_period;
  }
  volatile bool IsGreater(uint64_t n1, uint64_t n2) {
    if (n1 >= n2) {
      return true;
    } else if (n1 < 0x1000000000000000 && n2 > 0xF000000000000000) {
      //TODO is this ok?
      return true;
    } else {
      return false;
    }
  }
  volatile bool IsTimePassed(volatile uint64_t time) {
    volatile uint64_t cur = ReadMainCnt();
    return IsGreater(cur, time);
  }
  // ビジーループタイムアウト
  // 最適化回避のためにできるだけこの関数を使うべき
  void BusyUwait(int us) {
    volatile uint64_t cur = ReadMainCnt();
    volatile uint64_t end = GetCntAfterPeriod(cur, us);
    while(true) {
      volatile bool flag = IsTimePassed(end);
      if (flag) {
        break;
      }
    }
  }
  uint64_t GetCntClkPeriod() {
    return _cnt_clk_period;
  }
  uint64_t GetUsecFromCnt(uint64_t cnt) {
    return (cnt * _cnt_clk_period) / 1000;
  }
protected:
  // １カウントが何ナノ秒か
  uint64_t _cnt_clk_period = 1;
  virtual bool SetupSub() = 0;
private:
  bool _setup = false;
};

#endif /* __RAPH_KERNEL_TIMER_H__ */
