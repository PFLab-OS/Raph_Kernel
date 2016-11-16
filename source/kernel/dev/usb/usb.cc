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
 * reference: Universal Host Controller Interface (UHCI) Design Guide REVISION 1.1
 * 
 */

#include "usb.h"
#include <mem/virtmem.h>
#include <mem/physmem.h>
#include <mem/paging.h>

UsbCtrl UsbCtrl::_ctrl;
bool UsbCtrl::_is_initialized = false;

void UsbCtrl::Init() {
  new(&_ctrl) UsbCtrl;
    
  constexpr int dr_buf_size = _ctrl._dr_buf.GetBufSize();
  PhysAddr dr_buf_paddr;
  static_assert(dr_buf_size * sizeof(DeviceRequest) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(dr_buf_paddr, PagingCtrl::kPageSize);
  DeviceRequest *req_array = addr2ptr<DeviceRequest>(dr_buf_paddr.GetVirtAddr());
  for (int i = 0; i < dr_buf_size; i++) {
    kassert(_ctrl._dr_buf.Push(&req_array[i]));
  }

  constexpr int dd_buf_size = _ctrl._dd_buf.GetBufSize();
  PhysAddr dd_buf_paddr;
  static_assert(dd_buf_size * sizeof(DeviceDescriptor) <= PagingCtrl::kPageSize, "");
  physmem_ctrl->Alloc(dd_buf_paddr, PagingCtrl::kPageSize);
  DeviceDescriptor *desc_array = addr2ptr<DeviceDescriptor>(dd_buf_paddr.GetVirtAddr());
  for (int i = 0; i < dd_buf_size; i++) {
    kassert(_ctrl._dd_buf.Push(&desc_array[i]));
  }
  
  _ctrl._is_initialized = true;
}
