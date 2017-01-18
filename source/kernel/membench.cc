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

static bool is_knl() {
  return x86::get_display_family_model() == 0x0657;
}

static void membench() {
  if (!is_knl()) {
    return;
  }
  
  gtty->CprintfRaw("-%d-", cpu_ctrl->GetCpuId().GetRawId());
  
  int entry = 20 * 1024 * 1024 / sizeof(int);
  int *tmp = new int[entry];
  for(int i = 0; i < entry - 1; i++) {
    tmp[i] = i + 1;
  }
  // http://mementoo.info/archives/746
  for(int i=0;i<entry - 1;i++)
    {
      int j = rand()%(entry - 1);
      int t = tmp[i];
      tmp[i] = tmp[j];
      tmp[j] = t;
    }

  {      
    int *buf = new int[entry];
    // init
    {
      int j = 0;
      for (int i = 0; i < entry - 1; i++) {
        j = (buf[j] = tmp[i]);
      }
      buf[j] = -1;
    }
    // bench
    measure {
      int j = 0;
      do {
        j = buf[j];
      } while(buf[j] != -1);
    }
    delete buf;
  }
  {      
    PhysAddr paddr;
    physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(entry * sizeof(int)));
    int *buf = reinterpret_cast<int *>(paddr.GetVirtAddr());
    // init
    int sum_bkup = 0;
    {
      for (int i = 0; i < entry - 1; i++) {
        buf[i] = i;
        sum_bkup += buf[i];
      }
    }
    int sum = 0;
    // bench
    measure {
      for (int i = 0; i < entry - 1; i++) {
        sum += buf[i];
      }
    }
    assert(sum == sum_bkup);
    delete buf;
  }
  {      
    //int *buf = new int[entry];
    int *buf = reinterpret_cast<int *>(p2v(0x1840000000));
    // init
    {
      int j = 0;
      for (int i = 0; i < entry - 1; i++) {
        j = (buf[j] = tmp[i]);
      }
      buf[j] = -1;
    }
    // bench
    measure {
      int j = 0;
      do {
        j = buf[j];
      } while(buf[j] != -1);
    }
    // delete buf;
  }
  {      
    // int *buf = new int[entry];
    int *buf = reinterpret_cast<int *>(p2v(0x1840000000));
    // init
    int sum_bkup = 0;
    {
      for (int i = 0; i < entry - 1; i++) {
        buf[i] = i;
        sum_bkup += buf[i];
      }
    }
    int sum = 0;
    // bench
    measure {
      for (int i = 0; i < entry - 1; i++) {
        sum += buf[i];
      }
    }
    assert(sum == sum_bkup);
    // delete buf;
  }
  delete tmp;
}

int *init, *answer;
int *ddr, *mcdram;
static inline int min(int a, int b) {
  return a > b ? b : a;
}

static inline void sync(volatile int &l1, volatile int &l2, volatile int &l3) {
  int cpunum = cpu_ctrl->GetHowManyCpus();
  l2 = 0;
  while(true) {
    int tmp = l1;
    if (__sync_bool_compare_and_swap(&l1, tmp, tmp + 1)) {
      break;
    }
  }
  do {
  } while(!__sync_bool_compare_and_swap(&l1, cpunum, cpunum));
  l3 = 0;
  while(true) {
    int tmp = l2;
    if (__sync_bool_compare_and_swap(&l2, tmp, tmp + 1)) {
      break;
    }
  }
  do {
  } while(!__sync_bool_compare_and_swap(&l2, cpunum, cpunum));
  l1 = 0;
  while(true) {
    int tmp = l3;
    if (__sync_bool_compare_and_swap(&l3, tmp, tmp + 1)) {
      break;
    }
  }
  do {
  } while(!__sync_bool_compare_and_swap(&l3, cpunum, cpunum));
}

// syncのコア数制限版
static inline void sync2(int cpunum, volatile int &l1, volatile int &l2, volatile int &l3) {
  l2 = 0;
  while(true) {
    int tmp = l1;
    if (__sync_bool_compare_and_swap(&l1, tmp, tmp + 1)) {
      break;
    }
  }
  do {
  } while(!__sync_bool_compare_and_swap(&l1, cpunum, cpunum));
  l3 = 0;
  while(true) {
    int tmp = l2;
    if (__sync_bool_compare_and_swap(&l2, tmp, tmp + 1)) {
      break;
    }
  }
  do {
  } while(!__sync_bool_compare_and_swap(&l2, cpunum, cpunum));
  l1 = 0;
  while(true) {
    int tmp = l3;
    if (__sync_bool_compare_and_swap(&l3, tmp, tmp + 1)) {
      break;
    }
  }
  do {
  } while(!__sync_bool_compare_and_swap(&l3, cpunum, cpunum));
}

// syncのSpin On Read版
static inline void sync_read(volatile int &l1, volatile int &l2, volatile int &l3) {
  int cpunum = cpu_ctrl->GetHowManyCpus();
  l2 = 0;
  while(true) {
    int tmp = l1;
    if (__sync_bool_compare_and_swap(&l1, tmp, tmp + 1)) {
      break;
    }
  }
  while(true) {
    while(l1 != cpunum) {
    }
    if (__sync_bool_compare_and_swap(&l1, cpunum, cpunum)) {
      break;
    }
  }
  l3 = 0;
  while(true) {
    int tmp = l2;
    if (__sync_bool_compare_and_swap(&l2, tmp, tmp + 1)) {
      break;
    }
  }
  while(true) {
    while(l2 != cpunum) {
    }
    if (__sync_bool_compare_and_swap(&l2, cpunum, cpunum)) {
      break;
    }
  }
  l1 = 0;
  while(true) {
    int tmp = l3;
    if (__sync_bool_compare_and_swap(&l3, tmp, tmp + 1)) {
      break;
    }
  }
  while(true) {
    while(l3 != cpunum) {
    }
    if (__sync_bool_compare_and_swap(&l3, cpunum, cpunum)) {
      break;
    }
  }
}

// sync2のSpin On Read版
static inline void sync2_read(int cpunum, volatile int &l1, volatile int &l2, volatile int &l3) {
  l2 = 0;
  while(true) {
    int tmp = l1;
    if (__sync_bool_compare_and_swap(&l1, tmp, tmp + 1)) {
      break;
    }
  }
  while(true) {
    while(l1 != cpunum) {
    }
    if (__sync_bool_compare_and_swap(&l1, cpunum, cpunum)) {
      break;
    }
  }
  l3 = 0;
  while(true) {
    int tmp = l2;
    if (__sync_bool_compare_and_swap(&l2, tmp, tmp + 1)) {
      break;
    }
  }
  while(true) {
    while(l2 != cpunum) {
    }
    if (__sync_bool_compare_and_swap(&l2, cpunum, cpunum)) {
      break;
    }
  }
  l1 = 0;
  while(true) {
    int tmp = l3;
    if (__sync_bool_compare_and_swap(&l3, tmp, tmp + 1)) {
      break;
    }
  }
  while(true) {
    while(l3 != cpunum) {
    }
    if (__sync_bool_compare_and_swap(&l3, cpunum, cpunum)) {
      break;
    }
  }
}

struct SyncLow {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  void Do() {
    sync_read(top_level_lock1, top_level_lock2, top_level_lock3);
  }
};

// 1階層
template<int kSubStructNum, int kPhysAvailableCoreNum, int kGroupCoreNum>
struct Sync {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  struct SyncSub {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&lock, kGroupCoreNum, kGroupCoreNum));
    }
  };
  SyncSub second_level_lock1[kSubStructNum];
  SyncSub second_level_lock2[kSubStructNum];
  SyncSub second_level_lock3[kSubStructNum];
  void Init() {
    top_level_lock1 = 0;
    top_level_lock2 = 0;
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    second_level_lock2[apicid / kGroupCoreNum].Init();
    second_level_lock1[apicid / kGroupCoreNum].Do();
    second_level_lock3[apicid / kGroupCoreNum].Init();
    second_level_lock2[apicid / kGroupCoreNum].Do();
    if (apicid % kGroupCoreNum == 0) {
      top_level_lock2 = 0;
      while(true) {
        int tmp = top_level_lock1;
        if (__sync_bool_compare_and_swap(&top_level_lock1, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock1, kPhysAvailableCoreNum, kPhysAvailableCoreNum));
      top_level_lock3 = 0;
      while(true) {
        int tmp = top_level_lock2;
        if (__sync_bool_compare_and_swap(&top_level_lock2, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock2, kPhysAvailableCoreNum, kPhysAvailableCoreNum)); 
      top_level_lock1 = 0;
      while(true) {
        int tmp = top_level_lock3;
        if (__sync_bool_compare_and_swap(&top_level_lock3, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock3, kPhysAvailableCoreNum, kPhysAvailableCoreNum));
    }
    second_level_lock1[apicid / kGroupCoreNum].Init();
    second_level_lock3[apicid / kGroupCoreNum].Do();
  }
};
using SyncE3 = Sync<4, 4, 2>;

// 1階層 Spin On Read
template<int kSubStructNum, int kPhysAvailableCoreNum, int kGroupCoreNum>
struct SyncRead {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  struct SyncSub {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(lock != kGroupCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&lock, kGroupCoreNum, kGroupCoreNum)) {
          break;
        }
      }
    }
  };
  SyncSub second_level_lock1[kSubStructNum];
  SyncSub second_level_lock2[kSubStructNum];
  SyncSub second_level_lock3[kSubStructNum];
  void Init() {
    top_level_lock1 = 0;
    top_level_lock2 = 0;
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    second_level_lock2[apicid / kGroupCoreNum].Init();
    second_level_lock1[apicid / kGroupCoreNum].Do();
    second_level_lock3[apicid / kGroupCoreNum].Init();
    second_level_lock2[apicid / kGroupCoreNum].Do();
    if (apicid % kGroupCoreNum == 0) {
      top_level_lock2 = 0;
      while(true) {
        int tmp = top_level_lock1;
        if (__sync_bool_compare_and_swap(&top_level_lock1, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock1 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock1, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
      top_level_lock3 = 0;
      while(true) {
        int tmp = top_level_lock2;
        if (__sync_bool_compare_and_swap(&top_level_lock2, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock2 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock2, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
      top_level_lock1 = 0;
      while(true) {
        int tmp = top_level_lock3;
        if (__sync_bool_compare_and_swap(&top_level_lock3, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock3 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock3, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
    }
    second_level_lock1[apicid / kGroupCoreNum].Init();
    second_level_lock3[apicid / kGroupCoreNum].Do();
  }
};
using SyncReadE3 = SyncRead<4, 4, 2>;

// 2階層
template<int kSubStructNum, int kPhysAvailableCoreNum>
struct Sync2 {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  struct SyncSub {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&lock, 2, 2));
    }
  };
  SyncSub second_level_lock1[kSubStructNum];
  SyncSub second_level_lock2[kSubStructNum];
  SyncSub second_level_lock3[kSubStructNum];
  struct SyncSub2 {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&lock, 4, 4));
    }
  };
  SyncSub2 third_level_lock1[kSubStructNum*2];
  SyncSub2 third_level_lock2[kSubStructNum*2];
  SyncSub2 third_level_lock3[kSubStructNum*2];
  void Init() {
    top_level_lock1 = 0;
    top_level_lock2 = 0;
    top_level_lock3 = 0;
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    if (apicid % 8 == 0) {
      top_level_lock2 = 0;
      while(true) {
        int tmp = top_level_lock1;
        if (__sync_bool_compare_and_swap(&top_level_lock1, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock1, kPhysAvailableCoreNum, kPhysAvailableCoreNum));
      top_level_lock3 = 0;
      while(true) {
        int tmp = top_level_lock2;
        if (__sync_bool_compare_and_swap(&top_level_lock2, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock2, kPhysAvailableCoreNum, kPhysAvailableCoreNum)); 
      top_level_lock1 = 0;
      while(true) {
        int tmp = top_level_lock3;
        if (__sync_bool_compare_and_swap(&top_level_lock3, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock3, kPhysAvailableCoreNum, kPhysAvailableCoreNum));
    }
    third_level_lock1[apicid / 4].Init();
    third_level_lock3[apicid / 4].Do();
  }
};
using Sync2Phi = Sync2<37, 32>;

// 2階層 Spin On Read
template<int kSubStructNum, int kPhysAvailableCoreNum>
struct Sync2Read {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  struct SyncSub {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(lock != 2) {
        }
        if (__sync_bool_compare_and_swap(&lock, 2, 2)) {
          break;
        }
      }
    }
  };
  SyncSub second_level_lock1[kSubStructNum];
  SyncSub second_level_lock2[kSubStructNum];
  SyncSub second_level_lock3[kSubStructNum];
  struct SyncSub2 {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(lock != 4) {
        }
        if (__sync_bool_compare_and_swap(&lock, 4, 4)) {
          break;
        }
      }
    }
  };
  SyncSub2 third_level_lock1[kSubStructNum*2];
  SyncSub2 third_level_lock2[kSubStructNum*2];
  SyncSub2 third_level_lock3[kSubStructNum*2];
  void Init() {
    top_level_lock1 = 0;
    top_level_lock2 = 0;
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    if (apicid % 8 == 0) {
      top_level_lock2 = 0;
      while(true) {
        int tmp = top_level_lock1;
        if (__sync_bool_compare_and_swap(&top_level_lock1, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock1 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock1, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
      top_level_lock3 = 0;
      while(true) {
        int tmp = top_level_lock2;
        if (__sync_bool_compare_and_swap(&top_level_lock2, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock2 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock2, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
      top_level_lock1 = 0;
      while(true) {
        int tmp = top_level_lock3;
        if (__sync_bool_compare_and_swap(&top_level_lock3, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock3 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock3, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
    }
    third_level_lock1[apicid / 4].Init();
    third_level_lock3[apicid / 4].Do();
  }
};
using Sync2ReadPhi = Sync2Read<37, 32>;

// Sync2のコア数制限版
template<int kSubStructNum>
struct Sync3 {
  int kPhysAvailableCoreNum;
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  struct SyncSub {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&lock, 2, 2));
    }
  };
  SyncSub second_level_lock1[kSubStructNum];
  SyncSub second_level_lock2[kSubStructNum];
  SyncSub second_level_lock3[kSubStructNum];
  struct SyncSub2 {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&lock, 4, 4));
    }
  };
  SyncSub2 third_level_lock1[kSubStructNum*2];
  SyncSub2 third_level_lock2[kSubStructNum*2];
  SyncSub2 third_level_lock3[kSubStructNum*2];
  void Init(int substruct_num) {
    kPhysAvailableCoreNum = substruct_num;
    top_level_lock1 = 0;
    top_level_lock2 = 0;
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    if (apicid % 8 == 0) {
      top_level_lock2 = 0;
      while(true) {
        int tmp = top_level_lock1;
        if (__sync_bool_compare_and_swap(&top_level_lock1, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock1, kPhysAvailableCoreNum, kPhysAvailableCoreNum));
      top_level_lock3 = 0;
      while(true) {
        int tmp = top_level_lock2;
        if (__sync_bool_compare_and_swap(&top_level_lock2, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock2, kPhysAvailableCoreNum, kPhysAvailableCoreNum)); 
      top_level_lock1 = 0;
      while(true) {
        int tmp = top_level_lock3;
        if (__sync_bool_compare_and_swap(&top_level_lock3, tmp, tmp + 1)) {
          break;
        }
      }
      do {
      } while(!__sync_bool_compare_and_swap(&top_level_lock3, kPhysAvailableCoreNum, kPhysAvailableCoreNum));
    }
    third_level_lock1[apicid / 4].Init();
    third_level_lock3[apicid / 4].Do();
  }
};
using Sync3Phi = Sync3<37>;

// Sync3のSpinOnRead版
template<int kSubStructNum>
struct Sync3Read {
  int kPhysAvailableCoreNum;
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  struct SyncSub {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(lock != 2) {
        }
        if (__sync_bool_compare_and_swap(&lock, 2, 2)) {
          break;
        }
      }
    }
  };
  SyncSub second_level_lock1[kSubStructNum];
  SyncSub second_level_lock2[kSubStructNum];
  SyncSub second_level_lock3[kSubStructNum];
  struct SyncSub2 {
    int lock;
    void Init() {
      lock = 0;
    }
    void Do() {
      while(true) {
        int tmp = lock;
        if (__sync_bool_compare_and_swap(&lock, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(lock != 4) {
        }
        if (__sync_bool_compare_and_swap(&lock, 4, 4)) {
          break;
        }
      }
    }
  };
  SyncSub2 third_level_lock1[kSubStructNum*2];
  SyncSub2 third_level_lock2[kSubStructNum*2];
  SyncSub2 third_level_lock3[kSubStructNum*2];
  void Init(int substruct_num) {
    kPhysAvailableCoreNum = substruct_num;
    top_level_lock1 = 0;
    top_level_lock2 = 0;
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    if (apicid % 8 == 0) {
      top_level_lock2 = 0;
      while(true) {
        int tmp = top_level_lock1;
        if (__sync_bool_compare_and_swap(&top_level_lock1, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock1 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock1, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
      top_level_lock3 = 0;
      while(true) {
        int tmp = top_level_lock2;
        if (__sync_bool_compare_and_swap(&top_level_lock2, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock2 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock2, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
      top_level_lock1 = 0;
      while(true) {
        int tmp = top_level_lock3;
        if (__sync_bool_compare_and_swap(&top_level_lock3, tmp, tmp + 1)) {
          break;
        }
      }
      while(true) {
        while(top_level_lock3 != kPhysAvailableCoreNum) {
        }
        if (__sync_bool_compare_and_swap(&top_level_lock3, kPhysAvailableCoreNum, kPhysAvailableCoreNum)) {
          break;
        }
      }
    }
    third_level_lock1[apicid / 4].Init();
    third_level_lock3[apicid / 4].Do();
  }
};
using Sync3ReadPhi = Sync3Read<37>;

// ワーシャル-フロイド
static void membench2() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();

  for (int num = 100; num < 60000; num += 1000) {
    if (cpuid == 0) {
      gtty->CprintfRaw("\nnum: %d\n", num);
      
      PhysAddr paddr1;
      physmem_ctrl->Alloc(paddr1, PagingCtrl::ConvertNumToPageSize(num * num * sizeof(int)));
      init = reinterpret_cast<int *>(paddr1.GetVirtAddr());
      PhysAddr paddr2;
      physmem_ctrl->Alloc(paddr2, PagingCtrl::ConvertNumToPageSize(num * num * sizeof(int)));
      answer = reinterpret_cast<int *>(paddr2.GetVirtAddr());
      // init
      {
        for (int i = 0; i < num; i++) {
          for (int j = 0; j < i; j++) {
            int r = rand() % 999;
            init[i * num + j] = r;
            answer[i * num + j] = r;
            init[j * num + i] = r;
            answer[j * num + i] = r;
          }
          init[i * num + i] = 0;
          answer[i * num + i] = 0;
        }
      }

      uint64_t t1 = timer->ReadMainCnt();

      for (int k = 0; k < num; k++) {
        for (int i = 0; i < num; i++) {
          for (int j = 0; j < num; j++) {
            answer[i * num + j] = min(answer[i * num + j], answer[i * num + k] + answer[k * num + j]);
          }
        }
      }

      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }

    // if (cpuid == 0) {
    //   PhysAddr paddr;
    //   physmem_ctrl->Alloc(paddr, PagingCtrl::ConvertNumToPageSize(num * num * sizeof(int)));
    //   ddr = reinterpret_cast<int *>(paddr.GetVirtAddr());
    //   memcpy(ddr, init, num * num * sizeof(int));
    //   gtty->CprintfRaw("init ddr ");
    // }
  
    // {
    //   {
    //     static volatile int l1 = 0, l2 = 0;
    //     sync(l1, l2);
    //   }
  
    //   uint64_t t1 = timer->ReadMainCnt();

    
    //   static volatile int cnt = 0;
    //   while(true) {
    //     int k = cnt;
    //     if (k == num) {
    //       break;
    //     }
    //     if (!__sync_bool_compare_and_swap(&cnt, k, k + 1)) {
    //       continue;
    //     }
    //     for (int i = 0; i < num; i++) {
    //       for (int j = 0; j < num; j++) {
    //         ddr[i * num + j] = min(ddr[i * num + j], ddr[i * num + k] + ddr[k * num + j]);
    //       }
    //     }
    //   }

    //   {
    //     static volatile int l1 = 0, l2 = 0;
    //     sync(l1, l2);
    //   }
      
    //   if (cpuid == 0) {
    //     gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    //     cnt = 0;
    //   }
    // }


    if (cpuid == 0) {
      mcdram = reinterpret_cast<int *>(p2v(0x1840000000));
      memcpy(mcdram, init, num * num * sizeof(int));
      gtty->CprintfRaw("init mcdram ");
    }


    {
      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync(l1, l2, l3);
      }
  
      uint64_t t1 = timer->ReadMainCnt();

      for (int k = 0; k < num; k++) {
        {
          static volatile int l1 = 0, l2 = 0, l3 = 0;
          sync(l1, l2, l3);
        }
        static volatile int cnt = 0;
        while(true) {
          int i = cnt;
          if (i == num) {
            break;
          }
          if (__sync_bool_compare_and_swap(&cnt, i, i + 1)) {
            continue;
          }
          for (int j = 0; j < num; j++) {
            mcdram[i * num + j] = min(mcdram[i * num + j], mcdram[i * num + k] + mcdram[k * num + j]);
          }
        }
      }

      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync(l1, l2, l3);
      }
      if (cpuid == 0) {
        gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
        for (int i = 0; i < num; i++) {
          for (int j = 0; j < num; j++) {
            kassert(mcdram[i * num + j] == answer[i * num + j]);
          }
        }
      }
    }
  }
}

// ２コアで同期を取るベンチマーク
static void membench3() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();

  {
    static volatile int l1 = 0, l2 = 0, l3 = 0;
    sync(l1, l2, l3);
  }
  if (cpuid == 0) {
    gtty->CprintfRaw("bench start\n");
  }
  for (int i = 1; i < 256; i++) {
    static volatile int cnt = 0;
    if (cpuid == 0) {
      cnt = 0;
    }
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
    if (cpuid == 0) {
      uint64_t t1 = timer->ReadMainCnt();
      for (int j = 0; j < 0xFFF; j+=2) {
        do {
        } while(!__sync_bool_compare_and_swap(&cnt, j, j + 1));
      }
      {
        CpuId cpuid_(i);
        gtty->CprintfRaw("%d:%d:%d ", i, cpuid_.GetApicId(), (((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod())) / 1000);
      }
      for (int j = 1; j < cpu_ctrl->GetHowManyCpus(); j++) {
        if (i != j) {
          CpuId cpuid_(j);
          apic_ctrl->SendIpi(cpuid_.GetApicId());
        }
      }
    } else {
      if (cpuid == i) {
        for (int j = 1; j < 0xFFF; j+=2) {
          do {
          } while(!__sync_bool_compare_and_swap(&cnt, j, j + 1));
        }
      } else {
        asm volatile("hlt;");
      }
    }
  }
}

// うまく動かない
static void membench4() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();

  {
    static volatile int l1 = 0, l2 = 0, l3 = 0;
    sync(l1, l2, l3);
  }
  if (cpuid == 0) {
    gtty->CprintfRaw("bench start\n");
  }
  for (int i = 1; i < 256; i++) {
    static volatile int cnt = 0;
    if (cpuid == 0) {
      cnt = 0;
    }
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
    if (cpuid == 0) {
      uint64_t t1 = timer->ReadMainCnt();
      for (int j = 0; j < 0xFFF; j+=(i+1)) {
        do {
        } while(!__sync_bool_compare_and_swap(&cnt, j, j + 1));
      }
      {
        CpuId cpuid_(i);
        gtty->CprintfRaw("%d:%d:%d ", i, cpuid_.GetApicId(), (((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod())) / 1000);
      }
      for (int j = 1; j < 256; j++) {
        if (i < j) {
          CpuId cpuid_(j);
          apic_ctrl->SendIpi(cpuid_.GetApicId());
        }
      }
    } else {
      if (cpuid <= i) {
        for (int j = cpuid; j < 0xFFF; j+=(i+1)) {
          do {
          } while(!__sync_bool_compare_and_swap(&cnt, j, j + 1));
        }
      } else {
        asm volatile("hlt;");
      }
    }
  }
}

// syncのテスト
static void membench5() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();

  {
    static volatile int l1 = 0, l2 = 0, l3 = 0;
    sync(l1, l2, l3);
  }

  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>");
  }
  
  {
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t1 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t1 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync_read(l1, l2, l3);
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    static Sync2Phi sync_;
    sync_.Init();

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t2 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      sync_.Do();
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    static Sync2ReadPhi sync_;
    sync_.Init();

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t2 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      sync_.Do();
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod()) / 1000);
    }
  }
}

// コアの個数を変えた同期
static void membench6() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();

  {
    static volatile int l1 = 0, l2 = 0, l3 = 0;
    sync(l1, l2, l3);
  }

  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>");
  }
  
  for (unsigned int i = 1; i <= 1/*37*/; i++) {
    static volatile int cpu_num;
    if (cpuid == 0) {
      cpu_num = 0;
    }
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
    
    if (apicid < i * 8) {
      while(true) {
        int tmp = cpu_num;
        if (__sync_bool_compare_and_swap(&cpu_num, tmp, tmp + 1)) {
          break;
        }
      }
    }

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }

    if (apicid < i * 8) {
      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(cpu_num, l1, l2, l3);
      }
  
      uint64_t t1 = timer->ReadMainCnt();
      for (int j = 0; j < 100; j++) {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(cpu_num, l1, l2, l3);
      }
      if (apicid == 0) {
        gtty->CprintfRaw("<%d %d us> ", cpu_num, ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
        for (int j = 0; j < 256; j++) {
          CpuId cpuid_(j);
          if (cpuid_.GetApicId() != 0) {
            apic_ctrl->SendIpi(cpuid_.GetApicId());
          }
        }
      }
    } else {
      asm volatile("hlt;");
    }

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }

    if (apicid < i * 8) {
      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(cpu_num, l1, l2, l3);
      }
  
      uint64_t t1 = timer->ReadMainCnt();
      for (int j = 0; j < 100; j++) {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2_read(cpu_num, l1, l2, l3);
      }
      if (apicid == 0) {
        gtty->CprintfRaw("<%d %d us> ", cpu_num, ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
        for (int j = 0; j < 256; j++) {
          CpuId cpuid_(j);
          if (cpuid_.GetApicId() != 0) {
            apic_ctrl->SendIpi(cpuid_.GetApicId());
          }
        }
      }
    } else {
      asm volatile("hlt;");
    }
  }



  for (unsigned int i = 1; i <= 1/*37*/; i++) {
    static volatile int cpu_num;
    if (cpuid == 0) {
      cpu_num = 0;
    }
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
    
    if (apicid < i * 8) {
      while(true) {
        int tmp = cpu_num;
        if (__sync_bool_compare_and_swap(&cpu_num, tmp, tmp + 1)) {
          break;
        }
      }
    }

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }

    if (apicid < i * 8) {
      assert(cpu_num % 8 == 0);
      static Sync3Phi sync_;
      sync_.Init(cpu_num / 8);

      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(cpu_num, l1, l2, l3);
      }
  
      uint64_t t2 = timer->ReadMainCnt();
      for (int j = 0; j < 100; j++) {
        sync_.Do();
      }

      if (apicid == 0) {
        gtty->CprintfRaw("<%d %d us> ", cpu_num, ((timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod()) / 1000);
        for (int j = 0; j < 256; j++) {
          CpuId cpuid_(j);
          if (cpuid_.GetApicId() != 0) {
            apic_ctrl->SendIpi(cpuid_.GetApicId());
          }
        }
      }
    } else {
      asm volatile("hlt;");
    }

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }

    if (apicid < i * 8) {
      assert(cpu_num % 8 == 0);
      static Sync3ReadPhi sync_;
      sync_.Init(cpu_num / 8);

      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(cpu_num, l1, l2, l3);
      }
  
      uint64_t t2 = timer->ReadMainCnt();
      for (int j = 0; j < 100; j++) {
        sync_.Do();
      }

      if (apicid == 0) {
        gtty->CprintfRaw("<%d %d us> ", cpu_num, ((timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod()) / 1000);
        for (int j = 0; j < 256; j++) {
          CpuId cpuid_(j);
          if (cpuid_.GetApicId() != 0) {
            apic_ctrl->SendIpi(cpuid_.GetApicId());
          }
        }
      }
    } else {
      asm volatile("hlt;");
    }
  }

}

// SimpleSpinLock　単純
// McSpinLock1 1階層ロック
// McSpinLock2 2階層ロック
// SimpleSpinLockR 単純Spin On Read
// McSpinLock1R 1階層 Spin On Read
// McSpinLock2R 2階層 Spin On Read




// 単純
class SimpleSpinLock {
public:
  SimpleSpinLock() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    while(!__sync_bool_compare_and_swap(&_flag, 0, 1)) {
    }
  }
  void Unlock() {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
  }
private:
  volatile unsigned int _flag = 0;
};

// 1階層ロック
template <int kSubStructNum>
class McSpinLock1 {
public:
  McSpinLock1() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(!__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
    }
    while(!__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
  }
private:
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
};

// 2階層ロック
template <int kSubStructNum>
class McSpinLock2 {
public:
  McSpinLock2() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(!__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 0, 1)) {
    }
    while(!__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
    }
    while(!__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    // _third_flag[apicid / 4] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
    assert(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 1, 0));
  }
private:
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
  volatile unsigned int _third_flag[kSubStructNum*2] = {0};
};

// 単純 Spin On Read
class SimpleSpinLockR {
public:
  SimpleSpinLockR() {
  }
  bool TryLock() {
    if (_flag == 1) {
      return false;
    }
    return __sync_bool_compare_and_swap(&_flag, 0, 1);
  }
  void Lock() {
    while (true) {
      while(_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    // _flag = 0;
    assert(__sync_bool_compare_and_swap(&_flag, 1, 0));
  }
private:
  volatile unsigned int _flag = 0;
};

// 1階層 Spin On Read
template <int kSubStructNum>
class McSpinLock1R {
public:
  McSpinLock1R() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(true) {
      while(_second_flag[apicid / 8] == 1) {
      }
      if(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
        break;
      }
    }
    while (true) {
      while(_top_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
  }
private:
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
};

// 2階層 Spin On Read
class McSpinLock2R {
public:
  McSpinLock2R() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(true) {
      while(_third_flag[apicid / 4] == 1) {
      }
      if(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 0, 1)) {
        break;
      }
    }
    while(true) {
      while(_second_flag[apicid / 8] != 0) {
      }
      if(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
        break;
      }
    }
    while (true) {
      while(_top_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    // _third_flag[apicid / 4] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
    assert(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 1, 0));
  }
private:
  static const int kSubStructNum = 37;
  static const int kPhysAvailableCoreNum = 32;
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
  volatile unsigned int _third_flag[kSubStructNum*2] = {0};
};

// 2階層 Spin On Read
class McSpinLock2R2 {
public:
  McSpinLock2R2() {
  }
  bool TryLock() {
    assert(false);
  }
  void Lock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    while(!__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 0, 1)) {
    }
    while(true) {
      while(_second_flag[apicid / 8] != 0) {
      }
      if(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 0, 1)) {
        break;
      }
    }
    while (true) {
      while(_top_flag == 1) {
      }
      if (__sync_bool_compare_and_swap(&_top_flag, 0, 1)) {
        break;
      }
    }
  }
  void Unlock() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    // _top_flag = 0;
    // _second_flag[apicid / 8] = 0;
    // _third_flag[apicid / 4] = 0;
    assert(__sync_bool_compare_and_swap(&_top_flag, 1, 0));
    assert(__sync_bool_compare_and_swap(&_second_flag[apicid / 8], 1, 0));
    assert(__sync_bool_compare_and_swap(&_third_flag[apicid / 4], 1, 0));
  }
private:
  static const int kSubStructNum = 37;
  static const int kPhysAvailableCoreNum = 32;
  volatile unsigned int _top_flag = 0;
  volatile unsigned int _second_flag[kSubStructNum] = {0};
  volatile unsigned int _third_flag[kSubStructNum*2] = {0};
};

template <class L, int kListEntryNum>
class LinkedList {
public:
  LinkedList() {
    for (int j = 0; j < kListEntryNum; j++) {
      _first.i[j] = 0;
    }
    _first.next = nullptr;
    _last = &_first;
  }
  int Get() {
    Container *c = nullptr;
    _lock.Lock();
    if (_first.next != nullptr) {
      c = _first.next;
      _first.next = c->next;
      if (_last == c) {
        _last = &_first;
      }
    }
    _lock.Unlock();
    int i = 0;
    if (c != nullptr) {
      for (int l = 0; l < 1000; l++) {
        int k = 1;
        for (int j = 0; j < kListEntryNum; j++) {
          k *= c->i[j];
        }
        i += k;
      }
      delete c;
    }
    return i;
  }
  void Push(int i) {
    assert(i != 0);
    Container *c = new Container;
    for (int j = 0; j < kListEntryNum; j++) {
      c->i[j] = i;
    }
    c->next = nullptr;
    _lock.Lock();
    _last->next = c;
    _last = c;
    _lock.Unlock();
  }
  struct Container {
    Container *next;
    int i[kListEntryNum];
  };
private:
  L _lock;
  Container _first; // 番兵
  Container *_last;
};

template<int kListEntryNum>
struct Container {
  Container *next;
  int i[kListEntryNum];
  // char dummy[64 - kListEntryNum * 4 - 8];
  int Calc() {
    int x = 1;
    // if (c != nullptr) {
    //   for (int l = 0; l < 1000; l++) {
    //     int k = 1;
    //     for (int j = 0; j < kListEntryNum; j++) {
    //       k *= c->i[j];
    //     }
    //     i += k;
    //   }
    // }
    int k = 1;
    for (int l = 0; l < kListEntryNum; l++) {
      for (int j = 0; j < kListEntryNum; j++) {
        k += i[j] * i[l];
      }
      x *= k;
    }
    if (x == 0) {
      return 1;
    }
    return x;
  }
};

template <class L, int kListEntryNum>
class LinkedList2 {
public:
  LinkedList2() {
    for (int j = 0; j < kListEntryNum; j++) {
      _first.i[j] = 0;
    }
    _first.next = nullptr;
    _last = &_first;
  }
  Container<kListEntryNum> *Get() {
    Container<kListEntryNum> *c = nullptr;
    _lock.Lock();
    if (_first.next != nullptr) {
      c = _first.next;
      _first.next = c->next;
      if (_last == c) {
        _last = &_first;
      }
    }
    _lock.Unlock();
    return c;
  }
  void Push(Container<kListEntryNum> *c, int i) {
    assert(i != 0);
    for (int j = 0; j < kListEntryNum; j++) {
      c->i[j] = i;
    }
    _lock.Lock();
    PushSub(c);
    _lock.Unlock();
  }
  void PushSub(Container<kListEntryNum> *c) {
    c->next = nullptr;
    _last->next = c;
    _last = c;
  }
  bool Acquire() {
    return _lock.TryLock();
  }
  void Release() {
    _lock.Unlock();
  }
private:
  L _lock;
  Container<kListEntryNum> _first; // 番兵
  Container<kListEntryNum> *_last;
};

template <int kListEntryNum>
class LinkedList3 {
public:
  LinkedList3() {
    for (int j = 0; j < kListEntryNum; j++) {
      _first.i[j] = 0;
    }
    _first.next = nullptr;
    _last = &_first;
  }
  Container<kListEntryNum> *Get() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
    Container<kListEntryNum> *c = nullptr;
    while (true) {
      c = _list[apicid / 8].Get();
      if (c != nullptr) {
        break;
      }
      if (_list[apicid / 8].Acquire()) {
        _lock.Lock();
        bool flag = false;
        for (int i = 0; i < 16; i++) {
          if (_first.next != nullptr) {
            flag = true;
            Container<kListEntryNum> *c2 = _first.next;
            _first.next = c2->next;
            if (_last == c2) {
              _last = &_first;
            }
            _list[apicid / 8].PushSub(c2);
          }
        }
        _lock.Unlock();
        _list[apicid / 8].Release();
        if (!flag) {
          return nullptr;
        }
      }
    }
    
    return c;
  }
  void Push(Container<kListEntryNum> *c, int i) {
    assert(i != 0);
    for (int j = 0; j < kListEntryNum; j++) {
      c->i[j] = i;
    }
    c->next = nullptr;
    _lock.Lock();
    _last->next = c;
    _last = c;
    _lock.Unlock();
  }
private:
  SimpleSpinLockR _lock;
  LinkedList2<SimpleSpinLockR, kListEntryNum> _list[37];
  Container<kListEntryNum> _first; // 番兵
  Container<kListEntryNum> *_last;
};

static void membench8() {
  if (!is_knl()) {
    return;
  }

  // int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();

  {
    static volatile int l1 = 0, l2 = 0, l3 = 0;
    sync(l1, l2, l3);
  }
  if (apicid == 0) {
    gtty->CprintfRaw("bench start\n");
  }
  {
    static volatile int tmp = 0;
    if (apicid == 0 || apicid == 8) {
      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(2, l1, l2, l3);
      }
      uint64_t t1;
      if (apicid == 0) {
        t1 = timer->ReadMainCnt();
      }
      for (int i = 0; i < 100; i++) {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(2, l1, l2, l3);
      }
      if (apicid == 0) {
        gtty->CprintfRaw("<%d> ", (((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod())) / 1000);
        tmp = 1;
      }
    } else if (apicid == 24) {
      // int i = 0;
      while(__sync_fetch_and_or(&tmp, 0) == 0) {
        // for (int j = 0; j < 1024; j++) {
        //   for (int k = 0; k < 1024; k++) {
        //     buf[k] = i * j;
        //   }
        // }
      }
      for (int j = 0; j < 256; j++) {
        CpuId cpuid_(j);
        apic_ctrl->SendIpi(cpuid_.GetApicId());
      }
    } else {
      asm volatile("hlt;");
    }
  }
  {
    static volatile int tmp = 0;
    if (apicid == 0 || apicid == 8) {
      {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(2, l1, l2, l3);
      }
      uint64_t t1;
      if (apicid == 0) {
        t1 = timer->ReadMainCnt();
      }
      for (int i = 0; i < 100; i++) {
        static volatile int l1 = 0, l2 = 0, l3 = 0;
        sync2(2, l1, l2, l3);
      }
      if (apicid == 0) {
        gtty->CprintfRaw("<%d> ", (((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod())) / 1000);
        tmp = 1;
      }
    } else if (apicid == 295) {
      // int i = 0;
      while(__sync_fetch_and_or(&tmp, 0) == 0) {
        // for (int j = 0; j < 1024; j++) {
        //   for (int k = 0; k < 1024; k++) {
        //     buf[k] = i * j;
        //   }
        // }
      }
      for (int j = 0; j < 256; j++) {
        CpuId cpuid_(j);
        apic_ctrl->SendIpi(cpuid_.GetApicId());
      }
    } else {
      asm volatile("hlt;");
    }
  }
}

// for Xeon E3
static void membench9() {

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();

  {
    static volatile int l1 = 0, l2 = 0, l3 = 0;
    sync(l1, l2, l3);
  }

  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>");
  }
  
  {
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t1 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t1 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync_read(l1, l2, l3);
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    static SyncE3 sync_;
    sync_.Init();

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t2 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      sync_.Do();
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    static SyncReadE3 sync_;
    sync_.Init();

    {
      static volatile int l1 = 0, l2 = 0, l3 = 0;
      sync(l1, l2, l3);
    }
  
    uint64_t t2 = timer->ReadMainCnt();
    for (int i = 0; i < 100; i++) {
      sync_.Do();
    }
    if (cpuid == 0) {
      gtty->CprintfRaw("<%d us> ", ((timer->ReadMainCnt() - t2) * timer->GetCntClkPeriod()) / 1000);
    }
  }
}


void register_membench_callout() {
  static int id = 0;
  auto callout_ = make_sptr(new Callout);
  callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout){
          int cpuid_ = cpu_ctrl->GetCpuId().GetRawId();
          if (id != cpuid_) {
            task_ctrl->RegisterCallout(make_sptr(callout), 1000);
            return;
          }
          membench();
          id++;
        }, make_wptr(callout_))));
  task_ctrl->RegisterCallout(callout_, 10);
}

template<bool f, int i, class S, class List>
uint64_t func101() {
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

template<bool f, int i, class S, class List>
void func102() {
  static const int num = 8;
  uint64_t results[num];
  for (int j = 0; j < num; j++) {
    results[j] = func101<f, i, S, List>();
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
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("<%c %d(%d) %lld(%lld) us> ", f ? 'M' : 'C', sizeof(Container<i>), i, avg, variance);
  }
}

template<template<int>class List, class S, bool f, int i>
void func10() {
  // func102<f, i, S, List<i>>();
  func102<f, i, S, LinkedList2<SimpleSpinLock, i>>();
  // func102<f, i, S, LinkedList2<SimpleSpinLockR, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock1<37>, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock2<37>, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock1R<37>, i>>();
  // func102<f, i, S, LinkedList2<McSpinLock2R, i>>();
  func102<f, i, S, LinkedList2<McSpinLock2R2, i>>();
}

template<template<int>class List, class S, bool f, int i, int j, int... Num>
void func10() {
  func10<List, S, f, i>();
  func10<List, S, f, j, Num...>();
}

// リスト、要素数可変比較版
template<template<int>class List, class S>
static void membench10() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  if (cpuid == 0) {
    gtty->CprintfRaw("start >>>\n");
  }
  func10<List, S, true, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280>(); 
  // func10<List, S, false, 100, 120, 140, 160, 180, 200, 220, 240, 260, 280, 300>(); 
}

template<int i>
struct func103 {
  static void func() {
    func102<false, i, SyncLow, LinkedList2<SimpleSpinLockR, i>>();
    func103<i + 10>::func();
  }
};

template<>
struct func103<300> {
  static void func() {
    func102<false, 300, SyncLow, LinkedList2<SimpleSpinLockR, 300>>();
  }
};

void register_membench2_callout() {
  auto callout_ = make_sptr(new Callout);
  callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout){
          if (is_knl()) {
            // membench5();
            // membench7();
            membench10<LinkedList3, SyncLow>();
          } else {
            func103<10>::func(); 
            // membench9();
          }
          if (false) {
            membench2();
            membench3();
            membench4();
            membench5();
            membench6();
            membench8();
            membench9();
          }
        }, make_wptr(callout_))));
  task_ctrl->RegisterCallout(callout_, 10);
}

