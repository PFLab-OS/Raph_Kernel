/*
 *
 * Copyright (c) 2016 Project Raphine
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

#ifndef __RAPH_KERNEL_DEV_POLLING_H__
#define __RAPH_KERNEL_DEV_POLLING_H__
#include "timer.h"
#include "global.h"
#include "raph.h"

class Polling {
 public:
  virtual void Handle() = 0;
};

class PollingCtrl {
 public:
  PollingCtrl() {
    for(int i = 0; i < 100; i++) {
      _handlers[i] = nullptr;
    }
  }
  void Register(Polling *p) {
    for(int i = 0; i < 100; i++) {
      if (_handlers[i] == nullptr) {
        _handlers[i] = p;
        return;
      }
    }
    kassert(false);
  }
  void Remove(Polling *p) {
    for(int i = 0; i < 100; i++) {
      if (_handlers[i] == p) {
        _handlers[i] = nullptr;
        return;
      }
    }
  }
  void HandleAll() {
    while(true) {
      for(int i = 0; i < 100; i++) {
        if (_handlers[i] == nullptr) {
          continue;
        }
        _handlers[i]->Handle();
      }
      timer->BusyUwait(1000);
    }
  }
 private:
  Polling *_handlers[100]; // TODO なんとかする
};


#endif /* __RAPH_KERNEL_DEV_POLLING_H__ */
