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
 * reference: Universal Serial Bus Specification Revision 1.1
 * 
 */

#ifndef __RAPH_KERNEL_DEV_USB_USB_H__
#define __RAPH_KERNEL_DEV_USB_USB_H__

#include <stdint.h>
#include <buf.h>
#include <mem/physmem.h>

// maximum data payload size for Control Transfers
// quoted from 5.5.3 Control Transfer Packet Size Constraints
//
// The USB defines the allowable maximum control data payload sizes
// for full-speed devices to be either 8, 16, 32, or 64 bytes.
// Low-speed devices are limited to only an eight-byte maximum data
// payload size.

class UsbCtrl {
public:
  // see Table 9-4 Standard Request Codes
  enum class RequestCode : uint8_t {
    kGetStatus = 0,
    kClearFeature = 1,
    kSetFeature = 3,
    kSetAddress = 5,
    kGetDescriptor = 6,
    kSetDescriptor = 7,
    kGetConfiguration = 8,
    kSetConfiguration = 9,
    kGetInterface = 10,
    kSetInterface = 11,
    kSynchFrame = 12,
  };

  // see Table 9-5 Descriptor Types
  enum class DescriptorType : uint16_t {
    kDevice = 0x100,
    kConfiguration = 0x200,
    kString = 0x300,
    kInterface = 0x400,
    kEndpoint = 0x500,
  };
  
  // see Table 9-7 Standard Device Descriptor
  class DeviceDescriptor {
  public:
    phys_addr GetPhysAddr() {
      return v2p(ptr2virtaddr(this));
    }
    uint8_t length;
    uint8_t type;
    uint16_t usb_release_number;
    uint8_t class_code;
    uint8_t subclass_code;
    uint8_t protocol;
    uint8_t max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_release_number;
    uint8_t manufacture_index;
    uint8_t product_index;
    uint8_t serialnum_index;
    uint8_t config_index;
  } __attribute__((__packed__));
  static_assert(sizeof(DeviceDescriptor) == 0x12, "");

  // see Table 9-2
  class DeviceRequest {
  public:
    phys_addr GetPhysAddr() {
      return v2p(ptr2virtaddr(this));
    }
    void MakePacketOfGetDeviceRequest() {
      // see 9.4.2
      request_type = 0x80;
      request = static_cast<uint8_t>(RequestCode::kGetDescriptor);
      value = static_cast<uint16_t>(DescriptorType::kDevice);
      index = 0;
      length = sizeof(DeviceDescriptor);
    }
  private:
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
  } __attribute__((__packed__));
  static_assert(sizeof(DeviceRequest) == 8, "");

  static UsbCtrl &GetCtrl() {
    kassert(_is_initialized);
    return _ctrl;
  }
  static void Init();
  bool GetDeviceRequest(DeviceRequest *&request) {
    return _dr_buf.Pop(request);
  }
  void ReuseDeviceRequest(DeviceRequest *request) {
    _dr_buf.Push(request);
  }
  bool GetDeviceDescriptor(DeviceDescriptor *&descriptor) {
    return _dd_buf.Pop(descriptor);
  }
  void ReuseDeviceDescriptor(DeviceDescriptor *descriptor) {
    _dd_buf.Push(descriptor);
  }
private:
  static UsbCtrl _ctrl;
  static bool _is_initialized;

  RingBuffer<DeviceRequest *, 512> _dr_buf;
  RingBuffer<DeviceDescriptor *, 227> _dd_buf;

};

class DevUsbController {
public:
  virtual bool GetDeviceDescriptor(UsbCtrl::DeviceDescriptor *desc, int device_addr) = 0;
};

// !!! important !!!
// child class of DevUsb must implement a following function.
// static DevUsb *InitUsb(DevUsbController *controller, int addr);
class DevUsb {
public:
  DevUsb(DevUsbController *controller, int addr) : _controller(controller), _addr(addr) {
  }
private:
  const DevUsbController *_controller;
  const int _addr;
};

class DevUsbKeyboard : public DevUsb {
public:
  DevUsbKeyboard(DevUsbController *controller, int addr) : DevUsb(controller, addr) {
  }
  static DevUsb *InitUsb(DevUsbController *controller, int addr);
private:
  DevUsbKeyboard();
};

#endif // __RAPH_KERNEL_DEV_USB_USB_H__

