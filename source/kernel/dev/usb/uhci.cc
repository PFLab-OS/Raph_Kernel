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
  if (dev->ReadReg<uint8_t>(PciCtrl::kRegInterfaceClassCode) == 0x00 &&
      dev->ReadReg<uint8_t>(PciCtrl::kRegSubClassCode) == 0x03 &&
      dev->ReadReg<uint8_t>(PciCtrl::kRegBaseClassCode) == 0x0C) {
    dev->Init();
    return dev;
  } else {
    delete dev;
    return nullptr;
  }
}

void DevUhci::Init() {

  constexpr int td_buf_size = _td_buf.GetBufSize();
  PhysAddr td_buf_paddr;
  static_assert(td_buf_size * sizeof(TransferDescriptor) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(td_buf_paddr, PagingCtrl::kPageSize);
  TransferDescriptor *td_array = addr2ptr<TransferDescriptor>(td_buf_paddr.GetVirtAddr());
  for (int i = 0; i < td_buf_size; i++) {
    new (&td_array[i]) TransferDescriptor;
    kassert(_td_buf.Push(&td_array[i]));
  }

  constexpr int qh_buf_size = _qh_buf.GetBufSize();
  PhysAddr qh_buf_paddr;
  static_assert(qh_buf_size * sizeof(QueueHead) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(qh_buf_paddr, PagingCtrl::kPageSize);
  QueueHead *qh_array = addr2ptr<QueueHead>(qh_buf_paddr.GetVirtAddr());
  for (int i = 1; i < qh_buf_size; i++) {
    new(&qh_array[i]) QueueHead;
    kassert(_qh_buf.Push(&qh_array[i]));
  }

  _qh0 = &qh_array[0];
  _qh0->InitEmpty();

  for (int i = 0; i < 1024; i++) {
    _frlist->entries[i].Set(_qh0);
  }

  // legacy support
  WriteReg<uint16_t>(0xC0, 0x2000);
  
  _base_addr = ReadReg<uint32_t>(kBaseAddressReg);
  if ((_base_addr & 1) == 0) {
    kernel_panic("Uhci", "cannot get valid base address.");
  }
  _base_addr &= 0xFFE0;
  WriteReg<uint16_t>(PciCtrl::kCommandReg, ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  // halt controller
  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) & ~kCtrlRegCmdFlagRunStop);
  
  while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) == 0) {
    asm volatile("":::"memory");
  }

  // reset controller
  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) | kCtrlRegCmdFlagHcReset);
  
  while((ReadControllerReg<uint16_t>(kCtrlRegCmd) & kCtrlRegCmdFlagHcReset) != 0) {
    asm volatile("":::"memory");
  }  

  // allocate frame list
  assert(sizeof(FrameList) == PagingCtrl::kPageSize);
  PhysAddr frame_base_addr;
  physmem_ctrl->Alloc(frame_base_addr, PagingCtrl::kPageSize);
  _frlist = addr2ptr<FrameList>(frame_base_addr.GetVirtAddr());
  if (frame_base_addr.GetAddr() > 0xFFFFFFFF) {
    kernel_panic("DevUhci", "cannot allocate 32bit phys memory");
  }
  
  uint32_t frame_base_phys_addr = frame_base_addr.GetAddr();
  WriteControllerReg<uint32_t>(kCtrlRegFlBaseAddr, frame_base_phys_addr);
  WriteControllerReg<uint16_t>(kCtrlRegFrNum, 0);

  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) | kCtrlRegCmdFlagRunStop);
  while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) != 0) {
    asm volatile("":::"memory");
  }

  for (int i = 0; i < 2; i++) {
    if ((ReadControllerReg<uint16_t>(i) & kCtrlRegPortFlagCurrentConnectStatus) != 0) {
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

bool DevUhci::SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
  bool success = true;
  QueueHead *qh1 = nullptr;
  TransferDescriptor *td1 = nullptr;
  TransferDescriptor *td2 = nullptr;
  bool toggle = true;
  UsbCtrl::PacketIdentification direction = request->GetDirection();
  TransferDescriptor::PacketIdentification direction_of_data_stage;
  TransferDescriptor::PacketIdentification direction_of_status_stage;

  // see Figure 8-12 Control Read and Write Sequences
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

  TransferDescriptor *td_array[((data_size + 7) / 8) * 8];
  for (size_t offset = 0; offset < data_size; offset += 8) {
    if (!_td_buf.Pop(td_array[offset / 8])) {
      success = false;
    }
  }
  if (success == false) {
    goto release;
  }

  if (data_size == 0) {
    td1->SetNext(td2, true);
  } else {
    td1->SetNext(td_array[0], true);
  }
  td1->SetStatus(false, false, true, false);
  td1->SetToken(TransferDescriptor::PacketIdentification::kSetup, device_addr, 0, false, 8);
  td1->SetBuffer(ptr2virtaddr(request), 0);

  switch(direction) {
  case UsbCtrl::PacketIdentification::kOut:
    direction_of_data_stage = TransferDescriptor::PacketIdentification::kOut;
    break;
  case UsbCtrl::PacketIdentification::kIn:
    direction_of_data_stage = TransferDescriptor::PacketIdentification::kIn;
    break;
  default:
    assert(false);
  }

  for (size_t offset = 0; offset < data_size; offset += 8) {
    if (offset + 8 >= data_size) {
      td_array[offset / 8]->SetNext(td2, true);
    } else {
      td_array[offset / 8]->SetNext(td_array[offset / 8 + 1], true);
    }
    td_array[offset / 8]->SetStatus(false, false, true, false);
    td_array[offset / 8]->SetToken(direction_of_data_stage, device_addr, 0, toggle, 8);
    td_array[offset / 8]->SetBuffer(data, 0);

    toggle = ~ toggle;
  }

  switch(direction) {
  case UsbCtrl::PacketIdentification::kOut:
    direction_of_status_stage = TransferDescriptor::PacketIdentification::kIn;
    break;
  case UsbCtrl::PacketIdentification::kIn:
    direction_of_status_stage = TransferDescriptor::PacketIdentification::kOut;
    break;
  default:
    assert(false);
  }
  
  td2->SetNext(true);
  td2->SetStatus(false, false, true, false);
  td2->SetToken(direction_of_status_stage, device_addr, 0, true, 8);
  td2->SetBuffer();

  qh1->SetHorizontalNext(qh1);
  qh1->SetVerticalNext(td1);

  _qh0->SetHorizontalNext(qh1);

  while(td2->IsActiveOfStatus()) {
    if (td1->IsStalledOfStatus() || td1->IsCrcErrorOfStatus()) {
      break;
    }
    asm volatile("":::"memory");
  }
  if (td2->IsActiveOfStatus()) {
    success = false;
    goto release;
  }
  // TODO remove qh1 

 release:
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
  return make_sptr<DevUsbController::Manager>();
}
