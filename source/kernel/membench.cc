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
    second_level_lock2[apicid / kGroupCoreNum].Init();
    second_level_lock1[apicid / kGroupCoreNum].Do();
    second_level_lock3[apicid / kGroupCoreNum].Init();
    second_level_lock2[apicid / kGroupCoreNum].Do();
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
    second_level_lock2[apicid / kGroupCoreNum].Init();
    second_level_lock1[apicid / kGroupCoreNum].Do();
    second_level_lock3[apicid / kGroupCoreNum].Init();
    second_level_lock2[apicid / kGroupCoreNum].Do();
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
    for (int i = 0; i < kSubStructNum; i++) {
      second_level_lock1[i].Init();
      second_level_lock2[i].Init();
      second_level_lock3[i].Init();
    }
  }
  void Do() {
    uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();
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
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
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
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
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
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
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
    if (apicid % 4 == 0) {
      second_level_lock2[apicid / 8].Init();
      second_level_lock1[apicid / 8].Do();
      second_level_lock3[apicid / 8].Init();
      second_level_lock2[apicid / 8].Do();
      second_level_lock1[apicid / 8].Init();
      second_level_lock3[apicid / 8].Do();
    }
    third_level_lock2[apicid / 4].Init();
    third_level_lock1[apicid / 4].Do();
    third_level_lock3[apicid / 4].Init();
    third_level_lock2[apicid / 4].Do();
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

// 単純
class SimpleSpinLock {
public:
  SimpleSpinLock() {
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
class SimpleSpinLockRead {
public:
  SimpleSpinLockRead() {
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
class McSpinLock3 {
public:
  McSpinLock3() {
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
class McSpinLock4 {
public:
  McSpinLock4() {
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
private:
  struct Container {
    Container *next;
    int i[kListEntryNum];
  };
  L _lock;
  Container _first; // 番兵
  Container *_last;
};

template <class L>
using LinkedListFnum = LinkedList<L, 14>;

// リスト
static void membench7() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  // uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();

  static LinkedListFnum<SimpleSpinLock> _list;
  static LinkedListFnum<McSpinLock1<37>> _list2;
  static LinkedListFnum<McSpinLock2<37>> _list3;
  static LinkedListFnum<SimpleSpinLockRead> _list_sr;
  static LinkedListFnum<McSpinLock3<37>> _list4;
  static LinkedListFnum<McSpinLock4> _list5;
  LinkedListFnum<SimpleSpinLock> *list = &_list;
  LinkedListFnum<McSpinLock1<37>> *list2 = &_list2;
  LinkedListFnum<McSpinLock2<37>> *list3 = &_list3;
  LinkedListFnum<SimpleSpinLockRead> *list_sr = &_list_sr;
  LinkedListFnum<McSpinLock3<37>> *list4 = &_list4;
  LinkedListFnum<McSpinLock4> *list5 = &_list5;
   

  if (cpuid == 0) {
    gtty->CprintfRaw("(init)");
  }

  if (cpuid == 0) {
    new (list) LinkedListFnum<SimpleSpinLock>;
    new (list2) LinkedListFnum<McSpinLock1<37>>;
    new (list3) LinkedListFnum<McSpinLock2<37>>;
    new (list_sr) LinkedListFnum<SimpleSpinLockRead>;
    new (list4) LinkedListFnum<McSpinLock3<37>>;
    new (list5) LinkedListFnum<McSpinLock4>;
    for (int i = 1; i < 256 * 2000; i++) {
      list->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list2->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list3->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list_sr->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list4->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list5->Push(i);
    }
  }

  if (cpuid == 0) {
    gtty->CprintfRaw("start!>");
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<sync %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2ReadPhi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    {
      static Sync2ReadPhi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<syncread %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }


  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list->Get() != 0) {
    }

    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<s %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list_sr->Get() != 0) {
    }

    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<sr %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list2->Get() != 0) {
    }

    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<1 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list3->Get() != 0) {
    }

    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<2 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list4->Get() != 0) {
    }

    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<1sr %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }


  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list5->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<2sr %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }
}

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
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
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

// リスト、要素数可変比較版
static void membench10() {
  if (!is_knl()) {
    return;
  }

  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  // uint32_t apicid = cpu_ctrl->GetCpuId().GetApicId();

  static LinkedList<McSpinLock4, 3> _list1;
  static LinkedList<McSpinLock4, 5> _list13;
  static LinkedList<McSpinLock4, 7> _list14;
  static LinkedList<McSpinLock4, 10> _list15;
  static LinkedList<McSpinLock4, 11> _list17;
  static LinkedList<McSpinLock4, 12> _list18;
  static LinkedList<McSpinLock4, 13> _list19;
  static LinkedList<McSpinLock4, 14> _list2;
  static LinkedList<McSpinLock4, 15> _list3;
  static LinkedList<McSpinLock4, 20> _list4;
  static LinkedList<McSpinLock4, 40> _list5;
  static LinkedList<McSpinLock4, 80> _list6;
  LinkedList<McSpinLock4, 3> *list1 = &_list1;
  LinkedList<McSpinLock4, 5> *list13 = &_list13;
  LinkedList<McSpinLock4, 7> *list14 = &_list14;
  LinkedList<McSpinLock4, 10> *list15 = &_list15;
  LinkedList<McSpinLock4, 11> *list17 = &_list17;
  LinkedList<McSpinLock4, 12> *list18 = &_list18;
  LinkedList<McSpinLock4, 13> *list19 = &_list19;
  LinkedList<McSpinLock4, 14> *list2 = &_list2;
  LinkedList<McSpinLock4, 15> *list3 = &_list3;
  LinkedList<McSpinLock4, 20> *list4 = &_list4;
  LinkedList<McSpinLock4, 40> *list5 = &_list5;
  LinkedList<McSpinLock4, 80> *list6 = &_list6;
   

  if (cpuid == 0) {
    gtty->CprintfRaw("(init)");
  }

  if (cpuid == 0) {
    new (list1) LinkedList<McSpinLock4, 3>;
    new (list13) LinkedList<McSpinLock4, 5>;
    new (list14) LinkedList<McSpinLock4, 7>;
    new (list15) LinkedList<McSpinLock4, 10>;
    new (list17) LinkedList<McSpinLock4, 11>;
    new (list18) LinkedList<McSpinLock4, 12>;
    new (list19) LinkedList<McSpinLock4, 13>;
    new (list2) LinkedList<McSpinLock4, 14>;
    new (list3) LinkedList<McSpinLock4, 15>;
    new (list4) LinkedList<McSpinLock4, 20>;
    new (list5) LinkedList<McSpinLock4, 40>;
    new (list6) LinkedList<McSpinLock4, 80>;
    for (int i = 1; i < 256 * 2000; i++) {
      list1->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list13->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list14->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list15->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list17->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list18->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list19->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list2->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list3->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list4->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list5->Push(i);
    }
    for (int i = 1; i < 256 * 2000; i++) {
      list6->Push(i);
    }
  }

  if (cpuid == 0) {
    gtty->CprintfRaw("start!>");
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list1->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<3 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list13->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<5 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

    {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list14->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<7 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list15->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<10 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

    {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list17->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<11 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

      {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list18->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<12 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

        {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list19->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<13 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list2->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<14 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list3->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<15 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list4->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<20 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list5->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<40 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }

  {
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    uint64_t t1;
    if (cpuid == 0) {
      t1 = timer->ReadMainCnt();
    }
  
    while(list6->Get() != 0) {
    }
    
    {
      static Sync2Phi sync={0};
      sync.Do();
    }

    if (cpuid == 0) {
      gtty->CprintfRaw("<80 %d us> ", ((timer->ReadMainCnt() - t1) * timer->GetCntClkPeriod()) / 1000);
    }
  }
}

void register_membench2_callout() {
  int cpuid = cpu_ctrl->GetCpuId().GetRawId();
  auto callout_ = make_sptr(new Callout);
  callout_->Init(make_uptr(new Function<wptr<Callout>>([](wptr<Callout> callout){
          if (is_knl()) {
            // membench5();
            // membench7();
            membench10();
          } else {
            membench9();
          }
          if (false) {
            membench2();
            membench3();
            membench4();
            membench5();
            membench6();
            membench7();
            membench8();
          }
        }, make_wptr(callout_))));
  task_ctrl->RegisterCallout(callout_, 10);
}

