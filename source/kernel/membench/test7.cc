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

template<class L>
static __attribute__ ((noinline)) Pair func107_main(int cpunum) {
  uint64_t time = 0;
  uint64_t f_variance = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static L *lock;
  static const int kInitCnt = 100000;
  static volatile int __attribute__ ((aligned (64))) cnt = kInitCnt;
  PhysAddr paddr;
  int cpunum_ = 0;
  for (int apicid_ = 0; apicid_ <= apicid; apicid_++) {
    if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
      cpunum_++;
    }
  }  
  bool eflag = cpunum_ <= cpunum;

  {
    if (apicid == 0) {
      check_align(&cnt);
      physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(L)));
      lock = reinterpret_cast<L *>(paddr.GetVirtAddr());
      new(lock) L;
      for (int x = 0; x < 256; x++) {
        f_array[x].i = 0;
      }
    } else {
      monitor[apicid].i = 0;
    }
  }

  sync_1.Do();
  cache_ctrl->Clear(lock, sizeof(L));
  cache_ctrl->Clear(f_array, sizeof(Uint64) * 256);
  
  uint64_t t1;
  if (apicid == 0) {
    t1 = timer->ReadMainCnt();
  }

  if (eflag) {
    sync_2.Do(cpunum);

    while(true) {
      bool flag = true;
      lock->Lock(apicid);
      asm volatile("":::"memory");
      if (cnt > 0) {
        cnt--;
        f_array[cpunum_ - 1].i++;
      } else {
        flag = false;
      }
      lock->Unlock(apicid);
      if (!flag) {
        break;
      }
    }
    
    sync_3.Do(cpunum);
  }

  if (apicid == 0) {
    time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
    uint64_t f_avg = 0;
    for (int x = 0; x < cpunum; x++) {
      f_avg += f_array[x].i;
    }
    if (f_avg != kInitCnt) {
      kernel_panic("membench", "bad spinlock");
    }
    f_avg /= cpunum;
    for (int x = 0; x < cpunum; x++) {
      f_variance += (f_array[x].i - f_avg) * (f_array[x].i - f_avg);
    }
    f_variance /= cpunum;
    physmem_ctrl->Free(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(L)));
    cnt = kInitCnt;
    for (int j = 1; j < 37 * 8; j++) {
      if (apic_ctrl->GetCpuIdFromApicId(j) == -1) {
        continue;
      }
      uint64_t tmp = monitor[j].i;
      while(tmp == 0 || tmp == 1) {
        __sync_bool_compare_and_swap(&monitor[j].i, tmp, 1);
        tmp = monitor[j].i;
        asm volatile("":::"memory");
      }
    }
  } else {
    volatile uint64_t *monitor_addr = &monitor[apicid].i;
    while(true) {
      asm volatile("monitor;"::"a"(monitor_addr), "c"(0), "d"(0));
      asm volatile("mwait;"::"a"(0), "c"(0));
      asm volatile("":::"memory");
      if (*monitor_addr == 1) {
        __sync_lock_test_and_set(monitor_addr, 2);
        break;
      }
    }
  }
  
  sync_4.Do();
  Pair pair = { time: time, fairness: f_variance };
  return pair;
}

template<class L>
static __attribute__ ((noinline)) void func107_sub2(int cpunum) {
  int apicid = cpu_ctrl->GetCpuId().GetApicId();

  static const int num = 10;
  Pair results[num];
  func107_main<L>(cpunum);
  for (int j = 0; j < num; j++) {
    results[j] = func107_main<L>(cpunum);
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
  if (apicid == 0) {
    //    gtty->Printf("<%d %lld(%lld) us> ", avg, variance);
    //gtty->Flush();
    StringTty tty(200);
    tty.Printf("%d\t%d\t%d\t%d\n", cpunum, avg.time, avg.fairness, variance);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

static SyncLow sync_5={0};
static SyncLow sync_6={0};

template<class L>
static void func107_sub(sptr<TaskWithStack> task) {
  static volatile int flag = 0;
  __sync_fetch_and_add(&flag, 1);

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function2<sptr<Task>, sptr<TaskWithStack>>([](sptr<Task> ltask, sptr<TaskWithStack> task_) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask);
          } else {
            sync_5.Do();
            for (int cpunum = 1; cpunum <= 7; cpunum++) {
              func107_sub2<L>(cpunum);
            }
            if (is_knl()) {
              for (int cpunum = 8; cpunum <= 256; cpunum*=2) {
                func107_sub2<L>(cpunum);
              }
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
            sync_6.Do();
            task_->Execute();
          }
        }, ltask_, task)));
  task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask_);

  task->Wait(1);
}

template<class L>
static void func107(sptr<TaskWithStack> task, const char *name) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    StringTty tty(100);
    tty.Printf("# %s\n", name);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  
  func107_sub<L>(task);
}

#define FUNC(task, ...) func107<__VA_ARGS__>(task, #__VA_ARGS__);

// 最小マイクロベンチマーク
void membench7(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->Printf("start >>>\n");
    gtty->Flush();
  }
  if (cpuid == 0) {
    StringTty tty(100);
    tty.Printf("# count(%s)\n", __func__);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  FUNC(task, SimpleSpinLock);
  FUNC(task, TtsSpinLock);
  FUNC(task, TtsBackoffSpinLock);
  FUNC(task, TicketSpinLock);
  FUNC(task, AndersonSpinLock<1, 256>);
  FUNC(task, AndersonSpinLock<64, 256>);
  FUNC(task, ClhSpinLock);
  FUNC(task, McsSpinLock<64>);
  FUNC(task, HClhSpinLock);
  FUNC(task, ExpSpinLock10<ClhSpinLock, ClhSpinLock, 8>);
  FUNC(task, ExpSpinLock10<ClhSpinLock, TicketSpinLock, 8>);
  FUNC(task, ExpSpinLock10<ClhSpinLock, AndersonSpinLock<64, 8>, 8>);
  FUNC(task, ExpSpinLock10<ClhSpinLock, McsSpinLock<64>, 8>);
  FUNC(task, ExpSpinLock10<AndersonSpinLock<64, 64>, AndersonSpinLock<64, 8>, 8>); 
  FUNC(task, ExpSpinLock10<AndersonSpinLock<64, 64>, TicketSpinLock, 8>);
  FUNC(task, ExpSpinLock10<AndersonSpinLock<64, 64>, McsSpinLock<64>, 8>);
  FUNC(task, ExpSpinLock10<AndersonSpinLock<64, 64>, ClhSpinLock, 8>);
  FUNC(task, ExpSpinLock10<McsSpinLock<64>, McsSpinLock<64>, 8>);
  FUNC(task, ExpSpinLock10<McsSpinLock<64>, TicketSpinLock, 8>);
  FUNC(task, ExpSpinLock10<McsSpinLock<64>, ClhSpinLock, 8>);
  FUNC(task, ExpSpinLock10<McsSpinLock<64>, AndersonSpinLock<64, 8>, 8>);
  if (cpuid == 0) {
    gtty->Printf("<<< end\n");
    gtty->Flush();
  }
}

