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
 * Author: Levelfour
 * 
 */

#include "netdev.h"

const char *NetDevCtrl::kDefaultNetworkInterfaceName = "eth0";

bool NetDevCtrl::RegisterDevice(NetDev *dev, const char *name) {
  if(_curDevNumber < kMaxDevNumber) {
    // succeed to register
    _devTable[_curDevNumber++]->SetName(name);
    return true;
  } else {
    // fail to register
    return false;
  }
}

NetDev *NetDevCtrl::GetDevice(const char *name) {
  if(!name) name = kDefaultNetworkInterfaceName;
  for(uint32_t i = _curDevNumber; i > 0; i--) {
    NetDev *dev = _devTable[i];
    // search device by network interface name
    if(!strncmp(dev->GetName(), name, strlen(name))) {
      return dev;
    }
  }
  return nullptr;
}
