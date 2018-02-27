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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
 * Author: Liva
 *
 */

#include <mem/virtmem.h>
#include <dev/netdev.h>
// #include <net/pstack.h>
#include <ptr.h>
#include <global.h>

NetDev::NetDev() {
  extern CpuId network_cpu;
  _tx_buffered.SetFunction(network_cpu,
                           make_uptr(new ClassFunction1<void, NetDev, void *>(
                               this, &NetDev::Transmit, nullptr)));
}

bool NetDevCtrl::RegisterDevice(NetDev *dev, const char *prefix) {
  if (_current_device_number < kMaxDevNumber) {
    // TODO: use sprintf
    char name[kNetworkInterfaceNameLen];
    strncpy(name, prefix, strlen(prefix));
    name[strlen(prefix)] = _current_device_number + '0';
    name[strlen(prefix) + 1] = '\0';

    // succeed to register
    dev->SetName(name);

    _dev_table[_current_device_number].device = dev;

    // allocate protocol stack
    // ProtocolStack *addr =
    // reinterpret_cast<ProtocolStack*>(system_memory_space->GetKernelVirtmemCtrl()->Alloc(sizeof(ProtocolStack)));
    // ProtocolStack *ptcl_stack = new(addr) ProtocolStack();
    // ptcl_stack->Setup();

    // _dev_table[_current_device_number].ptcl_stack = ptcl_stack;
    // dev->SetProtocolStack(ptcl_stack);
    // ptcl_stack->SetDevice(dev);

    _current_device_number += 1;

    return true;
  } else {
    // fail to register
    return false;
  }
}

NetDevCtrl::NetDevInfo *NetDevCtrl::GetDeviceInfo(const char *name) {
  for (uint32_t i = _current_device_number; i > 0; i--) {
    NetDev *dev = _dev_table[i - 1].device;

    // search device by network interface name
    if (dev != nullptr && !strncmp(dev->GetName(), name, strlen(name))) {
      return &_dev_table[i - 1];
    }
  }
  return nullptr;
}

uptr<Array<const char *>> NetDevCtrl::GetNamesOfAllDevices() {
  int j = 0;
  for (uint32_t i = 0; i < _current_device_number; i++) {
    if (_dev_table[i].device != nullptr) {
      j++;
    }
  }

  auto list = make_uptr(new Array<const char *>(j));

  j = 0;
  for (uint32_t i = 0; i < _current_device_number; i++) {
    NetDev *dev = _dev_table[i].device;

    if (dev != nullptr) {
      (*list)[j] = dev->GetName();
      j++;
    }
  }

  return list;
}
