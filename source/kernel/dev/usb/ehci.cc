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

#include "ehci.h"
#include <mem/paging.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <dev/usb/usb11.h>

// for debug
#include <tty.h>
#include <global.h>

DevPci *DevEhci::InitPci(uint8_t bus, uint8_t device, uint8_t function) {
  DevEhci *dev = new DevEhci(bus, device, function);
  if (dev->ReadReg<uint8_t>(PciCtrl::kRegInterfaceClassCode) == 0x20 &&
      dev->ReadReg<uint8_t>(PciCtrl::kRegSubClassCode) == 0x03 &&
      dev->ReadReg<uint8_t>(PciCtrl::kRegBaseClassCode) == 0x0C) {
    dev->Init();
    return dev;
  } else {
    delete dev;
    return nullptr;
  }
}

void DevEhci::Init() {
  if ((_operational_reg_base_addr[kOperationalRegOffsetUsbSts] & kOperationalRegUsbStsFlagHcHalted) == 0) {
    // halt controller
    _operational_reg_base_addr[kOperationalRegOffsetUsbCmd] |= kOperationalRegUsbCmdFlagRunStop; 
  }

  while((READ_MEM_VOLATILE(&_operational_reg_base_addr[kOperationalRegOffsetUsbSts]) & kOperationalRegUsbStsFlagHcHalted) == 0) {
  }

  // reset controller
  _operational_reg_base_addr[kOperationalRegOffsetUsbCmd] |= kOperationalRegUsbCmdFlagHcReset;

  while((READ_MEM_VOLATILE(&_operational_reg_base_addr[kOperationalRegOffsetUsbCmd]) & kOperationalRegUsbCmdFlagHcReset) == 0) {
  }

  
  phys_addr addr = ReadReg<uint32_t>(kBaseAddressReg);
  if ((addr & 0x4) != 0) {
    // may be mapped into 64bit addressing space
    addr |= static_cast<phys_addr>(ReadReg<uint32_t>(kBaseAddressReg + 4)) << 32;
  }
  addr &= 0xFFFFFFFFFFFFFF00;
  _capability_reg_base_addr = addr2ptr<volatile uint8_t>(p2v(addr));
  _operational_reg_base_addr = reinterpret_cast<volatile uint32_t *>(_capability_reg_base_addr + ReadCapabilityRegCapLength());

  int n_ports = ReadCapabilityRegHcsParams() & 0xF;

  for (int i = 0; i < n_ports; i++) {
    if (_operational_reg_base_addr[kOperationalRegOffsetPortScBase + i] & kOperationalRegPortScFlagCurrentConnectStatus) {
      DisablePort(i);
    }
  }

  WriteReg<uint16_t>(PciCtrl::kCommandReg, ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  if ((ReadCapabilityRegHccParams() & 1) == 0) {
    _is_64bit_addressing = false;
    _sub = new DevEhciSub32();
  } else {
    _is_64bit_addressing = true;
    _sub = new DevEhciSub64();
  }
  
  _sub->Init();

  _operational_reg_base_addr[kOperationalRegOffsetUsbCmd] &= ~kOperationalRegUsbCmdFlagAsynchronousScheduleEnable;
  _operational_reg_base_addr[kOperationalRegOffsetUsbCmd] &= ~kOperationalRegUsbCmdFlagsPeriodicScheduleEnable;

  while((READ_MEM_VOLATILE(&_operational_reg_base_addr[kOperationalRegOffsetUsbSts]) & kOperationalRegUsbStsFlagAsynchronousSchedule) != 0) {
  }
  
  if (_is_64bit_addressing) {
    kassert(false);
  } else {
    _operational_reg_base_addr[kOperationalRegOffsetAsyncListAddr] = _sub->GetRepresentativeQueueHead();
  }

  _operational_reg_base_addr[kOperationalRegOffsetUsbCmd] |= kOperationalRegUsbCmdFlagAsynchronousScheduleEnable;

  while((READ_MEM_VOLATILE(&_operational_reg_base_addr[kOperationalRegOffsetUsbSts]) & kOperationalRegUsbStsFlagAsynchronousSchedule) == 0) {
  }
  
  for (int i = 0; i < n_ports; i++) {
    if (_operational_reg_base_addr[kOperationalRegOffsetPortScBase + i] & kOperationalRegPortScFlagCurrentConnectStatus) {
      ResetPort(i);
      while(true) {
      	Usb11Ctrl::DeviceRequest *request = nullptr;

      	if (!Usb11Ctrl::GetCtrl().AllocDeviceRequest(request)) {
      	  goto release;
      	}

      	// TODO
      	// horrible device address. fix it
      	request->MakePacketOfSetAddress(i + 1);

      	if (!SendControlTransfer(request, 0, 0, 0)) {
      	  goto release;
      	}

      	break;

      release:
      	if (request != nullptr) {
      	  assert(Usb11Ctrl::GetCtrl().ReuseDeviceRequest(request));
      	}
      }
      int dev = i + 1;
      DevUsbKeyboard::InitUsb(&_controller_dev, dev);
    }
  }
}

void DevEhci::DevEhciSub32::Init() {
  constexpr int td_buf_size = _td_buf.GetBufSize();
  PhysAddr td_buf_paddr;
  static_assert(td_buf_size * sizeof(TransferDescriptor) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(td_buf_paddr, PagingCtrl::kPageSize);
  assert(td_buf_paddr.GetAddr() <= 0xFFFFFFFF);
  TransferDescriptor *td_array = addr2ptr<TransferDescriptor>(td_buf_paddr.GetVirtAddr());
  for (int i = 0; i < td_buf_size; i++) {
    kassert(_td_buf.Push(&td_array[i]));
  }

  constexpr int qh_buf_size = _qh_buf.GetBufSize();
  PhysAddr qh_buf_paddr;
  static_assert((qh_buf_size + 1) * sizeof(QueueHead) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(qh_buf_paddr, PagingCtrl::kPageSize);
  assert(qh_buf_paddr.GetAddr() <= 0xFFFFFFFF);
  QueueHead *qh_array = addr2ptr<QueueHead>(qh_buf_paddr.GetVirtAddr());
  for (int i = 0; i < qh_buf_size; i++) {
    kassert(_qh_buf.Push(&qh_array[i]));
  }
  _qh0 = qh_array + qh_buf_size;
  _qh0->InitEmpty();
}

void DevEhci::DevEhciSub64::Init() {
  kernel_panic("Ehci", "needs implementation of 64-bit addressing");
}

bool DevEhci::DevEhciSub32::SendControlTransfer(Usb11Ctrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
  bool success = true;

  QueueHead *qh1 = nullptr;
  TransferDescriptor *td1 = nullptr;
  TransferDescriptor *td2 = nullptr;
  TransferDescriptor *td3 = nullptr;

  if (!_qh_buf.Pop(qh1)) {
    success = false;
    goto release;
  }
  if (!_td_buf.Pop(td1)) {
    success = false;
    goto release;
  }
  if (!_td_buf.Pop(td2)) {
    success = false;
    goto release;
  }
  if (!_td_buf.Pop(td3)) {
    success = false;
    goto release;
  }

  qh1->Init(QueueHead::EndpointSpeed::kHigh);
  qh1->SetHorizontalNext(_qh0);
  qh1->SetNextTd(td1);
  qh1->SetCharacteristics(64, Usb11Ctrl::TransactionType::kControl, false, true, 0, false, device_addr);

  // see Figure 8-12. Control Read and Write Sequence
  td1->Init();
  td1->SetNext(td2);
  td1->SetTokenAndBuffer(false, false, TransferDescriptor::PacketIdentification::kSetup, 8, ptr2virtaddr(request));

  if (data != 0) {
    bool direction = request->IsDirectionDeviceToHost();
    td2->Init();
    td2->SetNext(td3);
    td2->SetTokenAndBuffer(false, true, direction ? TransferDescriptor::PacketIdentification::kIn : TransferDescriptor::PacketIdentification::kOut, data_size, data);
    
    td3->Init();
    td3->SetNext();
    td3->SetTokenAndBuffer(false, true, direction ? TransferDescriptor::PacketIdentification::kOut : TransferDescriptor::PacketIdentification::kIn, 0);
  } else {
    td2->Init();
    td2->SetNext();
    td2->SetTokenAndBuffer(false, true, TransferDescriptor::PacketIdentification::kIn, 0);
  }

  _qh0->SetHorizontalNext(qh1);

  timer->BusyUwait(1000*1000);

 release:
  if (qh1 != nullptr) {
    assert(_qh_buf.Push(qh1));
  }
  if (td1 != nullptr) {
    assert(_td_buf.Push(td1));
  }
  if (td2 != nullptr) {
    assert(_td_buf.Push(td2));
  }
  if (td3 != nullptr) {
    assert(_td_buf.Push(td3));
  }

  return success;
}
