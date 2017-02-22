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
#include <mem/paging.h>
#include <mem/physmem.h>

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
static SyncLow sync_2={0};
static SyncLow sync_3={0};
static SyncLow sync_4={0};

template<class L>
static Pair func107() {
  int cpunum = 256;
  uint64_t time = 0;
  uint64_t f_variance = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static L *lock;
  static const int kInitCnt = 90000;
  static volatile int cnt = kInitCnt;
  static uint64_t f_array[256];
  PhysAddr paddr;
  int cpunum_ = 0;
  for (int apicid_ = 0; apicid_ <= apicid; apicid_++) {
    if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
      cpunum_++;
    }
  }  

  {
    if (apicid == 0) {
      physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(L)));
      lock = reinterpret_cast<L *>(paddr.GetVirtAddr());
      new(lock) L;
      for (int x = 0; x < 256; x++) {
        f_array[x] = 0;
      }
    }
  }

  sync_1.Do();
  cache_ctrl->Clear(lock, sizeof(L));
  cache_ctrl->Clear(f_array, sizeof(uint64_t) * 256);
  sync_2.Do();

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
    
  sync_3.Do();

  if (apicid == 0) {
    time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
    uint64_t f_avg = 0;
    for (int x = 0; x < cpunum; x++) {
      f_avg += f_array[x];
    }
    f_avg /= cpunum;
    for (int x = 0; x < cpunum; x++) {
      f_variance += (f_array[x] - f_avg) * (f_array[x] - f_avg);
    }
    f_variance /= cpunum;
    physmem_ctrl->Free(paddr, PagingCtrl::ConvertNumToPageSize(sizeof(L)));
    cnt = kInitCnt;
  }
  
  sync_4.Do();
  Pair pair = { time: time, fairness: f_variance };
  return pair;
}

template<class L>
static void func107_sub(int kMax) {
  int apicid = cpu_ctrl->GetCpuId().GetApicId();

  static const int num = 20;
  Pair results[num];
  func107<L>();
  for (int j = 0; j < num; j++) {
    results[j] = func107<L>();
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
    gtty->CprintfRaw("<%d %lld(%lld) us> ", kMax, avg, variance);
    StringTty tty(200);
    tty.CprintfRaw("%d\t%d\t%d\t%d\n", kMax, avg.time, avg.fairness, variance);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

template<class L1, class L2, int i>
static void func107_sub2() {
  func107_sub<ExpSpinLock10<L1, L2, i>>(i);
}

template<class L1, class L2, int i, int j, int... Num>
static void func107_sub2() {
  func107_sub2<L1, L2, i>();
  func107_sub2<L1, L2, j, Num...>();
}


static SyncLow sync_5={0};

template<class L1, class L2>
static void func107(sptr<TaskWithStack> task) {
  static int flag = 0;
  __sync_fetch_and_add(&flag, 1);

  auto ltask_ = make_sptr(new Task);
  ltask_->SetFunc(make_uptr(new Function2<sptr<Task>, sptr<TaskWithStack>>([](sptr<Task> ltask, sptr<TaskWithStack> task_) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask);
          } else {
            if (is_knl()) {
              func107_sub2<L1, L2, 8, 16, 32, 64, 128, 192, 256, 320, 384, 448, 512, 576, 640, 704, 768, 832, 896, 960, 1024, 1280, 1536, 1792, 2048, 2560, 3072, 3584, 4096, 5120, 6144, 7168, 8192>();
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

void membench5(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>\n");
  }
  if (cpuid == 0) {
    StringTty tty(10);
    tty.CprintfRaw("# count\n");
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
  func107<ClhSpinLock, ClhSpinLock>(task);
  func107<McsSpinLock<64>, ClhSpinLock>(task);
  if (cpuid == 0) {
    gtty->CprintfRaw("<<< end\n");
  }
}

