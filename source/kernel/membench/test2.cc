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
static Pair func107(bool mode, int cpunum, int i) {
  uint64_t time = 0;
  uint64_t f_variance = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static L *lock;
  static const int kInitCnt = 30000;
  static volatile int cnt = kInitCnt;
  static uint64_t f_array[256];
  int cpunum_ = 0;
  bool eflag;
  if (mode) {
    kassert(cpunum <= 32);
    if (apicid % 8 != 0) {
      cpunum_ = -1;
    } else {
      for (int apicid_ = 0; apicid_ <= apicid; apicid_+=8) {
        if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
          cpunum_++;
        }
      }
    }
  } else {
    for (int apicid_ = 0; apicid_ <= apicid; apicid_++) {
      if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
        cpunum_++;
      }
    }
  }
  if (cpunum_ < 0) {
    eflag = false;
  } else {
    eflag = cpunum_ <= cpunum;
  }
  

  {
    if (apicid == 0) {
      lock = new L;
      for (int x = 0; x < 256; x++) {
        f_array[x] = 0;
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
      if (cnt > 0) {
        cnt--;
        f_array[cpunum_ - 1]++;
      } else {
        flag = false;
      }
      lock->Unlock(apicid);
      if (!flag) {
        break;
      }
    }
    
    sync_3.Do(cpunum);

    if (apicid == 0) {
      time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
      for (int j = 1; j < cpu_ctrl->GetHowManyCpus(); j++) {
        CpuId cpuid_(j);
        apic_ctrl->SendIpi(cpuid_.GetApicId());
      }
      uint64_t f_avg = 0;
      for (int x = 0; x < cpunum; x++) {
        f_avg += f_array[x];
      }
      f_avg /= cpunum;
      for (int x = 0; x < cpunum; x++) {
        f_variance += (f_array[x] - f_avg) * (f_array[x] - f_avg);
      }
      f_variance /= cpunum;
      delete lock;
      cnt = kInitCnt;
    }
  } else {
    asm volatile("hlt");
  }
  
  sync_4.Do();
  Pair pair = { time: time, fairness: f_variance };
  return pair;
}

template<class L>
static void func107_sub(bool mode, int cpunum, int i) {
  static const int num = 20;
  Pair results[num];
  func107<L>(mode, cpunum, i);
  for (int j = 0; j < num; j++) {
    results[j] = func107<L>(mode, cpunum, i);
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
    gtty->CprintfRaw("<%d %d %lld(%lld) us> ", i, cpunum, avg, variance);
    StringTty tty(200);
    tty.CprintfRaw("%d\t%d\t%d\t%d\n", i, cpunum, avg.time, avg.fairness);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

static SyncLow sync_5={0};

template<bool mode, int i, class L>
static void func107(sptr<TaskWithStack> task) {
  static int flag = 0;
  __sync_fetch_and_add(&flag, 1);

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function2<sptr<Task>, sptr<TaskWithStack>>([](sptr<Task> ltask, sptr<TaskWithStack> task_) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask);
          } else {
            if (mode) {
              if (is_knl()) {
                for (int cpunum = 1; cpunum <= 32; cpunum++) {
                  func107_sub<L>(mode, cpunum, i);
                }
              }
            } else {
              if (is_knl()) {
                for (int cpunum = 1; cpunum <= 8; cpunum++) {
                  func107_sub<L>(mode, cpunum, i);
                }
              } else {
                for (int cpunum = 1; cpunum <= 8; cpunum++) {
                  func107_sub<L>(mode, cpunum, i);
                }
              }
            }
            int apicid = cpu_ctrl->GetCpuId().GetApicId();
            if (apicid == 0) {
              StringTty tty(4);
              tty.CprintfRaw("\n");
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

  task->Wait();
} 

template<bool mode, int i>
static void func106(sptr<TaskWithStack> task) {
}

template<bool mode, int i, class L, class... Locks>
static void func106(sptr<TaskWithStack> task) {
  func107<mode, i, L>(task);
  func106<mode, i, Locks...>(task);
}

// 同期だけのコストの計測
template<bool mode, int i>
static void func10(sptr<TaskWithStack> task) {
  func106<mode, i,
          McsSpinLock,
          TtsSpinLock,
          TicketSpinLock,
          AndersonSpinLock<1, 256>,
          ClhSpinLock,
          AndersonSpinLock<16, 256>,
          SimpleSpinLockR,
          ExpSpinLock10<TtsSpinLock, ClhSpinLock>,
          ExpSpinLock10<ClhSpinLock, AndersonSpinLock<16, 8>>,
          ExpSpinLock10<ClhSpinLock, ClhSpinLock>,
          ExpSpinLock10<ClhSpinLock, McsSpinLock>,
          ExpSpinLock10<AndersonSpinLock<16, 32>, AndersonSpinLock<16, 8>>,
          ExpSpinLock10<AndersonSpinLock<16, 32>, ClhSpinLock>,
          ExpSpinLock10<AndersonSpinLock<16, 32>, McsSpinLock>,
          ExpSpinLock10<McsSpinLock, AndersonSpinLock<16, 8>>,
          ExpSpinLock10<McsSpinLock, ClhSpinLock>,
          ExpSpinLock10<McsSpinLock, McsSpinLock>,
          AndersonSpinLock<16, 8>
          >(task);
}

template<bool mode, int i, int j, int... Num>
static void func10(sptr<TaskWithStack> task) {
  func10<mode, i>(task);
  func10<mode, j, Num...>(task);
}

// リスト、要素数可変比較版
void membench2(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>\n");
  }
  if (cpuid == 0) {
    StringTty tty(10);
    tty.CprintfRaw("# all\n");
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  func10<false, 1>(task); 
  if (cpuid == 0) {
    StringTty tty(10);
    tty.CprintfRaw("# tile\n");
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  // func10<true, 1>(task); 
  if (cpuid == 0) {
    gtty->CprintfRaw("<<< end\n");
  }
}

