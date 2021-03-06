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

template <>
void Rtl8139::WriteReg(uint32_t offset, uint8_t data) {
  outb(_mmio_addr + offset, data);
}

template <>
void Rtl8139::WriteReg(uint32_t offset, uint16_t data) {
  outw(_mmio_addr + offset, data);
}

template <>
void Rtl8139::WriteReg(uint32_t offset, uint32_t data) {
  outl(_mmio_addr + offset, data);
}

template <>
uint32_t Rtl8139::ReadReg(uint32_t offset) {
  return inl(_mmio_addr + offset);
}

template <>
uint16_t Rtl8139::ReadReg(uint32_t offset) {
  return inw(_mmio_addr + offset);
}

template <>
uint8_t Rtl8139::ReadReg(uint32_t offset) {
  return inb(_mmio_addr + offset);
}

DevPci *Rtl8139::InitPci(uint8_t bus, uint8_t device, uint8_t function) {
  Rtl8139 *dev = new Rtl8139(bus, device, function);
  uint16_t vid = dev->ReadPciReg<uint16_t>(PciCtrl::kVendorIDReg);
  uint16_t did = dev->ReadPciReg<uint16_t>(PciCtrl::kDeviceIDReg);

  if (vid == kVendorId && did == kDeviceId) {
    return dev;
  }

  delete dev;
  return nullptr;
}

void Rtl8139::Attach() {
  uint32_t temp_mmio = ReadPciReg<uint32_t>(PciCtrl::kBaseAddressReg0);
  _mmio_addr = (temp_mmio & (~0b11));

  _eth.Setup();

  _eth.SetRxTxConfigRegs();

  _eth.Start();
}

uint16_t Rtl8139::Rtl8139Ethernet::ReadEeprom(uint16_t offset,
                                              uint16_t length) {
  // cf http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html#:309

  _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x80);
  _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x08);

  int read_cmd = offset | (6 << length);

  for (int i = 4 + length; i >= 0; i--) {
    int dataval = (read_cmd & (1 << i)) ? 2 : 0;
    _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x80 | 0x08 | dataval);
    _master.ReadReg<uint16_t>(kReg93C46Cmd);
    _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x80 | 0x08 | dataval | 0x4);
    _master.ReadReg<uint16_t>(kReg93C46Cmd);
  }
  _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x80 | 0x08);
  _master.ReadReg<uint16_t>(kReg93C46Cmd);

  uint16_t retval = 0;
  for (int i = 16; i > 0; i--) {
    _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x80 | 0x08 | 0x4);
    _master.ReadReg<uint16_t>(kReg93C46Cmd);
    retval = (retval << 1) |
             ((_master.ReadReg<uint8_t>(kReg93C46Cmd) & 0x01) ? 1 : 0);
    _master.WriteReg<uint8_t>(kReg93C46Cmd, 0x80 | 0x08);
    _master.ReadReg<uint16_t>(kReg93C46Cmd);
  }

  _master.WriteReg<uint8_t>(kReg93C46Cmd, ~(0x80 | 0x08));
  return retval;
}

void Rtl8139::Rtl8139Ethernet::PollingHandler(Rtl8139 *that) {
  uint8_t *payload, *data;
  uint16_t length;
  Packet *packet;

  // Rx
  while (true) {
    if (!((that->ReadReg<uint8_t>(kRegCommand) & kCmdRxBufEmpty))) {
      goto tx;
    }
    data = reinterpret_cast<uint8_t *>(that->_eth._rx_buffer.GetVirtAddr() +
                                       that->_eth._rx_buffer_offset);
    length = *(uint16_t *)(data + 2);  // it contains CRC'S 4byte
    payload = data + 4;

    that->_eth._rx_buffer_offset =
        ((length + that->_eth._rx_buffer_offset + 4 + 3) % (64 * 1024 + 16)) &
        ~0b11;
    that->WriteReg<uint16_t>(kRegRxCAP, that->_eth._rx_buffer_offset - 0x10);

    if (that->_eth._rx_reserved.Pop(packet)) {
      memcpy(packet->GetBuffer(), payload, length);
      packet->len = length;
      if (!that->_eth._rx_buffered.Push(packet)) {
        kassert(that->_eth._rx_reserved.Push(packet));
      }
    }
  }
// Tx
tx:
  for (int i = 0; i < 4; i++) {
    uint32_t tx_status = that->ReadReg<uint32_t>(kRegTxStatus + i * 4);
    if (tx_status & (1 << 15)) {
      if (that->_eth._tx_buffered_blocked) {
        that->_eth._tx_buffered.UnBlock();
        that->_eth._tx_buffered_blocked = false;
      }
      that->_eth._tx_descriptor_status |= (1 << i);
    }
  }

  return;
}

void Rtl8139::Rtl8139Ethernet::UpdateLinkStatus() {
  uint8_t cmd = _master.ReadReg<uint8_t>(kRegCommand);
  if ((cmd & kCmdRxEnable) && (cmd & kCmdRxEnable)) {
    SetStatus(LinkStatus::kUp);
  } else {
    SetStatus(LinkStatus::kDown);
  }
}

void Rtl8139::Rtl8139Ethernet::GetEthAddr(uint8_t *buffer) {
  uint16_t tmp;
  for (int i = 0; i < 3; i++) {
    tmp = ReadEeprom(i + 7, 6);
    buffer[i * 2] = tmp % 0x100;
    buffer[i * 2 + 1] = tmp >> 8;
  }
}

void Rtl8139::Rtl8139Ethernet::ChangeHandleMethodToPolling() {
  _master.WriteReg<uint16_t>(kRegIrMask, 0);
  // TODO:disable LegacyInt

  _polling.Init(
      make_uptr(new Function1<void, Rtl8139 *>(PollingHandler, &_master)));
  _polling.Register(
      cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance));
}

void Rtl8139::Rtl8139Ethernet::ChangeHandleMethodToInt() {
  _polling.Remove();

  _master.WriteReg<uint16_t>(kRegIrMask, kIsrTok | kIsrRok);
  _master.SetLegacyInterrupt(InterruptHandler,
                             reinterpret_cast<void *>(&_master),
                             Idt::EoiType::kIoapic);
}

void Rtl8139::Rtl8139Ethernet::Transmit(void *) {
  uint32_t entry = _current_tx_descriptor;
  _master.WriteReg<uint8_t>(kRegCommand, kCmdTxEnable | kCmdRxEnable);
  uint16_t status = _master.ReadReg<uint8_t>(kRegCommand);
  if ((status & kCmdRxEnable) && (status & kCmdTxEnable)) {
    SetStatus(LinkStatus::kUp);
  }

  _master.WriteReg<uint16_t>(kRegIrMask, kIsrTok | kIsrRok);

  if (!(_tx_descriptor_status & (1 << entry))) {
    if (!_tx_buffered_blocked) {
      _tx_buffered.Block();
      _tx_buffered_blocked = true;
    }
    return;
  }
  // TxOK
  Packet *packet;
  if (!_tx_buffered.Pop(packet)) {
    return;
  }

  _current_tx_descriptor = (_current_tx_descriptor + 1) % 4;

  uint32_t length = packet->len;
  uint8_t *buf = packet->GetBuffer();
  memcpy(reinterpret_cast<uint8_t *>(_tx_buffer[entry].GetVirtAddr()), buf,
         length);
  _master.WriteReg<uint32_t>(
      kRegTxStatus + entry * 4,
      ((256 << 11) & 0x003f0000) |
          length);  // 256 means
                    // http://www.jbox.dk/sanos/source/sys/dev/rtl8139.c.html#:56
  _tx_descriptor_status = _tx_descriptor_status & ~(1 << entry);

  kassert(_tx_reserved.Push(packet));
}

void Rtl8139::Rtl8139Ethernet::InterruptHandler(void *p) {
  Rtl8139 *that = reinterpret_cast<Rtl8139 *>(p);
  uint16_t status = that->ReadReg<uint16_t>(kRegIrStatus);
  uint32_t length;
  uint8_t *payload, *data;
  Packet *packet;

  //ビットが立っているところに1を書き込むとクリア
  that->WriteReg<uint16_t>(kRegIrStatus, status);

  if (status & kIsrRok) {
    if (!(that->ReadReg<uint8_t>(kRegCommand) & kCmdRxBufEmpty)) {
      data = reinterpret_cast<uint8_t *>(that->_eth._rx_buffer.GetVirtAddr() +
                                         that->_eth._rx_buffer_offset);
      length = *(uint16_t *)(data + 2);  // it contains CRC'S 4byte
      payload = data + 4;

      if (that->_eth._rx_reserved.Pop(packet)) {
        packet->len = length;
        memcpy(packet->GetBuffer(), payload, length);
        if (!that->_eth._rx_buffered.Push(packet)) {
          kassert(that->_eth._rx_reserved.Push(packet));
        }
      }

      that->_eth._rx_buffer_offset =
          ((length + that->_eth._rx_buffer_offset + 4 + 3) % (64 * 1024 + 16)) &
          ~0b11;
      that->WriteReg<uint16_t>(kRegRxCAP, that->_eth._rx_buffer_offset - 0x10);
    }
  }
  if (status & kIsrTok) {
    if (that->_eth._tx_buffered_blocked) {
      that->_eth._tx_buffered.UnBlock();
      that->_eth._tx_buffered_blocked = false;
    }
    for (int i = 0; i < 4; i++) {
      uint32_t tx_status = that->ReadReg<uint32_t>(kRegTxStatus + i * 4);
      if (tx_status & (1 << 15)) {
        // OWN bit, transmit to FIFO
        that->_eth._tx_descriptor_status |= (1 << i);
      }
    }
  }
}

void Rtl8139::Rtl8139Ethernet::SetRxTxConfigRegs() {
  //受信バッファ設定
  physmem_ctrl->Alloc(_rx_buffer, PagingCtrl::kPageSize * 17);
  _master.WriteReg<uint32_t>(kRegRxAddr, _rx_buffer.GetAddr());
  // Receive Configuration Registerの設定
  //７bit目はwrapで最後にあふれた時に、リングの先頭に戻るか(０)、そのままはみでるか(１)
  // 1にするなら余裕を持って確保すること
  _master.WriteReg<uint32_t>(kRegRxConfig,
                             0xf | (1 << 7) | (1 << 5) | (0b11 << 11));
  // 0b11 means 64KB + 16 bytes 上ではwrap用の余分含めて66KB確保してある

  for (int i = 0; i < 4; i++) {
    physmem_ctrl->Alloc(_tx_buffer[i], PagingCtrl::kPageSize);
    _master.WriteReg<uint32_t>(kRegTxAddr + i * 4, _tx_buffer[i].GetAddr());
  }
  // CRCはハード側でつけてくれる
  _master.WriteReg<uint32_t>(kRegTxConfig, 4 << 8);
}

void Rtl8139::Rtl8139Ethernet::Setup() {
  // Enable BusMastering
  _master.WritePciReg<uint16_t>(
      PciCtrl::kCommandReg, _master.ReadPciReg<uint16_t>(PciCtrl::kCommandReg) |
                                PciCtrl::kCommandRegBusMasterEnableFlag);

  // Software Reset
  _master.WriteReg<uint8_t>(kRegCommand, kCmdReset);
  while ((_master.ReadReg<uint8_t>(0x37) & 0x10) != 0) {
  }

  _master.WriteReg<uint16_t>(kRegIrMask, 0);

  //_polling.Init(
  //    make_uptr(new Function1<void, Rtl8139 *>(PollingHandler, &_master)));
  //_polling.Register(
  //    cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kHighPerformance));

  // using Intrruption (default)
  _master.WriteReg<uint16_t>(kRegIrMask, kIsrTok | kIsrRok);
  _master.SetLegacyInterrupt(InterruptHandler,
                             reinterpret_cast<void *>(&_master),
                             Idt::EoiType::kIoapic);

  InitTxPacketBuffer();
  InitRxPacketBuffer();

  SetupNetInterface("rtl");
}

void Rtl8139::Rtl8139Ethernet::Start() {
  _master.WriteReg<uint8_t>(kRegCommand, kCmdTxEnable | kCmdRxEnable);
  uint16_t status = _master.ReadReg<uint8_t>(kRegCommand);
  if ((status & kCmdRxEnable) && (status & kCmdTxEnable)) {
    SetStatus(LinkStatus::kUp);
  }

  _status_check_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(
                                                 CpuPurpose::kLowPriority))
                             .AllocNewThread(Thread::StackState::kShared);
  _status_check_thread->CreateOperator().SetFunc(
      make_uptr(new Function1<void, Rtl8139Ethernet *>(
          Rtl8139::Rtl8139Ethernet::CheckHwState, this)));

  _status_check_thread->CreateOperator().Schedule(1000 * 1000);
}

void Rtl8139::Rtl8139Ethernet::CheckHwState(Rtl8139Ethernet *p) {
  uint16_t status = p->_master.ReadReg<uint8_t>(kRegCommand);
  if ((status & kCmdRxEnable) && (status & kCmdTxEnable)) {
    p->SetStatus(LinkStatus::kUp);
  } else {
    p->SetStatus(LinkStatus::kDown);
  }
  p->_status_check_thread->CreateOperator().Schedule(1000 * 1000);
}
