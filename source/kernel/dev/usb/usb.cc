/*
1;4205;0c *
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

// for debug
#include <tty.h>
#include <global.h>

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

  _ctrl._is_initialized = true;
}

void DevUsb::LoadDeviceDescriptor() {
  while(true) {
    UsbCtrl::DeviceRequest *request = nullptr;

    bool release = true;
    do {
      if (!UsbCtrl::GetCtrl().AllocDeviceRequest(request)) {
        break;
      }

      request->MakePacketOfGetDescriptorRequest(UsbCtrl::DescriptorType::kDevice, 0, sizeof(UsbCtrl::DeviceDescriptor));

      if (!_controller->SendControlTransfer(request, ptr2virtaddr(&_device_desc), sizeof(UsbCtrl::DeviceDescriptor), _addr)) {
        break;
      }

      release = false;
    } while(0);

    if (!release) {
      break;
    }
    
    if (request != nullptr) {
      assert(UsbCtrl::GetCtrl().ReuseDeviceRequest(request));
    }
  }
}

void DevUsb::LoadCombinedDescriptors() {
  // if (_device_desc.config_num != 1) {
  //   kernel_panic("DevUsb", "Configuration num > 1 is not supported");
  // }
  
  UsbCtrl::ConfigurationDescriptor config_desc;
  while(true) {
    UsbCtrl::DeviceRequest *request = nullptr;

    bool release = true;
    do {
      if (!UsbCtrl::GetCtrl().AllocDeviceRequest(request)) {
        break;
      }
      
      request->MakePacketOfGetDescriptorRequest(UsbCtrl::DescriptorType::kConfiguration, 0, sizeof(UsbCtrl::ConfigurationDescriptor));
      
      if (!_controller->SendControlTransfer(request, ptr2virtaddr(&config_desc), sizeof(UsbCtrl::ConfigurationDescriptor), _addr)) {
        break;
      }

      release = false;
    } while(0);

    if (!release) {
      break;
    }

    if (request != nullptr) {
      assert(UsbCtrl::GetCtrl().ReuseDeviceRequest(request));
    }
  }

  _combined_desc = new uint8_t[config_desc.total_length];
  while(true) {
    UsbCtrl::DeviceRequest *request = nullptr;

    bool release = true;
    do {
      if (!UsbCtrl::GetCtrl().AllocDeviceRequest(request)) {
        break;
      }
      
      request->MakePacketOfGetDescriptorRequest(UsbCtrl::DescriptorType::kConfiguration, 0, config_desc.total_length);
      
      if (!_controller->SendControlTransfer(request, ptr2virtaddr(_combined_desc), config_desc.total_length, _addr)) {
        break;
      }

      release = false;
    } while(0);

    if (!release) {
      break;
    }

    if (request != nullptr) {
      assert(UsbCtrl::GetCtrl().ReuseDeviceRequest(request));
    }
  }

}

UsbCtrl::DummyDescriptor *DevUsb::GetDescriptorInCombinedDescriptors(UsbCtrl::DescriptorType type, int desc_index) {
  assert(desc_index >= 0);

  UsbCtrl::ConfigurationDescriptor *config_desc = reinterpret_cast<UsbCtrl::ConfigurationDescriptor *>(_combined_desc);
  
  for (uint16_t index = 0; index < config_desc->total_length;) {
    UsbCtrl::DummyDescriptor *dummy_desc = reinterpret_cast<UsbCtrl::DummyDescriptor *>(_combined_desc + index);
    if (static_cast<UsbCtrl::DescriptorType>(dummy_desc->type) == type) {
      if (desc_index == 0) {
        return dummy_desc;
      } else {
        desc_index--;
      }
    }
    index += dummy_desc->length;
  }
  
  assert(false);
}

int DevUsb::GetDescriptorNumInCombinedDescriptors(UsbCtrl::DescriptorType type) {
  int num = 0;
  
  UsbCtrl::ConfigurationDescriptor *config_desc = reinterpret_cast<UsbCtrl::ConfigurationDescriptor *>(_combined_desc);
  
  for (uint16_t index = 0; index < config_desc->total_length;) {
    UsbCtrl::DummyDescriptor *dummy_desc = reinterpret_cast<UsbCtrl::DummyDescriptor *>(_combined_desc + index);
    if (static_cast<UsbCtrl::DescriptorType>(dummy_desc->type) == type) {
      num++;
    }
    index += dummy_desc->length;
  }

  return num;
}

DevUsb *DevUsbKeyboard::InitUsb(DevUsbController *controller, int addr) {
  DevUsbKeyboard *that = new DevUsbKeyboard(controller, addr);

  that->LoadDeviceDescriptor();

  that->LoadCombinedDescriptors();
  UsbCtrl::InterfaceDescriptor *interface_desc = that->GetInterfaceDescriptorInCombinedDescriptors(0);
  if (interface_desc->class_code == 3 && interface_desc->protocol_code == 1) {
    gtty->CprintfRaw("kbd: info: usb keyboard detected\n");
    that->Init();
    return that;
  } else {
    delete that;
    return nullptr;
  }
}

void DevUsbKeyboard::Init() {
  if (GetDescriptorNumInCombinedDescriptors(UsbCtrl::DescriptorType::kEndpoint) != 1) {
    kernel_panic("DevUsbKeyboard", "not supported");
  }
  gtty->CprintfRaw("<%x>", GetEndpointDescriptorInCombinedDescriptors(0)->address);
  assert((GetEndpointDescriptorInCombinedDescriptors(0)->attributes & 3) == 3);
}
