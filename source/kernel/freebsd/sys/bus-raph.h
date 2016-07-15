/*
 *
 * Copyright (c) 2016 Raphine Project
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

#include <sys/bus.h>
#include <raph.h>
#include <task.h>
#include <idt.h>
#include <global.h>
#include <dev/pci.h>

#ifndef __RAPH_KERNEL_FREEBSD_SYS_BUS_RAPH_H__
#define __RAPH_KERNEL_FREEBSD_SYS_BUS_RAPH_H__


// TODO
// PCI Specific
// should be BsdBus
class BsdDevPci : public DevPci {
public:
  class IntContainer {
  public:
    IntContainer() {
      Function func;
      func.Init(HandleSub, reinterpret_cast<void *>(this));
      // TODO cpuid
      _ctask.SetFunc(1, func);
    }
    void Handle() {
      if (_filter != nullptr) {
        _filter(_farg);
      }
      if (_ithread != nullptr) {
        _ctask.Inc();
      }
    }
    void SetFilter(driver_filter_t filter, void *arg) {
      _filter = filter;
      _farg = arg;
    }
    void SetIthread(driver_intr_t ithread, void *arg) {
      _ithread = ithread;
      _iarg = arg;
    }
  private:
    static void HandleSub(void *arg) {
      IntContainer *that = reinterpret_cast<IntContainer *>(arg);
      if (that->_ithread != nullptr) {
        that->_ithread(that->_iarg);
      }
    }
    CountableTask _ctask;
    
    driver_filter_t _filter = nullptr;
    void *_farg;
    
    driver_intr_t _ithread = nullptr;
    void *_iarg;
  };
  BsdDevPci(uint8_t bus, uint8_t device, uint8_t function) : DevPci(bus, device, function) {
  }
  virtual ~BsdDevPci() {
  }
  
  IntContainer *GetIntContainerStruct(int id) {
    if (_is_legacy_interrupt_enable) {
      if (id == 0) {
        if (_icontainer_list == nullptr) {
          SetupLegacyIntContainers();
        }
        return &_icontainer_list[id];
      } else {
        return NULL;
      }
    } else {
      if (id <= 0) {
        return NULL;
      } else {
        return &_icontainer_list[id - 1];
      }
    }
  }
  void SetupMsi() {
    ReleaseMsi();
    int count = GetMsiCount();
    if (count == 0) {
      return;
    }
    _icontainer_list = new IntContainer[count];
    int_callback callbacks[count];
    void *args[count];
    for (int i = 0; i < count; i++) {
      callbacks[i] = HandleSubInt;
      args[i] = reinterpret_cast<void *>(_icontainer_list + i);
    }
    // TODO cpuid
    int cpuid = 1;
    int vector = idt->SetIntCallback(cpuid, callbacks, args, count);
    SetMsi(cpuid, vector);
  }
  void ReleaseMsi() {
    if (_icontainer_list != nullptr) {
      // TODO 割り込み開放
      delete[] _icontainer_list;
    }
  }
private:
  void SetupLegacyIntContainers() {
    _icontainer_list = new IntContainer[1];
    for (int i = 0; i < 1; i++) {
      SetLegacyInterrupt(HandleSubLegacy, reinterpret_cast<void *>(_icontainer_list + i));
    }
  }
  static void HandleSubLegacy(void *arg) {
    IntContainer *icontainer = reinterpret_cast<IntContainer *>(arg);
    icontainer->Handle();
  }
  static void HandleSubInt(Regs *rs, void *arg) {
    IntContainer *icontainer = reinterpret_cast<IntContainer *>(arg);
    icontainer->Handle();
  }

  bool _is_legacy_interrupt_enable = true;
  IntContainer *_icontainer_list = nullptr;
};

#endif // __RAPH_KERNEL_FREEBSD_SYS_BUS_RAPH_H__
