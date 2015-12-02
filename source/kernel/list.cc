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

#include "list.h"
#include "mem/virtmem.h"
#include <future>
#include <random>
#include <iostream>

#ifdef __UNIT_TEST__

static void mem_check(std::map<virt_addr, int> &mmap) {
  virt_addr addr = 0;
  for(auto it = mmap.begin(); it != mmap.end(); it++) {
    assert(addr <= it->first);
    assert(it->second > 0);
    addr += it->second;
  }
}

static void test1() {
  List<int> list;
  auto th1 = std::thread([&list]{
      try{
        int *i = list.Alloc();
        list.Free(i);
        assert(i == list.Alloc());
      } catch(const char *str) {
        std::cout<<str<<std::endl;
        assert(false);
      }
    });
  th1.join();
}

static void test2() {
  volatile int flag = 0;
  std::map<virt_addr, int> mmap;
  List<int> list;

  std::random_device seed1;
  std::mt19937 rnd(seed1());
  std::uniform_int_distribution<int> rnd1(900, 1000);
  int th_num = rnd1(rnd);
  std::thread ths[th_num];
  for(int i = 0; i < th_num; i++) {
    ths[i] = std::thread([&list, &mmap, &flag]{
        try{
          std::random_device seed2;
          std::mt19937 random(seed2());
          std::uniform_int_distribution<int> rnd2(15, 20);
          for(int j = 0; j < rnd2(random); j++) {
            int *addr = list.Alloc();
            while(!__sync_bool_compare_and_swap(&flag, 0, 1)) {}
            mmap[reinterpret_cast<virt_addr>(addr)] = sizeof(int);
            __sync_bool_compare_and_swap(&flag, 1, 0);
          }
        } catch(const char *str) {
          std::cout<<str<<std::endl;
          assert(false);
        }
      });
  }
  for(int i = 0; i < th_num; i++) {
    ths[i].join();
  }

  mem_check(mmap);
}

static void test3() {
  volatile int flag = 0;
  std::map<virt_addr, int> mmap;
  List<int> list;

  std::random_device seed1;
  std::mt19937 rnd(seed1());
  std::uniform_int_distribution<int> rnd1(9000, 10000);
  int th_num = rnd1(rnd);
  std::thread ths1[th_num], ths2[th_num];
  for(int i = 0; i < th_num; i++) {
    ths1[i] = std::thread([&list, &mmap, &flag]{
        try{
          std::random_device seed2;
          std::mt19937 random(seed2());
          std::uniform_int_distribution<int> rnd2(15, 20);
          for(int j = 0; j < rnd2(random); j++) {
            int *addr = list.Alloc();
            while(!__sync_bool_compare_and_swap(&flag, 0, 1)) {}
            mmap[reinterpret_cast<virt_addr>(addr)] = sizeof(int);
            flag = 0;
          }
        } catch(const char *str) {
          std::cout<<str<<std::endl;
          assert(false);
        }
      });
  }
  for(int i = 0; i < th_num; i++) {
    ths2[i] = std::thread([&list, &mmap, &flag]{
        try{
          std::random_device seed2;
          std::mt19937 random(seed2());
          int *addr = nullptr;
          while(!__sync_bool_compare_and_swap(&flag, 0, 1)) {}
          std::uniform_int_distribution<int> rnd2(0, mmap.size());
          auto it = mmap.begin();
          for(int j = 0; j < rnd2(random); j++) {
            if (it == mmap.end()) {
              break;
            }
            it++;
          }
          if (it != mmap.end()) {
            addr = reinterpret_cast<int *>(it->first);
            mmap.erase(it);
          }
          flag = 0;
          if (addr != nullptr) {
            list.Free(addr);
          }
        } catch(const char *str) {
          std::cout<<str<<std::endl;
          assert(false);
        }
      });
  }
  for(int i = 0; i < th_num; i++) {
    ths1[i].join();
  }
  for(int i = 0; i < th_num; i++) {
    ths2[i].join();
  }
}

void list_test() {
  test1();
  test2();
  test3();
}

#endif // __UNIT_TEST__
