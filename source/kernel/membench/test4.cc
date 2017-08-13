/*
 *
 * Copyright (c) 2015 Raphine Project
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
#include <mem/physmem.h>

#include "sync.h"
#include "spinlock.h"
#include "list.h"

static bool is_knl() {
  return x86::get_display_family_model() == 0x0657;
}

void udpsend(int argc, const char *argv[]);

static SyncLow sync_1={0};
static SyncLow sync_4={0};

template<int mode>
static uint64_t func2(int apicid_) {
  uint64_t time = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static const int kCnt = 30000;
  bool eflag;
  if (apicid == apicid_) {
    eflag = true;
  } else {
    if (apic_ctrl->GetCpuIdFromApicId(apicid_) == -1) {
      return 0;
    }
    eflag = false;
  }
  

  sync_1.Do();
  if (eflag) {
    int *ptr = new int;
    *ptr = 0;
    uint64_t t1;
    t1 = timer->ReadMainCnt();

    for(int i = 0; i < kCnt; i++) {
      switch (mode) {
      case 1:
        __sync_lock_test_and_set(ptr, i);
        break;
      case 2:
        __sync_bool_compare_and_swap(ptr, i, i + 1);
        break;
      case 3:
        __sync_fetch_and_add(ptr, 1);
        break;
      case 4:
        __sync_fetch_and_add(ptr, 0);
        break;
      case 5:
        __sync_fetch_and_or(ptr, 0);
        break;
      case 6:
        __sync_add_and_fetch(ptr, 1);
        break;
      case 7:
        __sync_add_and_fetch(ptr, 0);
        break;
      case 8:
        __sync_or_and_fetch(ptr, 0);
        break;
      };
    }

    time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
    delete ptr;
    for (int j = 0; j < cpu_ctrl->GetHowManyCpus(); j++) {
      CpuId cpuid_(j);
      apic_ctrl->SendIpi(cpuid_.GetApicId());
    }
  } else {
    asm volatile("hlt");
  }
  
  sync_4.Do();
  return time;
}

template<int mode>
static void func1_sub(int apicid_) {
  static const int num = 20;
  uint64_t results[num];
  func2<mode>(apicid_);
  for (int j = 0; j < num; j++) {
    results[j] = func2<mode>(apicid_);
  }
  uint64_t avg = 0;
  for (int j = 0; j < num; j++) {
    avg += results[j];
  }
  avg /= num;

  uint64_t variance = 0;
  for (int j = 0; j < num; j++) {
    variance += (results[j] - avg) * (results[j] - avg);
  }
  variance /= num;
  int apicid = cpu_ctrl->GetCpuId().GetApicId();
  if (apicid == apicid_) {
    gtty->Printf("<%d %lld(%lld) us> ", mode, avg, variance);
    StringTty tty(200);
    tty.Printf("%d\t%d\t%d\n", apicid, avg, variance);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

static SyncLow sync_5={0};

template<int mode>
static void func1(sptr<TaskWithStack> task) {
  static int flag = 0;
  __sync_fetch_and_add(&flag, 1);

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function2<sptr<Task>, sptr<TaskWithStack>>([](sptr<Task> ltask, sptr<TaskWithStack> task_) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask);
          } else {
            if (is_knl()) {
              int apicid = cpu_ctrl->GetCpuId().GetApicId();
              if (apicid == 0) {
                StringTty tty(20);
                tty.Printf("# mode %d\n", mode);
                int argc = 4;
                const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
                udpsend(argc, argv);
              }
              for (int apicid_ = 0; apicid_ <= 37 * 8; apicid_++) {
                func1_sub<mode>(apicid_);
              }
              if (apicid == 0) {
                StringTty tty(4);
                tty.Printf("\n");
                int argc = 4;
                const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
                udpsend(argc, argv);
                flag = 0;
              }
            }
            sync_5.Do();
            task_->Execute();
          }
        }, ltask_, task)));
  task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask_);

  task->Wait(1);
} 

// メモリ間通信
void membench4(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->Printf("start >>>\n");
  }
  if (cpuid == 0) {
    StringTty tty(20);
    tty.Printf("# mem latency\n");
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  // func1<1>(task); 
  // func1<2>(task); 
  // func1<3>(task); 
  // func1<4>(task); 
  func1<5>(task); 
  func1<6>(task); 
  func1<7>(task); 
  func1<8>(task); 
  if (cpuid == 0) {
    gtty->Printf("<<< end\n");
  }
}

