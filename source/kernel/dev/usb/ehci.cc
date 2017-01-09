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
#include <dev/usb/usb.h>

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
  if ((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagHcHalted) == 0) {
    // halt controller
    _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagRunStop; 
  }

  while((READ_MEM_VOLATILE(&_op_reg_base_addr[kOpRegOffsetUsbSts]) & kOpRegUsbStsFlagHcHalted) == 0) {
  }

  // reset controller
  _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagHcReset;

  while((READ_MEM_VOLATILE(&_op_reg_base_addr[kOpRegOffsetUsbCmd]) & kOpRegUsbCmdFlagHcReset) == 0) {
  }

  
  phys_addr addr = ReadReg<uint32_t>(kBaseAddressReg);
  if ((addr & 0x4) != 0) {
    // may be mapped into 64bit addressing space
    addr |= static_cast<phys_addr>(ReadReg<uint32_t>(kBaseAddressReg + 4)) << 32;
  }
  addr &= 0xFFFFFFFFFFFFFF00;
  _capability_reg_base_addr = addr2ptr<volatile uint8_t>(p2v(addr));
  _op_reg_base_addr = reinterpret_cast<volatile uint32_t *>(_capability_reg_base_addr + ReadCapabilityRegCapLength());

  int n_ports = ReadCapabilityRegHcsParams() & 0xF;

  for (int i = 0; i < n_ports; i++) {
    if (_op_reg_base_addr[kOpRegOffsetPortScBase + i] & kOpRegPortScFlagCurrentConnectStatus) {
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

  _op_reg_base_addr[kOpRegOffsetUsbCmd] &= ~kOpRegUsbCmdFlagAsynchronousScheduleEnable;
  _op_reg_base_addr[kOpRegOffsetUsbCmd] &= ~kOpRegUsbCmdFlagPeriodicScheduleEnable;

  while((READ_MEM_VOLATILE(&_op_reg_base_addr[kOpRegOffsetUsbSts]) & kOpRegUsbStsFlagAsynchronousSchedule) != 0) {
  }
  
  _op_reg_base_addr[kOpRegOffsetAsyncListAddr] = _sub->GetRepresentativeQueueHead();
  _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagAsynchronousScheduleEnable;

  if ((_op_reg_base_addr[kOpRegOffsetUsbCmd] & kOpRegUsbCmdOffsetFrameListSize) != 0) {
    kernel_panic("DevEhci", "non supported function");
  }
  _op_reg_base_addr[kOpRegOffsetPeriodicListBase] = _sub->GetPeriodicFrameList();
  _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagPeriodicScheduleEnable;


  while((READ_MEM_VOLATILE(&_op_reg_base_addr[kOpRegOffsetUsbSts]) & kOpRegUsbStsFlagAsynchronousSchedule) == 0) {
  }
  while((READ_MEM_VOLATILE(&_op_reg_base_addr[kOpRegOffsetUsbSts]) & kOpRegUsbStsFlagPeriodicSchedule) == 0) {
  }
  
  for (int i = 0; i < n_ports; i++) {
    if (_op_reg_base_addr[kOpRegOffsetPortScBase + i] & kOpRegPortScFlagCurrentConnectStatus) {
      ResetPort(i);
      while(true) {
        UsbCtrl::DeviceRequest *request = nullptr;

      	if (!UsbCtrl::GetCtrl().AllocDeviceRequest(request)) {
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
      	  assert(UsbCtrl::GetCtrl().ReuseDeviceRequest(request));
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

  PhysAddr pframe_list_paddr;
  static_assert(sizeof(FrameListElementPointer) * 1024 == PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(pframe_list_paddr, PagingCtrl::kPageSize);
  assert(pframe_list_paddr.GetAddr() <= 0xFFFFFFFF);
  _periodic_frame_list = addr2ptr<FrameListElementPointer>(pframe_list_paddr.GetVirtAddr());
  for (int i = 0; i < GetPeriodicFrameListEntryNum(); i++) {
    _periodic_frame_list[i].Set();
  }
}

void DevEhci::DevEhciSub64::Init() {
  kernel_panic("Ehci", "needs implementation of 64-bit addressing");
}

bool DevEhci::DevEhciSub32::SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
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
  qh1->SetCharacteristics(64, UsbCtrl::TransferType::kControl, false, true, 0, false, device_addr);

  // see Figure 8-12. Control Read and Write Sequence
  td1->Init();
  td1->SetNext(td2);
  td1->SetTokenAndBuffer(false, false, UsbCtrl::PacketIdentification::kSetup, 8, ptr2virtaddr(request));

  if (data != 0) {
    UsbCtrl::PacketIdentification direction = request->GetDirection();
    td2->Init();
    td2->SetNext(td3);
    td2->SetTokenAndBuffer(false, true, direction, data_size, data);
    
    td3->Init();
    td3->SetNext();
    td3->SetTokenAndBuffer(false, true, direction, 0);
  } else {
    td2->Init();
    td2->SetNext();
    td2->SetTokenAndBuffer(false, true, UsbCtrl::PacketIdentification::kIn, 0);
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

void DevEhci::DevEhciSub32::SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer) {
  if (interval <= 0) {
    kernel_panic("Ehci", "unknown interval");
  }

  {
    int i = 0;
    while(interval != 1) {
      i++;
      interval >>= 1;
    }
    for (; i > 0; i--) {
      interval <<= 1;
    }
  }

  QueueHead *qh;
  TransferDescriptor *td[num_td];

  while(true) {
    QueueHead *tmp_qh = nullptr;
    TransferDescriptor *tmp_td[num_td];
    for (int i = 0; i < num_td; i++) {
      tmp_td[i] = nullptr;
    }

    if (!_qh_buf.Pop(tmp_qh)) {
      goto retry;
    }
    for (int i = 0; i < num_td; i++) {
      _td_buf.Pop(tmp_td[i]);
    }

    qh = tmp_qh;
    for (int i = 0; i < num_td; i++) {
      td[i] = tmp_td[i];
    }
    break;
    
  retry:
    if (tmp_qh != nullptr) {
      assert(_qh_buf.Push(tmp_qh));
    }
    for (int i = 0; i < num_td; i++) {
      if (tmp_td[i] != nullptr) {
        assert(_td_buf.Push(tmp_td[i]));
      }
    }
  }
  
  qh->Init(QueueHead::EndpointSpeed::kHigh);
  qh->SetHorizontalNext();
  qh->SetNextTd(td[0]);
  qh->SetCharacteristics(max_packetsize, UsbCtrl::TransferType::kInterrupt, false, true, endpt_address, false, device_addr);

  assert(direction != UsbCtrl::PacketIdentification::kSetup);

  for (int i = 0; i < num_td; i++) {
    td[i]->Init();
    if (i == num_td) {
      td[i]->SetNext();
    } else {
      td[i]->SetNext(td[i + 1]);
    }
    td[i]->SetTokenAndBuffer(false, false, direction, max_packetsize, ptr2virtaddr(buffer));
    buffer += max_packetsize;
  }

  for (int i = 0; i < GetPeriodicFrameListEntryNum(); i++) {
    if (i % interval == 0) {
      _periodic_frame_list[i].Set(qh);
    }
  }
}
