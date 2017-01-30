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

#pragma once

#include <cpu.h>

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

struct Sync2Low {
  int top_level_lock1;
  int top_level_lock2;
  int top_level_lock3;
  void Do(int tnum) {
    sync2_read(tnum, top_level_lock1, top_level_lock2, top_level_lock3);
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
