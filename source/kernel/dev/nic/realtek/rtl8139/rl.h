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
#pragma once
/*
 * This code is a devece driver for rtl8139 which is old realtek NIC.
 * Tested Environment:
 *    Qemu Emulation(only)
 *
 */


#include<polling.h>
#include<mem/paging.h>
#include<mem/virtmem.h>
#include<mem/physmem.h>
#include<tty.h>
#include<dev/netdev.h>
#include<global.h>
#include<stdint.h>


class Rtl8139: public DevPci{
public:
  Rtl8139(uint8_t bus, uint8_t device, uint8_t function) :DevPci(bus,device, function) { }

  static DevPci *InitPci(uint8_t bus, uint8_t device, uint8_t function);


  static const int kVendorId = 0x10ec;
  static const int kDeviceId = 0x8139;


  //Registers see datasheet p16
  static const int kRegTxAddr = 0x20; //dw * 4
  static const int kRegTxStatus = 0x10; //dw * 4
  static const int kRegTxConfig = 0x40; //dw
  static const int kRegRxAddr = 0x30;
  static const int kRegRxConfig = 0x44; //dw 
  static const int kRegRxCAP = 0x38;
  static const int kRegCommand = 0x37; //b
  static const int kRegIrStatus = 0x3e; //w
  static const int kRegIrMask = 0x3c; //w


  //Command  see datasheet p21
  static const uint8_t kCmdTxEnable = 0x4;
  static const uint8_t kCmdRxEnable = 0x8;
  static const uint8_t kCmdReset = 0x10;
  static const uint8_t kCmdRxBufEmpty = 0x1;


  volatile uint32_t *_mmioAddr = nullptr;


private:
  class Rlt8139Ethernet: public DevEthernet {

  };

  virtual void Attach() override;
  uint16_t ReadEeprom(uint16_t offset,uint16_t length);
}
