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

void udpsend(int argc, const char *argv[]);

static bool is_knl() {
  return x86::get_display_family_model() == 0x0657;
}

template<bool f, int i, class S, class L>
uint64_t func101() {
  using List = LinkedList2<L, i>;
  
  uint64_t time = 0;
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  
  // static LinkedList2<L, i> _list;
  // static LinkedList2<L, i> *list;// = &_list;
  static List *list;
  void *ptr;
  PhysAddr paddr;
  size_t alloc_size = 256 * 2000 * sizeof(Container<i>) + 1024;
  
  if (cpuid == 0) {
    if (f) {
      ptr = reinterpret_cast<void *>(p2v(0x1840000000));
    } else {
      physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(alloc_size));
      ptr = reinterpret_cast<void *>(paddr.GetVirtAddr());
    }
    void *ptr_ = reinterpret_cast<void *>(((reinterpret_cast<size_t>(ptr) + 1023) / 1024) * 1024);
    // list = new LinkedList2<L, i>();
    list = new List();
    for (int j = 1; j < 256 * 2000; j++) {
      list->Push(reinterpret_cast<Container<i> *>(reinterpret_cast<size_t>(ptr_) + sizeof(Container<i>) * j), j);
    }
  }
  {
    {
      static S sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }

    while(true) {
      Container<i> *c = list->Get();
      if (c == nullptr) {
        break;
      }
      c->Calc();
    }
    
    {
      static S sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
      // gtty->CprintfRaw("<%c %d(%d) %d us> ", f ? 'M' : 'C', sizeof(Container<i>), i, time);
      if (!f) {
        physmem_ctrl->Free(paddr, PagingCtrl::ConvertNumToPageSize(alloc_size));
      }
      delete list;
    }
  }
  return time;
}

// クリティカルセクションオンリー
template<bool f, int i, class S, class L>
uint64_t func109(int cpunum) {
  uint64_t time = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static L lock;
  static const int kInitCnt = 3000;
  static volatile int cnt = kInitCnt;
  static uint32_t buf1[100];
  uint32_t buf2[100];
  // static uint32_t *data = reinterpret_cast<uint32_t *>(p2v(0x1840000000));
  // uint32_t *buf = reinterpret_cast<uint32_t *>(p2v(0x1840000000 + i * 4 * (apicid * 2 + 1)));
  // uint32_t *buf2 = reinterpret_cast<uint32_t *>(p2v(0x1840000000 + i * 4 * (apicid * 2 + 2)));
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
      new (&lock) L;
      // for (int x = 0; x < i; x++) {
      //   data[x] = rand();
      // }
    }
  }

  {
    static SyncLow sync={0};
    sync.Do();
  }
  if (eflag) {
    {
      static Sync2Low sync={0};
      sync.Do(cpunum);
    }

    uint64_t t1;
    if (apicid == 0) {
      t1 = timer->ReadMainCnt();
    }

    bool flag = true;
    while(flag) {
      lock.Lock();
      if (cnt > 0) {
        cnt--;
        memcpy(buf2, buf1, 100);
      } else {
        flag = false;
      }
      // for (int y = 0; y < i; y++) {
      //   buf[y] = data[y];
      //   data[y]++;
      // }
      lock.Unlock();
      // for (int y = 0; y < i; y++) {
      //   uint32_t tmp = 0;
      //   for (int z = 0; z < i; z++) {
      //     if (tmp < buf[z]) {
      //       tmp = buf[z];
      //     }
      //   }
      //   buf2[y] += tmp;
      // }
    }
    
    {
      static Sync2Low sync={0};
      sync.Do(cpunum);
    }

    if (apicid == 0) {
      time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
      for (int j = 1; j < cpu_ctrl->GetHowManyCpus(); j++) {
        CpuId cpuid_(j);
        apic_ctrl->SendIpi(cpuid_.GetApicId());
      }
      cnt = kInitCnt;
    }
  } else {
    asm volatile("hlt");
  }
  {
    static SyncLow sync={0};
    sync.Do();
  }
  return time;
}

// 1タイル上で完結(false)と複数タイル分散(true)
template<bool f, int i, class S, class L>
uint64_t func108(int cpunum) {
  uint64_t time = 0;
  volatile int apicid = cpu_ctrl->GetCpuId().GetApicId();
  
  static L lock;
  static const int kInitCnt = 30000;
  static volatile int cnt = kInitCnt;
  static const int buf_size = 10;
  static uint32_t *buf1 = reinterpret_cast<uint32_t *>(p2v(0x1840000000));
  uint32_t *buf2 = reinterpret_cast<uint32_t *>(p2v(0x1840000000 + i * buf_size * (1 + apicid * 2)));
  uint32_t *buf3 = reinterpret_cast<uint32_t *>(p2v(0x1840000000 + i * buf_size * (2 + apicid * 2)));
  int cpunum_ = 0;
  bool eflag;
  if (f) {
    if (apicid % 8 == 0) {
      for (int apicid_ = 0; apicid_ <= apicid; apicid_+=8) {
        if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
          cpunum_++;
        }
      }
      eflag = cpunum_ <= cpunum;
    } else {
      eflag = false;
    }
  } else {
    for (int apicid_ = 0; apicid_ <= apicid; apicid_++) {
      if (apic_ctrl->GetCpuIdFromApicId(apicid_) != -1) {
        cpunum_++;
      }
    }
    eflag = cpunum_ <= cpunum;
  }
  

  {
    if (apicid == 0) {
      new (&lock) L;
      // for (int x = 0; x < i; x++) {
      //   data[x] = rand();
      // }
    }
  }

  {
    static SyncLow sync={0};
    sync.Do();
  }
  if (eflag) {
    {
      static Sync2Low sync={0};
      sync.Do(cpunum);
    }

    uint64_t t1;
    if (apicid == 0) {
      t1 = timer->ReadMainCnt();
    }

    while(true) {
      bool flag = true;
      lock.Lock();
      if (cnt > 0) {
        cnt--;
        for (int z = 0; z < i * buf_size; z ++) {
          buf2[z] = buf1[z];
          buf1[z] += cnt;
        }
      } else {
        flag = false;
      }
      lock.Unlock();
      if (!flag) {
        break;
      }
      for (int y = 0; y < i * buf_size; y++) {
        int tmp = 0;
        for (int z = 0; z < i * buf_size; z++) {
          tmp = buf2[y] * buf3[z];
        }
        buf3[y] = tmp;
      }
    }
    
    {
      static Sync2Low sync={0};
      sync.Do(cpunum);
    }

    if (apicid == 0) {
      time = ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000;
      for (int j = 1; j < cpu_ctrl->GetHowManyCpus(); j++) {
        CpuId cpuid_(j);
        apic_ctrl->SendIpi(cpuid_.GetApicId());
      }
      cnt = kInitCnt;
    }
  } else {
    asm volatile("hlt");
  }
  {
    static SyncLow sync={0};
    sync.Do();
  }
  return time;
}

template<bool f, int i, class S, class L>
void func102_sub(int cpunum) {
  static const int num = 20;
  uint64_t results[num];
  // func109<f, i, S, L>(cpunum);
  // for (int j = 0; j < num; j++) {
  //   results[j] = func109<f, i, S, L>(cpunum);
  // }
  func108<f, i, S, L>(cpunum);
  for (int j = 0; j < num; j++) {
    results[j] = func108<f, i, S, L>(cpunum);
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
  if (apicid == 0) {
    gtty->CprintfRaw("<%c %d(%d) %lld(%lld) us> ", f ? 'D' : 'A', sizeof(Container<i>), i, avg, variance);
    StringTty tty(200);
    tty.CprintfRaw("%c\t%d\t%d\t%d\t%d\n", f ? 'D' : 'A', i, avg, cpunum, avg / cpunum);
    int argc = 4;
    const char *argv[] = {"udpsend", "192.168.12.35", "1234", tty.GetRawPtr()};
    udpsend(argc, argv);
  }
}

template<int i, class S, class L>
void func102(sptr<TaskWithStack> task) {
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
            //for (int cpunum = 1; cpunum <= 256; cpunum++) {
            for (int cpunum = 1; cpunum <= 8; cpunum++) {
              func102_sub<false, i, S, L>(cpunum);
              func102_sub<true, i, S, L>(cpunum);
            }
            task_->Execute();
          }
        }, ltask_, task)));
  task_ctrl->Register(cpu_ctrl->GetCpuId(), ltask_);

  task->Wait();
} 

template<class S, int i>
void func10(sptr<TaskWithStack> task) {
  // func102<f, i, S, LinkedList2<SimpleSpinLockR, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock1<37>, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock2<37>, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock1R<37>, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock2R, i>>(task);
  func102<i, S, McsSpinLock>(task);
  func102<i, S, TicketSpinLock>(task);
  func102<i, S, TtsSpinLock>(task);
  //  func102<f, i, S, LinkedList2<TicketSpinLock, i>>(task);
  // func102<f, i, S, LinkedList2<McTtsSpinLock, i>>(task);
}

template<class S, int i, int j, int... Num>
void func10(sptr<TaskWithStack> task) {
  func10<S, i>(task);
  func10<S, j, Num...>(task);
}

// リスト、要素数可変比較版
template<class S>
static void membench10(sptr<TaskWithStack> task) {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>\n");
  }
  // func10<List, S, true, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280>(); 
  // func10<List, S, false, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300>(task); 
  // func10<S, false, 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 120, 140>(task); 
  func10<S, 4, 7, 10>(task); 
}

template<int i>
struct func103 {
  static void func(sptr<TaskWithStack> task) {
    func102<false, i, SyncLow, SimpleSpinLockR>>(task);
    func103<i + 10>::func(task);
  }
};

template<>
struct func103<300> {
  static void func(sptr<TaskWithStack> task) {
    func102<300, SyncLow, SimpleSpinLockR>(task);
  }
};

void register_membench2_callout() {
  for (int i = 0; i < cpu_ctrl->GetHowManyCpus(); i++) {
    CpuId cpuid(i);
    
    auto task_ = make_sptr(new TaskWithStack(cpuid));
    task_->Init();
    task_->SetFunc(make_uptr(new Function<sptr<TaskWithStack>>([](sptr<TaskWithStack> task){
            if (is_knl()) {
              membench10<SyncLow>(task);
            } else {
              membench10<SyncLow>(task);
              // func103<10>::func(task); 
            }
          }, task_)));
    task_ctrl->Register(cpuid, task_);
  }
}

