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

// not used

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
