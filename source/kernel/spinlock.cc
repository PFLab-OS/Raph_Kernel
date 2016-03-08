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

#include "spinlock.h"

#ifdef __UNIT_TEST__

#include <iostream>
#include <cassert>
#include <sys/time.h>

class SpinLock1 : public SpinLock {
private:
  virtual SpinLockId getLockId() {
    return static_cast<SpinLockId>(1);
  }
};

class SpinLock2 : public SpinLock {
private:
  virtual SpinLockId getLockId() {
    return static_cast<SpinLockId>(2);
  }
};

class SpinLock3 : public SpinLock {
private:
  virtual SpinLockId getLockId() {
    return static_cast<SpinLockId>(3);
  }
};

static void test1() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  auto th1 = std::thread([&lock1]{
      try {
        READ_LOCK(lock1) {
          sleep(2);
        }
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  sleep(1);
  assert(lock1.GetFlag() == 0);
  th1.join();
  assert(lock1.GetFlag() == 0);
}


static void test2() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  SpinLock2 lock2;
  auto th1 = std::thread([&lock1, &lock2]{
      try {
        READ_LOCK(lock2) {
          READ_LOCK(lock1) {
          }
        }
      } catch(const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  th1.join();
}

static void test3() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  SpinLock2 lock2;
  auto th1 = std::thread([&lock1, &lock2]{
      try {
        READ_LOCK(lock1) {
          READ_LOCK(lock2) {
            assert(false);
          }
        }
      } catch(const char *) {
      }
    });
  th1.join();
}

static void test4() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  auto th1 = std::thread([&lock1]{
      try {
        WRITE_LOCK(lock1) {
          sleep(2);
        }
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  sleep(1);
  assert(lock1.GetFlag() == 1);
  th1.join();
  assert(lock1.GetFlag() == 2);
}


static void test5() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  SpinLock2 lock2;
  auto th1 = std::thread([&lock1, &lock2]{
      try {
        WRITE_LOCK(lock2) {
          WRITE_LOCK(lock1) {
          }
        }
      } catch(const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  th1.join();
}

static void test6() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  SpinLock2 lock2;
  auto th1 = std::thread([&lock1, &lock2]{
      try {
        WRITE_LOCK(lock1) {
          WRITE_LOCK(lock2) {
          }
        }
      } catch(const char *) {
      }
    });
  th1.join();
}

static void test7() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  SpinLock2 lock2;
  auto th1 = std::thread([&lock1, &lock2]{
      try {
        WRITE_LOCK(lock2) {
          READ_LOCK(lock1) {
          }
        }
      } catch(const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  th1.join();
}

static void test8() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  SpinLock2 lock2;
  auto th1 = std::thread([&lock1, &lock2]{
      try {
        WRITE_LOCK(lock1) {
          READ_LOCK(lock2) {
            assert(false);
          }
        }
      } catch(const char *) {
      }
    });
  th1.join();
}

static void test9() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  int i = 0;
  volatile int j = 0;
  struct timeval tv0, tv1;
  auto th1 = std::thread([&lock1, &i, &j]{
      try {
        j++;
        while(j != 3) {}
        READ_LOCK(lock1) {
          i++;
          sleep(2);
        }
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  auto th2 = std::thread([&lock1, &i, &j]{
      try {
        j++;
        while(j != 3) {}
        READ_LOCK(lock1) {
          i++;
          sleep(2);
        }
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  j++;
  while(j != 3) {}
  gettimeofday(&tv0, NULL);
  sleep(1);
  assert(lock1.GetFlag() == 0);
  th1.join();
  th2.join();
  gettimeofday(&tv1, NULL);
  assert((tv1.tv_sec - tv0.tv_sec) * 1000 * 1000 + (tv1.tv_usec - tv0.tv_usec) < 3 * 1000 * 1000);
  assert(lock1.GetFlag() == 0);
  assert(i == 2);
}

static void test10() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  volatile int j = 0;
  volatile int k = 0;
  auto th1 = std::thread([&lock1, &j, &k]{
      try {
        j++;
        while(j != 3) {}
        int k1 = 0, k2 = 0;
        READ_LOCK(lock1) {
          k1 = k;
          sleep(2);
          k2 = k;
        }
        assert(k1 == k2);
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  auto th2 = std::thread([&lock1, &j, &k]{
      try {
        j++;
        while(j != 3) {}
        int k1 = 0, k2 = 0;
        READ_LOCK(lock1) {
          k1 = k;
          sleep(2);
          k2 = k;
        }
        assert(k1 == k2);
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  auto th3 = std::thread([&lock1, &j, &k]{
      try {
        j++;
        while(j != 3) {}
        sleep(1);
        WRITE_LOCK(lock1) {
          k++;
        }
      } catch (const char *str) {
        std::cout<<"error: "<<str<<std::endl;
        assert(false);
      }
    });
  th1.join();
  th2.join();
  th3.join();
  assert(lock1.GetFlag() == 2);
}

static void test11() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  volatile int k = 0;
  std::thread ths1[10000];
  std::thread ths2[1000];
  for(int i = 0; i < 10000; i++) {
    ths1[i] = std::thread([&lock1, &k]{
        try {
          for(int l = 0; l < 100; l++) {
            int k1 = 0, k2 = 0;
            READ_LOCK(lock1) {
              k1 = k;
              for(int j = 0; j < 1000; j++) {
                asm volatile("nop");
              }
              k2 = k;
            }
            assert(k1 == k2);
          }
        } catch (const char *str) {
          std::cout<<"error: "<<str<<std::endl;
          assert(false);
        }
      });
  }
  for(int i = 0; i < 1000; i++) {
    ths2[i] = std::thread([&lock1, &k]{
        try {
          for(int l = 0; l < 100; l++) {
            WRITE_LOCK(lock1) {
              k++;
            }
          }
        } catch (const char *str) {
          std::cout<<"error: "<<str<<std::endl;
          assert(false);
        }
      });
  }
  for(int i = 0; i < 10000; i++) {
    ths1[i].join();
  }
  for(int i = 0; i < 1000; i++) {
    ths2[i].join();
  }
}

static void test12() {
  SpinLockCtrlTest _spinlock_ctrl;
  spinlock_ctrl = &_spinlock_ctrl;
  SpinLock1 lock1;
  volatile int k = 0;
  std::thread ths[10000];
  for(int i = 0; i < 10000; i++) {
    ths[i] = std::thread([&lock1, &k]{
        try {
          int k1 = 0;
          RW_LOCK(lock1) {
          case ReadWriteLock::LockStatus::kReading:
            k1 = k;
            for(int j = 0; j < 10; j++) {
              asm volatile("nop");
            }
            continue;
          case ReadWriteLock::LockStatus::kReadEnd:
            for(int j = 0; j < 100; j++) {
              asm volatile("nop");
            }
            continue;
          case ReadWriteLock::LockStatus::kWriting:
            assert(k == k1);
            k++;
            continue;
          default:
            continue;
          }
        } catch (const char *str) {
          std::cout<<"error: "<<str<<std::endl;
          assert(false);
        }
      });
  }
  for(int i = 0; i < 10000; i++) {
    ths[i].join();
  }
}

void spinlock_test() {
  std::cout << "testing blocking mechanism. it will take few seconds." << std::endl;
  test1();
  test2();
  test3();
  test4();
  test5();
  test6();
  test7();
  test8();
  test9();
  test10();
  for(int i = 0; i < 100; i++) {
    test11();
  }
  for(int i = 0; i < 100; i++) {
    test12();
  }
  std::cout << "blocking mechanism test end." << std::endl;
}


#endif // __UNIT_TEST__

void SpinLock::Lock() {
  volatile unsigned int flag = GetFlag();
  while((flag % 2) == 1 || !SetFlag(flag, flag + 1)) {
    flag = GetFlag();
  }
}

void SpinLock::Unlock() {
  kassert((_flag % 2) == 0);
  _flag++;
}

int SpinLock::Trylock() {
  volatile unsigned int flag = GetFlag();
  if (((flag % 2) == 0) && SetFlag(flag, flag + 1)) {
    return 0;
  } else {
    return -1;
  }
}

ReadLock::ReadLock(SpinLock &lock) : _lock(lock), _tried(false) {
  _tmp_spid = spinlock_ctrl->getCurrentLock();
  assert(static_cast<int>(_tmp_spid)
         >= static_cast<int>(lock.getLockId()));
  spinlock_ctrl->setCurrentLock(lock.getLockId());
}

bool ReadLock::Check() {
  // 成功したらfalseを返す
  bool failed = !_tried || (_lock.GetFlag() != _flag);
  if (failed) {
    Wait();
  }
  _tried = true;
  return failed;
}

ReadLock::~ReadLock() {
  assert(static_cast<int>(spinlock_ctrl->getCurrentLock())
         == static_cast<int>(_lock.getLockId()));
  spinlock_ctrl->setCurrentLock(_tmp_spid);
}

WriteLock::WriteLock(SpinLock &lock) : _lock(lock), _tried(false) {
  _tmp_spid = spinlock_ctrl->getCurrentLock();
  assert(static_cast<int>(_tmp_spid)
         >= static_cast<int>(lock.getLockId()));
  spinlock_ctrl->setCurrentLock(lock.getLockId());
  _flag = lock.GetFlag();
  while((_flag % 2) == 1 || !lock.SetFlag(_flag, _flag + 1)) {
    _flag = lock.GetFlag();
  }
}

WriteLock::~WriteLock() {
  assert(static_cast<int>(spinlock_ctrl->getCurrentLock())
         == static_cast<int>(_lock.getLockId()));
  spinlock_ctrl->setCurrentLock(_tmp_spid);
  assert((_flag % 2) == 0);
  assert(_lock.SetFlag(_flag + 1, _flag + 2));
}

ReadWriteLock::ReadWriteLock (SpinLock &lock) : _status(LockStatus::kInit), _lock(lock) {
  _tmp_spid = spinlock_ctrl->getCurrentLock();
  assert(static_cast<int>(_tmp_spid)
         >= static_cast<int>(lock.getLockId()));
  spinlock_ctrl->setCurrentLock(lock.getLockId());
}
bool ReadWriteLock::Check() {
  switch(_status) {
  case LockStatus::kInit:
    _status = LockStatus::kReading;
    _lstatus = LockStatus::kReading;
    _flag = _lock.GetFlag();
    while((_flag % 2) == 1) {
      _flag = _lock.GetFlag();
    }
    break;
  case LockStatus::kReading:
    if (_lock.GetFlag() == _flag) {
      _status = LockStatus::kReadEnd;
      _lstatus = LockStatus::kInit;
      break;
    } else {
      _status = LockStatus::kInit;
      return Check();
    }
  case LockStatus::kReadEnd:
    if (!_lock.SetFlag(_flag, _flag + 1)) {
      _status = LockStatus::kInit;
      return Check();
    } else {
      _status = LockStatus::kWriting;
      _lstatus = LockStatus::kWriting;
      break;
    }
  case LockStatus::kWriting:
  default:
    return false;
  }
  return true;
}

ReadWriteLock::~ReadWriteLock() {
  assert(static_cast<int>(spinlock_ctrl->getCurrentLock())
         == static_cast<int>(_lock.getLockId()));
  spinlock_ctrl->setCurrentLock(_tmp_spid);
  switch(_lstatus) {
  case LockStatus::kWriting:
    assert((_flag % 2) == 0);
    assert(_lock.SetFlag(_flag + 1, _flag + 2));
    break;
  default:
    break;
  }
}
