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
#include <sys/time.h>

using namespace std;

Tester *tests[Tester::kMaxTests] = {};

Tester::Tester() {
  for (int i = 0; i < kMaxTests; i++) {
    if (tests[i] == nullptr) {
      tests[i] = this;
      return;
    }
  }
  std::cerr << "Not enough test slots!!" << std::endl;
}

int main(int argc, char *argv[]) {
  int passed = 0;
  for (int i = 0; i < Tester::kMaxTests; i++) {
    if (tests[i] == nullptr) {
      if (passed == i) {
        cout << "\x1b[32mAll tests have passed!! [" << passed << "/" << i << "]\x1b[0m" << endl;
      } else {
        cout << "\x1b[31mSome tests have failed!! [" << passed << "/" << i << "]\x1b[0m" << endl;
      }
      return 0;
    }
    bool rval = false;
    struct timeval s, e;
    try {
      gettimeofday(&s, NULL);
      rval = tests[i]->Test();
      gettimeofday(&e, NULL);
    } catch (ExceptionAssertionFailure t) {
      t.Show();
    } catch(...) {
      cout << "\x1b[31munknown exception!\x1b[0m" << endl;
    }
    if (rval) {
      passed++;
    }
    const type_info& id = typeid(*tests[i]);
    int stat;
    char *name = abi::__cxa_demangle(id.name(),0,0,&stat);
    if (name != nullptr) {
      if (rval) {
        cout << "\x1b[32m";
      } else {
        cout << "\x1b[31m";
      }
      if (stat == 0) {
        cout << std::right << std::setw(50) << name;
      } else {
        cout << std::right << std::setw(50) << "Unknown";
      }
      cout << "\x1b[0m";
      if (rval) {
        cout << " [" << (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6 << "s]";
      }
      cout << endl;
      free(name);
    }
    fflush(stdout);
  }
  cerr << "All tests have passed, but some tests were not executed..." << endl;
  return 1;
}
