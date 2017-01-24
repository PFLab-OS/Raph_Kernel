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
    kassert(_td_buf.Push(&td_array[i]));
  }

  constexpr int qh_buf_size = _qh_buf.GetBufSize();
  PhysAddr qh_buf_paddr;
  static_assert(qh_buf_size * sizeof(QueueHead) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(qh_buf_paddr, PagingCtrl::kPageSize);
  QueueHead *qh_array = addr2ptr<QueueHead>(qh_buf_paddr.GetVirtAddr());
  for (int i = 0; i < qh_buf_size; i++) {
    kassert(_qh_buf.Push(&qh_array[i]));
  }

  
  _base_addr = ReadReg<uint32_t>(kBaseAddressReg);
  if ((_base_addr & 1) == 0) {
    kernel_panic("Uhci", "cannot get valid base address.");
  }
  _base_addr &= 0xFFE0;
  WriteReg<uint16_t>(PciCtrl::kCommandReg, ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  // halt controller
  WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) & ~1);
  
  while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) == 0) {
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

  // WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) | 1);

  // while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) != 0) {
  // }
}

bool DevUhci::SendControlTransfer(UsbCtrl::DeviceRequest *request, virt_addr data, size_t data_size, int device_addr) {
  // TODO rewrite
  kernel_panic("Uhci", "");
 //  bool success = true;

 //  // debug
 //  memset(reinterpret_cast<uint8_t *>(desc), 0xFF, sizeof(UsbCtrl::DeviceDescriptor));

 //  // debug
 //  TransferDescriptor *td0;
 //  if (!_td_buf.Pop(td0)) {
 //    success = false;
 //    goto release;
 //  }
 //  QueueHead *qh0;
 //  if (!_qh_buf.Pop(qh0)) {
 //    success = false;
 //    goto release;
 //  }

 //  td0->SetNext(false);
 //  td0->SetStatus(false, false, false, false);
 //  td0->SetToken(TransferDescriptor::PacketIdentification::kIn, 127, 0, false, 0);
 //  td0->SetBuffer();

 //  qh0->SetHorizontalNext();
 //  qh0->SetVerticalNext(td0);

 //  for (int i = 0; i < 1024; i++) {
 //    _frlist->entries[i].Set(qh0);
 //  }

 //  // see Figure 8-12 Control Read and Write Sequences
 //  QueueHead *qh1;
 //  if (!_qh_buf.Pop(qh1)) {
 //    success = false;
 //    goto release;
 //  }
 //  TransferDescriptor *td1;
 //  if (!_td_buf.Pop(td1)) {
 //    success = false;
 //    goto release;
 //  }
 //  TransferDescriptor *td2;
 //  if (!_td_buf.Pop(td2)) {
 //    success = false;
 //    goto release;
 //  }
 //  TransferDescriptor *td3;
 //  if (!_td_buf.Pop(td3)) {
 //    success = false;
 //    goto release;
 //  }
 //  TransferDescriptor *td4;
 //  if (!_td_buf.Pop(td4)) {
 //    success = false;
 //    goto release;
 //  }
 //  TransferDescriptor *td5;
 //  if (!_td_buf.Pop(td5)) {
 //    success = false;
 //    goto release;
 //  }

 //  td1->SetNext(td2, true);
 //  td1->SetStatus(false, false, true, false);
 //  td1->SetToken(TransferDescriptor::PacketIdentification::kSetup, device_addr, 0, false, 8);
 //  td1->SetBuffer(request, 0);

 //  td2->SetNext(td3, true);
 //  td2->SetStatus(false, false, true, false);
 //  td2->SetToken(TransferDescriptor::PacketIdentification::kIn, device_addr, 0, true, 8);
 //  td2->SetBuffer(desc, 0);

 //  td3->SetNext(td4, true);
 //  td3->SetStatus(false, false, true, false);
 //  td3->SetToken(TransferDescriptor::PacketIdentification::kIn, device_addr, 0, false, 8);
 //  td3->SetBuffer(desc, 8);

 //  td4->SetNext(td5, true);
 //  td4->SetStatus(false, false, true, false);
 //  td4->SetToken(TransferDescriptor::PacketIdentification::kIn, device_addr, 0, true, 8);
 //  td4->SetBuffer(desc, 16);

 //  td5->SetNext(true);
 //  td5->SetStatus(false, false, true, false);
 //  td5->SetToken(TransferDescriptor::PacketIdentification::kOut, device_addr, 0, true, 8);
 //  td5->SetBuffer();

 //  qh1->SetHorizontalNext(qh1);
 //  qh1->SetVerticalNext(td1);

 //  _frlist->entries[GetCurrentFameListIndex()].Set(qh1);

 //  //gtty->CprintfRaw("<%d %d>", device_addr, GetCurrentFameListIndex());

 //  while(td5->IsActiveOfStatus()) {
 //    if (td1->IsStalledOfStatus() || td1->IsCrcErrorOfStatus()) {
 //      break;
 //    }
 //  }
 //  if (td5->IsActiveOfStatus()) {
 //    success = false;
 //    goto release;
 //  }
 //  gtty->CprintfRaw(" %x ", td1->GetStatus());
 //  gtty->CprintfRaw(" %x ", td2->GetStatus());
 //  gtty->CprintfRaw(" %x ", td3->GetStatus());
 //  gtty->CprintfRaw(" %x ", td4->GetStatus());
 //  gtty->CprintfRaw(" %x ", td5->GetStatus());
 // release:
 //  assert(_td_buf.Push(td0));
 //  assert(_td_buf.Push(td1));
 //  assert(_td_buf.Push(td2));
 //  assert(_td_buf.Push(td3));
 //  assert(_td_buf.Push(td4));
 //  assert(_td_buf.Push(td5));
 //  assert(_qh_buf.Push(qh0));
 //  assert(_qh_buf.Push(qh1));
  
 //  return success;
}

