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

#include <cpu.h>

static inline void sync(int cpunum, volatile int &l1, volatile int &l2, volatile int &l3) {
  l2 = 0;
  __sync_fetch_and_add(&l1, 1);
  while(l1 != cpunum) {
  }
  l3 = 0;
  __sync_fetch_and_add(&l2, 1);
  while(l2 != cpunum) {
  }
  l1 = 0;
  __sync_fetch_and_add(&l3, 1);
  while(l3 != cpunum) {
  }
}


struct SyncLow {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  void Do() {
    int cpunum = cpu_ctrl->GetHowManyCpus();
    sync(cpunum, top_level_lock1, top_level_lock2, top_level_lock3);
  }
};

struct Sync2Low {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  void Do(int tnum) {
    sync(tnum, top_level_lock1, top_level_lock2, top_level_lock3);
  }
};
