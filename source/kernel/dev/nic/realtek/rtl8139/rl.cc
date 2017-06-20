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
#include <dev/eth.h>
#include <dev/pci.h>
#include <global.h>
#include <stdint.h>

#include "rl.h"
 struct Packet {
  public:
    Packet() {}
    size_t len;
    uint8_t *GetBuffer() {
      return _data;
    }
  private:
    uint8_t _data[MCLBYTES];  // only accessed via buf
  };
    Packet pp;
DevPci *Rtl8139::InitPci(uint8_t bus,uint8_t device,uint8_t function){
  Rtl8139 *dev = new Rtl8139(bus,device,function);
  uint16_t vid = dev->ReadPciReg<uint16_t>(PciCtrl::kVendorIDReg);
  uint16_t did = dev->ReadPciReg<uint16_t>(PciCtrl::kDeviceIDReg);

  if( vid == kVendorId && did == kDeviceId ){
    return dev;
  }

  delete dev;
  return nullptr;
}

template<>
void Rtl8139::WriteReg(uint32_t offset,uint8_t data){
  outb(_mmio_addr + offset,data);
}

template<>
void Rtl8139::WriteReg(uint32_t offset,uint16_t data){
  outw(_mmio_addr + offset,data);
}

template<>
void Rtl8139::WriteReg(uint32_t offset,uint32_t data){
  outl(_mmio_addr + offset,data);
}

template<>
uint32_t Rtl8139::ReadReg(uint32_t offset){
  return inl(_mmio_addr + offset);
}

template<>
uint16_t Rtl8139::ReadReg(uint32_t offset){
  return inw(_mmio_addr + offset);
}

template<>
uint8_t Rtl8139::ReadReg(uint32_t offset){
  return inb(_mmio_addr + offset);
}

void Rtl8139::Attach(){
  uint32_t temp_mmio = ReadPciReg<uint32_t>(PciCtrl::kBaseAddressReg0);
  _mmio_addr = (temp_mmio & (~0b11));

  //Enable BusMastering
  WritePciReg<uint16_t>(PciCtrl::kCommandReg,ReadPciReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  //Software Reset
  WriteReg<uint8_t>(kRegCommand,kCmdReset); 
  while((ReadReg<uint8_t>(0x37) & 0x10) != 0) { }
  
  WriteReg<uint8_t>(kRegCommand,kCmdTxEnable | kCmdRxEnable);


  _eth.Setup();

  _eth.SetRxTxConfigRegs();

  WriteReg<uint8_t>(kRegCommand,kCmdTxEnable | kCmdRxEnable);

}

uint16_t Rtl8139::ReadEeprom(uint16_t offset,uint16_t length){
  //cf http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html#:309

  WriteReg<uint8_t>(kReg93C46Cmd,0x80); 
  WriteReg<uint8_t>(kReg93C46Cmd,0x08); 

  int read_cmd = offset | (6 << length);
  
  for(int i= 4 + length;i >= 0;i--){
    int dataval = (read_cmd & (1 << i))? 2 : 0;
    WriteReg<uint8_t>(kReg93C46Cmd,0x80 | 0x08 | dataval); 
    ReadReg<uint16_t>(kReg93C46Cmd);
    WriteReg<uint8_t>(kReg93C46Cmd,0x80 | 0x08 | dataval | 0x4);
    ReadReg<uint16_t>(kReg93C46Cmd);
  }
  WriteReg<uint8_t>(kReg93C46Cmd,0x80 | 0x08);
  ReadReg<uint16_t>(kReg93C46Cmd);

  uint16_t retval = 0;
  for(int i = 16;i > 0;i--){
    WriteReg<uint8_t>(kReg93C46Cmd,0x80 | 0x08 | 0x4);
    ReadReg<uint16_t>(kReg93C46Cmd);
    retval = (retval << 1) | ((ReadReg<uint8_t>(kReg93C46Cmd) & 0x01) ? 1 : 0);
    WriteReg<uint8_t>(kReg93C46Cmd,0x80 | 0x08);
    ReadReg<uint16_t>(kReg93C46Cmd);
  }

  WriteReg<uint8_t>(kReg93C46Cmd,~(0x80 | 0x08));
  return retval;
}

void Rtl8139::Rtl8139Ethernet::PollingHandler(Rtl8139 *that){
  uint8_t *payload,*data;
  uint16_t length;
  Packet *packet;
  
  //Rx
  while(true){
    if(that->ReadReg<uint8_t>(that->kRegCommand) & that->kCmdRxBufEmpty){
        break;
    }
    data = reinterpret_cast<uint8_t*>(that->_eth.RxBuffer.GetVirtAddr() + that->_eth.RxBufferOffset);
    length = *(uint16_t*)(data + 2); // it contains CRC'S 4byte
    payload = data + 4;

    that->_eth.RxBufferOffset = ((length + that->_eth.RxBufferOffset + 4 + 3) % (64 * 1024 + 16)) & ~0b11;
    that->WriteReg<uint16_t>(that->kRegRxCAP,that->_eth.RxBufferOffset - 0x10);

    if (that->_eth._rx_reserved.Pop(packet)) {
      memcpy(packet->GetBuffer(),payload,length);
      packet->len = length;
      if (!that->_eth._rx_buffered.Push(packet)) {
        kassert(that->_eth._rx_reserved.Push(packet));
      }
    }
  }
  //Tx 
  for(int i = 0;i < 4;i++){
    uint32_t tx_status = that->ReadReg<uint32_t>(kRegTxStatus + i*4);
    if(tx_status & (1 << 15)){
      that->_eth.TxDescriptorStatus |= (1 << i);
    }
  }

  return;
}

void Rtl8139::Rtl8139Ethernet::UpdateLinkStatus(){
  uint8_t cmd = _master.ReadReg<uint8_t>(kRegCommand);
  if((cmd & kCmdRxEnable) && (cmd & kCmdRxEnable)){
    SetStatus(LinkStatus::kUp);
  }else{
    SetStatus(LinkStatus::kDown);
  }
}

void Rtl8139::Rtl8139Ethernet::GetEthAddr(uint8_t *buffer){
    uint16_t tmp;
    for(int i=0;i<3;i++){
        tmp = _master.ReadEeprom(i + 7,6);
        buffer[i*2] = tmp % 0x100;
        buffer[i*2+1] = tmp >> 8;
    }
}

void Rtl8139::Rtl8139Ethernet::ChangeHandleMethodToPolling(){
  _master.WriteReg<uint16_t>(kRegIrMask,0);
  //TODO:disable LegacyInt

  _polling.Init(make_uptr(new Function<Rtl8139 *>(PollingHandler,&_master)));
  _polling.Register(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance));
}

void Rtl8139::Rtl8139Ethernet::ChangeHandleMethodToInt(){
  _polling.Remove();

  _master.WriteReg<uint16_t>(kRegIrMask,0b101);
  _master.SetLegacyInterrupt(InterruptHandler,reinterpret_cast<void*>(&_master),Idt::EoiType::kIoapic);
}

void Rtl8139::Rtl8139Ethernet::Transmit(void *buffer){
  Packet *packet = reinterpret_cast<Packet*>(buffer);
  uint32_t length = packet->len;
  uint8_t *buf = packet->GetBuffer();

  uint32_t entry = currentTxDescriptor;
  currentTxDescriptor = (currentTxDescriptor + 1)%4;

  if(!(TxDescriptorStatus & (1 << entry))){
    //bit立ってない＝TxOKがまだ
    //なんか処理する
    return;
  }
  //TxOK
  memcpy(reinterpret_cast<uint8_t*>(TxBuffer[entry].GetVirtAddr()),buf,length);
  _master.WriteReg<uint32_t>(Rtl8139::kRegTxStatus + entry*4,((256 << 11) & 0x003f0000) | length);  //256 means http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html#:56
  TxDescriptorStatus = TxDescriptorStatus & ~(1 << entry);

}

bool Rtl8139::Rtl8139Ethernet::IsLinkUp(){
  if(GetStatus() == LinkStatus::kUp){
    return true;
  }else{
    return false;
  }
} 

void Rtl8139::Rtl8139Ethernet::InterruptHandler(void *p){
  Rtl8139 *that = reinterpret_cast<Rtl8139*>(p);
  uint16_t status = that->ReadReg<uint16_t>(that->kRegIrStatus);
  uint32_t length;
  uint8_t *payload,*data;
  Packet *packet;

  //ビットが立っているところに1を書き込むとクリア
  that->WriteReg<uint16_t>(that->kRegIrStatus,status);

  if(status & 0b01){
    //RxOK
    if(!(that->ReadReg<uint8_t>(that->kRegCommand) & that->kCmdRxBufEmpty)){
      data = reinterpret_cast<uint8_t*>(that->_eth.RxBuffer.GetVirtAddr() + that->_eth.RxBufferOffset);
      length = *(uint16_t*)(data + 2); // it contains CRC'S 4byte
      payload = data + 4; 

      if(that->_eth._rx_reserved.Pop(packet)) {
        packet->len = length;
        memcpy(packet->GetBuffer(),payload,length);
        if(!that->_eth._rx_buffered.Push(packet)) {
          kassert(that->_eth._rx_reserved.Push(packet));
        }
      }

      that->_eth.RxBufferOffset = ((length + that->_eth.RxBufferOffset + 4 + 3) % (64 * 1024 + 16)) & ~0b11;
      that->WriteReg<uint16_t>(that->kRegRxCAP,that->_eth.RxBufferOffset - 0x10);
    }
  }
  if(status & 0b100){
    //TxOK
    for(int i = 0;i < 4;i++){
      uint32_t tx_status = that->ReadReg<uint32_t>(kRegTxStatus + i*4);
      if(tx_status & (1 << 15)){
        //OWN bit, transmit to FIFO
        that->_eth.TxDescriptorStatus |= (1 << i);
      }
    }
  }
}

void Rtl8139::Rtl8139Ethernet::SetRxTxConfigRegs(){
  //受信バッファ設定
  //physmem_ctrl->Alloc(RxBuffer,17 * 1024);
  physmem_ctrl->Alloc(RxBuffer,PagingCtrl::kPageSize * 17);
  _master.WriteReg<uint32_t>(kRegRxAddr,RxBuffer.GetAddr());
  //Receive Configuration Registerの設定
  //７bit目はwrapで最後にあふれた時に、リングの先頭に戻るか(０)、そのままはみでるか(１)
  //1にするなら余裕を持って確保すること
  _master.WriteReg<uint32_t>(kRegRxConfig,0xf | (1<<7) | (1<<5) | (0b11 << 11));
  //_master.WriteReg<uint32_t>(kRegRxConfig,0xf | (1<<7) | (1<<5));
      //0b11 means 64KB + 16 bytes 上ではwrap用の余分含めて66KB確保してある 
  
  for(int i = 0;i < 4;i++){
    physmem_ctrl->Alloc(TxBuffer[i],PagingCtrl::kPageSize);
    _master.WriteReg<uint32_t>(kRegTxAddr + i*4,TxBuffer[i].GetAddr());
  }
  //CRCはハード側でつけてくれる
  _master.WriteReg<uint32_t>(kRegTxConfig,4 << 8);
}

void Rtl8139::Rtl8139Ethernet::Setup(){
  _master.WriteReg<uint16_t>(kRegIrMask,0b101);
  _master.SetLegacyInterrupt(InterruptHandler,reinterpret_cast<void*>(&_master),Idt::EoiType::kIoapic);

  InitTxPacketBuffer();
  InitRxPacketBuffer();
  SetStatus(LinkStatus::kUp);

}

