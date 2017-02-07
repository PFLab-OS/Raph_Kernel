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
  static const uint64_t kMax = 3000;
  static volatile uint64_t cnt = 0;
  static int *buf1;
  int *buf2 = new int[i];
  int *buf3 = new int[i];
  int *buf4 = new int[i];
  static uint64_t f_array[256];
  static uint64_t v_array[256];
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
      lock = new L;
      buf1 = new int[i * kMax];
      for (int x = 0; x < i * cnt; x++) {
        buf1[x] = rand() % 256;
      }
      for (int x = 0; x < 256; x++) {
        f_array[x] = 0;
        v_array[x] = 0;
      }
    }
  }

  sync_1.Do();
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
      for (int x = 0; x < i; x++) {
        int tmp = 1;
        for (int y = 0; y < i; y++) {
          tmp *= buf2[y];
        }
        buf4[x] += tmp + buf3[x];
      }
      if (!flag) {
        break;
      }
    }

    // lock->Release(apicid);
    
    sync_3.Do(cpunum);

    if (apicid == 0) {
      time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
      for (int j = 1; j < cpu_ctrl->GetHowManyCpus(); j++) {
        CpuId cpuid_(j);
        apic_ctrl->SendIpi(cpuid_.GetApicId());
      }
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
      delete lock;
      delete[] buf1;
      cnt = 0;
    }
  } else {
    asm volatile("hlt");
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
    tty.CprintfRaw("%d\t%d\t%d\t%d\n", i, cpunum, avg.time, avg.fairness);
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
              for (int cpunum = 1; cpunum <= 256; cpunum++) {
                func107_sub<L>(cpunum, i);
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

template<int i>
static void func10(sptr<TaskWithStack> task) {
  func106<i,
          McsSpinLock,
          TtsSpinLock,
          TicketSpinLock,
          AndersonSpinLock<1, 256>,
          ClhSpinLock,
          AndersonSpinLock<16, 256>,
          SimpleSpinLockR,
          ExpSpinLock10<TtsSpinLock, ClhSpinLock>,
          ExpSpinLock10<ClhSpinLock, ClhSpinLock>,
          ExpSpinLock10<ClhSpinLock, AndersonSpinLock<16, 8>>,
          ExpSpinLock10<ClhSpinLock, McsSpinLock>,
          ExpSpinLock10<AndersonSpinLock<16, 32>, AndersonSpinLock<16, 8>>,
          ExpSpinLock10<AndersonSpinLock<16, 32>, ClhSpinLock>,
          ExpSpinLock10<AndersonSpinLock<16, 32>, McsSpinLock>,
          ExpSpinLock10<McsSpinLock, ClhSpinLock>,
          ExpSpinLock10<McsSpinLock, AndersonSpinLock<16, 8>>,
          ExpSpinLock10<McsSpinLock, McsSpinLock>
          >(task);
}

template<int i, int j, int... Num>
static void func10(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    StringTty tty(10);
    tty.CprintfRaw("# %d\n", i);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  func10<i>(task);
  func10<j, Num...>(task);
}

// リスト、要素数可変比較版
void membench3(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>\n");
  }
  func10<10, 20, 40, 80>(task); 
}

