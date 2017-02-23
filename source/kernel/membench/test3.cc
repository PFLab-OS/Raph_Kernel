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
#include <mem/paging.h>

#include "sync.h"
#include "spinlock.h"
#include "cache.h"

static bool is_knl() {
  return x86::get_display_family_model() == 0x0657;
}

void udpsend(int argc, const char *argv[]);

struct Pair {
  uint64_t time;
  uint64_t fairness;
};

static SyncLow sync_1={0};
static Sync2Low sync_2={0};
static Sync2Low sync_3={0};
static SyncLow sync_4={0};

template<class L>
static Pair func107(int cpunum, int i) {
  uint64_t time = 0;
  uint64_t f_variance = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static L *lock;
  static const uint64_t kMax = 9000;
  static volatile uint64_t cnt = 0;
  static int *buf1;
  int *buf2 = new int[i];
  int *buf3 = new int[i];
  int *buf4 = new int[i];
  static uint64_t f_array[256];
  static uint64_t monitor[37 * 8];
  static uint64_t v_array[256];
  PhysAddr paddr;  
  int cpunum_ = 0;
  bool eflag;
  for (int apicid_ = 0; apicid_ <= apicid; apicid_++) {
    if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
      cpunum_++;
    }
  }
  eflag = cpunum_ <= cpunum;
  

  {
    if (apicid == 0) {
      physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(L)));
      lock = reinterpret_cast<L *>(paddr.GetVirtAddr());
      new(lock) L;
      buf1 = new int[i * kMax];
      for (int x = 0; x < i * cnt; x++) {
        buf1[x] = rand() % 256;
      }
      for (int x = 0; x < 256; x++) {
        f_array[x] = 0;
        v_array[x] = 0;
      }
    } else {
      monitor[apicid] = 0;
    }
  }

  sync_1.Do();
  cache_ctrl->Clear(lock, sizeof(L));
  cache_ctrl->Clear(buf1, sizeof(int) * i * kMax);
  cache_ctrl->Clear(buf2, sizeof(int) * i);
  cache_ctrl->Clear(buf3, sizeof(int) * i);
  cache_ctrl->Clear(buf4, sizeof(int) * i);
  if (eflag) {
    sync_2.Do(cpunum);

    uint64_t t1;
    if (apicid == 0) {
      t1 = timer->ReadMainCnt();
    }

    while(true) {
      bool flag = true;
      lock->Lock(apicid);
      if (cnt < kMax) {
        f_array[cpunum_ - 1]++;
        v_array[cpunum_ - 1]++;
        memcpy(buf2, buf1 + i * cnt, i);
        cnt++;
      } else {
        flag = false;
      }
      lock->Unlock(apicid);
      int iter = i;
      for (int x = 0; x < iter; x++) {
        int tmp = 1;
        for (int y = 0; y < iter; y++) {
          tmp *= buf2[y];
        }
        buf4[x] += tmp + buf3[x];
      }
      if (!flag) {
        break;
      }
    }

    sync_3.Do(cpunum);

    if (apicid == 0) {
      time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
      uint64_t f_avg = 0;
      uint64_t varidation = 0;
      for (int x = 0; x < cpunum; x++) {
        f_avg += f_array[x];
        varidation += v_array[x];
      }
      // assert(varidation == kMax);
      if (varidation != kMax) {
        gtty->CprintfRaw("[[[%d]]]", varidation);
        assert(false);
      }
      f_avg /= cpunum;
      for (int x = 0; x < cpunum; x++) {
        f_variance += (f_array[x] - f_avg) * (f_array[x] - f_avg);
      }
      f_variance /= cpunum;
      physmem_ctrl->Free(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(L)));
      delete[] buf1;
      cnt = 0;
      for (int j = 1; j < 37 * 8; j++) {
        if (apic_ctrl->GetCpuIdFromApicId(j) == -1) {
          continue;
        }
        uint64_t tmp = monitor[j];
        while(tmp == 0 || tmp == 1) {
          __sync_bool_compare_and_swap(&monitor[j], tmp, 1);
          tmp = monitor[j];
        }
      }
    }
  }
  if (apicid != 0) {
    uint64_t *monitor_addr = &monitor[apicid];
    while(true) {
      asm volatile("monitor;"::"a"(monitor_addr), "c"(0), "d"(0));
      asm volatile("mwait;"::"a"(0), "c"(0));
      if (*monitor_addr == 1) {
        __sync_lock_test_and_set(monitor_addr, 2);
        break;
      }
    }
  }
  delete[] buf2;
  delete[] buf3;
  delete[] buf4;
  
  sync_4.Do();
  Pair pair = { time: time, fairness: f_variance };
  return pair;
}

template<class L>
static void func107_sub(int cpunum, int i) {
  static const int num = 20;
  Pair results[num];
  func107<L>(cpunum, i);
  for (int j = 0; j < num; j++) {
    results[j] = func107<L>(cpunum, i);
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
  int apicid = cpu_ctrl->GetCpuId().GetApicId();
  if (apicid == 0) {
    gtty->CprintfRaw("<%d %lld(%lld) us> ", i, avg, variance);
    StringTty tty(200);
    tty.CprintfRaw("%d\t%d\t%d\t%d\t%d\n", i, cpunum, avg.time, avg.fairness, variance);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

template<int i, class L>
static void func107(sptr<TaskWithStack> task) {
  static int flag = 0;
  int tmp = flag;
  while(!__sync_bool_compare_and_swap(&flag, tmp, tmp + 1)) {
    tmp = flag;
  }

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function2<sptr<Task>, sptr<TaskWithStack>>([](sptr<Task> ltask, sptr<TaskWithStack> task_) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask);
          } else {
            if (is_knl()) {
              if (kFastMeasurement) {
                for (int cpunum = 1; cpunum <= 7; cpunum++) {
                  func107_sub<L>(cpunum, i);
                }
                for (int cpunum = 8; cpunum <= 256; cpunum*=2) {
                  func107_sub<L>(cpunum, i);
                }
              } else {
                for (int cpunum = 1; cpunum <= 256; cpunum++) {
                  func107_sub<L>(cpunum, i);
                }
              }
            } else {
              for (int cpunum = 1; cpunum <= 8; cpunum++) {
                func107_sub<L>(cpunum, i);
              }
            }
            int apicid = cpu_ctrl->GetCpuId().GetApicId();
            if (apicid == 0) {
              StringTty tty(4);
              tty.CprintfRaw("\n");
              int argc = 4;
              const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
              udpsend(argc, argv);
            }
            task_->Execute();
          }
        }, ltask_, task)));
  task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask_);

  task->Wait();
} 

template<int i>
static void func106(sptr<TaskWithStack> task) {
}

template<int i, class L, class... Locks>
static void func106(sptr<TaskWithStack> task) {
  func107<i, L>(task);
  func106<i, Locks...>(task);
}

template<int kMax, int i>
static void func10(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    StringTty tty(10);
    tty.CprintfRaw("# %d\n", i);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  func106<i, McsSpinLock<64>>(task);
  func106<i, TtsSpinLock>(task);
  func106<i, TicketSpinLock>(task);
  func106<i, AndersonSpinLock<1, 256>>(task);
  func106<i, ClhSpinLock>(task);
  func106<i, AndersonSpinLock<64, 256>>(task);
  func106<i, SimpleSpinLockR>(task);
  func106<i, HClhSpinLock>(task);
  func106<i, ExpSpinLock10<TtsSpinLock, ClhSpinLock, kMax>>(task);
  func106<i, ExpSpinLock10<ClhSpinLock, AndersonSpinLock<64, 256>, kMax>>(task);
  func106<i, ExpSpinLock10<ClhSpinLock, ClhSpinLock, kMax>>(task);
  func106<i, ExpSpinLock10<ClhSpinLock, McsSpinLock<64>, kMax>>(task);
  func106<i, ExpSpinLock10<ClhSpinLock, TicketSpinLock, kMax>>(task);
  func106<i, ExpSpinLock10<AndersonSpinLock<64, 256>, AndersonSpinLock<64, 256>, kMax>>(task);
  func106<i, ExpSpinLock10<AndersonSpinLock<64, 256>, ClhSpinLock, kMax>>(task);
  func106<i, ExpSpinLock10<AndersonSpinLock<64, 256>, McsSpinLock<64>, kMax>>(task);
  func106<i, ExpSpinLock10<AndersonSpinLock<64, 256>, TicketSpinLock, kMax>>(task);
  func106<i, ExpSpinLock10<McsSpinLock<64>, AndersonSpinLock<64, 256>, kMax>>(task);
  func106<i, ExpSpinLock10<McsSpinLock<64>, ClhSpinLock, kMax>>(task);
  func106<i, ExpSpinLock10<McsSpinLock<64>, McsSpinLock<64>, kMax>>(task);
  func106<i, ExpSpinLock10<McsSpinLock<64>, TicketSpinLock, kMax>>(task);
}

template<int kMax, int i, int j, int... Num>
static void func10(sptr<TaskWithStack> task) {
  func10<kMax, i>(task);
  func10<kMax, j, Num...>(task);
}

void membench3(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>\n");
  }
  func10<100, 10, 20, 40, 80>(task); 
  if (cpuid == 0) {
    gtty->CprintfRaw("<<< end\n");
  }
}

