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
 * Author: mumumu
 * 
 */

#include <polling.h>
#include <mem/paging.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <tty.h>
#include <dev/netdev.h>
#include <dev/pci.h>
#include <global.h>
#include <stdint.h>

#include "rl.h"

DevPci *Rtl8139::InitPCI(uint8_t bus,uint8_t device,uint8_t function){
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
  _mmio_addr = (temp_mmio & (~0b11));

  //Enable BusMastering
  WriteReg<uint16_t>(PciCtrl::kCommandReg,ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  //Software Reset
  outb(_mmio_addr+Command,kCmdReset); 
  while((inb(_mmio_addr + 0x37) & 0x10) != 0) { }

  outb(_mmio_addr + Command,kCmdTxEnable | kCmdRxEnable);



  //intrrupt
  //RxOK only (TODO :add interruption)
  outw(_mmio_addr + kRegIrMask,0x01);


  //Receive Configuration Registerの設定
  //７bit目はwrapで最後にあふれた時に、リングの先頭に戻るか(０)、そのままはみでるか(１)
  //1にするなら余裕を持って確保すること
  outl(_mmio_addr + RxConfig,0xf | (1<<7) | (1<<5));

  //txconig
  outl(_mmio_addr + kRegTxConfig,4 << 8);


  //受信バッファ設定
  PhysAddr recv_buf_addr;
  physmem_ctrl->Alloc(recv_buf_addr,PagingCtrl::kPageSize);
  outl(_mmio_addr + kRegRxAddr,recv_buf_addr.GetAddr());


  outb(_mmio_addr + Command,kCmdTxEnable|kCmdRxEnable);

 }

//TODO: あまりに汚いのでリファクタリングする
uint16_t Rtl8139::ReadEeprom(uint16_t offset,uint16_t length){
  //cf http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html#:309

      outb(_mmio_addr + kReg93C46Cmd,0x80); 
      outb(_mmio_addr + kReg93C46Cmd,0x08); 

      int read_cmd = offset | (6 << len);
      
      for(int i= 4 + len; i>= 0; i--){
          int dataval = (read_cmd & (1 << i))? 2 : 0;
          outb(_mmio_addr + kReg93C46Cmd,0x80 | 0x08 | dataval); 
          inw(_mmio_addr + kReg93C46Cmd);
          outb(_mmio_addr + kReg93C46Cmd,0x80 | 0x08 | dataval | 0x4);
          inw(_mmio_addr + kReg93C46Cmd);
      }
      outb(_mmio_addr + kReg93C46Cmd,0x80 | 0x08);
      inw(_mmio_addr + kReg93C46Cmd);

      uint16_t retval = 0;
      for(int i = 16;i > 0; i--){
          outb(_mmio_addr + kReg93C46Cmd,0x80 | 0x08 | 0x4);
          inw(_mmio_addr + kReg93C46Cmd);
          retval = (retval << 1) | ((inb(_mmio_addr + kReg93C46Cmd) & 0x01) ? 1 : 0);
          outb(_mmio_addr + kReg93C46Cmd,0x80 | 0x08);
          inw(_mmio_addr + kReg93C46Cmd);
      }

      outb(_mmio_addr + kReg93C46Cmd,~(0x80 | 0x08));
      return retval;
  }


}
