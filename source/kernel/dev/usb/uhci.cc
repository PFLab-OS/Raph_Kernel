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

#include "uhci.h"
#include <mem/paging.h>
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <dev/usb/usb.h>

DevPci *DevUhci::InitPci(uint8_t bus, uint8_t device, uint8_t function) {
  DevUhci *dev = new DevUhci(bus, device, function);
  if (dev->ReadPciReg<uint8_t>(PciCtrl::kRegInterfaceClassCode) == 0x00 &&
      dev->ReadPciReg<uint8_t>(PciCtrl::kRegSubClassCode) == 0x03 &&
      dev->ReadPciReg<uint8_t>(PciCtrl::kRegBaseClassCode) == 0x0C) {
    dev->Init();
    return dev;
  } else {
    delete dev;
    return nullptr;
  }
}

void DevUhci::Init() {

  // allocate frame list
  assert(sizeof(FrameList) == PagingCtrl::kPageSize);
  PhysAddr frame_base_addr;
  physmem_ctrl->Alloc(frame_base_addr, PagingCtrl::kPageSize);
  _frlist = addr2ptr<FrameList>(frame_base_addr.GetVirtAddr());
  if (frame_base_addr.GetAddr() > 0xFFFFFFFF) {
    kernel_panic("DevUhci", "cannot allocate 32bit phys memory");
  }
  
  constexpr int td_buf_size = _td_buf.GetBufSize();
  PhysAddr td_buf_paddr;
  static_assert(td_buf_size * sizeof(TransferDescriptor) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(td_buf_paddr, PagingCtrl::kPageSize);
  assert(td_buf_paddr.GetAddr() <= 0xFFFFFFFF);
  TransferDescriptor *td_array = addr2ptr<TransferDescriptor>(td_buf_paddr.GetVirtAddr());
  for (int i = 0; i < td_buf_size; i++) {
    new (&td_array[i]) TransferDescriptor;
    kassert(_td_buf.Push(&td_array[i]));
  }

  constexpr int qh_buf_size = _qh_buf.GetBufSize();
  PhysAddr qh_buf_paddr;
  static_assert(qh_buf_size * sizeof(QueueHead) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(qh_buf_paddr, PagingCtrl::kPageSize);
  assert(qh_buf_paddr.GetAddr() <= 0xFFFFFFFF);
  QueueHead *qh_array = addr2ptr<QueueHead>(qh_buf_paddr.GetVirtAddr());
  for (int i = 2; i < qh_buf_size; i++) {
    new(&qh_array[i]) QueueHead;
    kassert(_qh_buf.Push(&qh_array[i]));
  }

  _qh_cb = &qh_array[0];
  _qh_cb->InitEmpty();

  PhysAddr qh_int_array_paddr;
  physmem_ctrl->Alloc(qh_int_array_paddr, PagingCtrl::ConvertNumToPageSize(kFrameListEntryNum * sizeof(QueueHead)));
  assert(qh_int_array_paddr.GetAddr() <= 0xFFFFFFFF);
  QueueHead *qh_int_array = addr2ptr<QueueHead>(qh_int_array_paddr.GetVirtAddr());
  for (int i = 0; i < kFrameListEntryNum; i++) {
    _qh_int[i] = &qh_int_array[i];
    _qh_int[i]->InitEmpty();
    _qh_int[i]->SetHorizontalNext(_qh_cb);
    _frlist->entries[i].Set(_qh_int[i]);
  }

  // legacy support
  WritePciReg<uint16_t>(0xC0, 0x2000);
  
  _base_addr = ReadPciReg<uint32_t>(kBaseAddressReg);
  if ((_base_addr & 1) == 0) {
    kernel_panic("Uhci", "cannot get valid base address.");
  }
  _base_addr &= 0xFFE0;
  WritePciReg<uint16_t>(PciCtrl::kCommandReg, ReadPciReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  // halt controller
  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) & ~kCtrlRegCmdFlagRunStop);
  
  while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) == 0) {
    asm volatile("":::"memory");
  }

  // global reset
  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) | kCtrlRegCmdFlagGlobalReset);

  timer->BusyUwait(10 * 1000); // 10ms wait

  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) & ~kCtrlRegCmdFlagGlobalReset);

  // reset controller
  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) | kCtrlRegCmdFlagHcReset);
  
  while((ReadControllerReg<uint16_t>(kCtrlRegCmd) & kCtrlRegCmdFlagHcReset) != 0) {
    asm volatile("":::"memory");
  }  
  
  uint32_t frame_base_phys_addr = frame_base_addr.GetAddr();
  WriteControllerReg<uint32_t>(kCtrlRegFlBaseAddr, frame_base_phys_addr);
  WriteControllerReg<uint16_t>(kCtrlRegFrNum, 0);

  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) | kCtrlRegCmdFlagRunStop | kCtrlRegCmdFlagMaxPacket);
  while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) != 0) {
    asm volatile("":::"memory");
  }

  _int_thread = ThreadCtrl::GetCtrl(cpu_ctrl->RetainCpuIdForPurpose(CpuPurpose::kLowPriority)).AllocNewThread(Thread::StackState::kShared);
  _int_thread->CreateOperator().SetFunc(make_uptr(new ClassFunction<DevUhci, void *>(this, &DevUhci::CheckQueuedTdIfCompleted, nullptr)));
  WriteControllerReg<uint16_t>(kCtrlRegStatus, kCtrlRegStatusFlagInt);
  WriteControllerReg<uint16_t>(kCtrlRegIntr, /*ReadControllerReg<uint16_t>(kCtrlRegIntr) | */kCtrlRegIntrFlagIoc);
  assert(HasLegacyInterrupt());
  SetLegacyInterrupt(Handler, reinterpret_cast<void *>(this), Idt::EoiType::kIoapic);

  for (int i = 0; i < 2; i++) {
    DisablePort(i);
  }
}

void DevUhci::Attach() {
  for (int i = 0; i < 2; i++) {
    if ((ReadControllerReg<uint16_t>(kCtrlRegPortBase + i * 2) & kCtrlRegPortFlagCurrentConnectStatus) != 0) {
      ResetPort(i);
      // TODO
      // horrible device address. fix it
      int dev = i + 1;
      bool retry = true;
      while(retry) {
        UsbCtrl::DeviceRequest *request = nullptr;

        do {
          if (!UsbCtrl::GetCtrl().AllocDeviceRequest(request)) {
            break;
          }

          request->MakePacketOfSetAddress(dev);

          if (!SendControlTransfer(request, 0, 0, 0)) {
            break;
          }

          retry = false;
        } while(0);
        
        if (request != nullptr) {
          assert(UsbCtrl::GetCtrl().ReuseDeviceRequest(request));
        }
      }
      _controller_dev.InitDevices(dev);
    }
  }
}

bool DevUhci::SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
  bool success = false;
  QueueHead *qh1 = nullptr;
  TransferDescriptor *td1 = nullptr;
  TransferDescriptor *td2 = nullptr;
  bool toggle = true;
  UsbCtrl::PacketIdentification direction = request->GetDirection();
  TransferDescriptor *td_array[((data_size + 7) / 8) * 8];

  do {
    // see Figure 8-12 Control Read and Write Sequences
    if (!_qh_buf.Pop(qh1)) {
      break;
    }
    if (!_td_buf.Pop(td1)) {
      break;
    }
    if (!_td_buf.Pop(td2)) {
      break;
    }

    for (size_t offset = 0; offset < data_size; offset += 8) {
      if (!_td_buf.Pop(td_array[offset / 8])) {
        break;
      }
    }

    if (data_size == 0) {
      td1->SetNext(td2, true);
    } else {
      td1->SetNext(td_array[0], true);
    }
    td1->SetStatus(false, false, true, false);
    td1->SetToken(UsbCtrl::PacketIdentification::kSetup, device_addr, 0, false, 8);
    td1->SetBuffer(ptr2virtaddr(request), 0);

    for (size_t offset = 0; offset < data_size; offset += 8) {
      if (offset + 8 >= data_size) {
        td_array[offset / 8]->SetNext(td2, true);
      } else {
        td_array[offset / 8]->SetNext(td_array[offset / 8 + 1], true);
      }
      td_array[offset / 8]->SetStatus(false, false, true, false);
      td_array[offset / 8]->SetToken(direction, device_addr, 0, toggle, 8);
      td_array[offset / 8]->SetBuffer(data, offset);

      toggle = !toggle;
    }

    td2->SetNext(true);
    td2->SetStatus(false, false, true, false);
    td2->SetToken(UsbCtrl::ReversePacketIdentification(direction), device_addr, 0, true, 8);
    td2->SetBuffer();

    qh1->SetHorizontalNext();
    qh1->SetVerticalNext(td1);

    _qh_cb->InsertHorizontalNext(qh1);

    while(td2->IsActiveOfStatus()) {
      if (td1->IsStalledOfStatus() || td1->IsCrcErrorOfStatus()) {
        break;
      }
      asm volatile("":::"memory");
    }
    if (td1->IsStalledOfStatus() || td1->IsCrcErrorOfStatus()) {
      break;
    }
    // TODO remove qh1

    success = true;
  } while(0);

  if (td1 != nullptr) {
    assert(_td_buf.Push(td1));
  }
  if (td2 != nullptr) {
    assert(_td_buf.Push(td2));
  }
  if (qh1 != nullptr) {
    assert(_qh_buf.Push(qh1));
  }
  for (size_t offset = 0; offset < data_size; offset += 8) {
    if (td_array[offset / 8] != nullptr) {
      _td_buf.Push(td_array[offset / 8]);
    }
  }
  
  return success;
}

sptr<DevUsbController::Manager> DevUhci::SetupInterruptTransfer(uint8_t endpt_address, int device_addr, int interval, UsbCtrl::PacketIdentification direction, int max_packetsize, int num_td, uint8_t *buffer, uptr<GenericFunction<uptr<Array<uint8_t>>>> func) {

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
    bool retry = true;

    do {
      for (int i = 0; i < num_td; i++) {
        tmp_td[i] = nullptr;
      }

      if (!_qh_buf.Pop(tmp_qh)) {
        break;
      }
      for (int i = 0; i < num_td; i++) {
        if (!_td_buf.Pop(tmp_td[i])) {
          break;
        }
      }

      qh = tmp_qh;
      for (int i = 0; i < num_td; i++) {
        td[i] = tmp_td[i];
      }

      retry = false;
    } while(0);

    if (!retry) {
      break;
    }

    if (tmp_qh != nullptr) {
      assert(_qh_buf.Push(tmp_qh));
    }
    for (int i = 0; i < num_td; i++) {
      if (tmp_td[i] != nullptr) {
        assert(_td_buf.Push(tmp_td[i]));
      }
    }
  }

  qh->SetHorizontalNext();
  qh->SetVerticalNext(td[0]);

  auto manager = make_sptr(new DevUhci::UhciManager(num_td, qh, func, this));

  for (int i = 0; i < num_td; i++) {
    if (i == num_td - 1) {
      td[i]->SetNext(true);
    } else {
      td[i]->SetNext(td[i + 1], true);
    }
    bool toggle = (i % 2) == 0;
    TdContainer tmp = {
      .interrupt_on_complete = true,
      .isochronous = false,
      .low_speed = true,
      .short_packet = false,
      .pid = direction,
      .device_address = device_addr,
      .endpoint = endpt_address,
      .data_toggle = toggle,
      .total_bytes = max_packetsize,
      .buf = buffer,
    };
    container_array[i] = tmp;
    td[i]->SetContainer(container_array[i]);
    td[i]->SetFunc(make_uptr(new Function2<wptr<DevUhci::UhciManager>, int>(&DevUhci::UhciManager::HandleInterrupt, make_wptr(manager), i)));
    _queueing_td_buf.PushBack(td[i]);
    buffer += max_packetsize;
  }

  for (int i = 0; i < kFrameListEntryNum; i++) {
    if (i % interval == 0) {
      _qh_int[i]->InsertHorizontalNext(qh);
    }
  }

  manager->CopyInfo(td, container_array);

  return manager;
}

void DevUhci::UhciManager::HandleInterruptSub(int index) {
  size_t len = _container_array[index].total_bytes;
  auto buf = make_uptr(new Array<uint8_t>(len));
  memcpy(buf.GetRawPtr()->GetRawPtr(), _container_array[index].buf, len);

  _func->Execute(buf);

  _td_array[index]->SetContainer(_container_array[index]);
  if (index == _num_td - 1) {
    _td_array[index]->SetNext(true);
  } else {
    _td_array[index]->SetNext(_td_array[index + 1], true);
  }

  if (_interrupt_qh->IsVerticalNextNull()) {
    for (int i = 0; i < _num_td; i++) {
      _master->_queueing_td_buf.PushBack(_td_array[i]);
    }
    _interrupt_qh->SetVerticalNext(_td_array[0]);
  }
}

void DevUhci::HandlerSub() {
  if ((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagInt) == 0) {
    return;
  }

  WriteControllerReg<uint16_t>(kCtrlRegIntr, ReadControllerReg<uint16_t>(kCtrlRegIntr) & ~kCtrlRegIntrFlagIoc);
  
  // tell controller to acknowledge interrupt
  WriteControllerReg<uint16_t>(kCtrlRegStatus, kCtrlRegStatusFlagInt);
  asm volatile("":::"memory");

  _int_thread->CreateOperator().Schedule();
}

void DevUhci::CheckQueuedTdIfCompleted(void *) {

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
  
  // TODO check not only TDs but also all interrupt sources before enable interrupt

  // enable new interrupt
  WriteControllerReg<uint16_t>(kCtrlRegIntr, ReadControllerReg<uint16_t>(kCtrlRegIntr) | kCtrlRegIntrFlagIoc);
}
