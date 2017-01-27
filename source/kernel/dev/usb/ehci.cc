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
  WriteReg<uint16_t>(PciCtrl::kCommandReg, ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  phys_addr addr = ReadReg<uint32_t>(kBaseAddressReg);
  if ((addr & 0x4) != 0) {
    // may be mapped into 64bit addressing space
    addr |= static_cast<phys_addr>(ReadReg<uint32_t>(kBaseAddressReg + 4)) << 32;
  }
  addr &= 0xFFFFFFFFFFFFFF00;
  _capability_reg_base_addr = addr2ptr<volatile uint8_t>(p2v(addr));
  _op_reg_base_addr = reinterpret_cast<volatile uint32_t *>(_capability_reg_base_addr + ReadCapabilityRegCapLength());

  if ((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagHcHalted) == 0) {
    // halt controller
    _op_reg_base_addr[kOpRegOffsetUsbCmd] &= ~kOpRegUsbCmdFlagRunStop; 
  }

  while((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagHcHalted) == 0) {
    asm volatile("":::"memory");
  }

  // reset controller
  _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagHcReset;

  while((_op_reg_base_addr[kOpRegOffsetUsbCmd] & kOpRegUsbCmdFlagHcReset) != 0) {
    asm volatile("":::"memory");
  }

  int n_ports = ReadCapabilityRegHcsParams() & 0xF;

  for (int i = 0; i < n_ports; i++) {
    if (_op_reg_base_addr[kOpRegOffsetPortScBase + i] & kOpRegPortScFlagCurrentConnectStatus) {
      DisablePort(i);
    }
  }

  _op_reg_base_addr[kOpRegOffsetCtrlDsSegment] = 0;

  _int_task->SetFunc(make_uptr(new ClassFunction<DevEhci, void *>(this, &DevEhci::CheckQueuedTdIfCompleted, nullptr)));
  assert(HasLegacyInterrupt());
  SetLegacyInterrupt(Handler, reinterpret_cast<void *>(this));
  _op_reg_base_addr[kOpRegOffsetUsbIntr] |= kOpRegUsbIntrFlagInterruptEnable;

  if ((ReadCapabilityRegHccParams() & 1) == 0) {
    _is_64bit_addressing = false;
    _sub = new DevEhciSub32();
  } else {
    _is_64bit_addressing = true;
    _sub = new DevEhciSub64();
  }
  
  _sub->Init();

  _op_reg_base_addr[kOpRegOffsetConfigFlag] |= kOpRegConfigFlag;

  _op_reg_base_addr[kOpRegOffsetUsbCmd] &= ~kOpRegUsbCmdFlagAsynchronousScheduleEnable & ~kOpRegUsbCmdFlagPeriodicScheduleEnable;

  while((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagAsynchronousSchedule) != 0) {
    asm volatile("":::"memory");
  }
  while((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagPeriodicSchedule) != 0) {
    asm volatile("":::"memory");
  }
  
  _op_reg_base_addr[kOpRegOffsetAsyncListAddr] = _sub->GetRepresentativeQueueHead();

  if ((_op_reg_base_addr[kOpRegOffsetUsbCmd] & kOpRegUsbCmdOffsetFrameListSize) != 0) {
    kernel_panic("DevEhci", "non supported function");
  }
  _op_reg_base_addr[kOpRegOffsetPeriodicListBase] = _sub->GetPeriodicFrameList();
  _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagAsynchronousScheduleEnable | kOpRegUsbCmdFlagPeriodicScheduleEnable;

  _op_reg_base_addr[kOpRegOffsetUsbCmd] |= kOpRegUsbCmdFlagRunStop; 
  
  while((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagPeriodicSchedule) == 0) {
    asm volatile("":::"memory");
  }
  while((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagAsynchronousSchedule) == 0) {
    asm volatile("":::"memory");
  }

  for (int i = 0; i < n_ports; i++) {
    if ((_op_reg_base_addr[kOpRegOffsetPortScBase + i] & kOpRegPortScFlagCurrentConnectStatus) != 0) {
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
      _controller_dev.InitDevices(dev);
    }
  }
}

void DevEhci::HandlerSub() {
  if ((_op_reg_base_addr[kOpRegOffsetUsbSts] & kOpRegUsbStsFlagInterrupt) == 0) {
    return;
  }

  if (!_int_task->IsRegistered()) {
    task_ctrl->Register(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority), _int_task);
  }
}

template<class QueueHead, class TransferDescriptor>
void DevEhci::DevEhciSub<QueueHead, TransferDescriptor>::Init() {
  constexpr int td_buf_size = _td_buf.GetBufSize();
  PhysAddr td_buf_paddr;
  static_assert(td_buf_size * sizeof(TransferDescriptor) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(td_buf_paddr, PagingCtrl::kPageSize);
  assert(td_buf_paddr.GetAddr() <= 0xFFFFFFFF);
  TransferDescriptor *td_array = addr2ptr<TransferDescriptor>(td_buf_paddr.GetVirtAddr());
  for (int i = 0; i < td_buf_size; i++) {
    new(&td_array[i]) TransferDescriptor;
    kassert(_td_buf.Push(&td_array[i]));
  }

  constexpr int qh_buf_size = _qh_buf.GetBufSize();
  PhysAddr qh_buf_paddr;
  static_assert(qh_buf_size * sizeof(QueueHead) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(qh_buf_paddr, PagingCtrl::kPageSize);
  assert(qh_buf_paddr.GetAddr() <= 0xFFFFFFFF);
  QueueHead *qh_array = addr2ptr<QueueHead>(qh_buf_paddr.GetVirtAddr());
  for (int i = 0; i < qh_buf_size; i++) {
    new(&qh_array[i]) QueueHead;
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

template<class QueueHead, class TransferDescriptor>
bool DevEhci::DevEhciSub<QueueHead, TransferDescriptor>::SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
  bool success = true;

  QueueHead *qh1 = nullptr;
  TransferDescriptor *td1 = nullptr;
  TransferDescriptor *td2 = nullptr;
  TransferDescriptor *td3 = nullptr;

  // (*td_array)[0] = td1;
  // (*td_array)[1] = td2;
  // (*td_array)[2] = td3;

  // auto td_array = make_uptr(new Array<TransferDescriptor *>(3));
  // auto func = make_uptr(new ClassFunction2<DevEhci::DevEhciSub, QueueHead *, uptr<Array<TransferDescriptor *>>>(this, &DevEhci::DevEhciSub::HandleCompletedStruct, qh1, td_array));
  //   td3->SetFunc(func);
  //   _queueing_td_buf.PushBack(td3);

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

  assert(td2->IsActiveOfStatus());
  _qh0->SetHorizontalNext(qh1);

  while(td2->IsActiveOfStatus()) {
  }
  assert(td2->GetStatus() == 0);
  
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

template<class QueueHead, class TransferDescriptor>
sptr<DevUsbController::Manager> DevEhci::DevEhciSub<QueueHead, TransferDescriptor>::SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) {
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
  TdContainer container_array[num_td];

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

  auto manager = make_sptr(new DevEhciSub::EhciManager(num_td, qh, func, this));

  for (int i = 0; i < num_td; i++) {
    td[i]->Init();
    if (i == num_td - 1) {
      td[i]->SetNext();
    } else {
      td[i]->SetNext(td[i + 1]);
    }
    TdContainer tmp = {
      .interrupt_on_complete = true,
      .data_toggle = true,
      .pid = direction,
      .total_bytes = max_packetsize,
      .buf = buffer,
    };
    container_array[i] = tmp;
    td[i]->SetTokenAndBuffer(container_array[i]);
    td[i]->SetFunc(make_uptr(new Function2<wptr<DevEhciSub::EhciManager>, int>(&DevEhciSub::EhciManager::HandleInterrupt, make_wptr(manager), i)));
    _queueing_td_buf.PushBack(td[i]);
    buffer += max_packetsize;
  }

  for (int i = 0; i < GetPeriodicFrameListEntryNum(); i++) {
    if (i % interval == 0) {
      _periodic_frame_list[i].Set(qh);
    }
  }

  manager->CopyInfo(td, container_array);

  return manager;
}

template<class QueueHead, class TransferDescriptor>
void DevEhci::DevEhciSub<QueueHead, TransferDescriptor>::HandleCompletedStruct(QueueHead *qh , uptr<Array<TransferDescriptor *>> td_array) {
  if (qh != nullptr) {
    assert(_qh_buf.Push(qh));
  }
  for (size_t i = 0; i < td_array->GetLen(); i++) {
    if ((*td_array)[i] != nullptr) {
      (*td_array)[i]->_func = make_uptr<GenericFunction<>>();
      assert(_td_buf.Push((*td_array)[i]));
    }
  }
}

template<class QueueHead, class TransferDescriptor>
void DevEhci::DevEhciSub<QueueHead, TransferDescriptor>::CheckQueuedTdIfCompleted() {
  auto iter = _queueing_td_buf.GetBegin();
  int i = 0;
  while (!iter.IsNull()) {
    i++;
    TransferDescriptor *t = *(*iter);
    if (!t->IsActiveOfStatus()) {
      t->Execute();
      iter = _queueing_td_buf.Remove(iter);
    } else {
      iter = iter->GetNext();
    }
  }
}

template<class QueueHead, class TransferDescriptor>
void DevEhci::DevEhciSub<QueueHead, TransferDescriptor>::EhciManager::HandleInterruptSub(int index) {
  size_t len = _container_array[index].total_bytes;
  auto buf = make_uptr(new Array<uint8_t>(len));
  memcpy(buf.GetRawPtr()->GetRawPtr(), _container_array[index].buf, len);

  _func->Execute(buf);
    
  _td_array[index]->SetTokenAndBuffer(_container_array[index]);
  if (index == _num_td - 1) {
    _td_array[index]->SetNext();
  } else {
    _td_array[index]->SetNext(_td_array[index + 1]);
  }

  if (_interrupt_qh->IsNextTdNull()) {
    for (int i = 0; i < _num_td; i++) {
      _master->_queueing_td_buf.PushBack(_td_array[i]);
    }
    _interrupt_qh->SetNextTd(_td_array[0]);
  }
}
