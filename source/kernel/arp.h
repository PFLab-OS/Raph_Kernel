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

#include <stdint.h>
#include <stdlib.h>
#include <dev/netdev.h>
#include <list.h>

class ArpTable {
public:
  void Set(uint32_t ip_addr, uint8_t *hw_addr, NetDev *dev) {
    auto iter = arp_table.GetBegin();
    while(!iter.IsNull()) {
      if ((*iter)->ip_addr == ip_addr) {
        memcpy((*iter)->hw_addr, hw_addr, 6);
        (*iter)->dev = dev;
        return;
      }
      iter = iter->GetNext();
    }
    arp_table.PushBack(ip_addr, hw_addr, dev);
  }
  NetDev *Search(uint32_t ip_addr, uint8_t *hw_addr) {
    auto iter = arp_table.GetBegin();
    while(!iter.IsNull()) {
      if ((*iter)->ip_addr == ip_addr) {
        memcpy(hw_addr, (*iter)->hw_addr, 6);
        return (*iter)->dev;
      }
      iter = iter->GetNext();
    }
    return nullptr;
  }
private:
  struct ArpEntry {
    ArpEntry(uint32_t ip_addr_, uint8_t *hw_addr_, NetDev *dev_) {
      ip_addr = ip_addr_;
      memcpy(hw_addr, hw_addr_, 6);
      dev = dev_;
    }
    uint32_t ip_addr;
    uint8_t hw_addr[6];
    NetDev *dev;
  };
  List<ArpEntry> arp_table;
};
extern ArpTable *arp_table;
