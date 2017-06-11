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
 * Author: mumumu
 * 
 */

#include<polling.h>
#include<mem/paging.h>
#include<mem/virtmem.h>
#include<mem/physmem.h>
#include<tty.h>
#include<dev/netdev.h>
#include<dev/pci.h>
#include<global.h>
#include<stdint.h>

#include"rl.h"

DevPci *Rtl8139::InitPCI(uint8_t bus, uint8_t device, uint8_t function){
  Rtl8139 *dev = new Rtl8139(bus,device,function);
  uint16_t vid = dev->ReadReg<uint16_t>(PciCtrl::kVendorIDReg);
  uint16_t did = dev->ReadReg<uint16_t>(PciCtrl::kDeviceIDReg);

  if( vid == kVendorId && did == kDeviceId ){
    return dev;
  }

  delete dev;
  return nullptr;
}

void Rtl8139::Attach(){
  volatile uint32_t temp_mmio = ReadReg<uint32_t>(kBaseAddressReg0);
  _mmioAddr = (temp_mmio & ( ~3));

  //Enable BusMastering
  WriteReg<uint16_t>(PciCtrl::kCommandReg, ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

//


}
