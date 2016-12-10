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

  phys_addr addr = ReadReg<uint32_t>(kBaseAddressReg);
  if ((addr & 0x4) != 0) {
    // may be mapped into 64bit addressing space
    addr |= static_cast<phys_addr>(ReadReg<uint32_t>(kBaseAddressReg + 4)) << 32;
  }
  addr &= 0xFFFFFFFFFFFFFF00;
  _capability_reg_base_addr = addr2ptr<volatile uint8_t>(p2v(addr));
  _operational_reg_base_addr = reinterpret_cast<volatile uint32_t *>(_capability_reg_base_addr + ReadCapabilityRegCapLength());

  int n_ports = ReadCapabilityRegHcsParams() & 0xF;

  // for (int i = 0; i < n_ports; i++) {
  //   if (_operational_reg_base_addr[kOperationalRegOffsetPortScBase + i] & 1) {
  //     gtty->CprintfRaw("usb>%x", _operational_reg_base_addr[kOperationalRegOffsetPortScBase + i]);
  //   }
  // }

  WriteReg<uint16_t>(PciCtrl::kCommandReg, ReadReg<uint16_t>(PciCtrl::kCommandReg) | PciCtrl::kCommandRegBusMasterEnableFlag);

  if ((ReadCapabilityRegHccParams() & 1) == 0) {
    _sub = new DevEhciSub32();
  } else {
    _sub = new DevEhciSub64();
  }
  
  _sub->Init();
  
  kassert(false);


  
  // // halt controller
  // WriteControllerReg<uint16_t>(kCtrlRegCmd, ReadControllerReg<uint16_t>(kCtrlRegCmd) & ~1);
  
  // while((ReadControllerReg<uint16_t>(kCtrlRegStatus) & kCtrlRegStatusFlagHalted) == 0) {
  // }

  for (int dev = 0; dev < 128; dev++) {
    DevUsbKeyboard::InitUsb(&_controller_dev, dev);
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
  _qh0.InitEmpty();

  // allocate frame list
  assert(sizeof(FrameList) == PagingCtrl::kPageSize);
  PhysAddr frame_base_addr;
  physmem_ctrl->Alloc(frame_base_addr, PagingCtrl::kPageSize);
  _frlist = addr2ptr<FrameList>(frame_base_addr.GetVirtAddr());
  if (frame_base_addr.GetAddr() > 0xFFFFFFFF) {
    kernel_panic("DevEhci", "cannot allocate 32bit phys memory");
  }

  // framelist initialization
  kassert(false);
}

void DevEhci::DevEhciSub64::Init() {
  kernel_panic("Ehci", "needs implementation of 64-bit addressing");
}

bool DevEhci::GetDeviceDescriptor(Usb11Ctrl::DeviceDescriptor *desc, int device_addr) {
  bool success = true;

  Usb11Ctrl::DeviceRequest *request;
  if (!Usb11Ctrl::GetCtrl().AllocDeviceRequest(request)) {
    success = false;
    goto release;
  }

  request->MakePacketOfGetDeviceRequest();

 release:
  assert(Usb11Ctrl::GetCtrl().ReuseDeviceRequest(request));
  
  return success;
}
