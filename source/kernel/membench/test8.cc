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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
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
/*
// not used
static bool is_knl() {
  return x86::get_display_family_model() == 0x0657;
}
*/

void udpsend(int argc, const char *argv[]);

struct Pair {
  uint64_t time;
  uint64_t fairness;
};

static bool kWorkloadB = true;

static const int kPrivateWork = 4096 * 4;
static int *private_work;
static virt_addr lock_addr;

template <class L>
static __attribute__((noinline)) Pair func107_main(int cpunum, int work) {
  uint64_t time = 0;
  uint64_t f_variance = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();

  L *lock = reinterpret_cast<L *>(lock_addr);
  static const int kInitCnt = 100000;
  static volatile int __attribute__((aligned(64))) cnt = kInitCnt;
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
      bzero(lock, sizeof(L));
      assert(sizeof(L) < 1024 * 1024);
      new (lock) L;
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

  Time t1 = 0;
  if (apicid == 0) {
    t1 = timer->ReadTime();
  }

  if (eflag) {
    sync_2.Do(cpunum);

    while (true) {
      bool flag = true;
      lock->Lock(apicid);
      asm volatile("" ::: "memory");
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
      if (kWorkloadB) {
        for (int i = 0; i < work; i++) {
          private_work[apicid * kPrivateWork + i]++;
        }
        int delay = rand() % work;
        for (int i = 0; i < delay; i++) {
          private_work[apicid * kPrivateWork + i]++;
        }
      }
    }

    sync_3.Do(cpunum);
  }

  if (apicid == 0) {
    time = timer->ReadTime() - t1;
    uint64_t f_avg = 0;
    for (int x = 0; x < cpunum; x++) {
      f_avg += f_array[x].i;
    }
    if (f_avg != kInitCnt) {
      StringTty tty(100);
      tty.Printf("bad spin lock(%d/%d)", f_avg, kInitCnt);
      kernel_panic("membench", tty.GetRawPtr());
    }
    f_avg /= cpunum;
    for (int x = 0; x < cpunum; x++) {
      f_variance += (f_array[x].i - f_avg) * (f_array[x].i - f_avg);
    }
    f_variance /= cpunum;
    cnt = kInitCnt;
    for (int j = 1; j < 37 * 8; j++) {
      if (apic_ctrl->GetCpuIdFromApicId(j) == -1) {
        continue;
      }
      uint64_t tmp = monitor[j].i;
      while (tmp == 0 || tmp == 1) {
        __sync_bool_compare_and_swap(&monitor[j].i, tmp, 1);
        tmp = monitor[j].i;
        asm volatile("" ::: "memory");
      }
    }
  } else {
    volatile uint64_t *monitor_addr = &monitor[apicid].i;
    while (true) {
      asm volatile("monitor;" ::"a"(monitor_addr), "c"(0), "d"(0));
      asm volatile("mwait;" ::"a"(0), "c"(0));
      asm volatile("" ::: "memory");
      if (*monitor_addr == 1) {
        __sync_lock_test_and_set(monitor_addr, 2);
        break;
      }
    }
  }

  sync_4.Do();
  Pair pair = {time : time, fairness : f_variance};
  return pair;
}

template <class L>
static __attribute__((noinline)) void func107_sub2(int cpunum, int work) {
  int apicid = cpu_ctrl->GetCpuId().GetApicId();

  static const int num = 10;
  Pair results[num];
  func107_main<L>(cpunum, work);
  for (int j = 0; j < num; j++) {
    results[j] = func107_main<L>(cpunum, work);
  }
  Pair avg = {time : 0, fairness : 0};
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
    // gtty->Flush();
    StringTty tty(200);
    tty.Printf("%d\t%d\t%d\t%d\n", kWorkloadB ? work : cpunum, avg.time,
               avg.fairness, variance);
    int argc = 4;
    const char *argv[] = {"udpsend", ip_addr, port, tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

static SyncLow sync_5 = {0};
static SyncLow sync_6 = {0};

template <class L>
static void func107_sub() {
  static volatile int flag = 0;
  __sync_fetch_and_add(&flag, 1);

  auto thread =
      ThreadCtrl::GetCurrentCtrl().AllocNewThread(Thread::StackState::kShared);
  do {
    auto t_op = thread->CreateOperator();
    t_op.SetFunc(make_uptr(new Function1<void, void *>(
        [](void *) {
          if (flag != cpu_ctrl->GetHowManyCpus()) {
            ThreadCtrl::GetCurrentThreadOperator().Schedule();
          } else {
            sync_5.Do();
            if (kWorkloadB) {
              for (int work = 1; work <= kPrivateWork; work += 500) {
                func107_sub2<L>(256, work);
              }
            } else {
              for (int cpunum = 32; cpunum <= 256; cpunum += 32) {
                func107_sub2<L>(cpunum, 0);
              }
            }
            int apicid = cpu_ctrl->GetCpuId().GetApicId();
            if (apicid == 0) {
              StringTty tty(4);
              tty.Printf("\n");
              int argc = 4;
              const char *argv[] = {"udpsend", ip_addr, port, tty.GetRawPtr()};
              udpsend(argc, argv);
              flag = 0;
            }
            sync_6.Do();
          }
        },
        nullptr)));
    t_op.Schedule();
  } while (0);
  thread->Join();
}

template <class L>
static void func107(const char *name) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    StringTty tty(100);
    tty.Printf("# %s\n", name);
    int argc = 4;
    const char *argv[] = {"udpsend", ip_addr, port, tty.GetRawPtr()};
    udpsend(argc, argv);
  }

  func107_sub<L>();
}

#define FUNC(...) func107<__VA_ARGS__>(#__VA_ARGS__);

// 全体
void membench8() {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    PhysAddr paddr2;
    physmem_ctrl->Alloc(paddr2, PagingCtrl::ConvertNumToPageSize(
                                    37 * 8 * kPrivateWork * sizeof(int)));
    private_work = reinterpret_cast<int *>(paddr2.GetVirtAddr());

    gtty->Printf("start >>>\n");
    gtty->Flush();

    StringTty tty(100);
    tty.Printf("# count(%s, %c)\n", __func__, kWorkloadB ? 'B' : 'A');
    int argc = 4;
    const char *argv[] = {"udpsend", ip_addr, port, tty.GetRawPtr()};
    udpsend(argc, argv);

    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, 1024 * 1024);
    lock_addr = paddr.GetVirtAddr();
  }
  // FUNC(SimpleSpinLock);
  // FUNC(TtsSpinLock);
  // FUNC(TtsBackoffSpinLock);
  // FUNC(TicketSpinLock);
  // FUNC(AndersonSpinLock<1, 256>);
  // FUNC(AndersonSpinLock<64, 256>);
  FUNC(ClhSpinLock);
  FUNC(McsSpinLock<64>);
  FUNC(HClhSpinLock);
  FUNC(ExpSpinLock11<McsSpinLock<64>, TicketSpinLockA>);
  FUNC(ExpSpinLock11<McsSpinLockA<64>, McsSpinLockA<64>>);
  FUNC(ExpSpinLock11<TicketSpinLockA, McsSpinLockA<64>>);
  // FUNC(ExpSpinLock10<ClhSpinLock, ClhSpinLock, 8>);
  // FUNC(ExpSpinLock10<ClhSpinLock, AndersonSpinLock<64, 8>, 8>);
  // FUNC(ExpSpinLock10<ClhSpinLock, McsSpinLock<64>, 8>);
  // FUNC(ExpSpinLock10<AndersonSpinLock<64, 37>, AndersonSpinLock<64, 8>,
  // 8>); FUNC(ExpSpinLock10<AndersonSpinLock<64, 37>, McsSpinLock<64>,
  // 8>); FUNC(ExpSpinLock10<AndersonSpinLock<64, 37>, ClhSpinLock, 8>);
  // FUNC(ExpSpinLock10<AndersonSpinLock<64, 64>, AndersonSpinLock<64, 8>,
  // 8>); FUNC(ExpSpinLock10<AndersonSpinLock<64, 64>, McsSpinLock<64>,
  // 8>); FUNC(ExpSpinLock10<AndersonSpinLock<64, 64>, ClhSpinLock, 8>);
  // FUNC(ExpSpinLock10<McsSpinLock<64>, McsSpinLock<64>, 8>);
  // FUNC(ExpSpinLock10<McsSpinLock<64>, ClhSpinLock, 8>);
  // FUNC(ExpSpinLock10<McsSpinLock<64>, AndersonSpinLock<64, 8>, 8>);

  if (cpuid == 0) {
    gtty->Printf("<<< end\n");
    gtty->Flush();
  }
}
