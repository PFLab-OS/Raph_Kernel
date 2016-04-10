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
 * Author: levelfour
 * 
 */

#ifndef __RAPH_KERNEL_POSIX_H__
#define __RAPH_KERNEL_POSIX_H__

#include <stdint.h>
#include <sys/time.h> 
#include <timer.h>

class PosixTimer : public Timer {
public:
  virtual bool Setup() {
    _cnt_clk_period = 1;

    struct timeval tv;
    gettimeofday(&tv, nullptr);
    _boot_time_cnt = tv.tv_sec * 1e+06 + tv.tv_usec;

    return true;
  }
  virtual volatile uint64_t ReadMainCnt() {
    // return the difference between current time and time on booting
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    uint64_t cnt = tv.tv_sec * 1e+06 + tv.tv_usec;
    return cnt - _boot_time_cnt;
  }

private:
  uint64_t _boot_time_cnt = 0;
};

#endif /* __RAPH_KERNEL_POSIX_H__ */
