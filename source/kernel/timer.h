/*
 *
 * Copyright (c) 2017 Raphine Project
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

#pragma once

#include <stdint.h>

class Time {
public:
  Time() : _time(0) {
  }
  Time(uint64_t us) : _time(us) {
  }
  bool operator < (const Time &t1) const {
    if (_time < t1._time) {
      return true;
    } else if (t1._time < 0x1000000000000000 && _time > 0xF000000000000000) {
      // TODO is this OK?
      return true;
    } else {
      return false;
    }
  };
  bool operator > (const Time &t1) const {
    if (_time > t1._time) {
      return true;
    } else if (_time < 0x1000000000000000 && t1._time > 0xF000000000000000) {
      // TODO is this OK?
      return true;
    } else {
      return false;
    }
  };
  bool operator >= (const Time &t1) const {
    return !(*this < t1);
  }
  bool operator <= (const Time &t1) const {
    return !(*this > t1);
  }
  bool operator == (const Time &t1) const {
    return _time == t1._time;
  }
  bool operator != (const Time &t1) const {
    return !(*this == t1);
  }
  const Time operator + (const int us) const {
    Time t(_time + us);
    return t;
  }
  const Time operator - (const int us) const {
    Time t(_time - us);
    return t;
  }
  const int64_t operator - (const Time& t) const {
    return _time - t._time;
  }
  uint64_t GetRaw() {
    return _time;
  }
private:
  uint64_t _time;
};

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
  Time ReadTime() {
    return Time((ReadMainCnt() * _cnt_clk_period) / 1000);
  }
  void BusyUwait(int us) {
    Time t = ReadTime() + us;
    while(ReadTime() < t) {
      asm volatile("":::"memory");
    }
  }
protected:
  virtual volatile uint64_t ReadMainCnt() = 0;
  uint64_t ConvertTimeToCnt(Time t) {
    return (t.GetRaw() * 1000) / _cnt_clk_period;
  }
  // １カウントが何ナノ秒か
  uint64_t _cnt_clk_period = 1;
  virtual bool SetupSub() = 0;
private:
  bool _setup = false;
};

