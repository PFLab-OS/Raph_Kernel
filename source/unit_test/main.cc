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

#include "test.h"
#include <raph.h>
#include <iostream>
#include <string>
#include <typeinfo>
#include <cxxabi.h>
#include <iomanip>
#include <thread>
#include <vector>
#include <future>
#include <sys/time.h>

using namespace std;

vector<Tester *> tests __attribute__((init_priority(101)));

Tester::Tester() {
  tests.push_back(this);
}

int main(int argc, char *argv[]) {
  uint32_t passed = 0;
  for(auto test = tests.begin(); test != tests.end(); ++test) {
    bool rval = false;
    bool timeout = false;
    struct timeval s, e;
    std::thread th([&]{
        try {
          gettimeofday(&s, NULL);
          rval = (*test)->Test();
          gettimeofday(&e, NULL);
        } catch (ExceptionAssertionFailure t) {
          t.Show();
        } catch(...) {
          cout << "\x1b[31munknown exception!\x1b[0m" << endl;
        }
      });
    auto future = std::async(std::launch::async, &std::thread::join, &th);
    if (future.wait_for(chrono::seconds(10)) == future_status::timeout) {
      timeout = true;
      rval = false;
    }
    if (rval) {
      passed++;
    }
    const type_info& id = typeid(**test);
    int stat;
    char *name = abi::__cxa_demangle(id.name(),0,0,&stat);
    if (name != nullptr) {
      if (rval) {
        cout << "\x1b[32m";
      } else {
        cout << "\x1b[31m";
      }
      if (stat == 0) {
        cout << right << setw(50) << name;
      } else {
        cout << right << setw(50) << "Unknown";
      }
      if (rval) {
        cout << "\x1b[0m [" << (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6 << "s]";
      } else {
        if (timeout) {
          cout << "\x1b[31m [timeout]";
        } else {
          cout << "\x1b[31m [---]";
        }
      }
      cout << endl;
      free(name);
    }
    fflush(stdout);
    if (timeout) {
      cout << "\x1b[31mStop tests due to timeout. [" << passed << "/" << tests.size() << "]\x1b[0m" << endl << endl;
      fflush(stdout);
      std::terminate();
      return 1;
    }
  }
  if (passed == tests.size()) {
    cout << "\x1b[32mAll tests have passed!! [" << passed << "/" << tests.size() << "]\x1b[0m" << endl << endl;
    return 0;
  } else {
    cout << "\x1b[31mSome tests have failed!! [" << passed << "/" << tests.size() << "]\x1b[0m" << endl << endl;
    fflush(stdout);
    std::terminate();
    return 1;
  }
}
