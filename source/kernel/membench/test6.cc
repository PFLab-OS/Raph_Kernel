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

#include <x86.h>
#include <stdlib.h>
#include <raph.h>
#include <cpu.h>
#include <measure.h>
#include <tty.h>
#include <global.h>
#include <mem/paging.h>
#include <mem/physmem.h>

#include "sync.h"
#include "spinlock.h"
#include "cache.h"
#include "membench.h"

static bool is_knl() {
  return x86::get_display_family_model() == 0x0657;
}

void udpsend(int argc, const char *argv[]);

struct Pair {
  uint64_t time;
  uint64_t fairness;
};

int memory[128];

static Pair func2(int cpuid) {
  uint64_t time = 0;
  volatile int myid = cpu_ctrl->GetCpuId().GetRawId();
  
  static uint64_t monitor[37 * 8];

  monitor[myid] = 0;

  sync_1.Do();

  if (cpuid == myid) {

    uint64_t t1;
    t1 = timer->ReadMainCnt();

    for (int i = 0; i < 100000; i++) {
      __sync_lock_test_and_set(&memory[64], i);
    }
    
    time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
    for (int j = 0; j < cpu_ctrl->GetHowManyCpus(); j++) {
      if (j == myid) {
        continue;
      }
      uint64_t tmp = monitor[j];
      while(tmp == 0 || tmp == 1) {
        __sync_bool_compare_and_swap(&monitor[j], tmp, 1);
        tmp = monitor[j];
      }
    }
  } else {
    uint64_t *monitor_addr = &monitor[myid];
    while(true) {
      asm volatile("monitor;"::"a"(monitor_addr), "c"(0), "d"(0));
      asm volatile("mwait;"::"a"(0), "c"(0));
      if (*monitor_addr == 1) {
        __sync_lock_test_and_set(monitor_addr, 2);
        break;
      }
    }
  }
  
  sync_4.Do();
  Pair pair = { time: time, fairness: 0 };
  return pair;
}

static void func2_sub(int cpuid) {
  int apicid = cpu_ctrl->GetCpuId().GetApicId();

  static const int num = 20;
  Pair results[num];
  func2(cpuid);
  for (int j = 0; j < num; j++) {
    results[j] = func2(cpuid);
  }
  Pair avg = { time: 0, fairness: 0 };
  for (int j = 0; j < num; j++) {
    avg.time += results[j].time;
    avg.fairness += results[j].fairness;
  }
  avg.time /= num;
  avg.fairness /= num;

  uint64_t variance = 0;
  for (int j = 0; j < num; j++) {
    variance += (results[j].time - avg.time) * (results[j].time - avg.time);
  }
  variance /= num;
  if (cpuid == cpu_ctrl->GetCpuId().GetRawId()) {
    gtty->Printf("<%d %lld(%lld) us> ", cpuid, avg, variance);
    StringTty tty(200);
    tty.Printf("%d\t%d\t%d\t%d\n", cpuid, avg.time, avg.fairness, variance);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

static SyncLow sync_5={0};

static void func(sptr<TaskWithStack> task) {
  static int flag = 0;
  __sync_fetch_and_add(&flag, 1);

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function2<sptr<Task>, sptr<TaskWithStack>>([](sptr<Task> ltask, sptr<TaskWithStack> task_) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask);
          } else {
            for (int i = 0; i < cpu_ctrl->GetHowManyCpus(); i++) {
              func2_sub(i);
            }
            int apicid = cpu_ctrl->GetCpuId().GetApicId();
            if (apicid == 0) {
              StringTty tty(4);
              tty.Printf("\n");
              int argc = 4;
              const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
              udpsend(argc, argv);
              flag = 0;
            }
            sync_5.Do();
            task_->Execute();
          }
        }, ltask_, task)));
  task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask_);

  task->Wait(1);
} 

// メインメモリ遅延測定
void membench6(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->Printf("start >>>\n");
  }
  if (cpuid == 0) {
    StringTty tty(10);
    tty.Printf("# count\n");
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  func(task);
  if (cpuid == 0) {
    gtty->Printf("<<< end\n");
  }
}

